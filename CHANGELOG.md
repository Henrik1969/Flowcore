# Changelog

This repository is a language-design and implementation lab. The numbered folders are intentionally preserved as standalone snapshots.

## Current checkpoint

- Current Flowmini active version: `Flowmini/flowmini_v22_unit_kinds`
- Suite baseline: `75 / 75`
- Source unit law introduced: `program` vs `unit`
- Examples categorized into `pass/`, `fail/`, `support/`, and `docs/`
- Environment-aware suite runner added.

## Flowmini v22_unit_kinds

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

## Flowmini v21_structural_bridge

- Integrated TokenTree as a structural token-tree inspection path.
- Integrated SymbolTable as a structural symbol projection path.
- Preserved existing execution behavior.

## Earlier Flowmini snapshots

Earlier snapshots remain in place under `Flowmini/` and are indexed in `Flowmini/VERSION_INDEX.md`.
