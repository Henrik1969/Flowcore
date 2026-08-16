# Flowmini v0.25 — SymbolTable Projection Maturation

This is the active Flowmini implementation line on branch:

```text
v25-symboltable-projection
```

It inherits the tagged `flowmini-v0.24-frontend-border` implementation. v0.24
proved that an independent process can consume the exported raw AST and
SymbolTable without linking Flowmini internals or reparsing source.

## Purpose

v0.25 matures that structural projection boundary:

- broaden and harden AST-to-SymbolTable projection coverage;
- preserve factual metadata and source provenance;
- make AST-to-symbol and AST-to-scope origins precise and testable;
- expand projection and independent-consumer goldens;
- harden the versioned frontend bundle contract;
- prepare a trustworthy input boundary for later semantic analysis.

v0.25 does not perform semantic type resolution, alias normalization, contract
satisfaction, Graph IR construction, or runtime lowering.

The authoritative line status is
[v0.25 SymbolTable projection status](docs/v0.25-symboltable-projection-status.md).

## Build

From this directory:

```bash
cmake -S . -B cmake-build-debug -G Ninja
cmake --build cmake-build-debug -j20
```

## Gates

```bash
cmake --build cmake-build-debug --target flowmini_frontend_bundle_tests
cmake --build cmake-build-debug --target flowmini_symbol_projection_tests
cmake --build cmake-build-debug --target flowmini_ast_golden_tests
cmake --build cmake-build-debug --target flowmini_suite
ctest --test-dir cmake-build-debug --output-on-failure
```

Opening baseline:

```text
AST golden tests:          26/26
Symbol projection tests:   11/11
Frontend bundle tests:      5 golden, 1 isolated, 11 negative
Flowmini suite:             78/78
CTest:                       2/2
```

Build trees, runtime build output, test reports, and cache files are local
artifacts and must not be committed.
