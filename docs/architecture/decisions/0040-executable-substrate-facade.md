# ADR-0040: Executable substrate facade pattern

**Status:** accepted direction, binding architectural pattern  
**Date:** 2026-08-20  
**Scope:** executable substrate capabilities

## Decision

Any executable substrate capability with a callable contract may receive a
Frankencore facade. The facade preserves the existing executable's default
identity and call behavior, then exposes additive policy- and schema-directed
behavior through explicit controls.

```text
existing executable / callable substrate
                    ↓
          compatibility-preserving facade
                    ↓
      policy + schema + provenance control plane
```

This is the general form of the `ls` replacement pattern. It applies to
ordinary executable files, package tools, helper programs, provider commands,
interpreter entry points, and other callable substrate boundaries where their
invocation contract can be identified.

## Default facade law

When invoked through the ordinary name and ordinary options, the facade must
preserve the declared native compatibility profile. Existing callers do not
receive Frankencore semantics merely because the executable is now resolved
through a facade.

The facade may add:

- policy selection;
- schema selection;
- structured projections;
- provenance and diagnostics;
- provider selection;
- isolation and execution controls;
- parallel or optimized execution policies.

These changes require explicit caller or resolved-policy participation.

## Applicability boundary

The executable permission alone does not prove that a safe facade is
possible. Before replacement, the provider must identify the relevant
contract: arguments, environment, streams, exit status, signals, filesystem
effects, IPC/ABI, privilege behavior, and reentrancy/locking rules.

If the contract cannot be observed or preserved, the capability remains an
adapter or separate projection rather than a transparent replacement.

## Identity and recovery

The facade records the native backend identity, selected policy and schema,
and invocation result. Backend resolution must not depend on an ambiguous
`PATH` lookup or recurse through the facade. Every facade requires a tested
rollback path and a declared compatibility profile.

## Architectural meaning

The facade is a projection layer over a capability, not a forced replacement
of the substrate. It allows FrankenPOP to add provenance, policy, and richer
consumers incrementally while the existing system remains recognizable and
usable.

## Revisit triggers

Revisit when implementing the first executable facade outside package tools,
when handling an ABI/IPC rather than a CLI contract, or when defining the
common facade discovery and backend-resolution mechanism.
