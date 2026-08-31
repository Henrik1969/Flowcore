#!/usr/bin/env bash
set -euo pipefail
package=flow-terminal; version=1.0.0
source_sha=08043cd3173533552ad79a2eb195b4ef728a42d8e9b5a22903557a2ef595a9c6
archive=/var/tmp/flowpkg-input/$package-$version/$package-$version.tar.xz
build_root=/var/lib/flowbuild/$package-$version; source_root=$build_root/source; stage_root=$build_root/stage
evidence_root=/flow/evidence/$package-$version; store_root=/flow/store
object_pointer=$store_root/packages/$package/$version/object; projection=$store_root/projections/$package-$version
die(){ printf 'FLOWPKG_FLOW_TERMINAL_ERROR %s\n' "$*" >&2; exit 1; }
require_root(){ test "$(id -u)" -eq 0||die "$1 requires root"; }
require_builder(){ test "$(id -un)" = flowbuilder||die "$1 requires flowbuilder"; }
prepare(){ require_root prepare; install -d -m0755 "$store_root/objects" "$store_root/packages/$package/$version" "$store_root/projections"; install -d -o flowbuilder -g flowbuilder -m0750 "${archive%/*}" "$build_root" "$evidence_root"; printf 'FLOWPKG_FLOW_TERMINAL_PREPARED\n'; }
build(){
  require_builder build; cd /; printf '%s  %s\n' "$source_sha" "$archive"|sha256sum -c -
  tar -tf "$archive"|awk '/^\//{b=1}{n=split($0,a,"/");for(i=1;i<=n;i++)if(a[i]=="..")b=1}END{exit b}'||die 'unsafe archive'
  test ! -d "$stage_root"||chmod -R u+w "$stage_root"; rm -rf "$source_root" "$stage_root"; install -d -m0750 "$source_root" "$stage_root/usr/lib" "$stage_root/usr/include/flowcore" "$stage_root/usr/share/doc/flow-terminal-$version"
  tar -xf "$archive" -C "$source_root" --strip-components=1; cd "$source_root"
  printf '%s  flow_terminal.c\n%s  flow_terminal.h\n%s  provider_test.c\n' bc44fba6a7209d91f6a9c914abc494d78abcdaf58464cba1570045b7da8981ed 843d80d8acb42223a5b7ce22f9a76fe179402c81fe33ce50c3b8586174eb5ced 53c68cfd08be84ba75c396f986e467a2fe925a3121b9808fdeb03b5fbdce9cb9|sha256sum -c -
  gcc -std=c17 -D_DEFAULT_SOURCE -O2 -fPIC -I. -shared -Wl,-soname,libflowterminal.so.1 flow_terminal.c -o "$stage_root/usr/lib/libflowterminal.so.1.0.0"
  ln -s libflowterminal.so.1.0.0 "$stage_root/usr/lib/libflowterminal.so.1"; ln -s libflowterminal.so.1 "$stage_root/usr/lib/libflowterminal.so"
  install -m0644 flow_terminal.h "$stage_root/usr/include/flowcore/"; install -m0644 flow_terminal.c provider_test.c "$stage_root/usr/share/doc/flow-terminal-$version/"
  gcc -std=c17 -D_DEFAULT_SOURCE -I. provider_test.c -L"$stage_root/usr/lib" -Wl,-rpath,"$stage_root/usr/lib" -lflowterminal -lutil -o "$build_root/provider-test"
  "$build_root/provider-test"|tee "$evidence_root/provider-test.log"
  { printf 'package=%s\nversion=%s\nsource_sha256=%s\nterminal_brand_dependency=none\nrestoration_guardian=process-liveness\nbuilder=%s\n' "$package" "$version" "$source_sha" "$(id)"; gcc --version|sed -n 1p; } >"$evidence_root/environment.txt"
  printf 'FLOWPKG_FLOW_TERMINAL_BUILD_PASS\n'
}
admit(){ require_root admit; test -f "$stage_root/usr/lib/libflowterminal.so.1.0.0" -a -f "$evidence_root/provider-test.log"||die 'completed build absent'; chmod -R a+rX "$stage_root"; chmod 0755 "$stage_root"; digest=$(tar --sort=name --mtime=@0 --owner=0 --group=0 --numeric-owner -C "$stage_root" -cf - .|sha256sum|awk '{print $1}'); object=$store_root/objects/sha256-$digest; if test ! -e "$object"; then incoming=$store_root/objects/.incoming-$package-$digest-$$; install -d -m0755 "$incoming"; mv "$stage_root" "$incoming/root"; chown -R root:root "$incoming/root"; (cd "$incoming/root"&&find . -mindepth 1 -printf '%y\t%m\t%u\t%g\t%p\t%l\n'|LC_ALL=C sort)>"$incoming/manifest.tsv"; cp "$evidence_root/environment.txt" "$incoming/derivation.txt"; printf 'output_tree_sha256=%s\n' "$digest">>"$incoming/derivation.txt"; chmod -R a-w "$incoming"; mv "$incoming" "$object"; fi; ln -sfn "$object" "$object_pointer"; printf '%s\n' "$object">"$evidence_root/admitted-object.txt"; printf 'FLOWPKG_FLOW_TERMINAL_ADMIT_PASS object=%s\n' "$object"; }
project(){ require_root project; object=$(readlink -f "$object_pointer"); root=$object/root; test -f "$root/usr/lib/libflowterminal.so.1"||die 'object absent'; test ! -e "$projection"||die 'projection active'; while IFS= read -r -d '' e; do d=${e#"$root"}; test ! -e "$d" -a ! -L "$d"||die "collision $d"; done < <(find "$root" -mindepth 1 \! -type d -print0|sort -z); install -d -m0755 "$projection"; :>"$projection/links.list"; :>"$projection/dirs.list"; while IFS= read -r -d '' e; do d=${e#"$root"}; if test ! -d "$d"; then mkdir "$d"; printf '%s\n' "$d">>"$projection/dirs.list"; fi; done < <(find "$root" -mindepth 1 -type d -print0|sort -z); while IFS= read -r -d '' e; do d=${e#"$root"}; ln -s "$e" "$d"; printf '%s\t%s\n' "$d" "$e">>"$projection/links.list"; done < <(find "$root" -mindepth 1 \! -type d -print0|sort -z); ldconfig; printf '%s\n' "$object">"$projection/object"; printf 'FLOWPKG_FLOW_TERMINAL_PROJECT_PASS\n'; }
verify(){ require_root verify; test -d "$projection"||die 'projection inactive'; ldconfig -p|grep -Fq libflowterminal.so.1||die 'provider unavailable'; grep -Fq 'sigkill_restore=pass' "$evidence_root/provider-test.log"||die 'guardian evidence absent'; test "${TERM:-}" = xterm-kitty||:; systemctl is-active --quiet sshd.service||die 'sshd inactive'; printf 'FLOWPKG_FLOW_TERMINAL_VERIFY_PASS terminfo=unused sigkill_restore=pass\n'; }
rollback(){ require_root rollback; test -d "$projection"||die 'projection inactive'; while IFS=$'\t' read -r d t; do test -L "$d" -a "$(readlink "$d")" = "$t"||die "changed $d"; rm "$d"; done<"$projection/links.list"; tac "$projection/dirs.list"|while IFS= read -r d; do rmdir "$d" 2>/dev/null||:; done; ldconfig; mv "$projection" "$projection.rolled-back.$(date -u +%Y%m%dT%H%M%SZ)"; printf 'FLOWPKG_FLOW_TERMINAL_ROLLBACK_PASS\n'; }
case "${1:-}" in prepare)prepare;;build)build;;admit)admit;;project)project;;verify)verify;;rollback)rollback;;*)die 'usage: prepare|build|admit|project|verify|rollback';;esac
