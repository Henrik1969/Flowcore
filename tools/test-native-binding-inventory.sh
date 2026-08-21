#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
test -x "$root/tools/scan-native-bindings.sh"
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT

"$root/tools/scan-native-bindings.sh" --output "$tmpdir/inventory.json"
jq -e '
    .format == "flowcore.native_binding_inventory" and
    .version == 1 and
    .status == "inventory-only" and
    (.libraries | length) > 0 and
    (.headers | length) > 0 and
    (.development_packages | length) >= 0 and
    .policy.authorization == "not-performed" and
    .policy.execution == "not-performed" and
    .policy.binding_generation == "not-performed"
' "$tmpdir/inventory.json" >/dev/null

printf '%s\n' 'Native binding inventory: PASS'
