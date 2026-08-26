# Flowcore

Flowcore is experimental language and system-architecture work.

The current active implementation is:

```text
Flowcore v0.28 typed artifact-contract language-chain slice
```

Active implementation path:

```text
Flowmini/flowmini_v25_symboltable_projection
```

This repository is not a finished language or runtime. It is a design and implementation workspace for testing language structure, AST modeling, diagnostics, staged execution, contracts, graph-shaped execution ideas, and tooling architecture.

## Current status

```text
status: experimental
production-ready: no
active branch: v25-symboltable-projection
active prototype: Flowcore v0.28 typed artifact-contract language-chain slice
current milestone: independently validated, identity-preserving frontend-to-ELF chain
```

Current known green gates:

```text
normal CMake/Ninja build:      PASS
AST golden tests:              PASS (28/28)
Symbol projection tests:       PASS (14/14)
frontend bundle:               PASS (8 golden, 1 isolated, 19 negative)
downstream sibling CTest:      PASS
independent flowvalidate:       PASS
typed identity mutation gate:  PASS
flowcat native ELF:            PASS
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

Flowmini/flowmini_v25_symboltable_projection/
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

The repository root now provides the canonical clean-tree superbuild for the
current sibling chain and Frankencore reference tools:

```bash
cmake -S . -B /tmp/flowcore-build -G Ninja
cmake --build /tmp/flowcore-build
ctest --test-dir /tmp/flowcore-build --output-on-failure
```

The clean root build currently registers 57 tests, including the larger
integration corpus, pipeline matrix, pass corpus, sibling CTest suites, CUDA
provider contracts, and Frankencore conformance probes.

Individual sibling builds remain valid for focused development:

```bash
cd Flowmini/flowmini_v25_symboltable_projection

cmake -S . -B cmake-build-debug
cmake --build cmake-build-debug -j20
```

Adjust `-j20` to match your machine.

The root superbuild does not replace the individual project contracts. It
assembles them into one build graph so clean-tree dependencies and cross-stage
tests are visible to CMake, Ninja, and IDEs.

## Test quickstart

```bash
cd Flowmini/flowmini_v25_symboltable_projection

cmake --build cmake-build-debug --target flowmini_ast_golden_tests
cmake --build cmake-build-debug --target flowmini_suite
ctest --test-dir cmake-build-debug --output-on-failure
```

Expected current result:

```text
normal CMake/Ninja build:      PASS
flowmini_ast_golden_tests:     PASS (28/28)
flowmini_symbol_projection:    PASS (12/12)
flowmini_frontend_bundle:      PASS (7 golden, 1 isolated, 19 negative)
flowmini_suite:                PASS (78/78)
CTest:                         PASS (2/2)
```

For the complete root build, the current result is:

```text
root CTest:                    PASS (57/57)
integration corpus:            PASS (3/3)
pipeline matrix:               PASS (9 accepted, 1 blocked)
pass corpus:                   PASS (43 programs)
stdlib boundary:               PASS (6 ABI modules; libc/file I/O/memory/kernel ready at binding boundary)
```

## Recommended reading

```text
Flowmini/README.md
docs/checkpoints/2026-08-26-v0.28-typed-artifact-contracts.md
Flowmini/CURRENT.md
Flowmini/flowmini_v25_symboltable_projection/docs/v0.25-symboltable-projection-status.md
docs/flowmini/v0.25-origin-maturity-audit.md
docs/flowmini/v0.25-frontend-bundle.md
Flowmini/flowmini_v24_explicit_ast/docs/v0.24-shallow-expression-ast-sitrep.md (historical checkpoint)
docs/development/project-hygiene.md
```

The current project boundary and its conformance to the FrankenCore
architectural laws are recorded in the
[Flowcore FrankenCore conformance declaration](docs/architecture/frankencore-conformance.md).

## License

This project is licensed under the MIT License. See `LICENSE`.

## Disclaimer

This project is provided "as is" without warranty.

It is not intended for production, safety-critical, security-critical, financial, legal, medical, or operational use.

See `DISCLAIMER.md`.
