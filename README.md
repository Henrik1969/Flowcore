# Flowcore

Flowcore is experimental language and system-architecture work.

The current active implementation is:

```text
Flowmini v0.24 explicit AST
```

Active implementation path:

```text
Flowmini/flowmini_v24_explicit_ast
```

This repository is not a finished language or runtime. It is a design and implementation workspace for testing language structure, AST modeling, diagnostics, staged execution, contracts, graph-shaped execution ideas, and tooling architecture.

## Current status

```text
status: experimental
production-ready: no
active branch: v24-explicit-ast
active prototype: Flowmini v0.24 explicit AST
current milestone: raw frontend maturation; C5 typed statements complete
```

Current known green gates:

```text
normal CMake/Ninja build:      PASS
flowmini_ast_golden_tests:     PASS (21)
flowmini_symbol_projection:    PASS (2/2)
flowmini_suite:                PASS (78/78)
CTest:                         PASS (2/2)
```

## What is Flowmini?

Flowmini is the executable prototype language used to test and harden Flowcore ideas.

It is not the final Flowcore language. It is the lab where syntax, AST structure, semantic rules, lowering ideas, diagnostics, and tooling are made visible before becoming larger Flowcore architecture.

Start here:

```text
Flowmini/README.md
```

## Why does this look ordinary?

At the surface, early Flowmini examples may look like a small conventional programming language.

That is intentional.

The current goal is not novelty syntax first. The current goal is to build a visible and testable language pipeline underneath ordinary-looking source code:

```text
source text
    -> tokens
    -> source structure
    -> explicit AST
    -> semantic facts
    -> contracts/scopes
    -> graph-shaped IR
    -> executable system projection
```

The larger Flowcore direction is not merely "another syntax for functions and variables." The goal is a contract-governed, graph-shaped system model where programs can later be understood as nodes, ports, wires, policies, capabilities, and executable projections.

## Core idea

Flowcore explores programs as graphs of communicating work nodes.

```text
nodes do work
ports expose node inputs and outputs
wires connect ports
wires are contracts, not values
signals/payloads move through wires
diagnostics and failure can flow alongside data
scheduling should eventually be derivable from graph topology plus declared effects/resources
```

## Design rule

> Sugar may remove typing, but must not remove meaning.

Syntax sugar is acceptable only when direction, endpoints, contracts, payload movement, and failure paths remain semantically recoverable.

## Repository map

```text
Flowmini/
    executable Flowmini prototype language

Flowmini/flowmini_v24_explicit_ast/
    current active implementation line

subprojects/TokenTree/
    structural token tree library experiment

subprojects/SymbolTable/
    symbol table library experiment

docs/
    architecture, language, development, notes, and session documentation

tools/
    helper scripts and project tooling

_archive/
    intentionally preserved historical material
```

## Build quickstart

```bash
cd Flowmini/flowmini_v24_explicit_ast

cmake -S . -B cmake-build-debug
cmake --build cmake-build-debug -j20
```

Adjust `-j20` to match your machine.

The canonical v24 build and test scope is
`Flowmini/flowmini_v24_explicit_ast` and its CMake targets. A repository-root
build tree is not a canonical v24 build. References there to `Handwritten_V1`,
`flowcheck`, or `flowoptimize` smoke scripts belong to a legacy root-superbuild
configuration and are not active v24 test requirements.

## Test quickstart

```bash
cd Flowmini/flowmini_v24_explicit_ast

cmake --build cmake-build-debug --target flowmini_ast_golden_tests
cmake --build cmake-build-debug --target flowmini_suite
ctest --test-dir cmake-build-debug --output-on-failure
```

Expected current result:

```text
normal CMake/Ninja build:      PASS
flowmini_ast_golden_tests:     PASS (21)
flowmini_symbol_projection:    PASS (2/2)
flowmini_suite:                PASS (78/78)
CTest:                         PASS (2/2)
```

## Recommended reading

```text
Flowmini/README.md
Flowmini/CURRENT.md
Flowmini/flowmini_v24_explicit_ast/docs/v0.24-explicit-ast-status.md
Flowmini/flowmini_v24_explicit_ast/docs/v0.24-shallow-expression-ast-sitrep.md
docs/development/project-hygiene.md
```

## License

This project is licensed under the MIT License. See `LICENSE`.

## Disclaimer

This project is provided "as is" without warranty.

It is not intended for production, safety-critical, security-critical, financial, legal, medical, or operational use.

See `DISCLAIMER.md`.
