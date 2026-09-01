#!/usr/bin/env bash
set -euo pipefail
package=flow-desktop-defaults; version=0.1.0; store=/flow/store
archive=/var/tmp/flowpkg-input/$package-$version/$package-$version.tar.xz
source_sha=${FLOWPKG_SOURCE_SHA256:?source digest required}
build=/var/lib/flowbuild/$package-$version; stage=$build/stage
evidence=/flow/evidence/$package-$version; pointer=$store/packages/$package/$version/object
projection=$store/projections/$package-$version
die(){ printf 'FLOWPKG_DESKTOP_DEFAULTS_ERROR %s\n' "$*" >&2; exit 1; }
root(){ test "$(id -u)" -eq 0||die root; }
builder(){ test "$(id -un)" = flowbuilder||die builder; }
prepare(){ root; command -v foot >/dev/null; command -v flowselect >/dev/null; install -d -m0755 "$store/packages/$package/$version"; install -d -o flowbuilder -g flowbuilder -m0750 "${archive%/*}" "$build" "$evidence"; }
build_package(){ builder; printf '%s  %s\n' "$source_sha" "$archive"|sha256sum -c -; rm -rf "$build/source" "$stage"; install -d "$build/source" "$stage/usr/bin" "$stage/usr/libexec/flow-desktop" "$stage/etc/flowcore/desktop" "$stage/etc/xdg/weston"; tar -xf "$archive" -C "$build/source" --strip-components=1; install -m0755 "$build/source"/flow-terminal "$build/source"/flow-app-editor "$build/source"/flow-app-files "$build/source"/flow-app-monitor "$build/source"/flow-desktop-launcher "$stage/usr/bin/"; install -m0755 "$build/source/flow-desktop-menu" "$stage/usr/libexec/flow-desktop/menu"; install -m0644 "$build/source/terminal-provider" "$stage/etc/flowcore/desktop/terminal-provider"; install -m0644 "$build/source/weston.ini" "$stage/etc/xdg/weston/weston.ini"; for command in flow-terminal flow-app-editor flow-app-files flow-app-monitor flow-desktop-launcher; do test -x "$stage/usr/bin/$command"; done; grep -Fq 'path=/usr/bin/flow-desktop-launcher' "$stage/etc/xdg/weston/weston.ini"; printf 'FLOWPKG_DESKTOP_DEFAULTS_BUILD_PASS\n'; }
admit(){ root; chmod -R a+rX "$stage"; digest=$(tar --sort=name --mtime=@0 --owner=0 --group=0 --numeric-owner -C "$stage" -cf - .|sha256sum|awk '{print $1}'); object=$store/objects/sha256-$digest; if test ! -e "$object"; then incoming=$store/objects/.incoming-$package-$digest-$$; mkdir "$incoming"; mv "$stage" "$incoming/root"; chown -R root:root "$incoming/root"; (cd "$incoming/root"&&find . -mindepth 1 -printf '%y\t%m\t%u\t%g\t%p\t%l\n'|LC_ALL=C sort)>"$incoming/manifest.tsv"; printf 'source_sha256=%s\noutput_tree_sha256=%s\nauthority=desktop defaults only; owner state excluded\n' "$source_sha" "$digest">"$incoming/derivation.txt"; chmod -R a-w "$incoming"; mv "$incoming" "$object"; fi; ln -sfn "$object" "$pointer"; printf 'FLOWPKG_DESKTOP_DEFAULTS_ADMIT_PASS object=%s\n' "$object"; }
project(){ root; object=$(readlink -f "$pointer"); /usr/local/sbin/flowpkg-projector project "$object" / "$projection"; }
verify(){ root; test "$(cat "$projection/object")" = "$(readlink -f "$pointer")"; test "$(cat /etc/flowcore/desktop/terminal-provider)" = foot; command -v flow-terminal flow-app-editor flow-app-files flow-app-monitor flow-desktop-launcher >/dev/null; grep -Fq 'path=/usr/bin/flow-app-editor' /etc/xdg/weston/weston.ini; printf 'FLOWPKG_DESKTOP_DEFAULTS_VERIFY_PASS launchers=5 terminal_provider=foot\n'; }
case "${1:-}" in prepare)prepare;; build)build_package;; admit)admit;; project)project;; verify)verify;; *)die action;; esac
