# ADR-0007: Policy-resolution provider contract

**Status:** accepted direction, provisional contract
**Date:** 2026-08-20
**Scope:** Frankencore policy resolution

## Decision

Frankencore requires a policy-resolution capability with a defined contract,
but does not permanently require one implementation.

ConfigResolve is the current default provider and the first explicit
implementation of that capability. Its use must remain behind the
Frankencore policy-resolution adapter boundary.

```text
Frankencore policy-resolution contract
                 ↑
      ConfigResolve default provider
                 ↑
 future compatible providers/adapters
```

Users, projects, and systems may derive or supply another resolver when their
requirements justify it, provided that it preserves the contract and reports
unsupported semantics explicitly.

## Provider contract boundary

A compatible provider must be able to:

- accept scoped policy facts and source provenance;
- represent authority, precedence, conditions, and conflict behavior;
- resolve to an unambiguous allow, deny, or unresolved result;
- fail closed for missing or contradictory authorization;
- return diagnostics and an explanation;
- identify the policies and revisions used in the decision;
- preserve correlation with the relevant operation and provenance events;
- expose a versioned capability and compatibility declaration.

The contract defines semantic obligations, not internal data structures,
configuration syntax, storage, programming language, or algorithm.

## Default-provider rule

ConfigResolve is the default because it is already mature, tested, packaged,
and provides layered facts, precedence, conflicts, diagnostics, explanation,
scoped views, and transactional mutation. Default status is a policy choice,
not constitutional authority over future implementations.

## Revisit triggers

Revisit when defining the first stable adapter ABI, accepting a second mature
provider, or discovering a required semantic obligation ConfigResolve cannot
represent without weakening its own generic contract.
