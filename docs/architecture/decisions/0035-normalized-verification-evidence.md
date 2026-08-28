# ADR-0035: Normalized verification evidence

**Status:** accepted direction, provisional vocabulary  
**Date:** 2026-08-20  
**Scope:** verification adapters and admission policy

## Decision

Verification adapters normalize concrete substrate facts into a small semantic
evidence record. The record separates authentication, integrity, authority,
and local use. No single field is allowed to imply the others.

```text
substrate evidence
  → artifact identity and digest
  → signer and key state
  → source and scope
  → verification result
  → provenance and diagnostics
  → policy admission
```

## Minimum evidence dimensions

A future stable result must be able to represent, at minimum:

- `artifact`: identity, version, kind, platform, and digest;
- `substrate`: provider identity, version, and native verification method;
- `source`: repository, mirror, release channel, or acquisition reference;
- `signer`: supplier key identity and fingerprint when available;
- `key_state`: trusted, unknown, expired, revoked, invalid, or unavailable;
- `integrity`: digest matched, mismatched, absent, or not applicable;
- `authenticity`: supplier-authenticated, owner-attested, unverified, or
  unknown;
- `operator_override`: whether local policy deliberately relaxed verification;
- `provenance`: evidence references, timestamps, correlation, and history;
- `diagnostics`: human- and machine-consumable explanation of the result;
- `policy_outcome`: allowed, allowed-with-isolation, requires-confirmation,
  quarantined, rejected, or unresolved.

These dimensions are semantic obligations, not a commitment to JSON, a C++
layout, a database schema, or a particular signature encoding.

## Separation laws

- A matching digest establishes integrity, not origin.
- A valid supplier signature establishes supplier authentication within the
  key's declared scope, not owner attestation.
- A trusted key identifies an accepted authority, not safe software.
- An owner attestation requires the control defined by ADR-0034.
- An operator override explains local use, but never upgrades evidence.
- Policy outcome is a decision derived from evidence; it is not evidence.

## Conservative defaults

Missing or contradictory dimensions remain explicit. The adapter does not
invent a signer, infer authority from a URL, or convert an operator override
into authentication. Unknown or unsupported evidence is handled according to
the existing quarantine and admission policies.

## Evolution rule

The vocabulary may gain additive dimensions when concrete substrates require
them. Existing meanings must not be silently broadened, and a provider must
declare which dimensions it can actually produce. A consumer must degrade
conservatively when a provider lacks a dimension.

## Revisit triggers

Revisit when the Debian-family APT adapter and signed-release adapter are
implemented, when a second package ecosystem is examined, or when the first
stable C++ API/ABI and exchange schema are designed.
