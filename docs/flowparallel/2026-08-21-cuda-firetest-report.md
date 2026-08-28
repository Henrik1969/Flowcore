---
title: Flowparallel CUDA execution firetest report
status: passed-with-correction
date: 2026-08-21
---

# Flowparallel CUDA execution firetest

This campaign attacked the first real CUDA execution projection through the
host-level askpass path, so the test process could access the actual NVIDIA
device namespace.

## Boundary under test

```text
Flowanalyst graph/matrix metadata
  -> Flowparallel execution plan
  -> CUDA capability/provider contract
  -> cuBLAS matrix execution
  -> host result and independent baseline
```

The graph remains canonical. The matrix is a derived view, and the CPU remains
the required fallback.

## Campaign

Matrix sizes were:

```text
2, 3, 7, 16, 31, 64, 127, 256, 512
```

Each size was executed three times. The shell campaign independently computed
the expected result checksum and checked the JSON result, device count, status,
and maximum numerical error. It also attacked invalid sizes `0`, `1`, `4097`,
`-2`, and an unknown option. Finally, it verified the provider contract still
reported CUDA availability and retained CPU fallback.

## Result

| Check | Result |
|---|---:|
| GPU executions | PASS, 27/27 |
| Independent checksum comparison | PASS |
| Maximum numerical error | 0 |
| Repeated-run consistency | PASS |
| Invalid argument rejection | PASS |
| CUDA provider contract | PASS |
| Device count | 1 |
| GPU | NVIDIA GeForce RTX 5060 Ti |
| Driver | 580.173.02 |

## Defect exposed and corrected

The first campaign found nine apparent mismatches at sizes 127, 256, and 512.
The GPU results themselves were correct (`max_error: 0`); the JSON serializer
used default six-digit floating-point precision and truncated the checksum.
The execution artifact now emits sufficient precision for machine comparison.
The complete campaign was rerun afterward with zero failures.

## Reproduction

```sh
cmake --build Flowparallel/build --target flowparallel_cuda_execute
FLOWPARALLEL_CUDA_EXECUTE_BIN="$PWD/Flowparallel/build/flowparallel_cuda_execute" \
FLOWPARALLEL_CUDA_BIN="$PWD/Flowparallel/build/flowparallel_cuda" \
FLOWPARALLEL_PLAN=/path/to/flowparallel-plan.json \
  Flowparallel/tests/run-cuda-firetest.sh
```

The command must run in a context with access to the real CUDA device. A
restricted shell may report no `/dev/nvidia*` devices even when the desktop
session and host GPU are healthy.
