# ADR-0024: Secret-bearing questions and secure resolution

**Status:** accepted constitutional direction, provisional contract
**Date:** 2026-08-20
**Scope:** Frankencore secrets, credentials, and authority questions

## Decision

Secret-bearing material is never delegated through a public or ordinary
presentation provider. CLI, GUI, IDE, agent, remote UI, logs, catalogs,
provenance records, and JSON exchange artifacts receive only a redacted
question or a secure reference.

```text
public question provider
    redacted request + evidence + secure-reference metadata

secure secret provider
    protected prompt, key agent, vault, hardware token, or OS mechanism

resolver
    minimum derived fact or authorization result
```

The secret itself must remain inside the approved secure mechanism. It must
not appear in source, project metadata, trust stores, catalogs, history,
diagnostics, screenshots, command lines, or ordinary environment exports.

## Redacted artifact

A public or inspectable artifact may state:

```text
secret_required
secret_kind
secure_provider_required
reference_id
fingerprint or digest, where safe
redaction_reason
expiry or validity state
```

It must not contain the secret value, private key, reusable token, or enough
material to reconstruct them.

## Secure answer contract

The secure provider may return only the minimum result required by policy,
such as:

```text
verified
denied
unavailable
expired
invalid
capability_handle
```

If a downstream operation genuinely needs secret bytes, the operation must
consume them through an explicitly authorized secure capability rather than
through the general question or provenance channel.

## Safety rules

- Secret prompts identify the secure mechanism and purpose before use.
- Secret values are never echoed or copied into ordinary diagnostics.
- Redacted artifacts remain inspectable without disclosing credentials.
- Secure-provider failure is explicit and fail-closed for sensitive actions.
- A reference ID is not itself a credential.
- Secret-bearing operations retain only non-sensitive decision provenance.

## Revisit triggers

Revisit when defining vault adapters, hardware-backed keys, secure input UI,
secret capability handles, memory-clearing guarantees, or remote secure
attestation.
