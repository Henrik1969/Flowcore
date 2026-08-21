# ADR-0010: Operating profile and contract conformance

**Status:** accepted direction, provisional schema
**Date:** 2026-08-20
**Scope:** Frankencore interoperability declarations

## Decision

Frankencore declares operating state on two independent axes:

```text
contract status
    whether required semantic contracts are satisfied

policy profile
    which policy choices the owner has selected
```

Contract status values are:

```text
conformant
partially_conformant
non_conformant
unknown
```

Policy profile values are:

```text
recommended
custom
experimental
```

A custom policy may remain fully contract-conformant. Policy freedom must not
be mislabelled as system failure.

## Profile declaration

A minimal declaration contains:

```json
{
  "contract_status": "conformant",
  "policy_profile": "custom",
  "profile_id": "project.example.desktop",
  "policy_revision": "01K..."
}
```

Declared deviations may be included with the original policy, selected policy,
and explanation. Extensions are additive and versioned.

Profile declarations are signed by default. Unverified declarations may be
observed but are not authoritative unless an explicit, visible policy
relaxation applies.

## Disclosure levels

Profile information may be disclosed at three levels:

```text
basic     contract status, profile name, revision
standard  policy identifiers and declared deviations
audit     authority chain, decisions, diagnostics, and provenance
```

The applicable disclosure policy determines which level a consumer receives.
The minimal profile declaration remains available unless policy explicitly
requires it to be restricted.

## Rationale

Separating compatibility from policy choice preserves both interoperability
and owner sovereignty. A consumer can determine whether it understands the
contract without assuming that every owner selected the recommended defaults.

## Revisit triggers

Revisit when freezing the profile schema, adding signed profile declarations,
or defining cross-project profile negotiation.
