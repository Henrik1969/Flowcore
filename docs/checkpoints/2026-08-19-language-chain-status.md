---
title: Flowcore v0.26 language-chain status
status: current-experimental
date: 2026-08-19
---

# Flowcore v0.26 language-chain status

This is the current implementation summary for the active
`v25-symboltable-projection` branch. It supersedes older status wording that
described semantic analysis and lowering as entirely future work. Historical
v0.24 and early-v0.25 documents remain available as evidence for their own
checkpoints.

## Verified chain

```text
Flowcore source
  -> FlowMini AST + SymbolTable + frontend bundle v2
  -> Flowanalyst semantic report v1
  -> Flowbind capability and ABI verification
  -> Flowoptimize optimization report v1
  -> Flowlower LLVM IR
  -> clang
  -> native ELF executable
```

The [`flowcat` application example](../../Flowmini/flowmini_v25_symboltable_projection/examples/apps/flowcat/README.md)
executes this chain and verifies its output.

## Implemented boundaries

FlowMini provides typed AST and SymbolTable projection, structural provenance,
parameterized `main`, named-target structure, and versioned frontend export.

Flowanalyst provides frontend-diagnostic preservation, declared type checks,
identifier and call resolution, call arity, refined invariants, record fields,
external ABI requirements, target entrypoint checks, analysis regions, and a
Boolean dependency matrix.

Flowbind verifies explicitly granted dynamic-library symbols and supported C
ABI signatures without executing foreign code.

Flowoptimize preserves the semantic boundary and profile metadata. Its current
transform list is empty; this is an identity optimization stage, not an
optimizer claim.

Flowlower emits LLVM for explicitly accepted profiles:

- `empty_program_main`;
- `abi_abs_main`;
- `abi_strlen_main`;
- `flowcat_argv_main`.

## Multitarget status

Named targets are represented and semantically checked. A source may contain,
for example, `cli` and `daemon`, each with exactly one `main`. Target selection
and separate target artifact emission are not yet implemented by Flowlower.

## Current limits

- general source lowering is not implemented;
- Flowoptimize has no transformation yet;
- `flowcat` prints argv strings, not file contents;
- file I/O, general list/string libraries, parallelism policies, and CUDA
  providers are future capability stages;
- self-hosting and bootstrap builds are future stages;
- the project remains experimental and is not production-ready.

## Verification

The current source gates contain 28 AST golden tests and 14 SymbolTable
projection tests. The sibling CTest gates cover Flowanalyst, Flowbind,
Flowoptimize, and Flowlower. The application runner is:

```sh
Flowmini/flowmini_v25_symboltable_projection/examples/apps/flowcat/run-flowcat.sh
```

Use `--keep-build` to preserve the bundle, reports, LLVM, and ELF binary for
inspection.
