# Changelog

This repository is a language-design and implementation lab. Numbered Flowmini
stages are intentionally preserved as implementation checkpoints.

## Current checkpoint

- Active implementation: `Flowmini/flowmini_v25_symboltable_projection`
- Active branch: `v25-symboltable-projection`
- Milestone: factual SymbolTable projection maturation and frontend-bundle
  hardening before semantic analysis
- Current integration baseline: AST 26/26, SymbolTable projection 12/12,
  frontend bundle seven goldens plus one isolated run and nineteen negative
  attacks, categorized suite 78/78, CTest 2/2
- Current bundle contract: `flowmini.frontend_bundle` version 2

The detailed implementation history is maintained in
[`Flowmini/CHANGELOG.md`](Flowmini/CHANGELOG.md), and the active/historical stage
map is maintained in [`Flowmini/VERSION_INDEX.md`](Flowmini/VERSION_INDEX.md).

## Flowmini v25_symboltable_projection

- Activated from the tagged `flowmini-v0.24-frontend-border` checkpoint.
- Preserved v0.24 as a closed implementation line.
- Added typed AST-to-symbol and AST-to-scope origin provenance.
- Hardened the versioned frontend bundle and independent consumer.
- Kept semantic analysis, Graph IR, and runtime redesign outside the v0.25
  boundary.

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
