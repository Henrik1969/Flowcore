# ADR-0034: Owner-controlled attestation

**Status:** accepted, binding law  
**Date:** 2026-08-20  
**Scope:** Frankencore release and supplier trust

## Decision

An attestation may be treated as an authoritative statement by an owner only
when the owner made the artifact, or had exclusive authority and practical
ability to inspect, mutate, and build the artifact-producing process.

Possession of another party's signature, a repository URL, a reputation, or a
local `trusted=yes` setting does not allow Frankencore—or an operator acting
for Frankencore—to sign off on that party's work.

## Consequences

- Supplier signatures remain supplier evidence and must be evaluated within
  their declared scope.
- A local operator may explicitly choose to use unverified material, but that
  choice is a policy override, not an owner attestation.
- `trusted=yes` can express local use, but can never establish `verified`
  provenance or authorize a supplier claim.
- An owner-controlled build may attest to its resulting artifact only after
  preserving the relevant source, toolchain, inputs, configuration, and build
  evidence according to the applicable reproducibility policy.
- When exclusive inspection or build control cannot be established, the
  result remains supplier-authenticated, externally attested, unverified, or
  unknown—not owner-attested.

## Boundary

This law does not require every user to build everything themselves. It
defines what the owner is personally permitted to claim. Users may choose to
trust external suppliers through their own policies and adapters, but that is
not silently elevated to the owner's attestation.

## Revisit triggers

Revisit only if the owner explicitly changes this law or defines a stronger
multi-party attestation model that preserves clear authorship and authority.
