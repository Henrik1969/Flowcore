# Flowmini Roadmap

## Current checkpoint

```text
v22_unit_kinds
build: OK
suite: 75 / 75
```

## Immediate

1. Commit expected suite outputs and diagnostics.
2. Clean compiler warnings.
3. Add programmer's manual draft.
4. Establish Valgrind baseline.
5. Establish GDB-on-failure baseline.

## Next structural versions

### v23_token_tree_parser_bridge

Move parser input toward TokenTree without changing language behavior.

### v24_explicit_ast

Introduce explicit AST as parser output.

### v25_symboltable_from_ast

Populate SymbolTable from AST declarations before lowering.

### v26_semantic_checker_split

Separate syntax parsing, symbol collection, semantic checking, and lowering.

## Later runtime work

- FlowIR bytecode format
- portable switch-dispatch VM
- GNU computed-goto threaded VM backend
- C++ emitter backend
- precompiled bytecode artifacts
- precompiled unit/program artifacts
