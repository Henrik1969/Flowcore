# Frankencore Current Conformance Status

**Status:** audit baseline  
**Date:** 2026-08-20  
**Authority:** repository state and executable gates at audit time

This document supersedes historical conformance claims for present-tense
status. Historical declarations remain valuable evidence for their recorded
revisions but are not current implementation authority.

## Current assessment

| Area | Status |
| --- | --- |
| FlowMini structural projection | implemented and versioned |
| Independent semantic consumer | implemented and versioned |
| Policy-authorized ABI provider boundary | narrow prototype implemented |
| Optimization boundary | implemented identity boundary |
| LLVM/ELF lowering | narrow profiles implemented |
| Frankencore constitution | proposed; baseline now documented |
| General capability meta-model | not implemented |
| Lifecycle/error meta-model | stage-local; not generalized |
| Mutation provenance | C++20 core API, JSON projection, and contract tests implemented |
| Clock reference object | implemented Linux adapter/CLI reference |
| Provider selection explainability | not generalized |
| Forbidden dependency enforcement | strict architecture check passes |
| Constitutional conformance harness | implemented and passing |
| Canonical Graph IR | not implemented |

## Evidence

At audit time, the active CTest gates passed:

```text
FlowMini:       2/2
Flowanalyst:    1/1
Flowbind:       2/2
Flowoptimize:   1/1
Flowlower:     3/3
```

The Flowtools CLion/Ninja flowcat project also configured and lowered
successfully.

The Flowtools Clock reference object builds and passes monotonic, realtime, and
unknown-clock failure gates. Its report exposes semantic identity, properties,
capability version, provider/backend identity, and policy decision.

The reusable `flowcore_conformance` target checks constitutional invariant
presence, contract inventory syntax, strict architecture boundaries, the Clock
contract, fail-closed capability-policy behavior, and the mutation-provenance
evidence contract. The latter demonstrates the required evidence shape
without selecting a runtime storage model.

## Current limitation

This is a conformance status for the repository's experimental implementation,
not a production security, portability, or complete Frankencore claim.
