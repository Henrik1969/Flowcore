# Architecture Notes

Project-wide architecture notes belong here.

Current intended compiler direction:

```text
source files
  -> import/source loading
  -> lexer
  -> TokenTree
  -> parser / AST builder
  -> semantic AST
  -> SymbolTable / name and type resolution
  -> semantic validation and canonicalization
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

Current active implementation work is Flowmini v0.24 explicit AST
stabilization. Graph IR, optimizer policy, target lowering, and provider
selection remain future work.

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
- [Prerequisites](prerequisites.md)
