# ADR-0018: Contextual and earned trust

**Status:** accepted constitutional direction, provisional model
**Date:** 2026-08-20
**Scope:** Frankencore trust decisions

## Decision

Trust is contextual, bounded, earned, and revocable. Frankencore must not
model trust as a single permanent boolean or score.

The relevant question is:

```text
Who or what is trusted,
for which action,
against which resource,
under which evidence,
for what period,
and subject to which revocation policy?
```

Initial trust is unavoidable: without trust anchors, identity providers,
package signatures, or configured authorities, a system can do little beyond
quarantine. Initial trust is therefore explicit bootstrap policy, not proof
that the trusted party is universally safe or above the law.

## Trust progression

Trust may progress through bounded states:

```text
unknown
    no useful evidence

identified
    identity or origin linked to a verifiable reference

authenticated
    control of the relevant key or channel established

authorized
    policy grants a defined action and scope

trusted_for_action
    evidence and authorization satisfy the requested operation

revoked / expired / challenged
    prior trust no longer applies without renewed verification
```

Authentication does not imply authorization. Authorization does not imply
universal safety. Trust for one action does not automatically transfer to
another action, resource, or time period.

## Verification pattern

The default trust pattern is:

```text
claim
  → identify source
  → verify through an independent or configured channel
  → authorize narrowly
  → act within scope
  → retain evidence
  → revalidate or revoke when conditions change
```

This is why a signed package, a repository, a callback number, or an official
credential can be useful evidence without becoming unconditional authority.

## System behavior

Frankencore should prefer bounded trust over either blind acceptance or
paranoid isolation. When evidence is insufficient for the requested action,
it should seek a safer lower-risk action such as inspection or isolated
execution. When no safe action exists, it must quarantine or reject and
explain why.

## Revisit triggers

Revisit when implementing trust anchors, challenge-response, delegated
authority, revocation, continuous verification, or risk-sensitive action
profiles.
