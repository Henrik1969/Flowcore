# Typed artifact-contract progression

## Architectural progression

```text
v0.27 reusable native chain
  -> v0.28 typed and independently validated artifact boundaries
  -> v0.29 Canonical Graph IR
  -> v0.30 executable admission and safety laws
```

v0.28 hardens meaning already carried by the compiler chain. It does not widen
the surface language or claim that Canonical Graph IR already exists.

Implementation status on 2026-08-26: the v0.28 progression below is complete
for the current required chain and adjacent authority-bearing planners. See the
[current status](../checkpoints/2026-08-26-v0.28-typed-artifact-contracts.md)
and maturation ledger for executable evidence.

## Implementation progression

Use vertical, independently testable slices:

```text
strict JSON value/parser
  -> common envelope, identity, provenance, diagnostic primitives
  -> one complete artifact ADT and canonical serializer
  -> producer -> flowvalidate -> consumer proof
  -> next artifact boundary
```

Shared contracts own structural validity, versioning, typed public identities,
deterministic representation, and common cross-field invariants. Compiler stages
retain stage-specific semantic admission. The independent validator must not
link private stage implementations.

## Compatibility decisions

- Unknown-field behavior is explicit per artifact/version, never accidental.
- Ordered arrays remain ordered; semantic sets receive documented canonical
  ordering before serialization.
- Canonical JSON must define number and string escaping deterministically so it
  can later support stable evidence identities.
- Positive `c_pointer(N)` remains provisional compatibility behavior.
