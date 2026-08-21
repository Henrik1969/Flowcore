#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
optimizer=${FLOWOPTIMIZE_BIN:?FLOWOPTIMIZE_BIN is required}
analyst=${FLOWANALYST_BIN:-$root/Flowanalyst/build/flowanalyst}
parallel=${FLOWPARALLEL_BIN:-$root/Flowparallel/build/flowparallel}
flowmini=${FLOWMINI_BIN:-$root/Flowmini/flowmini_v25_symboltable_projection/cmake-build-debug/flowmini}
fixture="$root/Flowmini/flowmini_v25_symboltable_projection/examples/ast/call_expression_probe.flow"
test -x "$flowmini"
test -x "$parallel"
test -x "$optimizer"

report=$("$flowmini" --dump-frontend-bundle "$fixture" | "$analyst" | "$parallel" | "$optimizer")
printf '%s\n' "$report" | grep -q '"format": "flowoptimize.optimization_report"'
printf '%s\n' "$report" | grep -q '"status": "ready"'
printf '%s\n' "$report" | jq -e '.transforms[0].kind == "coo_deduplicate" and .transforms[0].status == "not-needed" and .state.transformation == "identity"' >/dev/null
printf '%s\n' "$report" | jq -e --arg source "$fixture" '
  .source.path == $source and
  .projections[0].kind == "graph_to_matrix" and
  .projections[0].status == "available" and
  .provider_policy.selection == "runtime" and
  (.provider_policy.candidates | index("cuda")) != null and
  .provider_policy.cuda.fallback == "cpu"
' >/dev/null

decision=$(mktemp)
trap 'rm -f "$decision"' EXIT
printf '%s\n' '{"format":"flowparallel.graph_provider_decision","version":1,"status":"verified","provider":"cpu.reference","representation":"sparse","reason":"test fallback","fallback":"cpu.reference"}' > "$decision"
decision_report=$($flowmini --dump-frontend-bundle "$fixture" | "$analyst" | "$parallel" | "$optimizer" --provider-decision "$decision")
printf '%s\n' "$decision_report" | jq -e '.status == "ready" and .provider_policy.decision.status == "verified" and .provider_policy.decision.provider == "cpu.reference" and .state.canonical_graph == "unchanged" and .state.transformation == "identity"' >/dev/null

duplicate_report=$(printf '%s\n' '{"format": "flowparallel.execution_plan","version":1,"status":"ready","graph_projection":{"name":"region_dependency","rows":2,"columns":2,"entries":[{"row":0,"column":1},{"row":0,"column":1},{"row":1,"column":0}]}}' | "$optimizer")
printf '%s\n' "$duplicate_report" | jq -e '.transforms[0].status == "applied" and .transforms[0].input_entries == 3 and .transforms[0].output_entries == 2 and .transforms[0].semantics_preserved == true' >/dev/null

printf '%s\n' '{"format":"flowparallel.graph_provider_decision","version":1,"status":"verified","provider":"unknown.provider","representation":"dense"}' > "$decision"
if $flowmini --dump-frontend-bundle "$fixture" | "$analyst" | "$parallel" | "$optimizer" --provider-decision "$decision" >/dev/null 2>&1; then
    echo 'unsupported provider decision unexpectedly accepted' >&2
    exit 1
fi

if printf '%s' '{"format":"flowparallel.execution_plan","version":1,"status":"blocked"}' | "$optimizer" >/dev/null 2>&1; then
    echo 'rejected semantic report unexpectedly accepted' >&2
    exit 1
fi
echo 'Flowoptimize tests: PASS'
