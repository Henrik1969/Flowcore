# Lyraform compiler

This directory contains the current Lyraform compiler implementation. It is
the continuation of the executable Flowmini prototype that formerly served as
the active Flowcore language line.

## Current stage

```text
implementation: Lyraform/compiler
lineage:       v0.29 reusable native language chain
authority:     main
toolchain:     igor
status:        experimental / unstable / not production-ready
```

The compiler carries source through AST, structural symbols, semantic analysis,
exact capability binding, planning, optimization, and generic LLVM lowering.
Scalar source graphs execute fresh receiver frames, and `flow_less` owns its
pager semantics in Flow. The [verified ledger](../docs/checkpoints/2026-09-07-reusable-flow-chain-result.md)
records the accepted surface and evidence.

## Focused development

```bash
cd Lyraform/compiler
cmake -S . -B cmake-build-debug -G Ninja
cmake --build cmake-build-debug
ctest --test-dir cmake-build-debug --output-on-failure
```

The repository-root build is the canonical clean-checkout gate. The stage
executable is still named `flowmini` for compatibility with existing scripts
and artifact fixtures; `igor` is the human-facing Lyraform driver.

## Historical lineage

The earlier `flowmini_v24_explicit_ast` tree is retained as a historical
checkpoint. Version labels such as v23, v24, v25, v28, and v29 are technical
lineage markers, not alternate current project identities.

The current language and semantic model are Lyraform. Flowmini remains a
historical prototype name and a compatibility namespace where recorded
artifacts require it.
