# Flowmini

Flowmini is the executable prototype language used to test and harden Flowcore ideas.

It is not the final Flowcore language. It is the laboratory where syntax, AST structure, semantic rules, lowering ideas, diagnostics, and tooling are made visible before they become larger Flowcore architecture.

## Current active stage

The current active implementation is:

```text
Flowmini/flowmini_v25_symboltable_projection
```

Current stage theme:

```text
Flowmini v0.25 SymbolTable projection maturation
```

The v0.25 line matures the factual AST-to-SymbolTable projection and its
independent export boundary without beginning semantic analysis.

## Current status

Flowmini is experimental and unfinished.

```text
lexer/token groundwork             usable
TokenTree/source structure          observable
explicit AST                        v0.24 raw-frontend coverage gate passed
recursive expression AST            implemented and golden-guarded
C5 typed statements/blocks          complete
structural SymbolTable projection   typed origins independently validated
canonical type policy               binding, runtime unimplemented
semantic analysis                   mostly future work
type checking                       mostly future work
Graph IR lowering                   mostly future work
runtime semantics                   mostly future work
```

The inherited v0.24 AST represents a recursive, precedence-aware expression graph
for the established expression surface, including:

```text
function calls
unary expressions
binary expressions
index expressions
field access
list literals
record literals
```

Expressions use typed payloads and recursively owned child IDs. Parenthesized
grouping, operator precedence, associativity, prefix operators, and nested
postfix call/index/field forms are regression-guarded. The v0.24 raw
frontend/export border is closed; v0.25 now strengthens projection coverage,
factual metadata, cross-links, provenance, and bundle stability.

## Why does this look ordinary?

At the surface, early Flowmini examples may look like a small conventional programming language.

That is intentional.

The current goal is not novelty syntax first. The current goal is to make the language pipeline visible and testable:

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

Flowmini v0.25 is focused on SymbolTable projection maturation.

## Build quickstart

From the active v0.25 directory:

```bash
cd Flowmini/flowmini_v25_symboltable_projection

cmake -S . -B cmake-build-debug
cmake --build cmake-build-debug -j20
```

Adjust `-j20` to match your machine.

The canonical v25 scope is this implementation directory and its CMake targets.
Repository-root build trees are legacy/noncanonical for the active branch.

## Test quickstart

Run the AST golden regression tests:

```bash
cmake --build cmake-build-debug --target flowmini_ast_golden_tests
```

Run the full Flowmini suite:

```bash
cmake --build cmake-build-debug --target flowmini_suite
```

Run the registered CTest gates:

```bash
ctest --test-dir cmake-build-debug --output-on-failure
```

Expected current result:

```text
normal CMake/Ninja build:      PASS
flowmini_ast_golden_tests:     PASS (26)
flowmini_symbol_projection:    PASS (12/12)
flowmini_frontend_bundle:      PASS (7 golden, 1 isolated, 19 negative)
flowmini_suite:                PASS (78/78)
CTest:                         PASS (2/2)
```

## Useful commands

Dump an AST:

```bash
./cmake-build-debug/flowmini --dump-ast examples/ast/operator_expression_probe.flow
```

Run the AST golden helper directly:

```bash
tools/run-flowmini-ast-golden-tests.sh
```

Update AST goldens intentionally after an expected AST output change:

```bash
FLOWMINI_UPDATE_AST_GOLDENS=1 tools/run-flowmini-ast-golden-tests.sh
```

Always inspect the diff before committing updated goldens.

## Important paths

```text
flowmini_v25_symboltable_projection/include/
    public headers

flowmini_v25_symboltable_projection/src/
    implementation

flowmini_v25_symboltable_projection/examples/ast/
    AST-focused source examples

flowmini_v25_symboltable_projection/tests/expected/ast/
    golden AST JSON outputs

flowmini_v25_symboltable_projection/tools/
    maintained helper scripts

flowmini_v25_symboltable_projection/docs/
    status notes and implementation documentation
```

## Archived earlier stages

Earlier Flowmini stages are preserved in the repository archive.

They are useful historical material, but they are not the active implementation line.

Current active development should happen in:

```text
Flowmini/flowmini_v25_symboltable_projection
```

## Design direction

Flowmini follows the broader Flowcore principle:

```text
model the system before mutating it
```

Important current distinction:

```text
TokenTree remembers what the source looked like.
AST states what the source means.
```

The current v0.25 work is about making its SymbolTable projection a trustworthy
input for independent tools and later semantic analysis.

## Documentation map

Recommended reading order:

```text
Flowmini/README.md
Flowmini/CURRENT.md
Flowmini/flowmini_v25_symboltable_projection/docs/v0.25-symboltable-projection-status.md
docs/flowmini/v0.24-accepted-language-coverage.md
docs/flowmini/v0.24-type-policy.md
Flowmini/flowmini_v24_explicit_ast/docs/v0.24-c5-statement-payload-sitrep.md
Flowmini/flowmini_v24_explicit_ast/docs/v0.24-shallow-expression-ast-sitrep.md (historical checkpoint)
Flowmini/flowmini_v24_explicit_ast/docs/v0.24-future-transformation-boundary.md
docs/architecture/compiler-transformation-revision-model.md
Flowmini/flowmini_v24_explicit_ast/examples/ast/README.md
docs/language/named-targets.md
```

## Warning

This project is experimental.

Do not use Flowmini for production, safety-critical, security-critical, financial, legal, medical, or operational workloads.

The implementation is a research/prototype line intended to make language and system architecture decisions visible and testable.
