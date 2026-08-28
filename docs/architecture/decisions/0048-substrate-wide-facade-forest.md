# ADR-0048: Substrate-wide facade forest

**Status:** accepted direction, phased implementation  
**Date:** 2026-08-20  
**Scope:** system-wide executable facade adoption

## Decision

The `ls` facade is the first smoke test for a general substrate-wide facade
forest. The same compatibility pattern may eventually cover the executable
tools and callable providers that form the operating environment.

```text
substrate executable inventory
        ↓
contract/profile discovery
        ↓
user-local observe/delegate facade
        ↓
differential and failure testing
        ↓
scoped policy opt-in
        ↓
promoted FrankenPOP facade
```

The goal is not to replace the substrate blindly. The goal is for the system
to acquire Frankencore identity, provenance, policy, diagnostics, and richer
projections while remaining compatible with existing callers.

## Expansion rule

Each executable or callable capability is promoted independently:

1. inventory its native identity, version, ABI/CLI, and effects;
2. record a compatibility profile;
3. build a user-local facade that delegates to the native backend;
4. compare native and facade behavior;
5. add explicit Frankencore policy/schema behavior;
6. run failure, recovery, privilege, signal, and machine-consumer tests;
7. promote only the proven profile through explicit policy.

One facade's success does not imply another facade's compatibility. A
facade-specific failure remains isolated and must not damage the native
substrate or unrelated facades.

## Scope and rollout

The initial rollout is user-local and reversible. System-wide names such as
`/usr/bin/ls`, `apt`, or `dpkg` are not replaced until the corresponding
profile, rollback path, and policy are proven. XDG configuration selects
facades and backends during development; promotion to administrator-managed
locations is a separate explicit operation.

## Whole-system meaning

When enough facade profiles are proven, the substrate can be experienced as a
coherent FrankenPOP system without requiring every underlying tool to be
rewritten. Native tools remain available as backends, compatibility surfaces,
and recovery paths. Frankencore becomes the additive semantic and policy layer
over the executable forest.

## Revisit triggers

Revisit when the first facade inventory is complete, when a second command
family is promoted, when an ABI/service facade is attempted, or when a
system-wide promotion policy is designed.
