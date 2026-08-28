# TokenTree

TokenTree is a small C ABI library for creating, inspecting, and manipulating a
lossless structural tree over externally supplied tokens.

It is deliberately **not** a parser, grammar checker, semantic analyzer, or AST
builder. The host application owns tokenization, token enum values, source text,
source locations, language rules, diagnostics, and semantic meaning. TokenTree
owns only the generic parent/child token tree.

## Boundary

Host responsibility:

- tokenize source text
- define token kinds
- classify open/close/atom tokens for structural grouping
- validate syntax
- build grammar-specific ASTs
- perform semantic analysis
- own token text lifetime

TokenTree responsibility:

- accept a host token ABI adapter
- store one node per supplied token plus a root node
- group open/close token pairs structurally by `group_id`
- keep close-group tokens in the tree so the tree is lossless
- expose traversal, basic mutation, stable node ids, and status results

## Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Build products:

```text
build/libTokenTree.so
build/libTokenTree.a
```

## Public header

```c
#include "tokentree.h"
```

Main opaque handles:

```c
typedef struct TokenTreeContext TokenTreeContext;
typedef struct TokenTree TokenTree;
typedef uint32_t TokenTreeNodeId;
```

## Token adapter

The host passes a contiguous token buffer and an adapter:

```c
typedef TokenTreeTokenView (*TokenTreeReadTokenFn)(const void *token, void *user_data);
```

The adapter returns:

```c
typedef struct TokenTreeTokenView {
    uint32_t kind;
    TokenTreeTokenClass token_class;
    uint32_t group_id;
    TokenTreeText text;
    TokenTreeSourceRange range;
} TokenTreeTokenView;
```

The `kind` value is host-defined. `token_class` is only a structural hint:

```c
TOKENTREE_TOKEN_CLASS_ATOM
TOKENTREE_TOKEN_CLASS_OPEN_GROUP
TOKENTREE_TOKEN_CLASS_CLOSE_GROUP
```

`TokenTreeText` is borrowed. TokenTree does not copy text bytes. The host must
keep referenced text alive for as long as the tree may be inspected.

## Flowcore interpretation

TokenTree can sit between lexer and parser:

```text
source -> lexer tokens -> TokenTree -> parser -> AST -> semantic checker -> lowerer -> FlowIR
```

In Flowcore terms:

- the token buffer is external signal storage
- `TokenTreeTokenAbi` is the adapter contract
- `tokentree_from_tokens` is a structural BuildTokenTree node
- `TokenTree` is the emitted token-tree artifact
- `TokenTreeStatus` is explicit status/failure flow
