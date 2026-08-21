# ADR-0027: Operation risk and assurance selection

**Status:** accepted direction, provisional profiles
**Date:** 2026-08-20
**Scope:** Frankencore operation admission and broker assurance

## Decision

Operation risk is derived from explicit properties rather than a universal
numeric trust score.

An operation may declare:

```text
resource sensitivity
required privilege
network exposure
trust-domain distance
reversibility
blast radius
data disclosure
persistence
foreign-code execution
```

The resolver classifies the operation into:

```text
routine
protected
high
critical
```

## Default profiles

```text
routine:
    local IPC or in-process operation; ordinary policy resolution

protected:
    authenticated local broker; restricted resources; explicit provenance

high:
    verified isolation; signed/audience-bound assertion; replay protection;
    independent verification where available

critical:
    strongest compatible isolation; independent verification; explicit
    operator approval; multi-channel or multi-party authorization when policy
    requires it
```

## Selection law

Risk is monotonic: higher consequence must never receive weaker assurance.
When operation properties conflict, the stricter profile wins. If the system
cannot classify an operation, it treats it as `high` or `unresolved`, never as
`routine`.

ConfigResolve mechanically combines:

```text
operation properties
+ project policy
+ system/user policy
+ provider capabilities
+ available evidence
= required assurance profile
```

The admission result is one of:

```text
allowed
allowed_with_isolation
requires_confirmation
unresolved
denied
```

Any relaxation records the selected policy, responsibility, reduced assurance,
and provenance.

## Revisit triggers

Revisit when defining risk-property schemas, critical-operation profiles,
policy negotiation, or automated assurance selection.
