# Flowmini Documentation

Flowmini is the executable prototype/lab language used to explore Flowcore ideas.

Current active version:

```text
Flowmini/flowmini_v24_explicit_ast
```

Current baseline:

```text
build: PASS
AST golden tests: 21
Symbol projection tests: 7
suite: 78 / 78
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
- [v0.24 accepted-language coverage matrix](v0.24-accepted-language-coverage.md)
- [v0.24 canonical type policy](v0.24-type-policy.md)
- [C5 typed-statement sitrep](../../Flowmini/flowmini_v24_explicit_ast/docs/v0.24-c5-statement-payload-sitrep.md)
- [Testing](testing.md)

Active v0.24 implementation notes:

- [Explicit AST status](../../Flowmini/flowmini_v24_explicit_ast/docs/v0.24-explicit-ast-status.md)
- [Historical shallow-expression AST sitrep](../../Flowmini/flowmini_v24_explicit_ast/docs/v0.24-shallow-expression-ast-sitrep.md)
- [Future transformation boundary](../../Flowmini/flowmini_v24_explicit_ast/docs/v0.24-future-transformation-boundary.md)

Project-wide architecture:

- [Transformation and revision architecture](../architecture/compiler-transformation-revision-model.md)

Planned:

- programmer's manual
- language reference
- ABI notes
- examples guide
