# ADR-0026: Adaptive broker transport and assurance

**Status:** accepted direction, provisional profiles
**Date:** 2026-08-20
**Scope:** Frankencore broker communication and trust negotiation

## Decision

Broker transport and assurance are selected according to scope, locality,
threat model, requested action, and policy. The semantic assertion contract
remains stable while transport and verification effort adapt.

```text
local same-user applications
    in-process or local IPC, OS identity, bounded assertion

local privilege boundary
    local broker, explicit identity checks, signed or MAC-protected assertion

remote ordinary boundary
    authenticated channel, signed audience-bound assertion, replay protection

high-risk / hostile boundary
    independent channels, stronger attestation, multi-party or multi-factor
    confirmation, explicit operator policy
```

The system must not invoke a remote or global authority for a local decision
when the local policy and OS boundary are sufficient. Conversely, local
convenience must not be used to satisfy a high-risk remote trust requirement.

## Stable semantic contract

Every transport must preserve the same assertion meaning:

- issuer and subject;
- target/audience;
- action and scope;
- assurance level;
- issue/expiry and replay protection;
- policy and provenance correlation;
- signature or channel-protection evidence.

Transport is an adapter projection. A local message, signed JSON document,
binary token, or multiplexed independent-channel exchange may all represent
the same semantic assertion.

## Multiplexed high-assurance exchange

For high-risk operations, policy may require several independent channels or
authorities. The resolver must state the required quorum, independence
assumptions, timeout, conflict behavior, and failure outcome. More channels do
not automatically mean more trust; correlated channels or one compromised
authority do not provide independence.

## Revisit triggers

Revisit when defining local IPC adapters, remote assertion transport,
multi-channel quorum, attestation, or broker federation.
