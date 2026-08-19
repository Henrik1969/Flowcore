# Architecture Notes

Project-wide architecture notes belong here.

## Document authority

Architecture documents distinguish:

- **Binding** architectural laws and approved boundaries that implementations
  must preserve.
- **Provisional** mechanisms, syntax, or engineering defaults that express an
  approved direction but still require focused design and evidence.
- **Exploratory** alternatives and research notes that are not decisions.
- **Historical/current-state** evidence that records what was observed at a
  particular checkpoint rather than permanent law.

A document may contain more than one class, but it must label the boundary
between them. Stated intent is preserved; an implementation sketch does not
become binding merely by appearing beside a binding law.

Current intended compiler direction:

```text
source files
  -> import/source loading
  -> lexer
  -> TokenTree
  -> parser / AST builder
  -> complete raw AST
  -> structural SymbolTable projection
  -> semantic analysis / facts / contracts / diagnostics
  -> Graph IR
  -> graph analysis / pruning / transformation / policy
  -> optimized Graph IR
  -> target lowering
  -> target IR
  -> emitter
  -> artifact/runtime
```

The important architectural boundaries are documented before their full
implementations exist so the current default path does not accidentally become
the permanent definition of Flowcore.

Current active implementation is a verified v0.25 language-chain slice. It
inherits the closed v0.24 raw frontend/export border, establishes semantic
analysis and policy-binding consumers, preserves an optimization boundary, and
lowers selected profiles to executable LLVM/ELF artifacts. Graph IR, real
optimizer transforms, generalized target selection, parallelism policies, CUDA
providers, and self-hosting remain future expansion work.

Planned and active architecture topics:

- pipeline boundaries
- TokenTree
- SymbolTable
- semantic AST
- semantic checker split
- Graph IR
- target-independent transformation and pruning
- policy-directed optimization
- revisioned IR and transformation provenance
- target lowering and emitters
- runtime backends
- bytecode
- effects and failure-flow

Foundational notes:

- [Flowcore core promise](flowcore-core-promise.md)
- [Transformation and revision architecture](compiler-transformation-revision-model.md)
- [Revisioned node identity](revisioned-node-identity.md)
- [Prerequisites](prerequisites.md)
- [FrankenCore conformance declaration](frankencore-conformance.md)

`flowcore-core-promise.md` is binding foundational intent.
`compiler-transformation-revision-model.md` contains binding stage laws and
provisional implementation mechanisms. `prerequisites.md` contains a binding
environment-contract rule with provisional syntax and taxonomy.

## Flowmini and AstLib boundary

Flowmini keeps its dedicated, typed AST and parser. AstLib is an independent,
language-neutral capability intended to mature through other language
experiments. Flowmini is not to be rewritten around AstLib merely to unify tree
storage. A future narrow adapter may be considered after AstLib matures and a
measurable benefit is demonstrated; it must preserve Flowmini's typed semantic
nodes and ownership laws.
