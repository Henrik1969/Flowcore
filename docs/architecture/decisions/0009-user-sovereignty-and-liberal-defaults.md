# ADR-0009: User sovereignty and liberal defaults

**Status:** accepted constitutional direction
**Date:** 2026-08-20
**Scope:** Frankencore governance, defaults, and policy

## Decision

Frankencore defines the minimum laws and contracts required for the system to
remain identifiable, interoperable, inspectable, and mechanically resolvable.
It does not impose one universal policy regime on owners, administrators, or
future consumers.

The system belongs to its owner or authorized administrator. They may choose
providers, policies, disclosures, storage, and operational trade-offs within
the declared contract boundaries. If they deliberately operate outside the
recommended defaults, the system must make the deviation and responsibility
clear rather than pretending that the result is conformant.

## Required versus recommended

```text
constitutional requirements
    required for identity, versioning, interoperability, provenance,
    explicit failure, and safe contract boundaries

recommended defaults
    sensible starting policies for ordinary users and projects

owner policy
    locally chosen behavior within the declared contract

experimental override
    permitted when explicitly identified and responsibility is accepted
```

Defaults are additive conveniences, not hidden authority. A consumer may
replace them when it can still understand the resulting contract or clearly
reports that it is operating outside the recommended profile.

## Liberal policy law

The architecture must prefer:

- explicit choice over hidden enforcement;
- inspectable decisions over opaque authority;
- additive extension over needless prohibition;
- scoped authority over universal control;
- graceful degradation where correctness permits;
- fail-closed behavior only where ambiguity would create unsafe or dishonest
  results;
- documented responsibility when an owner chooses a non-default policy.

Liberal does not mean undefined. A provider or owner may choose differently,
but the system must retain enough identity, provenance, diagnostics, and
contract information for consumers to understand what was chosen.

## Revisit triggers

Revisit when defining security-critical substrate boundaries, delegated
authority, multi-user trust, or a mandatory distribution profile.
