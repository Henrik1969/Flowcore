#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
bin=${FLOWPARALLEL_BIN:?FLOWPARALLEL_BIN is required}
flowmini=${FLOWMINI_BIN:-$root/Flowmini/flowmini_v25_symboltable_projection/cmake-build-debug/flowmini}
analyst=${FLOWANALYST_BIN:-$root/Flowanalyst/build/flowanalyst}
fixture="$root/Flowmini/flowmini_v25_symboltable_projection/examples/ast/parallel_independence_probe.flow"
test -x "$bin"
test -x "$flowmini"
test -x "$analyst"

report=$("$flowmini" --dump-frontend-bundle "$fixture" | "$analyst" | "$bin")
printf '%s\n' "$report" | jq -e --arg source "$fixture" '
  .format == "flowparallel.execution_plan" and
  .version == 1 and
  .status == "ready" and
  .source.path == $source and
  .cost_model.status == "deferred" and
  .cost_model.minimum_speedup == 1.25 and
  .provider_selection.status == "deferred" and
  .dependency_analysis.parallel_candidates == 2 and
  .fallback.required == true and
  .runtime.capabilities_format == "frankencore.runtime_capabilities"
' >/dev/null

set +e
blocked=$(printf '%s' '{"format":"flowanalyst.semantic_report","version":1,"status":"error"}' | "$bin")
blocked_rc=$?
set -e
test "$blocked_rc" -eq 2
printf '%s\n' "$blocked" | jq -e '.status == "blocked"' >/dev/null
echo 'Flowparallel tests: PASS'
