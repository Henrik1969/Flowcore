# Flowmini Documentation

Flowmini is the executable prototype/lab language used to explore Flowcore ideas.

Current active version:

```text
Flowmini/flowmini_v25_symboltable_projection
```

Current baseline:

```text
build: PASS
AST golden tests: 28
Symbol projection tests: 14
downstream language-chain CTest gates: PASS
flowcat native ELF example: PASS
```

Current architecture checkpoint:

```text
TokenTree remembers what the source looked like.
AST states what the source means.
```

The v0.24 frontend-export border is closed. The active v0.25 line makes its
factual SymbolTable projection consumable by Flowanalyst, Flowbind,
Flowoptimize, and Flowlower, with `flowcat` proving a native ELF artifact.

Documents in this directory:

- [Roadmap](roadmap.md)
- [v0.24 accepted-language coverage matrix](v0.24-accepted-language-coverage.md)
- [v0.24 canonical type policy](v0.24-type-policy.md)
- [v0.24 frontend bundle contract](v0.24-frontend-bundle.md)
- [v0.25 frontend bundle contract](v0.25-frontend-bundle.md)
- [v0.25 structural-origin maturity audit](v0.25-origin-maturity-audit.md)
- [C5 typed-statement sitrep](../../Flowmini/flowmini_v24_explicit_ast/docs/v0.24-c5-statement-payload-sitrep.md)
- [Testing](testing.md)
- [v0.24 frontend checkpoint Firetest report](v0.24-firetest-report.md)
- [v0.24 frontend-border Firetest report](v0.24-frontend-border-firetest-report.md)
- [v0.25 SymbolTable projection status](../../Flowmini/flowmini_v25_symboltable_projection/docs/v0.25-symboltable-projection-status.md)
- [Project-wide verification gates and Firetest policy](../development/verification-gates.md)

Active v0.25 implementation note:

- [SymbolTable projection status](../../Flowmini/flowmini_v25_symboltable_projection/docs/v0.25-symboltable-projection-status.md)

Closed v0.24 implementation notes:

- [Explicit AST status](../../Flowmini/flowmini_v24_explicit_ast/docs/v0.24-explicit-ast-status.md)
- [Historical shallow-expression AST sitrep](../../Flowmini/flowmini_v24_explicit_ast/docs/v0.24-shallow-expression-ast-sitrep.md)
- [Future transformation boundary](../../Flowmini/flowmini_v24_explicit_ast/docs/v0.24-future-transformation-boundary.md)

Project-wide architecture:

- [Transformation and revision architecture](../architecture/compiler-transformation-revision-model.md)

Current chain and application example:

- [Parameterized main](v0.25-parameterized-main.md)
- [Flowcat application](../../Flowmini/flowmini_v25_symboltable_projection/examples/apps/flowcat/README.md)
- [Named targets](../language/named-targets.md)
- [Target artifact model](../language/target-artifact-model.md)
