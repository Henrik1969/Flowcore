# ADR-0008: Language-neutral policy contract and adapters

**Status:** accepted direction, provisional schema
**Date:** 2026-08-20
**Scope:** Frankencore policy-resolution interoperability

## Decision

The policy-resolution contract is defined in three layers:

```text
constitutional semantic contract
    language- and representation-neutral laws

versioned exchange schema
    canonical structured contract for interchange and inspection

native adapters
    C++, C ABI, JSON/IPC, Rust, Lisp, Flowmini, and future systems
```

The semantic contract is authoritative. No programming language, ABI, wire
format, or implementation owns the meaning by itself.

## Exchange projection

JSON is the mandatory inspectable exchange projection because it is readable,
archivable, searchable, and available across languages and paradigms. It must
be versioned, schema-validated, explicit about failure, and suitable for
conformance fixtures.

Binary projections may be added for performance or transport efficiency. They
must map losslessly to the versioned semantic schema and may not introduce
meaning unavailable in the inspectable JSON projection.

## Adapter rule

Every provider or consumer adapter must preserve:

- authority and scope;
- policy identity and revision;
- conditions and conflict semantics;
- allow, deny, and unresolved outcomes;
- diagnostics and explanations;
- provenance and correlation identities;
- version and compatibility information.

An adapter that cannot preserve a semantic field must reject the exchange or
mark the result non-authoritative. Silent loss is forbidden.

## Conformance

Conformance is behavioral and schema-based, not language-based. A future
provider written in an unknown language may participate if it can consume and
produce the exchange contract and pass the same law and failure tests.

ConfigResolve remains the current native C++ default provider. Its C++ API is
an adapter projection, not the universal contract.

## Revisit triggers

Revisit when freezing the first stable exchange schema, adding a binary
projection, or accepting the first independent non-C++ provider.
