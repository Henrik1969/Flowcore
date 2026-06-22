# Flowmini v21 — Structural Bridge

This version is a consolidation step. It does not add new Flowmini syntax.
Instead, it introduces the first explicit integration points for the reusable
structural bricks that will support the next frontend refactor.

## New structural subprojects

```text
subprojects/
├── TokenTree/tokentree_v1
└── SymbolTable/SymbolTable_v1_1
```

`TokenTree` is a lossless token/group tree. It sits after lexing and before the
future syntax parser.

`SymbolTable` is policy-free symbol/fact/scope storage. It stores facts; policy
and semantic layers interpret those facts later.

## What changed

- Flowmini now builds with C++20.
- TokenTree is built as a subproject.
- SymbolTable is built as a subproject.
- Flowmini links against both structural libraries.
- A new structural bridge module was added:

```text
include/flowmini_structural.h
src/flowmini_structural.cpp
```

## New inspection modes

Dump the TokenTree created from Flowmini lexer tokens:

```bash
./build/flowmini --dump-token-tree - examples/bool_demo.flow
```

Dump a SymbolTable projection of the lowered FlowIR module:

```bash
./build/flowmini --dump-symbols - examples/bool_demo.flow
```

The normal runtime path is unchanged:

```bash
./build/flowmini examples/bool_demo.flow
```

## Important design boundary

This is intentionally a bridge step, not the final AST refactor.

Current pipeline:

```text
source
  -> import expansion
  -> lexer
  -> TokenTree inspection path
  -> existing parser/lowerer
  -> FlowIR
  -> SymbolTable inspection path over FlowIR
  -> runtime
```

Future target pipeline:

```text
source
  -> source/import manager
  -> lexer
  -> TokenTree
  -> parser / syntax checker
  -> AST
  -> SymbolTable population
  -> semantic checker
  -> lowerer
  -> FlowIR
  -> interpreter / emitter
```

The old parser still consumes the raw token vector in this version. TokenTree is
now proven as a buildable linked structural component and is exposed through a
dump path. SymbolTable is now proven as a buildable linked structural component
and is used to inspect lowered FlowIR names, nodes, wires, and policies.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j20
ctest --test-dir build --output-on-failure
```

## Tested

- TokenTree smoke tests
- SymbolTable tests
- `examples/bool_demo.flow`
- `examples/bool_loop_demo.flow`
- `examples/fn_bool_demo.flow`
- `examples/internal_record_demo.flow`
- `examples/comment_demo.flow`
- TokenTree dump path
- SymbolTable dump path

## Why this version matters

This creates the first clean seam for the frontend refactor.

The next structural step can make the parser consume TokenTree instead of the raw
token vector. After that, SymbolTable can be populated from parsed AST declarations
rather than from lowered FlowIR.
