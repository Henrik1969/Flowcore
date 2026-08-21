#!/bin/sh
set -eu

planner=${FLOWPARALLEL_RUNTIME_PLANNER_BIN:?FLOWPARALLEL_RUNTIME_PLANNER_BIN is required}
plan=$(mktemp)
capabilities=$(mktemp)
calibration=$(mktemp)
trap 'rm -f "$plan" "$capabilities" "$calibration"' EXIT

printf '%s\n' '{"format":"flowparallel.execution_plan","version":1,"status":"ready"}' >"$plan"
printf '%s\n' '{"format":"frankencore.runtime_capabilities","version":1,"cpu":{"logical_processors":8},"memory":{"total_bytes":1,"available_bytes":1},"cuda":{"status":"available","driver":"test","device_count":1,"diagnostic":"ok"}}' >"$capabilities"
printf '%s\n' '{"format":"flowparallel.matrix_benchmark","status":"verified","end_to_end_speedup":3.0}' >"$calibration"

selected=$("$planner" --plan "$plan" --capabilities "$capabilities" --calibration "$calibration")
printf '%s\n' "$selected" | jq -e '.status == "selected" and .selection.provider == "cuda.cublas" and .fallback.required == true' >/dev/null

cpu=$("$planner" --plan "$plan" --capabilities "$capabilities" --calibration "$calibration" --min-speedup 4.0)
printf '%s\n' "$cpu" | jq -e '.selection.provider == "cpu.serial" and .evidence.measured_end_to_end_speedup == 3' >/dev/null

no_calibration=$("$planner" --plan "$plan" --capabilities "$capabilities")
printf '%s\n' "$no_calibration" | jq -e '.selection.provider == "cpu.serial" and .evidence.calibration_verified == false' >/dev/null

printf '%s\n' '{"format":"frankencore.runtime_capabilities","version":1,"cpu":{"logical_processors":8},"memory":{"total_bytes":1,"available_bytes":1},"cuda":{"status":"unavailable","driver":"","device_count":0,"diagnostic":"none"}}' >"$capabilities"
unavailable=$("$planner" --plan "$plan" --capabilities "$capabilities" --calibration "$calibration")
printf '%s\n' "$unavailable" | jq -e '.selection.provider == "cpu.serial" and .evidence.cuda_available == false' >/dev/null

echo 'Flowparallel runtime planner: PASS'
