# Documentation Index

This repository is organized as a lab notebook with project islands.


## Flowcore's core promise: 

> source-level architecture becomes compiler-visible graph structure.
> The core philosophy is semantic clarity: meaning must be explicit, understandable, and checkable.
> Syntax is subordinate to semantics.

This is the soul of Flowcore.

## Main areas

- [Flowmini](../Flowmini/README.md)
- [Subprojects](../subprojects/README.md)
- [Pattern explored](../Pattern_explored/README.md)
- [Architecture notes](architecture/README.md)
- [Flowmini docs](flowmini/README.md)
- [Flowmini language comparisons](language-comparisons/README.md)
- [Flowmini evidence-based future work](FUTURE_WORK_EVIDENCE.md)
- [Session notes](sessions/)

## Foundational architecture

- [Current v0.29 language-maturation status](checkpoints/2026-08-26-v028-maturation-ledger.md)

- [Flowcore core promise](architecture/flowcore-core-promise.md)
- [Transformation and revision architecture](architecture/compiler-transformation-revision-model.md)
- [Current v0.28 typed artifact-contract status](checkpoints/2026-08-26-v0.28-typed-artifact-contracts.md)
- [Historical v0.27 namespaced provider language-chain status](checkpoints/2026-08-21-v0.27-namespaced-provider-chain.md)
- [FrankenCore conformance declaration](architecture/frankencore-conformance.md)
- [Prerequisites](architecture/prerequisites.md)
- [Frankencore Constitution v0.1](architecture/FRANKENCORE-CONSTITUTION.md)
- [Frankencore contract inventory](architecture/frankencore-contract-inventory.json)
- [Current Frankencore conformance](architecture/frankencore-current-conformance.md)
- [Frankencore repository audit](architecture/FRANKENCORE-AUDIT-2026-08-20.md)

## Development policy

- [Verification gates and Firetest policy](development/verification-gates.md)
- [Project hygiene](development/project-hygiene.md)
- [Documentation style](development/documentation-style.md)

## Current active Flowmini version

See:

- [Flowmini current version](../Flowmini/CURRENT.md)
- [Flowmini version index](../Flowmini/VERSION_INDEX.md)
- [Flowmini changelog](../Flowmini/CHANGELOG.md)

Current checkpoint:

```text
Flowcore v0.29 language-maturation and cross-backend parity slice
 AST golden tests: 28
 Symbol projection tests: 14
 Root CTest: 81/82 in the current build (one baseline failure in the variant-construction JSON probe)
 Flowmini categorized fixture suite: 87/136 (49 known parser/ABI/profile gaps)
 flowvalidate / identity mutation gates: CTest green
```
