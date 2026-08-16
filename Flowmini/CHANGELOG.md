# Flowmini Changelog

## v24_explicit_ast — active

- Added observable typed AST payloads for expressions, type references, and
  statements.
- Added recursive precedence-aware expression ownership and validation.
- Added arena-owned statements and blocks with stable IDs and structural-parent
  validation.
- Completed the C5 typed-statement migration, including explicit conditional,
  loop, placement, assignable-target, and return source-form ownership.
- Added structural SymbolTable projection from the AST; current focused golden
  coverage is 2/2 and remains incomplete across the accepted language.
- Established the accepted-language coverage matrix and canonical type policy.
- Current gates: AST goldens 21/21, SymbolTable projections 2/2, suite 78/78,
  and CTest 2/2.
- Semantic analysis, complete ABI/refined-contract AST coverage, and Graph IR
  remain future work.

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
