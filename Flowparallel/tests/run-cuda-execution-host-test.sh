#!/bin/sh
set -eu

cuda_execute=${FLOWPARALLEL_CUDA_EXECUTE_BIN:?FLOWPARALLEL_CUDA_EXECUTE_BIN is required}
test -x "$cuda_execute"

report=$("$cuda_execute" --size "${FLOWPARALLEL_CUDA_MATRIX_SIZE:-64}")
printf '%s\n' "$report"
printf '%s\n' "$report" | jq -e '
  .format == "flowparallel.cuda_execution" and
  .status == "verified" and
  .provider == "cuda.cublas" and
  .device_count >= 1 and
  .max_error < 0.001
' >/dev/null
echo 'Flowparallel CUDA execution host gate: PASS'
