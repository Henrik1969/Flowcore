# ADR-0022: Trust discovery and authority negotiation

**Status:** accepted direction, provisional protocol
**Date:** 2026-08-20
**Scope:** Frankencore trust-store adapters and ConfigResolve integration

## Decision

Trust-store resolution is a bounded discovery-and-negotiation process, not a
blind directory read.

```text
discover candidate facts
        ↓
classify source and scope
        ↓
identify applicable authorities
        ↓
ask through explicit adapters
        ↓
collect signed/verified responses
        ↓
ConfigResolve evaluates policy
        ↓
provide decision, explanation, or focused question
```

## Discovery

The adapter scans only policy-approved known locations and sources, including
XDG trust paths, project metadata, configured system stores, and explicitly
enabled SSH or external providers. Discovery produces candidate facts, not
trust decisions.

Each candidate records its location, source, digest/fingerprint, scope,
availability, and verification state. Unknown files and malformed material
become diagnostics rather than implicit trust.

## Authority questions

The resolver identifies which authority can answer each missing or ambiguous
fact. An adapter may ask for:

- key confirmation;
- ownership or project identity;
- validity or revocation state;
- scope of authority;
- provider support;
- policy acceptance or relaxation;
- independent-channel confirmation.

Questions must be specific, explain why the answer is needed, identify the
requesting scope, and state the consequences of each answer. The system must
not ask users to make technical trust decisions without presenting the
relevant evidence.

## Negotiation law

Negotiation may collect facts and obtain authorization, but it may not grant
authority merely because an endpoint responds. Responses must be authenticated
where required, scoped, versioned, and evaluated by ConfigResolve. Conflicting
answers remain unresolved or denied according to policy.

The user, administrator, or root is consulted only when policy assigns them
authority and the available facts do not mechanically decide the matter.

## Outcomes

The adapter returns:

```text
resolved       sufficient facts and policy decision
unresolved     focused authority question is required
denied         policy or evidence forbids the request
unavailable    required authority/provider cannot be reached
diagnostic     malformed or contradictory input requires attention
```

All outcomes retain source, authority, policy, question/answer, and
correlation provenance.

## Revisit triggers

Revisit when implementing interactive trust confirmation, remote authority
providers, offline negotiation, or asynchronous trust refresh.
