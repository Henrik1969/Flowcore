# Current Flowmini Version

Current active implementation:

```text
flowmini_v25_symboltable_projection
```

Current milestone:

```text
Flowmini v0.25 SymbolTable projection maturation
factual projection and frontend-bundle hardening; no semantic analysis yet
```

## Status

```text
normal CMake/Ninja build:      PASS
flowmini_ast_golden_tests:     PASS (26)
flowmini_symbol_projection:    PASS (12/12)
flowmini_frontend_bundle:      PASS (7 golden, 1 isolated, 19 negative)
flowmini_suite:                PASS (78/78)
CTest:                         PASS (2/2)
```

Flowmini is still experimental and unfinished.

The v0.25 line inherits the closed, observable, regression-guarded v0.24 expression
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
v0.25 now exports typed structural origins through frontend bundle version 2.
The independent consumer verifies precise roles, canonical AST IDs, source
provenance, uniqueness, scope ownership, and reverse lookup without name
guessing. It does not
own semantic type resolution, legality checking, contract satisfaction, Graph
IR, or runtime lowering.

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

This directory is the canonical v25 build and test scope. Repository-root build
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
flowmini_ast_golden_tests:     PASS (26)
flowmini_symbol_projection:    PASS (12/12)
flowmini_frontend_bundle:      PASS (7 golden, 1 isolated, 19 negative)
flowmini_suite:                PASS (78/78)
CTest:                         PASS (2/2)
```

## Current important architecture law

```text
TokenTree remembers what the source looked like.
AST states what the source means.
```

## Architecture checkpoint for future lowering

The closed v0.24 AST/export boundary is the inherited base for the v0.25
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

Active v0.25 inherits the v0.24 rule:

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
ordinary capability/provider call model for output; no core Print statement
semantic name resolution
type checking
contract and refined-type validation
Graph IR lowering
runtime execution semantics
provider/capability resolution
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
