#!/bin/sh
set -eu

cuda=${FLOWPARALLEL_GRAPH_CUDA_BIN:?FLOWPARALLEL_GRAPH_CUDA_BIN is required}
reference=${FLOWPARALLEL_GRAPH_REFERENCE_BIN:?FLOWPARALLEL_GRAPH_REFERENCE_BIN is required}
flowmini=${FLOWMINI_BIN:?FLOWMINI_BIN is required}
analyst=${FLOWANALYST_BIN:?FLOWANALYST_BIN is required}
fixture=${FLOW_GRAPH_FIXTURE:?FLOW_GRAPH_FIXTURE is required}
test -x "$cuda"; test -x "$reference"; test -x "$flowmini"; test -x "$analyst"

bundle=$($flowmini --dump-frontend-bundle "$fixture")
semantic=$(printf '%s\n' "$bundle" | "$analyst")
cpu=$(printf '%s\n' "$semantic" | "$reference")
gpu=$(printf '%s\n' "$semantic" | "$cuda")
printf '%s\n' "$gpu"
jq -n --argjson cpu "$cpu" --argjson gpu "$gpu" -e '$gpu.status == "verified" and $gpu.provider == "cuda.cublas.boolean_threshold" and $gpu.reachable_pairs == $cpu.reachable_pairs and $gpu.device_count >= 1' >/dev/null
echo 'Flowparallel CUDA graph differential: PASS'
