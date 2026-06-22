# Flowcore

Flowcore is an experimental graph/contract/flow-oriented programming language project.

The current implementation level is **flowmini**, an early prototype/lab used to test syntax, lowering, diagnostics, staged execution, and implementation strategies.

This repository is not a finished language. It is a design and implementation workspace.

## Core idea

Flowcore explores programs as graphs of communicating work nodes.

- nodes do work
- ports expose node inputs and outputs
- wires connect ports
- wires are contracts, not values
- signals/payloads move through wires
- diagnostics and failure can flow alongside data
- scheduling should eventually be derivable from graph topology plus declared effects/resources

## Design rule

> Sugar may remove typing, but must not remove meaning.

Syntax sugar is acceptable only when direction, endpoints, contracts, payload movement, and failure paths remain semantically recoverable.

## Current repository state

This repository currently contains a raw import of experimental work:

- `Flowmini/` — flowmini prototype versions and syntax experiments
- `subprojects/TokenTree/` — structural token tree library experiment
- `subprojects/SymbolTable/` — symbol table library experiment
- `Pattern_explored/` — earlier explored implementation patterns
- `tools/` — helper scripts and project tooling

The structure will be cleaned up over time.

## Status

Experimental. Unstable. Not production-ready.

The current goal is to explore the language model, implementation boundaries, and tooling architecture.
