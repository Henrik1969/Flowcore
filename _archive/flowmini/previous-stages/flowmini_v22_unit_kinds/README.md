# Flowmini v22 — Unit Kinds

This version introduces the first hard source-role boundary for Flowmini.

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
$TOP/tools/run-flowmini-test-suite.sh --root Flowmini/flowmini_v22_unit_kinds
```

or, with environment defaults:

```bash
export FLOWMINI_ROOT="$TOP/Flowmini/flowmini_v22_unit_kinds"
$TOP/tools/run-flowmini-test-suite.sh
```

## Verified

- CMake Debug build succeeds.
- TokenTree smoke tests pass.
- SymbolTable tests pass.
- Categorized Flowmini suite passes: 75 / 75.

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

v22 only defines the source role boundary.
