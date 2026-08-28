#!/bin/sh
set -eu

cuda=${FLOWPARALLEL_GRAPH_CUDA_BIN:?FLOWPARALLEL_GRAPH_CUDA_BIN is required}
reference=${FLOWPARALLEL_GRAPH_REFERENCE_BIN:?FLOWPARALLEL_GRAPH_REFERENCE_BIN is required}
test -x "$cuda"; test -x "$reference"; command -v jq >/dev/null
fixture=$(mktemp)
trap 'rm -f "$fixture"' EXIT

printf '%s\n' '{"format":"flowparallel.graph_calibration","version":1,"status":"verified","cases":['
first=1
for n in 4 8 16; do
    for kind in sparse dense; do
        jq -c -n --argjson n "$n" --arg kind "$kind" '[range(0; $n) as $row | range(0; $n) as $column | select(($kind == "dense" and $row != $column) or ($kind == "sparse" and $column == ($row + 1))) | {row: $row, column: $column}] as $entries | {format:"flowanalyst.semantic_report", version:1, status:"ok", matrix_views:[{name:"region_dependency", rows:$n, columns:$n, entries:$entries}]}' > "$fixture"
        cpu=$($reference "$fixture")
        gpu=$($cuda "$fixture")
        jq -n --argjson cpu "$cpu" --argjson gpu "$gpu" -e '$gpu.status == "verified" and $gpu.reachable_pairs == $gpu.cpu_reachable_pairs and $gpu.reachable_pairs == $cpu.reachable_pairs' >/dev/null
        if [ "$first" -eq 0 ]; then printf ',\n'; fi
        first=0
        jq -c --arg kind "$kind" --argjson n "$n" '{size:$n, shape:$kind, reachable_pairs:.reachable_pairs, cpu_reference_ms:.cpu_reference_ms, cuda_end_to_end_ms:.cuda_end_to_end_ms, end_to_end_speedup:.end_to_end_speedup}' <<EOF
$gpu
EOF
    done
done
printf '%s\n' ']}'
echo 'Flowparallel graph calibration: 6/6 cases passed' >&2
