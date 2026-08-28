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
Flowmini v0.27 namespaced provider language-chain slice
```

The v0.27 line now carries a source program through AST, SymbolTable,
Flowanalyst, Flowoptimize, Flowlower, LLVM, and a native ELF executable for a
small accepted application subset.

## Current status

Flowmini is experimental and unfinished, but the current chain is executable.

```text
lexer/token groundwork             usable
TokenTree/source structure          observable
explicit AST                        v0.24 raw-frontend coverage gate passed
recursive expression AST            implemented and golden-guarded
C5 typed statements/blocks          complete
structural SymbolTable projection   typed origins independently validated
canonical type policy               represented and checked at current boundary
semantic analysis                   initial report, regions, names, calls, types
type checking                       accepted subset and explicit diagnostics
Graph IR lowering                   downstream boundary not yet generalized
runtime semantics                   executable profiles only
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
frontend/export border is closed; v0.26 now uses the strengthened projection coverage,
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

Flowmini v0.26 is focused on the verified language-chain slice.

## Build quickstart

From the active v0.26 implementation base:

```bash
cd Flowmini/flowmini_v25_symboltable_projection

cmake -S . -B cmake-build-debug
cmake --build cmake-build-debug -j20
```

Adjust `-j20` to match your machine.

The canonical v26 scope is this implementation directory and its CMake targets.
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
AST golden tests:              PASS (28)
Symbol projection tests:       PASS (14/14)
downstream sibling CTest gates: PASS
flowcat native ELF example:    PASS
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

The current v0.26 work makes the SymbolTable projection a trustworthy input for
independent tools and carries selected programs through the complete language
chain.

## Documentation map

Recommended reading order:

```text
docs/language/flowmini-programmers-manual.md
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
docs/language/target-artifact-model.md
```

## Warning

This project is experimental.

Do not use Flowmini for production, safety-critical, security-critical, financial, legal, medical, or operational workloads.

The implementation is a research/prototype line intended to make language and system architecture decisions visible and testable.
