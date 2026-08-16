# Flowmini Roadmap

Authority: current gates are verified state. Milestone order expresses approved
direction; version numbers and detailed implementation mechanisms remain
provisional until each milestone is activated.

## Current checkpoint

```text
v25_symboltable_projection
build: OK
AST golden tests: 26 / 26
Symbol projection tests: 11 / 11
Frontend bundle tests: 5 golden, 1 isolated, 11 negative
suite: 78 / 78
CTest: 2 / 2
```

## Immediate

1. Mature structural SymbolTable projection coverage and factual metadata.
2. Harden AST-to-symbol origins, source provenance, and the frontend bundle.
3. Expand projection and independent-consumer goldens.
4. Keep semantic interpretation outside v0.25.

## Next structural versions

### Historical: v23_token_tree_parser_bridge

Move parser input toward TokenTree without changing language behavior.

### Closed: v24_explicit_ast

Introduce explicit AST as parser output.

Advance to v0.25 only when every accepted source construct has a canonical,
lossless raw-AST representation; accepted syntax does not fall through generic
unknown placeholders; structurally valid but semantically invalid programs can
still be represented; ownership, identity, parentage, source provenance, and
arena references are validated; focused goldens and the complete regression
suite pass; and current-state documentation describes the boundary truthfully.

This exit rule passed and was formally declared closed on 2026-08-16.

### Active: v25_symboltable_projection

Mature the factual AST-to-SymbolTable projection and independent export boundary
before semantic analysis.

Advance to v0.26 only when the complete AST and lossless structural SymbolTable
projection form a mature frontend border with stable cross-links, scope
coverage, source locations, and unresolved declared type/contract facts.

### Provisional later label: v26_semantic_checker_split

Separate syntax parsing, symbol collection, semantic checking, and lowering.
This is where resolution, derived facts, contract satisfaction, type
correctness, and semantic diagnostics begin.

## Later runtime work

- FlowIR bytecode format
- portable switch-dispatch VM
- GNU computed-goto threaded VM backend
- C++ emitter backend
- precompiled bytecode artifacts
- precompiled unit/program artifacts
