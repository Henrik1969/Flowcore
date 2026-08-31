#!/usr/bin/env bash
set -euo pipefail
tool=${1:?tool required}; root=$(mktemp -d); trap 'chmod -R u+w "$root" 2>/dev/null || :; rm -rf "$root"' EXIT
object=$root/object; target=$root/target; projection=$root/projection
mkdir -p "$object/root/usr/bin" "$object/root/usr/share/demo" "$target"
printf demo >"$object/root/usr/bin/demo"; printf data >"$object/root/usr/share/demo/data"
chmod -R a-w "$object"
"$tool" project "$object" "$target" "$projection"
test "$(readlink "$target/usr/bin/demo")" = "$object/root/usr/bin/demo"
test "$(cat "$target/usr/bin/demo")" = demo
archive=$root/projection.rolled-back; "$tool" rollback "$object" "$target" "$projection" "$archive"
test ! -e "$target/usr/bin/demo"; test -f "$archive/links.list"

collision=$root/collision; mkdir -p "$collision/usr/bin"; printf owner >"$collision/usr/bin/demo"
if "$tool" project "$object" "$collision" "$root/collision-projection" 2>/dev/null; then echo collision accepted >&2; exit 1; fi
test "$(cat "$collision/usr/bin/demo")" = owner; test ! -e "$collision/usr/share/demo/data"

changed=$root/changed; mkdir "$changed"; "$tool" project "$object" "$changed" "$root/changed-projection" >/dev/null
rm "$changed/usr/bin/demo"; ln -s /not-owned "$changed/usr/bin/demo"
if "$tool" rollback "$object" "$changed" "$root/changed-projection" "$root/changed-archive" 2>/dev/null; then echo changed ownership accepted >&2; exit 1; fi
test -L "$changed/usr/share/demo/data"
rm "$changed/usr/bin/demo"; ln -s "$object/root/usr/bin/demo" "$changed/usr/bin/demo"
"$tool" rollback "$object" "$changed" "$root/changed-projection" "$root/changed-archive" >/dev/null

interrupted=$root/interrupted; mkdir "$interrupted"
set +e; FLOWPKG_PROJECTOR_TEST_STOP_AFTER=1 "$tool" project "$object" "$interrupted" "$root/interrupted-projection" >/dev/null 2>&1; status=$?; set -e
test "$status" -eq 99; test -L "$interrupted/usr/bin/demo"
"$tool" recover "$object" "$interrupted" "$root/interrupted-projection"
test ! -e "$interrupted/usr/bin/demo"; test ! -e "$interrupted/usr/share/demo/data"
echo FLOWPKG_PROJECTOR_CONFORMANCE_PASS
