# ADR-0011: Signed operating-profile declarations

**Status:** accepted direction, implementation future
**Date:** 2026-08-20
**Scope:** Frankencore profile authenticity and trust

## Decision

Operating-profile declarations are signed by default. A signed declaration is
required before a profile may be treated as an authoritative owner or
administrator claim.

An unsigned, unverifiable, expired, or revoked declaration may still be
observed as information, but its contract status is `unknown` or
non-authoritative unless an explicit policy says otherwise.

Relaxing signature requirements is an owner decision that must be:

- explicit;
- policy-identified and versioned;
- visible in the operating-profile declaration;
- recorded with the resulting decision provenance;
- treated as a reduction in trust, not as equivalent to verified authority.

## Signed declaration requirements

A future signed profile contract must identify:

- profile and project identity;
- signing key identity;
- signature algorithm and schema version;
- signed content digest;
- policy/profile revision;
- issued and expiry times, if applicable;
- trust-anchor or key-resolution reference;
- revocation or supersession state.

Signature verification establishes authenticity and integrity. It does not by
itself grant permission; policy still decides whether the signer is trusted for
the requested scope and action.

## Trust outcomes

```text
verified       signature valid and signer trusted for scope
unverified     signature absent or cannot currently be checked
invalid        signature fails or content changed
revoked        signer or declaration explicitly withdrawn
expired        declaration outside its validity period
```

Only `verified` is authoritative under the default profile. The other states
remain diagnosable and inspectable, but sensitive operations fail closed unless
an explicit relaxation policy applies.

## Revisit triggers

Revisit when choosing the signing format, key storage/trust roots, rotation,
offline verification, or multi-administrator delegation rules.
