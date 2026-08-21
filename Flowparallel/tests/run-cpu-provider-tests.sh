#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
cpu=${FLOWPARALLEL_CPU_BIN:?FLOWPARALLEL_CPU_BIN is required}
planner=${FLOWPARALLEL_BIN:?FLOWPARALLEL_BIN is required}
flowmini=${FLOWMINI_BIN:-$root/Flowmini/flowmini_v25_symboltable_projection/cmake-build-debug/flowmini}
analyst=${FLOWANALYST_BIN:-$root/Flowanalyst/build/flowanalyst}
fixture="$root/Flowmini/flowmini_v25_symboltable_projection/examples/ast/parallel_independence_probe.flow"
test -x "$cpu"
test -x "$planner"
test -x "$flowmini"
test -x "$analyst"

plan=$("$flowmini" --dump-frontend-bundle "$fixture" | "$analyst" | "$planner")
parallel=$(printf '%s\n' "$plan" | "$cpu" --observed-speedup 2.0 --minimum-speedup 1.25 --workers 4)
printf '%s\n' "$parallel" | jq -e '.status == "ready" and .decision == "parallel" and .provider == "cpu.threadpool" and .workers >= 2 and .execution == "not-performed"' >/dev/null

serial=$(printf '%s\n' "$plan" | "$cpu" --observed-speedup 0.5 --minimum-speedup 1.25 --workers 4)
printf '%s\n' "$serial" | jq -e '.status == "ready" and .decision == "serial" and .provider == "cpu.serial" and .workers == 1' >/dev/null

set +e
blocked=$(printf '%s' '{"format":"flowparallel.execution_plan","version":1,"status":"blocked"}' | "$cpu")
blocked_rc=$?
set -e
test "$blocked_rc" -eq 2
printf '%s\n' "$blocked" | jq -e '.status == "blocked"' >/dev/null
echo 'Flowparallel CPU provider: PASS'
