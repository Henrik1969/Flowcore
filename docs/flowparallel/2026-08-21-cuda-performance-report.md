---
title: Flowparallel CUDA performance baseline
status: measured
date: 2026-08-21
---

# CUDA versus CPU baseline

This is a local measurement on the NVIDIA GeForce RTX 5060 Ti using driver
580.173.02 and cuBLAS. It compares the same matrix multiplication against an
optimized single-thread CPU baseline. Every sample matched numerically with
maximum error `0`.

## Results

| Matrix | CPU ms | CUDA compute ms | CUDA end-to-end ms | End-to-end speedup |
|---:|---:|---:|---:|---:|
| 32×32 | 0.0147 | 0.0078 | 0.0203 | 0.72× |
| 64×64 | 0.1382 | 0.0080 | 0.0448 | 3.08× |
| 128×128 | 1.1781 | 0.0090 | 0.0464 | 25.37× |
| 256×256 | 8.0757 | 0.0152 | 0.1181 | 68.35× |
| 512×512 | 61.4661 | 0.0691 | 0.5211 | 117.94× |

End-to-end timing includes host-to-device copies, cuBLAS execution,
synchronization, and device-to-host copy-back. The CPU baseline is one thread.

## Decision

Yes, this workload gains substantial performance on the available GPU, but not
for every size. The 32×32 case loses because transfer and launch overheads
dominate. The 64×64 case is already profitable on this machine, but that is
evidence—not a universal compile-time threshold.

The optimizer must therefore select CUDA only when runtime calibration confirms
the expected benefit under local policy. Otherwise it uses the CPU provider.
The current plan contract already carries the required policy inputs:

```text
runtime capability + measured cost benefit + provider contract
→ CUDA or CPU fallback
```

## Reproduction

```sh
cmake --build Flowparallel/build --target flowparallel_matrix_benchmark
sudo -A Flowparallel/build/flowparallel_matrix_benchmark --size 512 --iterations 5
```

The benchmark is hardware-, driver-, thermal-, clock-, and workload-dependent;
these numbers are local evidence, not portable performance promises.
