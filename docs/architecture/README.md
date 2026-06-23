# Architecture Notes

Project-wide architecture notes belong here.

Current direction:

```text
source files
  -> import/source loading
  -> lexer
  -> TokenTree
  -> parser
  -> AST
  -> SymbolTable population
  -> semantic checker
  -> lowerer
  -> FlowIR
  -> runtime/backend
```

Current reality as of Flowmini v22:

```text
source files
  -> import/source loading
  -> lexer
  -> TokenTree inspection path
  -> current parser/lowerer
  -> FlowIR/runtime representation
  -> runtime interpreter
```

Planned architecture topics:

- pipeline boundaries
- TokenTree
- SymbolTable
- AST
- semantic checker split
- runtime backends
- bytecode
- effects and failure-flow

- [Flowcore core promise](flowcore-core-promise.md)
