#!/bin/sh
set -eu

root=${FLOWCORE_ROOT:?}
validate=${FLOWVALIDATE_BIN:?}
inventory="$root/docs/bootstrap/remaining-bootstrap-inventory-v1.json"
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT

"$validate" --canonical "$inventory" > "$tmpdir/canonical.json"
cmp -s "$inventory" "$tmpdir/canonical.json"
"$validate" "$inventory" | grep -q '"classification":"valid"'
jq '.gaps[1].id = .gaps[0].id' "$inventory" > "$tmpdir/duplicate.json"
if "$validate" "$tmpdir/duplicate.json" >/dev/null 2>&1; then echo 'validator accepted duplicate bootstrap gap identity' >&2; exit 1; fi
jq '.gaps[0].requires = ["unknown-gap"]' "$inventory" > "$tmpdir/dependency.json"
if "$validate" "$tmpdir/dependency.json" >/dev/null 2>&1; then echo 'validator accepted unknown bootstrap dependency' >&2; exit 1; fi
jq '.stages[1].stage = 3' "$inventory" > "$tmpdir/stage.json"
if "$validate" "$tmpdir/stage.json" >/dev/null 2>&1; then echo 'validator accepted unordered bootstrap stages' >&2; exit 1; fi

echo 'bootstrap gap inventory: PASS'
