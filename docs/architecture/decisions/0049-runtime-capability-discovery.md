# ADR 0049: Runtime capability discovery is factual and policy-neutral

## Status

Accepted for the first runtime capability slice.

## Decision

Flowcore artifacts may query a read-only Frankencore runtime provider at
startup. The provider emits versioned capability facts for the current host,
including logical CPU capacity, memory, and optional CUDA driver/device
availability.

The provider does not select a parallel strategy, create workers, enable CUDA,
or override policy. A later runtime planner combines these facts with the
compiled execution plan and active policy.

The inspectable projection is defined by
`docs/architecture/schemas/frankencore-runtime-capabilities-v1.json` and is
implemented by `Frankencore::Runtime`.

## Consequences

Positive:

- artifacts remain portable across machines with different hardware;
- CPU-only and CUDA-capable hosts can use the same compiled plan;
- discovery, policy, and execution remain separate capabilities;
- unavailable or uncertain providers become explicit runtime facts.

Trade-offs:

- provider choice is deferred to runtime;
- artifacts must carry compatible fallback strategies;
- capability facts can change during process lifetime and should be treated as
  a startup snapshot unless a provider explicitly supports refresh.

## Invariant

Compile time proves what execution strategies are legal. Runtime discovery
reports what exists here. Policy decides what is permitted. No layer may infer
another machine's hardware from the build host.
