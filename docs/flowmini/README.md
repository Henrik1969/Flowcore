# Flowmini Documentation

Flowmini is the executable prototype/lab language used to explore Flowcore ideas.

Current active version:

```text
Flowmini/flowmini_v24_explicit_ast
```

Current baseline:

```text
build: PASS
AST golden tests: 11
suite: 76 / 76
```

Current architecture checkpoint:

```text
TokenTree remembers what the source looked like.
AST states what the source means.
```

The v0.24 AST is being stabilized so later semantic analysis, Graph IR,
target-independent transformation, policy, and target lowering can be added
behind explicit stage boundaries.

Documents in this directory:

- [Roadmap](roadmap.md)
- [Testing](testing.md)

Active v0.24 implementation notes:

- [Explicit AST status](../../Flowmini/flowmini_v24_explicit_ast/docs/v0.24-explicit-ast-status.md)
- [Shallow expression AST sitrep](../../Flowmini/flowmini_v24_explicit_ast/docs/v0.24-shallow-expression-ast-sitrep.md)
- [Future transformation boundary](../../Flowmini/flowmini_v24_explicit_ast/docs/v0.24-future-transformation-boundary.md)

Project-wide architecture:

- [Transformation and revision architecture](../architecture/compiler-transformation-revision-model.md)

Planned:

- programmer's manual
- language reference
- ABI notes
- examples guide
