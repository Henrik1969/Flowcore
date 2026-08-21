#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT

"$root/tools/scan-native-bindings.sh" --output "$tmpdir/inventory.json"
jq '{format:"flowcore.native_binding_selection",version:1,status:"review-required",inventory_format:.format,
    selections:[(.libraries[] | select(.soname=="libc.so.6" and (.path|contains("x86_64"))) | {
      id:"test-libc",namespace:"flowcore.test",capability:"c-runtime",strategy:"bind-existing",
      provider:.,contract:"test/contract.flow",rationale:"inventory validator test"})],
    policy:{authorization:"not-performed",execution:"not-performed",binding_generation:"not-performed"}}' \
    "$tmpdir/inventory.json" > "$tmpdir/selection.json"

"$root/tools/validate-native-binding-selection.sh" \
    "$tmpdir/inventory.json" "$tmpdir/selection.json" --output "$tmpdir/result.json" >/dev/null
jq -e '.format == "flowcore.native_binding_selection_result" and .status == "valid" and .policy.authorization == "not-performed"' \
    "$tmpdir/result.json" >/dev/null

jq '.selections[0].provider.path = "/does/not/exist.so"' "$tmpdir/selection.json" > "$tmpdir/bad.json"
if "$root/tools/validate-native-binding-selection.sh" "$tmpdir/inventory.json" "$tmpdir/bad.json" >/dev/null 2>&1; then
    printf '%s\n' 'native binding selection test: invalid provider was accepted' >&2
    exit 1
fi

printf '%s\n' 'Native binding selection: PASS'
