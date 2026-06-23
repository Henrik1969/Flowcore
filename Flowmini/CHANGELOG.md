# Flowmini Changelog

## v22_unit_kinds

- Added explicit source roles:
  - `program`
  - `unit`
- Enforced import law.
- Categorized examples into `pass`, `fail`, `support`, and `docs`.
- Added expected stdout and diagnostic checks.
- Suite baseline: `75 / 75`.

## v21_structural_bridge

- Integrated TokenTree as a structural token-tree inspection path.
- Integrated SymbolTable as a structural symbol projection path.
- Preserved the existing execution path.

## v20_bool

- Added internal `Bool`.
- Added `true` and `false`.
- Comparisons produce `Bool`.
- `if` and `while` require `Bool`.
- No implicit integer truthiness.

## Earlier versions

See `VERSION_INDEX.md`.
