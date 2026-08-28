# ADR-0006: ConfigResolve as the policy-resolution substrate

**Status:** accepted direction, integration not yet implemented
**Date:** 2026-08-20
**Scope:** Frankencore policy and configuration resolution

## Decision

Frankencore uses ConfigResolve as its general policy-resolution mechanism. It
must not grow a competing policy engine inside the Frankencore core.

Frankencore supplies an adapter that translates its scoped, authoritative
policy facts into ConfigResolve inputs and translates the resolved decision,
diagnostics, and explanation back into Frankencore contracts.

```text
Frankencore policy facts
        ↓
ConfigResolve adapter
        ↓
ConfigResolve facts + PolicySet
        ↓
mechanical resolution
        ↓
decision + diagnostics + explanation
        ↓
Frankencore policy/provenance projection
```

## Existing substrate capability

ConfigResolve already provides:

- facts with source and precedence;
- layered source resolution;
- explicit conflict policies;
- type, range, and allow-list validation;
- diagnostics as first-class output;
- provenance and explanation of chosen values;
- scoped read-only views;
- transactional runtime mutation;
- C and C++ consumption boundaries.

These are the mechanism capabilities. Frankencore remains responsible for
semantic authority, constitutional limits, scope, policy identity, issuer,
revision, disclosure classification, and decision provenance.

## Adapter boundary

The adapter must preserve:

- policy identity and revision;
- issuer and authority level;
- scope and target;
- allow/deny effect and conditions;
- source provenance;
- conflict and fail-closed behavior;
- selected policies and explanation;
- error-state and mutation-event correlation identities.

The adapter must not silently discard a policy field that ConfigResolve cannot
yet represent. Unsupported semantics produce an explicit diagnostic and a
non-authoritative result until the substrate or adapter is extended.

ConfigResolve remains generic and Frankencore-agnostic. Frankencore-specific
meaning belongs in the adapter and project policy layers.

## Revisit triggers

Revisit if ConfigResolve cannot represent the required authority model without
weakening its generic contract, or when another independently mature resolver
provides a materially stronger compatible capability.
