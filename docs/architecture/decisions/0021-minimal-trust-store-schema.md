# ADR-0021: Minimal extensible trust-store schema

**Status:** accepted direction, provisional schema
**Date:** 2026-08-20
**Scope:** Frankencore trust facts and ConfigResolve inputs

## Decision

The trust store remains a minimal fact store. ConfigResolve remains responsible
for resolving trust facts into an operational decision.

Minimal envelope:

```json
{
  "format": "frankencore.trust_store",
  "version": 1,
  "scope": "user",
  "entries": []
}
```

Minimal entry fields:

```text
key_id
fingerprint
subject
scope
purpose
state
source
```

The public key may be embedded or resolved through an approved key provider.
Private keys never belong in the store or project artifacts.

## Extensions

Optional fields may add validity, rotation, revocation, confirmation,
administrator, action restrictions, provider, lineage, and verification
evidence. Unknown fields must not override core fields. Namespaced extensions
are preferred:

```json
{
  "extensions": {
    "ssh": {},
    "configresolve": {},
    "example.org/custom": {}
  }
}
```

Extensions are additive and versioned. Unsupported extensions produce a
diagnostic when their semantics are needed for a decision.

## Scope precedence

Trust scope follows bounded authority:

```text
constitutional/system
administrator
user
project
session/operation
```

A higher layer defines the maximum permitted trust. A lower layer may restrict
that trust but cannot broaden it. An explicit denial at any applicable scope
denies the operation.

## Fact versus decision

The store records facts such as:

```text
this key was trusted by this authority for this subject and scope
```

ConfigResolve decides whether those facts authorize the requested operation
under current policies. The trust store is not a second policy engine.

## Revisit triggers

Revisit when implementing the XDG trust-store adapter, persistent schema,
rotation/revocation records, or multi-administrator trust delegation.
