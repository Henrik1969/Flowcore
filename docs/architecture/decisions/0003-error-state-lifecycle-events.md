# ADR-0003: Error-state lifecycle events

**Status:** accepted direction, provisional implementation
**Date:** 2026-08-20
**Scope:** Frankencore operational recovery and provenance

## Decision

Error-state lifecycle is represented by a dedicated semantic event type,
`ErrorStateEvent`, rather than by overloading mutation statuses.

Every error state has a mandatory `error_state_id` ULID. Each lifecycle event
also has its own `event_id` and refers to the relevant `attempt_id` and
`correlation_id`.

The core semantic families are:

```text
MutationRecord       committed state transition
MutationRejection    rejected state transition attempt
ErrorStateEvent      operational failure and resolution lifecycle
```

The eventual transport may place all three inside the shared
`frankencore.mutation_event` envelope, but their semantic types remain
distinct.

## Lifecycle

An error state may move through:

```text
opened
diagnosed
recovery_attempted
resolved
escalated
reopened
```

The event must identify the error state, related event/attempt/correlation
identities, diagnosis, attempted recovery, outcome, and whether operator
action was required. Resolving an error state is not automatically a mutation.

Ownership is layered:

```text
core       structural validity and status vocabulary
resolver   legal lifecycle transitions and ordering
policy     authorization for exceptional transitions
history    immutable record of what actually happened
```

The core must not enforce the resolver's transition graph. For example, it
may represent `reopened` structurally even when the normal resolver rejects
that transition. An exceptional transition requires explicit policy
authorization and a recorded reason.

## Extensible status vocabulary

The foundational statuses are additive vocabulary, not a closed enum. The
core reserves the established statuses and validates only structural status
requirements. A project, provider, or user may introduce an additional status
when it does not violate constitutional laws, capability boundaries, or
applicable policy. The resolver and policy layer must understand or explicitly
reject the addition before it becomes authoritative history.

Extensions should use a documented owner or namespace when collision is
possible. Adding a status must not silently change the meaning of an existing
status or bypass lifecycle, authorization, provenance, or recovery rules.

## Status discovery and authority

Project metadata defines the meaning, lifecycle rules, and policy for an
extension status. Providers declare the statuses they can emit or support.
The resolver verifies that the provider declaration matches the project
definition before authoritative publication:

```text
project vocabulary
        ↓
provider capability claim
        ↓
resolver validation
        ↓
history publication
```

A provider may propose or emit a status, but it does not define project
semantics alone. If its declaration is absent, incompatible, or ambiguous,
the resolver rejects authoritative publication or records the result as a
non-authoritative unrecognized event.

## Scope law

Declaration scope follows capability scope:

```text
project-specific meaning       project metadata
shared provider capability     provider/project integration metadata
universal capability           discoverable system-level contract
```

A declaration must not be promoted to a broader registry merely because it
could be useful there. It becomes broader only when independent consumers
need a stable shared meaning and the relevant authority, versioning, and
compatibility rules have been defined at that scope.

The scope owner is responsible for explaining who should consume the
declaration, what authority it carries, and how it is versioned. Narrow scope
is the default; promotion is an explicit architectural decision.

## Temporary artifacts

Diagnostics, quarantine fragments, recovery plans, and repair transcripts may
belong to the unresolved error state. They remain visible while unresolved and
may be removed after explicit resolution. Permanent history retains the error
state lifecycle and outcome, not the temporary repair material.

## Rationale

Recovery is an operational lifecycle, not necessarily a state transition.
Keeping it separate prevents audit tools, replay engines, and consumers from
mistaking repair activity for ordinary project mutation.

This separation prevents lifecycle policy from bleeding into the neutral core
and keeps project-specific recovery, migration, and administrative workflows
possible without weakening the permanent audit trail.

## Revisit triggers

Revisit when implementing the first durable error-state store or when recovery
needs cross-project escalation, long-term retention, or automated remediation.
