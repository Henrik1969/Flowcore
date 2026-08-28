# ADR-0025: Trusted intermediary and scoped assertions

**Status:** accepted direction, implementation future
**Date:** 2026-08-20
**Scope:** Frankencore identity, secrets, and authority negotiation

## Decision

Frankencore supports a trusted-intermediary pattern for cases where two
parties should not exchange secrets or establish direct trust themselves.

```text
requesting consumer ── request ──> trusted intermediary
requesting consumer <─ scoped assertion ─ intermediary
consumer ── assertion ──> target authority/service
```

The intermediary authenticates or verifies the requester, evaluates its own
authority policy, and returns a scoped, time-bounded assertion. The requester
does not disclose the secret used with the intermediary to the target service.
The target service does not need to trust the requester directly; it verifies
the intermediary's assertion.

## Assertion requirements

An assertion should include:

- issuer identity;
- subject identity;
- target/audience;
- permitted action and scope;
- issue and expiry time;
- nonce or replay protection;
- policy/assurance level;
- correlation and provenance references;
- signature and key-resolution reference.

An assertion must not grant more authority than requested or than the
intermediary is authorized to delegate. It is not a transferable identity or
universal permission.

## Frankencore use

The broker can mediate:

- project-owner confirmation;
- profile signature trust;
- package/source acquisition authority;
- secure secret use;
- administrator approval;
- provider capability authorization;
- remote or isolated execution admission.

The question provider may ask the intermediary rather than exposing a secret
or demanding that an ordinary user understand cryptographic details.

## Trust and limitations

The intermediary becomes an important trust anchor and must therefore expose
identity, scope, policy, assurance, expiry, revocation, and decision
provenance. Trust in the intermediary is not blind: it is itself configured
through trust-store facts and ConfigResolve policy.

Systems may use several brokers or local-only operation when policy requires
it. No single global intermediary is constitutionally required.

## Revisit triggers

Revisit when defining the assertion wire format, replay protection, broker
discovery, federation, revocation, or privacy guarantees.
