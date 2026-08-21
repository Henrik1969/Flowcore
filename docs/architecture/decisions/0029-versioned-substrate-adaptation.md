# ADR-0029: Versioned substrate adaptation

**Status:** accepted direction, provisional implementation
**Date:** 2026-08-20
**Scope:** Frankencore substrate and dependency evolution

## Decision

Frankencore treats substrate and dependency evolution as a permanent operating
condition. It identifies known components through their official version and
identity schemes, then combines those facts with provenance, capability
discovery, compatibility checks, and current policy.

```text
component identity/version
        ↓
source and integrity facts
        ↓
capability and ABI probing
        ↓
known compatibility profile
        ↓
ConfigResolve policy decision
        ↓
trusted, adapted, isolated, unresolved, or rejected
```

## Version facts

For a known component such as a kernel, package, or program, the system records
the official version string, component identity, source/provenance, digest or
package identity where available, ABI/API information, and observed
capabilities. Version is an identity and compatibility fact; it is not trust
by itself.

Official schemes are used as long as the upstream ecosystem preserves them.
Unexpected version syntax, missing identity, or a changed scheme produces a
diagnostic and conservative handling rather than silent acceptance.

## Adaptation profiles

An adapter may declare compatibility for a version range or capability set.
The system must revalidate when:

- the component version changes;
- the package source or signing key changes;
- ABI/API behavior changes;
- a security or revocation state changes;
- dependencies change materially;
- observed capabilities differ from the known profile;
- policy or requested operation sensitivity changes.

Profiles are additive and versioned. Unknown versions may be inspected and
probed, but they do not inherit the strongest known profile automatically.

## Ownership boundary

Frankencore does not control upstream kernel, package, or program evolution.
It adapts through providers and policies. If a user retrieves, mutates, or
builds substrate components independently, the resulting provenance,
version, capability, and policy facts determine the local operating profile.

## Revisit triggers

Revisit when implementing package inventory, kernel capability providers,
ABI compatibility checks, update monitoring, or automatic profile refresh.
