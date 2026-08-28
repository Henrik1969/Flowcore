---
title: Flowparallel graph representation calibration
status: host-verified
date: 2026-08-21
---

# Result

The six-case host sweep compared the CPU Boolean reachability reference with
the CUDA/cuBLAS projection for sparse and dense graphs at sizes 4, 8, and 16.
All six cases agreed exactly on reachable-pair counts.

| Size | Shape | Reachable pairs | CPU ms | CUDA end-to-end ms | Speedup |
|---:|---|---:|---:|---:|---:|
| 4 | sparse | 6 | 0.002691 | 188.175 | 0.0000143x |
| 4 | dense | 16 | 0.001137 | 138.871 | 0.0000082x |
| 8 | sparse | 28 | 0.003289 | 137.736 | 0.0000239x |
| 8 | dense | 64 | 0.003714 | 141.404 | 0.0000263x |
| 16 | sparse | 120 | 0.016526 | 142.549 | 0.000116x |
| 16 | dense | 256 | 0.022208 | 144.631 | 0.000154x |

## Decision

The current evidence does not justify CUDA for these graph sizes. The runtime
planner must retain `cpu.reference` under the default `1.25` minimum speedup
policy. This is expected: the current CUDA provider performs repeated dense
matrix multiplications and host/device transfers, so launch and transfer
overhead dominate small graphs.

This report is a calibration artifact, not a universal performance claim. A
future provider may use sparse kernels, batching, persistent device residency,
or a different algebra implementation. Such a provider must produce new
versioned evidence and still pass the CPU differential gate.

Reproduce with:

```sh
FLOWPARALLEL_GRAPH_CUDA_BIN=/tmp/flowparallel-build/flowparallel_graph_cuda \
FLOWPARALLEL_GRAPH_REFERENCE_BIN=/tmp/flowparallel-build/flowparallel_graph_reference \
  Flowparallel/tests/run-graph-calibration-host-test.sh
```
