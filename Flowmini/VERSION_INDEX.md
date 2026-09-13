# Flowmini Version Index

Flowmini has gone through several numbered implementation stages.

Earlier stages are historical snapshots. They are useful for understanding development history, but they are not the active implementation line.

Current active version:

```text
v29 — reusable native language chain
```

## Active version

| Version | Directory | Main purpose |
|---:|---|---|
| v29 | `flowmini_v29_reusable_native_chain` | Reusable native language chain from frontend export through semantic analysis, capability binding, LLVM, and native ELF |

## Recent historical versions

| Version | Directory | Main purpose |
|---:|---|---|
| v26 | `flowmini_v25_symboltable_projection` | Historical verified language chain retained as the implementation base for v27 |
| v25 | `flowmini_v25_symboltable_projection` | Historical projection milestone retained as the implementation base for v26 |
| v24 | `flowmini_v24_explicit_ast` | Closed raw frontend/export border with typed explicit AST |
| v23 | `flowmini_v23_token_tree_bridge` | TokenTree bridge made visible and trustworthy |
| v22 | `flowmini_v22_unit_kinds` | `program`/`unit`, categorized examples, expected suite |
| v21 | `flowmini_v21_structural_bridge` | TokenTree + SymbolTable structural bridge |
| v20 | `flowmini_v20_bool` | Bool / predicate results |
| v19 | `flowmini_v19_comments` | Comments |

## Earlier historical versions

| Version | Directory | Main purpose |
|---:|---|---|
| v1 | `flowmini_v1_pattern_introduction` | Initial pattern introduction |
| v2 | `flowmini_v2_general` | Generalization step |
| v3 | `flowmini_v3_layers` | Layering step |
| v4 | `flowmini_v4_lists` | Lists |
| v5 | `flowmini_v5_frontend` | Frontend work |
| v6 | `flowmini_v6_scopes` | Scopes |
| v7 | `flowmini_v7_if_else` | If/else |
| v8 | `flowmini_v8_list_indexing` | List indexing |
| v9 | `flowmini_v9_break_continue` | Break/continue |
| v10 | `flowmini_v10_compound_expressions` | Compound expressions |
| v12 | `flowmini_v12_fn_value_ports` | Functions with value ports |
| v13 | `flowmini_v13_imports` | Imports |
| v15 | `flowmini_v15_abi_bindings` | ABI bindings |
| v16 | `flowmini_v16_abi_pointer_contracts` | ABI pointer contracts |

Missing version numbers are historical gaps or discarded intermediate experiments.

## Current location policy

Current active development should happen in:

```text
Flowmini/flowmini_v29_reusable_native_chain
```

The v25-named directory is retained in the historical index for provenance;
the active v0.29 implementation has an explicit current directory name.

Older versions should not be mistaken for the active implementation.
