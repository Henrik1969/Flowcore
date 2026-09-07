# Flowmini Documentation

Flowmini is the executable prototype/lab language used to explore Flowcore ideas.

Current active version:

```text
Flowmini v0.29 language-maturation and cross-backend parity slice

Implementation base: `Flowmini/flowmini_v25_symboltable_projection` (retained
historical directory name)
```

Current baseline:

```text
root CTest: 81/82 (one baseline failure in the variant-construction JSON probe)
AST golden tests: 28 PASS
Symbol projection tests: 14 PASS
categorized Flowmini fixture suite: 87/136 (49 known gaps)
```

Current architecture checkpoint:

```text
TokenTree remembers what the source looked like.
AST states what the source means.
```

The structural frontend export remains consumable by Flowanalyst, Flowbind,
Flowoptimize, and Flowlower. Normal runtime execution still uses a separate
parser; variant payload lowering is explicitly unsupported. See the
[Programmer's Guide](../language/flowmini-programmers-manual.md),
[comparisons](../language-comparisons/README.md), and
[future-work evidence](../FUTURE_WORK_EVIDENCE.md).

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
- [v0.26 language-chain status](../checkpoints/2026-08-19-language-chain-status.md)
- [Project-wide verification gates and Firetest policy](../development/verification-gates.md)

Implementation-base note:

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
