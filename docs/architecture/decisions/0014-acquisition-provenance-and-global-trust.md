# ADR-0014: Acquisition provenance and global trust

**Status:** accepted direction, provisional implementation
**Date:** 2026-08-20
**Scope:** Frankencore project acquisition and external material

## Decision

Frankencore does not infer trust from geography, nationality, hosting service,
username, or presentation quality. Trust is evaluated from an acquisition
provenance chain and local policy.

The system must distinguish:

```text
origin claim       where the material says it came from
transport          how it was obtained
integrity          whether content changed in transit
authorship         which key or authority signed it
authority          what scope the signer controls
reputation         optional external/community assessment
authorization      whether this user/system may use it
execution safety   whether it may be inspected, built, or run
```

None of these facts alone proves that material is safe or authoritative.

## Acquisition classes

The initial policy vocabulary distinguishes at least:

```text
verified_package
    signed package, known source, digest, manifest, and policy-compatible

known_repository
    acquired from a configured source with inspectable revision history

unsigned_source
    source obtained intact but without trusted author attestation

untrusted_fragment
    snippet, paste, or material with incomplete provenance

unknown
    origin or integrity cannot be established
```

Untrusted and unknown material may be retained for inspection, but it is not
authoritative and must not be executed, installed, or allowed to alter trusted
project state without explicit policy and an appropriate isolation boundary.

## Package and source envelope

An acquired project or source bundle should carry:

- project identity and lineage, if known;
- acquisition event and correlation identities;
- origin/source reference;
- retrieval time and method;
- content digest;
- manifest of included artifacts;
- signatures and key references, if present;
- dependency/source declarations;
- trust and quarantine status;
- inspection and verification results.

Raw snippets may enter through a separate quarantine path and must never be
silently promoted into a project package.

## Trust policy

Trust roots are selected by the user, administrator, project, or distribution
according to scope. A global discovery catalog may report that a source or
capability exists, but it does not grant trust or execution authority.

Verification, review, sandboxing, and execution are separate decisions. A
signed package may still be denied execution by local policy; an unsigned
source may be inspected without being trusted for installation.

## Revisit triggers

Revisit when defining package manifests, repository attestations, quarantine
storage, dependency trust, remote execution, or federation between trust
domains.
