# ADR 0050: Runtime specialization is an optional deployment projection

## Status

Accepted as the deployment direction; implementation begins with the
Flowparallel plan boundary and Frankencore runtime discovery.

## Decision

A deployed Flowcore artifact may contain portable lowering output, an
inspectable execution plan, and one or more provider strategies. A Frankencore
runtime may discover local capabilities and specialize or JIT a permitted
strategy at startup.

Runtime specialization is additive. It must not be required for correctness,
and the artifact must retain a valid serial or otherwise conservative fallback.
The runtime may cache a specialized result only with a provenance link to the
source artifact, plan, capability snapshot, policy revision, and provider.

## Relation to Java-style deployment

This provides the useful part of a managed runtime/JIT model without requiring
Flowcore to become a single VM:

- the artifact is portable and carries intermediate execution knowledge;
- runtime discovery resolves the local machine;
- policy authorizes provider use;
- a runtime provider may specialize hot or eligible regions;
- native fallback remains available.

The runtime is a capability provider and projection layer, not a new semantic
authority. It cannot reinterpret an unresolved semantic result as executable.

## Required runtime record

Every specialization decision should be able to identify:

```text
source artifact
execution-plan revision
runtime capability snapshot
policy revision
selected provider
fallback provider
specialized artifact digest
```

If a runtime cannot explain or safely cache a specialization, it must execute
the fallback or report an explicit unresolved state.
