#!/bin/sh
set -eu
planner=${FLOWPARALLEL_GRAPH_PLANNER_BIN:?FLOWPARALLEL_GRAPH_PLANNER_BIN is required}
test -x "$planner"
graph=$(mktemp); capabilities=$(mktemp); calibration=$(mktemp)
trap 'rm -f "$graph" "$capabilities" "$calibration"' EXIT
printf '%s\n' '{"format":"flowanalyst.semantic_report","version":1,"status":"ok","matrix_views":[{"name":"region_dependency","rows":4,"columns":4,"entries":[{"row":0,"column":1},{"row":2,"column":3}]}]}' > "$graph"
printf '%s\n' '{"format":"flowcore.runtime_capabilities","version":1,"status":"available","device_count":1}' > "$capabilities"
printf '%s\n' '{"format":"flowparallel.graph_cuda","version":1,"status":"verified","end_to_end_speedup":10.0}' > "$calibration"
result=$("$planner" --graph "$graph" --capabilities "$capabilities" --calibration "$calibration")
printf '%s\n' "$result" | jq -e '.provider == "cpu.reference" and .representation == "sparse" and .graph.edges == 2' >/dev/null
result=$("$planner" --graph "$graph" --capabilities "$capabilities" --calibration "$calibration" --density-threshold 0.01)
printf '%s\n' "$result" | jq -e '.provider == "cuda.cublas.boolean_threshold" and .representation == "dense"' >/dev/null
result=$("$planner" --graph "$graph" --capabilities "$capabilities" --density-threshold 0.01)
printf '%s\n' "$result" | jq -e '.provider == "cpu.reference" and (.reason | contains("no verified CUDA calibration"))' >/dev/null
printf '%s\n' '{"format":"flowcore.runtime_capabilities","version":1,"status":"unavailable","device_count":0}' > "$capabilities"
result=$("$planner" --graph "$graph" --capabilities "$capabilities" --calibration "$calibration" --density-threshold 0.01)
printf '%s\n' "$result" | jq -e '.provider == "cpu.reference" and (.reason | contains("CUDA is unavailable"))' >/dev/null
echo 'Flowparallel graph planner: PASS'
