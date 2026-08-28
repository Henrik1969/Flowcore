# ADR-0020: ConfigResolve as trust-decision arbiter

**Status:** accepted direction, integration future
**Date:** 2026-08-20
**Scope:** Frankencore trust-store resolution and policy

## Decision

ConfigResolve is the default mechanical arbiter for trust decisions. It does
not become the source of trust or an infallible authority; it resolves the
available facts according to explicit policy and delegates acquisition and
verification to adapters.

```text
XDG/project/system trust facts
        ↓
ConfigResolve loaders and adapters
        ↓
policy-governed resolution
        ↓
trust decision + diagnostics + explanation
        ↓
Frankencore admission and provenance
```

## Responsibilities

ConfigResolve provides:

- source precedence and scope resolution;
- conflict detection;
- policy and rule evaluation;
- diagnostics for missing, ambiguous, expired, or contradictory facts;
- explanation of the selected trust decision;
- transactional policy updates where supported.

Adapters provide:

- public-key and fingerprint discovery;
- signature verification;
- XDG trust-store loading;
- system/distribution trust-store integration;
- SSH-style key import through explicit policy;
- revocation, expiry, and rotation evidence;
- independent-channel confirmation evidence.

Frankencore provides the semantic trust states, admission rules, identity
correlation, isolation decisions, and provenance projection.

## Mechanical arbiter law

ConfigResolve must not guess when policies or evidence conflict. It returns an
explicit allow, deny, unresolved, or non-authoritative result with the facts,
policy revisions, adapters, and explanation used. A wrong policy can produce a
wrong decision, as in any governed human system, but the decision remains
inspectable, reproducible, and changeable through explicit policy.

## Revisit triggers

Revisit when implementing the Frankencore adapter, XDG trust-store schema,
signature verification provider, policy decision exchange, or delegated
multi-administrator trust.
