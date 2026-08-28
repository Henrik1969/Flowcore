# Current Flowmini Version

Current active implementation:

```text
flowmini_v25_symboltable_projection
```

Current milestone:

```text
Flowmini v0.27 namespaced provider language-chain slice
frontend export, semantic analysis, policy binding, optimization boundary,
LLVM lowering, executable application proof, and qualified provider imports
```

## Status

```text
normal CMake/Ninja build:      PASS
AST golden tests:              PASS (28)
Symbol projection tests:       PASS (14/14)
downstream sibling CTest gates: PASS
flowcat native ELF example:    PASS
```

Flowmini is still experimental and unfinished.

The active v0.26 line inherits the closed, observable, regression-guarded v0.24 expression
and type-reference graph plus arena-owned declaration/statement/block
structure. C5 is now
complete: every statement kind is selected by its typed payload, conditional
and loop ownership lives in those payloads, canonical arrow placement preserves
closed assignable targets and source form, and obsolete shared statement fields
are gone. Structural SymbolTable projection now records factual declaration
locations, source-unit kind, and unresolved declared/return/refined/ABI type
spellings and contract clauses for represented AST forms. Refined declarations
own invariant expression IDs; ABI blocks own ordered library, convention, type,
struct, and extern members. Projection performs no type resolution, contract
checking, effect interpretation, or ABI lowering. On 2026-08-16 the project
owner formally declared the v0.24 raw frontend/export border passed after the
accepted-language matrix, independent-consumer gate, and Tier 3 Firetest passed.
The implementation base exports typed structural origins through frontend bundle version 2.
The independent consumer verifies precise roles, canonical AST IDs, source
provenance, uniqueness, scope ownership, and reverse lookup without name
guessing. Flowanalyst now consumes that export and establishes the initial
semantic report, including type/name/call checks, target entrypoint checks,
analysis regions, and a Boolean dependency matrix. Flowbind authorizes selected
external capabilities, Flowoptimize preserves the report boundary, and
Flowlower emits LLVM for explicitly accepted profiles.

Top-level declarations have stable `DeclarationId` values in a canonical arena.
The source unit owns each declaration exactly once. Type references, fields,
parameters, invariant clauses, and ABI clauses remain parent-owned values rather
than receiving unnecessary global IDs.

## Run the current build

```bash
cd Flowmini/flowmini_v25_symboltable_projection

cmake -S . -B cmake-build-debug
cmake --build cmake-build-debug -j20
```

Adjust `-j20` to match your machine.

This directory is the canonical v26 build and test scope. Repository-root build
trees and their legacy superbuild test registrations are noncanonical for this
branch.

## Run the current tests

```bash
cd Flowmini/flowmini_v25_symboltable_projection

cmake --build cmake-build-debug --target flowmini_ast_golden_tests
cmake --build cmake-build-debug --target flowmini_suite
ctest --test-dir cmake-build-debug --output-on-failure
```

Expected result:

```text
normal CMake/Ninja build:      PASS
AST golden tests:              PASS (28)
Symbol projection tests:       PASS (14/14)
downstream sibling CTest gates: PASS
flowcat native ELF example:    PASS
```

## Current important architecture law

```text
TokenTree remembers what the source looked like.
AST states what the source means.
```

## Architecture checkpoint for future lowering

The closed v0.24 AST/export boundary is the inherited base for the v0.26
SymbolTable projection line and future Flowcore transformation pipeline.

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

Active v0.26 supports the inherited root form and the structural named-target
form. Flowanalyst checks target entrypoint completeness; artifact selection and
target-specific lowering remain downstream build work:

```text
A root program may use one root main block or named targets.
```

Named targets are structurally represented; each owns target-local declaration
IDs and may contain a main declaration.

That future direction is documented in:

```text
docs/language/named-targets.md
```

## Current boundary and remaining work

The following are still future or incomplete work:

```text
general source lowering beyond accepted profiles
target selection and separate artifact emission for named multitarget programs
general list/string/file I/O standard library
optimizer transformations beyond the identity boundary
parallelism and CUDA execution policies
self-hosting and bootstrap builds
```

The closed v0.24 completion checklist is:

```text
docs/flowmini/v0.24-accepted-language-coverage.md
```

The binding canonical type identities and layer boundary are recorded in:

```text
docs/flowmini/v0.24-type-policy.md
```

The raw AST preserves written type references. Alias resolution, type checking,
ABI lowering, and complete runtime storage for the canonical primitive family
are not yet implemented.

The completed C5 implementation checkpoint is recorded in:

```text
flowmini_v24_explicit_ast/docs/v0.24-c5-statement-payload-sitrep.md
```
