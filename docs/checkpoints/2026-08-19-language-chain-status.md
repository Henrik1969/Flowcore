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

## Vertical-slice maturity gate

The current narrow source-to-ELF slice is mature enough for continued additive
development: every stage has a versioned JSON boundary, rejected input stops
the chain, accepted profiles are executable-tested, and the source path
survives Flowanalyst -> Flowoptimize -> Flowlower as inspectable provenance.
This is a maturity claim for the proven profiles and test corpus only; it is
not a claim that Flowmini is a complete general-purpose language compiler.

## Verified chain

```text
Flowcore source
  -> FlowMini AST + SymbolTable + frontend bundle v2
  -> Flowanalyst semantic report v1
  -> Flowparallel execution plan v1
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

Flowparallel derives a conservative execution-plan artifact from semantic
analysis. It preserves provenance, carries a derived Boolean COO graph matrix
view, proves a narrow class of pure/disjoint parallel candidates, and retains a
serial fallback. CPU execution is implemented only behind the approved task
API. CUDA capability probing, cuBLAS execution, host firetests, and runtime
benchmark evidence are available as optional deployment projections.

Flowoptimize preserves the semantic boundary and profile metadata. Its first
transform is the identity-preserving `coo_deduplicate` pass over the derived
Boolean graph-to-matrix projection, with proof and before/after counts. This is
still not a general optimizer:
selection is runtime- and cost-deferred, and the source path remains provenance.

Flowlower emits LLVM for explicitly accepted profiles:

- `empty_program_main`;
- `abi_abs_main`;
- `abi_strlen_main`;
- `flowcat_argv_main`;
- `flowcat_file_main`, using an explicit `file_io` capability contract and
  policy-authorized `open`/`read`/`write`/`close` symbols.

Its report preserves the same source path and identifies the LLVM projection.

## Multitarget status

Named targets are represented and semantically checked. A source may contain,
for example, `cli` and `daemon`, each with exactly one `main`. Target selection
is now explicit at Flowlower with `--target`; missing or unknown selections are
blocked, while targetless programs retain the `main` default. The accepted
`empty_program_main` profile now emits separate target-specific LLVM artifacts;
broader per-target lowering remains future work.

## Current limits

- general source lowering is not implemented;
- Flowoptimize has only the identity-preserving `coo_deduplicate` transform;
- `flowcat_file_main` is intentionally narrow: paths are argv inputs, data is
  streamed through a fixed 4096-byte buffer, and failures return status 1;
- general list/string libraries, richer file metadata/errors, broader
  parallelism policies, and general graph algebra transforms remain future
  capability stages; the first CUDA provider and matrix execution projection
  are now experimentally verified;
- self-hosting and bootstrap builds are future stages;
- the project remains experimental and is not production-ready.

## Verification

The current source gates contain 28 AST golden tests and 14 SymbolTable
projection tests. The sibling CTest gates cover Flowanalyst, Flowbind,
Flowoptimize, and Flowlower; the current language-chain suites pass 1/1,
2/2, 1/1, and 3/3 tests respectively. The integrated matrix covers 9
accepted and 1 deliberately blocked semantic case, and the pass corpus carries
43 programs through semantic and lowering boundaries. The application runner
is:

```sh
Flowmini/flowmini_v25_symboltable_projection/examples/apps/flowcat/run-flowcat.sh
```

Use `--keep-build` to preserve the bundle, reports, LLVM, and ELF binary for
inspection.

The larger integration corpus is run with:

```sh
tools/run-flowcore-integration-corpus.sh
```

It currently carries three multi-feature programs through the complete
Flowmini -> Flowanalyst -> Flowparallel -> Flowoptimize chain.
