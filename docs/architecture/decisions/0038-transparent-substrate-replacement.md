# ADR-0038: Transparent substrate replacement

**Status:** accepted direction, future implementation  
**Date:** 2026-08-20  
**Scope:** Frankencore replacement of substrate command projections

## Decision

Frankencore may eventually replace a substrate command through a
compatibility-preserving projection. An external caller should be able to
invoke the Frankencore projection under the established command name and
continue to receive the expected behavior.

```text
caller / script / package tool
             ↓ unchanged command contract
      FrankenPOP apt-compatible projection
             ↓ policy, provenance, diagnostics
       native apt/dpkg/nala backend or replacement
```

The first implementation must be a delegating proxy. It preserves the native
tool as the fallback backend while Frankencore learns and proves the boundary.
Only later may individual capabilities be replaced by native Frankencore
implementations.

## Compatibility contract

An apt-compatible projection must preserve, for the supported command profile:

- command name and accepted argument forms;
- environment and working-directory behavior;
- standard input, output, and error stream meaning;
- exit-status conventions;
- signal and cancellation behavior;
- locking and privilege expectations;
- machine-readable output modes used by scripts;
- documented file and database side effects;
- diagnostics sufficient for existing callers and Frankencore consumers.

Compatibility is declared per command, option, and version profile. The
projection must not claim universal compatibility while unsupported behavior
remains delegated or unresolved.

## Backend and recursion law

Replacing a command name must retain an unambiguous reference to the native
backend. The proxy must not rediscover itself through `PATH`, recurse through
the symlink, or accidentally invoke a different provider after an update.
Backend identity, path, version, and invocation result are recorded.

The native backend remains the initial operational authority. Direct editing
of dpkg or APT state is forbidden. The proxy may inspect, authorize, observe,
and delegate; mutation semantics remain those of the selected backend until a
separate replacement capability is proven.

## Rollout and recovery

Transparent replacement is introduced per capability and canary profile:

1. observe native behavior;
2. delegate through the compatibility proxy;
3. compare declared and observed results;
4. enable additive Frankencore behavior for a scoped operation;
5. retain an explicit rollback path to the native backend.

An incompatibility, ambiguous result, or failed recovery must produce a
diagnosis and stop or delegate according to resolved policy. A symlink alone
is not a sufficient rollout or rollback mechanism.

## Boundary

This decision concerns command compatibility, not permission to impersonate a
supplier or weaken trust. Verification, owner attestation, policy authority,
and provenance laws remain unchanged.

## Revisit triggers

Revisit when implementing the first apt-compatible proxy, selecting the
backend-discovery mechanism, or defining the first command compatibility
profile and conformance tests.
