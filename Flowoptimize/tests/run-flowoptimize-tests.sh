#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
optimizer=${FLOWOPTIMIZE_BIN:?FLOWOPTIMIZE_BIN is required}
analyst=${FLOWANALYST_BIN:?FLOWANALYST_BIN is required}
flowmini="$root/Flowmini/flowmini_v25_symboltable_projection/cmake-build-debug/flowmini"
fixture="$root/Flowmini/flowmini_v25_symboltable_projection/examples/ast/call_expression_probe.flow"

report=$("$flowmini" --dump-frontend-bundle "$fixture" | "$analyst" | "$optimizer")
printf '%s\n' "$report" | grep -q '"format": "flowoptimize.optimization_report"'
printf '%s\n' "$report" | grep -q '"status": "ready"'
printf '%s\n' "$report" | grep -q '"transforms": \[\]'

if printf '%s' '{"format":"flowanalyst.semantic_report","version":1,"status":"error"}' | "$optimizer" >/dev/null 2>&1; then
    echo 'rejected semantic report unexpectedly accepted' >&2
    exit 1
fi
echo 'Flowoptimize tests: PASS'
