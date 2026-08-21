# ADR-0046: Declarative language-chain build policy

**Status:** accepted direction, provisional policy artifact  
**Date:** 2026-08-20  
**Scope:** Flowcore language chain, targets, prerequisites, and lowering

## Decision

The complete language chain may be directed by an explicit, inspectable build
policy. The policy declares prerequisites, supported substrate ranges, target
profiles, optimizer and lowering choices, and required capabilities.

```text
source
  → language map / dialect
  → prerequisites and environment facts
  → semantic analysis
  → optimizer policy
  → lowering policy
  → target artifact
```

The policy is data. Flowmini, Flowanalyst, Flowoptimize, Flowlower, and future
providers consume the relevant sections and report facts and diagnostics back
through the canonical pipeline.

## Example policy shape

```json
{
  "format": "frankencore.chain-policy",
  "version": "1",
  "name": "desktop-and-android",
  "language": {
    "map": "Flowmini",
    "dialect": "Flowmini.v25",
    "profile": "project-default"
  },
  "prerequisites": [
    {"capability": "llvm", "version": ">=18 <20"},
    {"capability": "cmake", "version": ">=3.20"},
    {"capability": "ninja", "version": ">=1.11"}
  ],
  "targets": [
    {
      "name": "windows-desktop",
      "substrate": "windows",
      "version": ">=11",
      "optimizer": "safe-default",
      "lowering": "llvm-windows-x86_64"
    },
    {
      "name": "mac-desktop",
      "substrate": "macos",
      "version": ">=14",
      "optimizer": "safe-default",
      "lowering": "llvm-macos-arm64"
    },
    {
      "name": "android",
      "substrate": "android",
      "version": ">=13",
      "optimizer": "safe-default",
      "lowering": "llvm-android-arm64"
    }
  ],
  "failure_policy": "diagnose-and-stop"
}
```

The exact schema is provisional. The important contract is that every
requirement, range, optimizer, lowering, and target is explicit and
inspectable.

## Resolution model

The chain resolves a policy against observed facts:

1. identify the language map and dialect;
2. inspect substrate and toolchain versions;
3. evaluate prerequisites and capability ranges;
4. select compatible target profiles;
5. run semantic analysis;
6. apply the selected optimizer policy;
7. apply the selected lowering provider;
8. emit artifacts and a policy/provenance report.

An unsatisfied prerequisite, ambiguous version range, unavailable provider, or
incompatible target produces an explicit diagnostic. The chain does not
silently substitute a different optimizer or lowering mechanism.

## Policy boundaries

Policies may select among compatible providers and add constraints. They may
not weaken constitutional safety laws, trust requirements, provenance rules,
or owner-attestation boundaries. A project may define a local policy; its
scope and deviations remain visible.

## Partial compilation

The policy supports partial compilation. Compatible targets may proceed while
incompatible targets produce isolated diagnostics and remain unresolved. A
final aggregate pass must still report the complete target matrix before a
multi-target release is considered successful.

## Revisit triggers

Revisit when defining the stable chain-policy schema, implementing version
range evaluation, adding multi-target lowering, or binding policies to
ConfigResolve.
