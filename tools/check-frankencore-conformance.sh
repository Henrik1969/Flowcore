#!/usr/bin/env bash
set -euo pipefail

root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
clock_bin=${CLOCK_BIN:?CLOCK_BIN is required}
revision_bin=${REVISION_BIN:?REVISION_BIN is required}
flowbind_bin=${FLOWBIND_BIN:-$root/Flowbind/build/flowbind}

constitution="$root/docs/architecture/FRANKENCORE-CONSTITUTION.md"
inventory="$root/docs/architecture/frankencore-contract-inventory.json"

for law in FC-I01 FC-I02 FC-I03 FC-I04 FC-I05 FC-I06 FC-I07 FC-I08 FC-I09 FC-I10 FC-I11 FC-I12 FC-I13 FC-I14 FC-I15; do
    grep -q "$law" "$constitution"
done

jq empty "$inventory"
"$root/tools/check-frankencore-architecture.sh" --strict
CLOCK_BIN="$clock_bin" "$root/tools/check-frankencore-clock.sh"
REVISION_BIN="$revision_bin" "$root/tools/check-frankencore-mutation-provenance.sh"

set +e
blocked=$(printf '%s' '{"format":"flowanalyst.semantic_report","version":1,"status":"ok","binding_requirements":[{"contract":"law-test","library":"libc.so.6","convention":"c","symbol":"frankencore_missing_symbol","effect":"io","parameter_types":"c_int","return_type":"c_int"}]}' | "$flowbind_bin" --policy /dev/null)
blocked_rc=$?
set -e
test "$blocked_rc" -eq 2
grep -q 'blocked' <<<"$blocked"
grep -q 'denied by capability policy' <<<"$blocked"

echo 'Frankencore conformance laws: PASS'
