# ADR-0001: Mutation event envelope and semantic result types

**Status:** accepted direction, provisional implementation
**Date:** 2026-08-20
**Scope:** Frankencore mutation provenance

## Decision

Successful state publication and rejected mutation attempts remain separate
C++ semantic types:

- `MutationRecord` represents a successfully published state transition.
- `MutationRejection` represents an attempted transition that was not
  published.
- `MutationAttempt` contains metadata shared by both results.

The eventual transport and inspection model should use a shared
`frankencore.mutation_event` envelope. The current
`frankencore.mutation_record/1` success projection remains valid for
compatibility until the shared envelope has sufficient consumers and test
coverage.

## Rationale

The semantic distinction prevents rejected work from being mistaken for
published state, while a shared future envelope gives event streams,
inspectors, and generic tooling one transport-level model.

The C++ types remain authoritative. JSON is a projection and must not define
the in-process state model.

## Current implementation

The core API is in `Frankencore/Core/Provenance` and is exposed as the CMake
target `Frankencore::Provenance`.

Current projections are:

- `frankencore.mutation_record/1` for committed records;
- `frankencore.mutation_event/1` with `status: rejected` for rejections.

## Revisit triggers

Revisit this decision when at least one of the following is true:

- a durable event stream consumes both result kinds;
- rollback, supersession, or retry events require common transport semantics;
- replay or audit tooling needs one envelope;
- versioned consumers exist that can migrate from `mutation_record/1`;
- retention and correlation policy have been specified.

Any migration must preserve the old success projection for an explicitly
documented compatibility period.

## Identity requirements

Every emitted result has a mandatory `event_id`. Every execution attempt has
mandatory `attempt_id` and `correlation_id` values. Retries create new event
and attempt identities while retaining the correlation identity.

All of these identities use the core-owned canonical ULID grammar. Callers may
attach labels, but they do not define alternate identifier formats.
