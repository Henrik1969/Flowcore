# ADR-0019: Scoped trust bootstrap

**Status:** accepted direction, implementation future
**Date:** 2026-08-20
**Scope:** Frankencore trust anchors and first contact

## Decision

Trust anchors are established by explicit, scoped policy rather than by the
mere presence of a key in acquired material. A project may carry its public
key and fingerprint, but that material is an assertion until independently
confirmed or deliberately accepted under a local policy.

## Bootstrap sources

Trust may originate from:

```text
system roots       distribution/vendor scope
user/admin roots   explicitly configured local scope
owner keys         project scope after confirmation
TOFU keys          provisional first-contact scope
SSH-style keys     source/transport identity through an explicit adapter
```

No source automatically grants universal authority. A trusted key is:

```text
public key + independently established trust decision + explicit scope
```

## First-contact procedure

For a new project or provider:

1. acquire the material and public key;
2. compute and display the key fingerprint;
3. confirm the fingerprint through an independent channel where practical;
4. store the trust decision with project, scope, and policy identity;
5. verify subsequent profiles, packages, and provenance against the key;
6. require explicit policy for rotation, replacement, or emergency recovery.

TOFU may support discovery and provisional operation, but is not authoritative
for sensitive actions by default.

## Self-authored material

Self-authorship does not imply safety or correctness. A key controlled by the
owner can still sign a broken, compromised, or dangerously configured
artifact. Testing, inspection, isolation, policy checks, and independent
verification remain valid for owner-created material.

The system must make the trust decision and operating responsibility visible
without treating the owner as infallible.

## Revisit triggers

Revisit when implementing trust-store persistence, key confirmation UX,
offline/fallback verification, recovery after key loss, or multi-device owner
identity.
