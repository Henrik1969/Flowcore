# Current Flowmini Version

Current active implementation:

```text
flowmini_v24_explicit_ast
```

Current milestone:

```text
Flowmini v0.24 explicit AST
active C5 statement and frontend maturation
```

## Status

```text
normal CMake/Ninja build:      PASS
flowmini_ast_golden_tests:     PASS (20)
flowmini_symbol_projection:    PASS (2/2)
flowmini_suite:                PASS (78/78)
CTest:                         PASS (2/2)
```

Flowmini is still experimental and unfinished.

The current v0.24 line provides an observable, regression-guarded expression
and type-reference graph plus arena-owned statement/block structure. It is not
yet a complete raw representation of the accepted language.

## Run the current build

```bash
cd Flowmini/flowmini_v24_explicit_ast

cmake -S . -B cmake-build-debug
cmake --build cmake-build-debug -j20
```

Adjust `-j20` to match your machine.

This directory is the canonical v24 build and test scope. Repository-root build
trees and their legacy superbuild test registrations are noncanonical for this
branch.

## Run the current tests

```bash
cd Flowmini/flowmini_v24_explicit_ast

cmake --build cmake-build-debug --target flowmini_ast_golden_tests
cmake --build cmake-build-debug --target flowmini_suite
ctest --test-dir cmake-build-debug --output-on-failure
```

Expected result:

```text
normal CMake/Ninja build:      PASS
flowmini_ast_golden_tests:     PASS (20)
flowmini_symbol_projection:    PASS (2/2)
flowmini_suite:                PASS (78/78)
CTest:                         PASS (2/2)
```

## Current important architecture law

```text
TokenTree remembers what the source looked like.
AST states what the source means.
```

## Architecture checkpoint for future lowering

The v0.24 AST is being stabilized against the future Flowcore transformation
pipeline.

Current work should preserve semantic meaning and source provenance without
encoding optimizer, scheduler, runtime, or target-specific realization choices.

The project-wide transformation/revision design is recorded in:

```text
docs/architecture/compiler-transformation-revision-model.md
```

The v0.24-specific boundary note is:

```text
Flowmini/flowmini_v24_explicit_ast/docs/v0.24-future-transformation-boundary.md
```

## Current important language rule

For v0.24:

```text
A root program supports exactly one root main block.
```

Future Flowmini/Flowcore may support named targets, where each target owns its own main block.

That future direction is documented in:

```text
docs/language/named-targets.md
```

## Not yet complete

The following are still future or incomplete work:

```text
complete statement coverage for the accepted language
complete ABI, flow-placement, print, and executable source-form coverage
complete structural SymbolTable projection across the accepted language
semantic name resolution
type checking
Graph IR lowering
runtime execution semantics
provider/capability resolution
```
