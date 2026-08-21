# ADR-0047: Mechanically resolvable laws and philosophies

**Status:** accepted, binding architectural principle  
**Date:** 2026-08-20  
**Scope:** Frankencore laws, policies, providers, and language-chain decisions

## Decision

Frankencore laws and philosophies should be expressed wherever practical as
mechanically resolvable contracts, facts, policies, and provider results.
Prose remains necessary for meaning, intent, examples, and human review, but
an operational rule must have a machine-consumable form before it can govern
an operation automatically.

```text
constitutional law / philosophy
        ↓ explicit contract and policy vocabulary
observed facts + provenance
        ↓ resolver/provider
allow / deny / isolate / quarantine / unresolved
        ↓ explanation and audit evidence
operation or diagnostic
```

## Resolution law

- A rule that can be resolved mechanically must not be reimplemented as
  scattered caller-specific intuition.
- A fact, policy, provider claim, and decision remain distinct records.
- Ambiguity, missing evidence, contradiction, and unsupported semantics
  produce explicit unresolved results or conservative failure states.
- A resolver must explain the facts, policy revisions, provider, and scope
  that produced its result.
- Mechanical resolution does not make a policy correct; it makes the selected
  policy consistent, inspectable, revisable, and testable.

## Minimalism and extension

The core vocabulary remains minimal. New statuses, providers, languages,
substrates, policies, and projections are additive when they preserve existing
meanings and declare their scope and compatibility. Local policy may choose a
different permitted behavior, but the deviation must remain explicit and
provenanced.

## Boundary

This principle does not attempt to mechanize every human judgment or replace
owners, administrators, or legitimate policy authority. It requires the
system to expose the decision boundary honestly: what was resolved, what was
delegated, what was assumed by policy, and what remains unknown.

## Revisit triggers

Revisit when a new architectural law cannot be represented by the current
contract/policy vocabulary, when a resolver produces ambiguous results, or
when a provider needs to broaden semantics rather than add an adapter.
