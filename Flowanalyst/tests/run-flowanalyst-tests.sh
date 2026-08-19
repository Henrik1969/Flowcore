#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
bin=${FLOWANALYST_BIN:?FLOWANALYST_BIN is required}
flowmini="$root/Flowmini/flowmini_v25_symboltable_projection/cmake-build-debug/flowmini"
fixture="$root/Flowmini/flowmini_v25_symboltable_projection/examples/ast/target_projection_probe.flow"

report=$("$flowmini" --dump-frontend-bundle "$fixture" | "$bin")
printf '%s\n' "$report" | grep -q '"status": "ok"'
printf '%s\n' "$report" | grep -q '"targets":2'
printf '%s\n' "$report" | grep -q '"analysis_regions"'
printf '%s\n' "$report" | grep -q '"id":"target:cli"'
printf '%s\n' "$report" | grep -q '"format":"flowanalyst.analysis_graph"'
printf '%s\n' "$report" | grep -q '"name":"region_dependency"'

call_fixture="$root/Flowmini/flowmini_v25_symboltable_projection/examples/ast/call_expression_probe.flow"
call_report=$("$flowmini" --dump-frontend-bundle "$call_fixture" | "$bin")
printf '%s\n' "$call_report" | grep -q '"resolved_names":12'
printf '%s\n' "$call_report" | grep -q '"name":"square"'

refined_fixture="$root/Flowmini/flowmini_v25_symboltable_projection/examples/ast/refined_contract_probe.flow"
refined_report=$("$flowmini" --dump-frontend-bundle "$refined_fixture" | "$bin")
printf '%s\n' "$refined_report" | grep -q '"status": "ok"'
printf '%s\n' "$refined_report" | grep -q '"refined_types":2'

if printf '%s' '{"format":"wrong","version":2}' | "$bin" >/dev/null 2>&1; then
    echo 'invalid bundle unexpectedly accepted' >&2
    exit 1
fi
echo 'Flowanalyst tests: PASS'
