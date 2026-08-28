# Flowmini v0.24 — Closed Explicit AST and Frontend Export

Status: frozen historical implementation checkpoint

This directory contains the tagged v0.24 raw frontend/export implementation.
It inherited the v22 source-role boundary and subsequently added the typed raw
AST, factual SymbolTable projection, versioned frontend bundle, independent
consumer, and the recorded Tier 3 border gate.

Active development now happens in:

```text
Flowmini/flowmini_v25_symboltable_projection
```

The authoritative v0.24 checkpoint description is
[`docs/v0.24-explicit-ast-status.md`](docs/v0.24-explicit-ast-status.md). Current
project state is recorded in [`../CURRENT.md`](../CURRENT.md).

## Inherited source-unit rule

Flowmini source files now have an explicit unit kind:

```flow
program demo
```

or:

```flow
unit math_helpers
```

## Rule

```text
program = executable/root source unit
unit    = defining/importable source unit
```

A `program` may contain `main` and may be run as the root input.

A `unit` may be imported and must not contain `main`.

## Import law

```text
root execution:
    accepts program with main
    rejects unit as root input
    rejects program without main

import:
    accepts unit
    rejects program
    rejects any imported file that defines main
```

This separates executable units from defining/header/library units, inspired by
C/C++ headers/source separation, Turbo Pascal units, and Visual Basic-style
units/modules.

## Compatibility note

The parser still accepts legacy `module` as a spelling so old FlowIR/internal
paths continue to work during the transition. The examples and std files in this
version have been migrated to `program` / `unit`.

## Test-oriented example layout

The examples directory is now categorized by intent:

```text
examples/
├── pass/       root programs expected to execute successfully
├── fail/       negative tests expected to fail with diagnostics
├── support/    defining/support files, not normally root-run
└── docs/       documentation
```

This supports the external suite runner:

```bash
$TOP/tools/run-flowmini-test-suite.sh --root Flowmini/flowmini_v24_explicit_ast
```

or, with environment defaults:

```bash
export FLOWMINI_ROOT="$TOP/Flowmini/flowmini_v24_explicit_ast"
$TOP/tools/run-flowmini-test-suite.sh
```

## Verified

- CMake Debug build succeeds.
- TokenTree smoke tests pass.
- SymbolTable tests pass.
- Categorized Flowmini suite passes: 78 / 78.

Valgrind was not run in this packaging environment because Valgrind is not
installed here.

## Intent

This is a language-structure cleanup step, not a bytecode step.

Later versions may add artifact kinds such as precompiled bytecode units, but
that will be a separate concern:

```text
source role:    program | unit
artifact form:  source | bytecode | cache | object
```

The source-role rule originated in v22. The v0.24 checkpoint preserves it as
part of the closed frontend contract.
