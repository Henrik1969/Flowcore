# ADR-0028: Provider risk declarations and renewed trust

**Status:** accepted direction, provisional defaults
**Date:** 2026-08-20
**Scope:** Frankencore provider adaptation and trust

## Decision

Frankencore starts with sane, versioned defaults for known substrates and
adapts when the underlying operating system, provider, or external ecosystem
evolves. It does not assume that external providers will remain static or
wait for their permission to preserve its own contracts.

## Provider risk data

A provider's declaration of its own risks is useful evidence about mechanism:

```text
provider risk declaration
    what the provider can do, expose, restrict, or fail to contain
```

It is not proof of trustworthiness:

```text
risk knowledge ≠ authorization
capability claim ≠ trust
self-assessment ≠ independent verification
```

The system uses provider risk data to choose a conservative assurance profile,
then evaluates trust separately through identity, provenance, verification,
policy, and history.

## Trust renewal

Trust is not permanent. A trusted provider or actor must be reconsidered when:

- its key rotates or is revoked;
- its behavior or declared capability changes;
- its dependencies or substrate change materially;
- its policy scope changes;
- evidence expires or becomes contradictory;
- a failure, compromise, or suspicious event occurs;
- the requested action becomes more sensitive.

The practical question is always contextual:

```text
Can I trust this actor/provider for this action,
on this resource, under the current evidence and policy?
```

## Adaptation

When the substrate evolves without Frankencore participation, adapters may
map the new facts into existing contracts. Unknown or changed behavior is
classified conservatively until the adapter and defaults are updated. Existing
identity, provenance, and policy boundaries remain stable while mechanisms
change underneath them.

## Revisit triggers

Revisit when defining provider manifests, default substrate profiles,
continuous verification, compromise response, or automatic trust renewal.
