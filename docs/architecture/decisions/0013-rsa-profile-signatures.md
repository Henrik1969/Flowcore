# ADR-0013: RSA signatures for profile declarations

**Status:** accepted direction, implementation future
**Date:** 2026-08-20
**Scope:** Frankencore profile signatures

## Decision

Frankencore uses RSA public-key signatures for authoritative profile
declarations because interoperability with existing systems, trust stores,
and SSH-adjacent tooling is important.

The required default profile is:

```text
signature: RSA-PSS
hash:      SHA-256 or stronger
key size:  minimum 3072 bits
```

4096-bit keys may be selected where longer operational lifetime or local
policy justifies the additional cost.

## Safety constraints

- New profiles must not use legacy RSA PKCS#1 v1.5 signatures.
- Signature parameters must be explicit and included in the verification
  contract.
- Verification must validate the signed content digest and schema version.
- Public keys may be distributed; private keys must remain in approved key
  storage and never enter project artifacts.
- Rotation, revocation, expiry, successor relationships, and trust scope are
  policy-controlled.
- A valid RSA signature proves key control, not authority for every scope.

## Interoperability

The public-key representation should support established PEM/DER and SSH
public-key import/export through explicit adapters. Importing a key does not
automatically trust it. Trust anchors remain a policy decision.

## Revisit triggers

Revisit if cryptographic policy changes, a required platform lacks acceptable
RSA support, or a future version adopts algorithm agility while preserving
the signed profile contract.
