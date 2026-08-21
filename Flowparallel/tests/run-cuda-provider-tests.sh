#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
cuda=${FLOWPARALLEL_CUDA_BIN:?FLOWPARALLEL_CUDA_BIN is required}
planner=${FLOWPARALLEL_BIN:?FLOWPARALLEL_BIN is required}
flowmini=${FLOWMINI_BIN:-$root/Flowmini/flowmini_v25_symboltable_projection/cmake-build-debug/flowmini}
analyst=${FLOWANALYST_BIN:-$root/Flowanalyst/build/flowanalyst}
fixture="$root/Flowmini/flowmini_v25_symboltable_projection/examples/ast/parallel_independence_probe.flow"
test -x "$cuda"
test -x "$planner"
test -x "$flowmini"
test -x "$analyst"

plan=$("$flowmini" --dump-frontend-bundle "$fixture" | "$analyst" | "$planner")
report=$(printf '%s\n' "$plan" | "$cuda" --matrix-size 64)
printf '%s\n' "$report" | jq -e '
  .format == "flowparallel.cuda_selection" and
  .status == "ready" and
  .provider == "cuda" and
  .workload.operation == "matrix_multiply" and
  .transfer_cost.calibration == "runtime" and
  .execution == "not-performed" and
  .fallback.required == true
' >/dev/null
echo 'Flowparallel CUDA provider boundary: PASS'
