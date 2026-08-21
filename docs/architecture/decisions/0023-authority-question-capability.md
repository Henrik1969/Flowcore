# ADR-0023: Authority-question capability

**Status:** accepted direction, provisional contract
**Date:** 2026-08-20
**Scope:** Frankencore interactive trust and policy negotiation

## Decision

Authority questions are a neutral capability boundary, analogous to the
selector capability:

```text
resolver
    structured authority question
        ↓
question provider
    CLI / GUI / IDE / agent / remote interface
        ↓
resolver
    structured answer and policy decision
```

The resolver owns the question's semantic meaning and evaluates the answer.
Presentation layers own interaction and may not reinterpret policy semantics.

## Question contract

A question must identify:

- `question_id`;
- `correlation_id`;
- requesting authority;
- subject and scope;
- evidence presented;
- question and allowed answers;
- risk and consequences;
- expiry;
- noninteractive behavior.

Answers must identify the question, answerer, answer, time, authority scope,
and supporting provenance. Invalid, expired, ambiguous, or unauthorized answers
remain unresolved.

## Safety defaults

High-risk questions require explicit confirmation, present the relevant
evidence, have no implicit affirmative default, and expire. If no authorized
answer provider is available, the resolver returns unresolved or denied
according to policy.

## Provider neutrality

The same question may be projected into a terminal prompt, desktop dialog,
IDE panel, agent workflow, web interface, or remote operator process. A
provider may add presentation-specific options, but the canonical question
and answer remain versioned structured data.

## Revisit triggers

Revisit when defining the exchange schema, secret-bearing questions,
multi-operator approval, remote answer authentication, or unattended
execution policy.
