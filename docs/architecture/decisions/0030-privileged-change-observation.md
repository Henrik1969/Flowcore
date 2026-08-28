# ADR-0030: Privileged substrate-change observation

**Status:** accepted direction, provisional implementation
**Date:** 2026-08-20
**Scope:** Frankencore system-wide substrate awareness

## Decision

Privileged identity is an actor, not a permanent trust exemption. When UID 0,
an administrator, a package manager, an update service, or another privileged
provider changes substrate-relevant state, Frankencore should record the
change, explain its significance, and trigger policy-directed revalidation.

```text
privileged action observed
        ↓
identify actor/session/target
        ↓
record before/after evidence where possible
        ↓
classify substrate impact
        ↓
ask policy/resolver what must be revalidated
        ↓
continue, isolate, require confirmation, or fail closed
```

## Relevant changes

Observation should cover, where the substrate can provide reliable evidence:

- kernel, boot, firmware, module, and driver changes;
- package installation, removal, upgrade, and source changes;
- executable, library, interpreter, and loader changes;
- trust-store, key, policy, and authorization changes;
- service, container, namespace, filesystem, and network-boundary changes;
- security-control or audit-configuration changes;
- changes to the observers or their own trust anchors.

The system need not question every privileged command. It must identify actions
that can invalidate identity, provenance, capability, policy, or isolation
assumptions.

## Response

Substrate-change responses are policy-directed:

```text
inform       record and continue
revalidate   refresh affected profiles and trust facts
confirm      ask an authorized operator
isolate      reduce execution scope while uncertain
invalidate   withdraw affected trust/profile claims
fail_closed  stop sensitive operations until resolved
```

The decision includes actor, session, target, time, reason, affected profiles,
policy revision, and resulting provenance. Root may deliberately override a
default, but the override must remain visible and responsibility must be
recorded.

## Observation limits

User-space observers cannot guarantee awareness if a privileged actor replaces
the kernel, boot chain, audit mechanism, or observer itself. High-assurance
profiles therefore require an observation anchor outside the mutable substrate
when available, such as hardware-backed measurement, an immutable boot layer,
or an independent audit sink.

If observer integrity is uncertain, the system downgrades assurance rather than
claiming complete history.

## Revisit triggers

Revisit when implementing Linux audit adapters, package/boot observers,
hardware-backed measurements, independent audit sinks, or the first
system-wide revalidation service.
