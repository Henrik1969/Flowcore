# SymbolTable v1

Policy-free C++20 symbol table library for language-development work.

## Design law

The SymbolTable stores facts.  
The Policy layer interprets facts.  
A future TreeView may expose a policy-filtered projection.  
The Semantic Analyzer makes language decisions.

## What this version contains

- Strong `SymbolId` and `ScopeId` wrappers.
- Global scope creation.
- Child scope creation.
- Symbol insertion.
- Local lookup.
- Upward scope lookup.
- Duplicate/overload preservation.
- First-class symbol fields for common indexing facts.
- Extensible `Factoid` attachment model.
- Debug dump.
- Static and shared library targets.
- Plain assertion-based tests.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j20
ctest --test-dir build --output-on-failure
```

## Outputs

When both build options are enabled:

- `libsymboltable.a`
- `libsymboltable.so`
- headers under `include/symboltable/`

## Intended boundary

The table records facts such as name, kind, scope, visibility marker, source location, declaration state, and custom factoids.

It does not decide whether private access is legal, whether shadowing is allowed, whether overloads are valid, or whether a type conversion is acceptable.
