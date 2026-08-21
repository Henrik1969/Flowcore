# ADR 0051: Runtime provider decisions require facts and calibration

## Status

Accepted for the first Flowparallel provider-planner slice.

## Decision

Provider selection consumes four explicit inputs:

```text
execution plan
runtime capability snapshot
active policy
verified calibration
```

CUDA may be selected only when its capability is available, the calibration is
verified, and measured end-to-end benefit meets the active minimum-speedup
policy. Otherwise the planner selects the serial CPU provider and records the
reason. The CPU fallback remains mandatory even when CUDA is selected.

The planner emits `flowparallel.provider_decision` v1. It does not discover
hardware during compile-time optimization and does not treat the build host as
evidence about a deployment host.

## Consequences

Positive:

- small workloads can remain on CPU when transfer overhead dominates;
- unavailable, uncertain, or policy-denied CUDA degrades explicitly;
- provider choices are explainable and reproducible from recorded inputs;
- calibration thresholds remain runtime and policy controlled.

Trade-offs:

- a missing calibration prevents CUDA selection even when a GPU exists;
- the first version uses a scalar minimum-speedup policy;
- provider selection and actual graph-algebra execution remain separate gates.

## Invariant

No layer may claim that CUDA is the best provider merely because CUDA exists.
Availability is necessary evidence, not sufficient evidence.
