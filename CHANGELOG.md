# Changelog

This repository is a language-design and implementation lab. Numbered Flowmini
stages are intentionally preserved as implementation checkpoints.

## Current checkpoint

- Added the first executable read-only kernel profiles: `getpid` and
  `clock_gettime`, followed by the one-byte `getrandom` profile; each is
  policy-gated and tested through native LLVM-to-ELF execution. The remaining
  kernel declarations remain binding-ready but lowering-deferred.

- First real I/O capability slice: `flowcat_file_main` reads argv-supplied
  files through policy-authorized `open`/`read`/`write`/`close` bindings.

- Active milestone: Flowcore v0.26 language-chain vertical slice
- Implementation base: `Flowmini/flowmini_v25_symboltable_projection`
- Active branch retained for continuity: `v25-symboltable-projection`
- Current integration baseline: AST 28/28, SymbolTable projection 14/14,
  frontend bundle eight goldens plus one isolated run and nineteen negative
  attacks, downstream CTest green, and native `flowcat` ELF execution
- Current bundle contract: `flowmini.frontend_bundle` version 2

The detailed implementation history is maintained in
[`Flowmini/CHANGELOG.md`](Flowmini/CHANGELOG.md), and the active/historical stage
map is maintained in [`Flowmini/VERSION_INDEX.md`](Flowmini/VERSION_INDEX.md).

## Flowcore v0.26 language-chain vertical slice

- Activated from the tagged `flowmini-v0.24-frontend-border` checkpoint.
- Preserved v0.24 as a closed implementation line.
- Added typed AST-to-symbol and AST-to-scope origin provenance.
- Hardened the versioned frontend bundle and independent consumer.
- Added semantic analysis, capability binding, optimization preservation,
  explicit LLVM lowering profiles, and the first native ELF application.

## Flowmini v25_symboltable_projection — historical projection milestone

- Established the typed AST-to-symbol and AST-to-scope origin boundary.
- Hardened the versioned frontend bundle and independent consumer.

## Flowmini v24_explicit_ast

- Completed the typed raw AST and factual structural SymbolTable projection.
- Established the versioned frontend export and independent-consumer boundary.
- Closed the raw frontend/export border with recorded Tier 3 Firetest evidence.

## Flowmini v22_unit_kinds — historical

- Added explicit source unit roles:
  - `program` = executable/root source unit
  - `unit` = defining/importable source unit
- Enforced import law:
  - units may be imported
  - programs may not be imported as ordinary units
  - imported files must not define `main`
  - root execution requires a program with `main`
- Categorized examples.
- Added expected stdout and diagnostic substring suite support.
- Suite passes `75 / 75`.

## Flowmini v21_structural_bridge — historical

- Integrated TokenTree as a structural token-tree inspection path.
- Integrated SymbolTable as a structural symbol projection path.
- Preserved existing execution behavior.

## Earlier Flowmini snapshots

Earlier snapshots are preserved under `_archive/flowmini/previous-stages/` and
indexed in `Flowmini/VERSION_INDEX.md`.
