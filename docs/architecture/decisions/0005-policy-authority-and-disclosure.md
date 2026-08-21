# ADR-0005: Policy authority and disclosure decisions

**Status:** accepted direction, provisional implementation
**Date:** 2026-08-20
**Scope:** Frankencore policy resolution and identity disclosure

## Decision

Policies are not equal. Every applicable policy must have an explicit:

- policy identity and revision;
- issuer/authority;
- scope and target;
- effect and conditions;
- validity period, if applicable;
- precedence or authority level;
- explanation suitable for a decision record.

The resolver must produce one unambiguous decision or a fail-closed result.

## Authority model

Authority is layered and bounded:

```text
constitutional law     defines non-overridable boundaries
system policy           defines machine/environment limits
project policy          defines project meaning and disclosure
provider policy         defines provider capability constraints
operation policy        restricts one invocation or session
```

A lower layer may restrict a higher layer's permission but may not silently
broaden it. A provider cannot grant access forbidden by project or system
policy. An operation may further restrict access for safety.

## Conflict resolution

The resolver applies these rules in order:

1. Reject policies that are malformed, expired, out of scope, or unverifiable.
2. Apply constitutional prohibitions first; they cannot be overridden.
3. Require every applicable authority layer to permit a sensitive action.
4. Treat any explicit applicable denial as a denial.
5. Treat same-authority contradictions as an unresolved conflict, not as an
   arbitrary ordering.
6. Default to denial when no policy authorizes disclosure or capability use.
7. Emit the selected policies, revisions, conflict result, and explanation in
   the decision provenance.

This makes policy composition restrictive by default and prevents accidental
authority escalation.

## Disclosure

Identity may be public while associated metadata is policy-controlled. A
consumer receives the least-privileged projection authorized for its identity,
capability, purpose, and scope. Restricted resolution requires an explicit
authority and produces an explainable decision record.

## Revisit triggers

Revisit when implementing delegated authority, cross-project trust, signed
policies, emergency override, or policy federation.
