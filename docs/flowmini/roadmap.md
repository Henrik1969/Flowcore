# Flowmini Roadmap

Authority: current gates are verified state. Milestone order expresses approved
direction; version numbers and detailed implementation mechanisms remain
provisional until each milestone is activated.

## Current checkpoint

```text
v25_symboltable_projection
build: OK
AST golden tests: 28 / 28
Symbol projection tests: 14 / 14
Frontend bundle tests: 8 golden, 1 isolated, 19 negative
downstream language-chain CTest: PASS
flowcat ELF example: PASS
CTest: 2 / 2
```

## Immediate

1. Maintain structural SymbolTable coverage and factual metadata.
2. Maintain the typed-origin and source-provenance contract as the language
   chain grows.
3. Expand semantic checks and accepted lowering profiles one explicit contract
   at a time.
4. Keep every stage independently consumable and provenance-preserving.

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

Maintain the factual AST-to-SymbolTable projection and the independent export
boundary while integrating semantic analysis, capability binding, optimization,
and explicit LLVM lowering profiles.

Advance to v0.26 only when the complete AST and lossless structural SymbolTable
projection form a mature frontend border with stable cross-links, scope
coverage, source locations, and unresolved declared type/contract facts.

### Provisional later label: v26_semantic_checker_split

Separate and deepen syntax parsing, symbol collection, semantic checking, and
lowering. The initial resolution, derived facts, contract satisfaction, type
correctness, and semantic diagnostics already exist in the v25 sibling chain;
v26 expands their coverage and integrity guarantees.

## Later runtime work

- FlowIR bytecode format
- portable switch-dispatch VM
- GNU computed-goto threaded VM backend
- C++ emitter backend
- precompiled bytecode artifacts
- precompiled unit/program artifacts
