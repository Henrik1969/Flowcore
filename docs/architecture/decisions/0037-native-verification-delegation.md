# ADR-0037: Native verification delegation

**Status:** accepted direction, provisional integration contract  
**Date:** 2026-08-20  
**Scope:** Frankencore verification providers

## Decision

Frankencore delegates substrate-native verification to the mechanism already
trusted by that substrate, then adds its own capability and policy layer
around the result.

```text
native verifier
    ↓
Frankencore provider brick
    ├─ discovers capability and configuration
    ├─ resolves policy before sensitive operations
    ├─ invokes or observes the native boundary
    ├─ records provenance and diagnostics
    └─ projects normalized evidence and admission
```

For Debian-family systems, this means using APT and its configured keyrings
for repository authentication, while Frankencore adds the surrounding
identity, scope, policy, history, and consumer projections. It does not
reimplement OpenPGP verification, duplicate APT's dependency solver, or write
APT/dpkg state directly.

## What Frankencore adds

The provider brick contributes capabilities that the substrate tools do not
need to own:

- provider and substrate identity/version discovery;
- inspection of effective repository and keyring configuration;
- policy resolution before acquisition or mutation;
- normalized verification and admission results;
- operation correlation and mutation provenance;
- durable diagnostics for failure, override, recovery, and revalidation;
- JSON, CLI, GUI, IDE, and API projections;
- compatibility checks when native tools or metadata change.

## Delegation boundary

The native verifier remains authoritative for the verification mechanism. The
Frankencore provider may invoke it, consume its documented API, or observe its
resulting metadata, depending on the substrate. The provider must preserve
the native result and invocation context; it must not report a stronger result
than the substrate supplied.

Where a native mechanism offers no safe read-only verification query, the
provider must not simulate one from weak clues. It reports the limitation and
uses the existing unknown, quarantine, or explicit-override policy path.

The first Debian-family integration uses APT's read-only
`apt-get indextargets` interface to discover the native metadata targets,
their local filenames, and their source URIs. This proves the delegation
boundary without triggering `apt-get update` or any package mutation. The
presence of an index target remains metadata evidence; it is not independently
promoted to cryptographic verification by the Frankencore parser.

## Safety law

Native commands with mutation or network side effects require an explicit
operation policy and provenance boundary. The initial provider remains
read-only. Adding package acquisition or mutation is a separate capability,
not an incidental extension of inventory or inspection.

## Revisit triggers

Revisit when implementing the first APT invocation boundary, selecting a
documented APT API versus subprocess delegation, or adding package mutation
capabilities.
