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
- [Session notes](sessions/)

## Foundational architecture

- [Flowcore core promise](architecture/flowcore-core-promise.md)
- [Transformation and revision architecture](architecture/compiler-transformation-revision-model.md)
- [Prerequisites](architecture/prerequisites.md)

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
Flowmini v0.25 SymbolTable projection maturation
AST golden tests: 26
Symbol projection tests: 11
Frontend bundle tests: 5 golden, 1 isolated, 11 negative
suite: 78 / 78
```
