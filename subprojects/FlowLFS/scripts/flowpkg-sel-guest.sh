#!/usr/bin/env bash
set -euo pipefail
package=sel; version=0.1.0
source_sha=50180307ec1435036812cf842ee1097d7e18170bdb543fe4f547988fe3c9e6c8
flow_sha=96a4dc767c71cdb551be3e29543e53f973b3a47bd58eab0fdd4708bb26163772
assembly_sha=9c863ed860607da64e57676570dc66418d3e271a24f91c29b3deb9a70bade1bf
archive=/var/tmp/flowpkg-input/$package-$version/$package-$version.tar.xz
build_root=/var/lib/flowbuild/$package-$version; source_root=$build_root/source; stage_root=$build_root/stage
evidence_root=/flow/evidence/$package-$version; store_root=/flow/store
object_pointer=$store_root/packages/$package/$version/object; projection=$store_root/projections/$package-$version
die(){ printf 'FLOWPKG_SEL_ERROR %s\n' "$*" >&2; exit 1; }; require_root(){ test "$(id -u)" -eq 0||die "$1 requires root"; }; require_builder(){ test "$(id -un)" = flowbuilder||die "$1 requires flowbuilder"; }

prepare(){ require_root prepare; ldconfig -p|grep -Fq libncursesw.so.6||die 'ncursesw capability absent'; install -d -m0755 "$store_root/objects" "$store_root/packages/$package/$version" "$store_root/projections"; install -d -o flowbuilder -g flowbuilder -m0750 "${archive%/*}" "$build_root" "$evidence_root"; printf 'FLOWPKG_SEL_PREPARED\n'; }
build(){
  require_builder build; cd /; printf '%s  %s\n' "$source_sha" "$archive"|sha256sum -c -
  tar -tf "$archive"|awk '/^\//{b=1}{n=split($0,a,"/");for(i=1;i<=n;i++)if(a[i]=="..")b=1}END{exit b}'||die 'unsafe archive'
  test ! -d "$stage_root"||chmod -R u+w "$stage_root"; rm -rf "$source_root" "$stage_root"; install -d -m0750 "$source_root" "$stage_root/usr/bin" "$stage_root/usr/share/doc/sel-$version"
  tar -xf "$archive" -C "$source_root"; cd "$source_root"
  printf '%s  sel.flow\n%s  sel.s\n' "$flow_sha" "$assembly_sha"|sha256sum -c -
  grep -Fq 'status=source-derived assembly cache' derivation.txt||die 'derivation boundary absent'
  sed '/^[[:space:]]*\.addrsig$/d' sel.s >sel.gas.s
  printf '%s  sel.gas.s\n' 14a8e15650d895f5fce17228c760ff4014bd0afe1f0dac1baa367f44af72504f|sha256sum -c -
  diff -u sel.s sel.gas.s >"$evidence_root/clang-to-gas.patch" || test "$?" -eq 1
  test "$(wc -l <"$evidence_root/clang-to-gas.patch")" -eq 7||die 'adaptation exceeded declared one-directive boundary'
  gcc -fPIE -pie -Wl,--build-id=sha1 sel.gas.s -lncursesw -o "$stage_root/usr/bin/sel" 2>&1|tee "$evidence_root/build.log"
  cp sel.flow sel.policy derivation.txt frontend.json semantic.json optimized.json binding.json lowering.json sel.ll sel.s sel.gas.s "$stage_root/usr/share/doc/sel-$version/"
  TERM=xterm script -qec "printf 'stdin-alpha\\nstdin-beta\\n' | '$stage_root/usr/bin/sel'" "$evidence_root/terminal.log" >/dev/null
  grep -q 'alpha' "$evidence_root/terminal.log"; grep -q 'stdin-alpha' "$evidence_root/terminal.log"
  { printf 'package=sel\nversion=%s\nsource_sha256=%s\nflow_source_sha256=%s\nassembly_sha256=%s\nbuilder=%s\n' "$version" "$source_sha" "$flow_sha" "$assembly_sha" "$(id)"; gcc --version|sed -n 1p; } >"$evidence_root/environment.txt"
  printf 'FLOWPKG_SEL_BUILD_PASS stage=%s\n' "$stage_root"
}
admit(){ require_root admit; test -x "$stage_root/usr/bin/sel" -a -f "$evidence_root/terminal.log"||die 'completed build absent'; test "$(stat -c %U "$stage_root")" = flowbuilder||die 'stage owner mismatch'; chmod 0755 "$stage_root"; digest=$(tar --sort=name --mtime=@0 --owner=0 --group=0 --numeric-owner -C "$stage_root" -cf - .|sha256sum|awk '{print $1}'); object=$store_root/objects/sha256-$digest; if test -e "$object"; then test -d "$object/root" -a -f "$object/manifest.tsv" -a -f "$object/derivation.txt"||die 'existing object invalid'; else incoming=$store_root/objects/.incoming-sel-$digest-$$; install -d -m0755 "$incoming"; mv "$stage_root" "$incoming/root"; chown -R root:root "$incoming/root"; (cd "$incoming/root"&&find . -mindepth 1 -printf '%y\t%m\t%u\t%g\t%p\t%l\n'|LC_ALL=C sort)>"$incoming/manifest.tsv"; cp "$evidence_root/environment.txt" "$incoming/derivation.txt"; printf 'output_tree_sha256=%s\n' "$digest">>"$incoming/derivation.txt"; chmod -R a-w "$incoming"; mv "$incoming" "$object"; fi; ln -sfn "$object" "$object_pointer"; printf '%s\n' "$object">"$evidence_root/admitted-object.txt"; printf 'FLOWPKG_SEL_ADMIT_PASS object=%s\n' "$object"; }
project(){ require_root project; object=$(readlink -f "$object_pointer"); root=$object/root; test -x "$root/usr/bin/sel"||die 'object absent'; test ! -e "$projection"||die 'projection active'; while IFS= read -r -d '' e; do d=${e#"$root"}; test ! -e "$d" -a ! -L "$d"||die "collision $d"; done < <(find "$root" -mindepth 1 \! -type d -print0|sort -z); install -d -m0755 "$projection"; :>"$projection/links.list"; :>"$projection/dirs.list"; while IFS= read -r -d '' e; do d=${e#"$root"}; if test ! -d "$d"; then mkdir "$d"; printf '%s\n' "$d">>"$projection/dirs.list"; fi; done < <(find "$root" -mindepth 1 -type d -print0|sort -z); while IFS= read -r -d '' e; do d=${e#"$root"}; ln -s "$e" "$d"; printf '%s\t%s\n' "$d" "$e">>"$projection/links.list"; done < <(find "$root" -mindepth 1 \! -type d -print0|sort -z); printf '%s\n' "$object">"$projection/object"; printf 'FLOWPKG_SEL_PROJECT_PASS object=%s\n' "$object"; }
verify(){ require_root verify; test -d "$projection"||die 'projection inactive'; su -s /bin/bash flowbuilder -c 'test -x /usr/bin/sel; test -r /usr/share/doc/sel-0.1.0/sel.flow; TERM=xterm script -qec "printf '\''probe-alpha\\nprobe-beta\\n'\'' | /usr/bin/sel" /tmp/sel-runtime.log >/dev/null; grep -q probe-alpha /tmp/sel-runtime.log; rm -f /tmp/sel-runtime.log'||die 'ordinary-user sel test failed'; systemctl is-active --quiet sshd.service||die 'sshd inactive'; printf 'FLOWPKG_SEL_VERIFY_PASS\n'; }
rollback(){ require_root rollback; test -d "$projection"||die 'projection inactive'; while IFS=$'\t' read -r d t; do test -L "$d" -a "$(readlink "$d")" = "$t"||die "changed $d"; rm "$d"; done<"$projection/links.list"; tac "$projection/dirs.list"|while IFS= read -r d; do rmdir "$d" 2>/dev/null||:; done; mv "$projection" "$projection.rolled-back.$(date -u +%Y%m%dT%H%M%SZ)"; printf 'FLOWPKG_SEL_ROLLBACK_PASS\n'; }
case "${1:-}" in prepare)prepare;;build)build;;admit)admit;;project)project;;verify)verify;;rollback)rollback;;*)die 'usage: prepare|build|admit|project|verify|rollback';;esac
