# ADR-0015: Delegation and system coherence

**Status:** accepted constitutional direction, provisional profiles
**Date:** 2026-08-20
**Scope:** Frankencore delegation, quarantine, and owner authority

## Decision

Frankencore preserves coherence through a small non-delegable floor and allows
owners, administrators, and root to choose operational policy above that floor.

```text
constitutional floor
    identity, contract versioning, provenance honesty, explicit status,
    diagnostics, and no silent authority escalation

delegable policy
    trust roots, disclosure, quarantine handling, execution permissions,
    provider choice, retention, and operational defaults
```

Delegation changes behavior and responsibility; it does not rewrite facts or
turn an unverified claim into a verified one.

## Non-delegable coherence laws

No owner or provider may cause the system to:

- present unknown or unverified material as verified;
- erase the identity or lineage of an acquired project;
- silently discard diagnostics or provenance;
- claim a capability that was not declared or verified;
- hide an applicable policy override;
- publish partial or ambiguous state as authoritative;
- confuse observation with authorization;
- confuse inspection with execution approval.

An owner may choose to accept risk, but the system must label that decision and
retain the responsible policy and provenance references.

## Delegable quarantine policy

The default profile places unknown and untrusted material in quarantine. An
owner or administrator may permit inspection, building, installation, or
execution at explicitly selected scopes, subject to the available mechanisms.

Each relaxation must identify:

- who authorized it;
- what material and scope it covers;
- which safeguards are bypassed;
- when it expires, if applicable;
- what profile/conformance impact results.

If the implementation cannot enforce a requested boundary, it must report
that limitation rather than claiming the boundary exists.

## Coherence rule

The system remains one coherent entity when its defaults are changed because
its identity, contracts, provenance, and declarations remain stable and
inspectable. Liberal operation is compatible with coherence when responsibility
and deviations are explicit.

## Revisit triggers

Revisit when implementing privilege separation, sandboxing, remote execution,
multi-user administration, or a distribution-wide security profile.
