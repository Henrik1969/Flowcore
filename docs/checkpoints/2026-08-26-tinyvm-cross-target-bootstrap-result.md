# TinyVM cross-target/bootstrap mission result

**Date:** 2026-08-26

**Branch:** `v25-symboltable-projection`

**Closure tag:** `tinyvm-cross-target-bootstrap-complete`

## Achieved

- Recovered and froze the historical calculated-goto TinyVM, retained its
  portable switch engine, and proved engine equivalence.
- Defined deterministic, independently validated TinyVM executable artifacts
  with typed values, constants, storage, imports and provenance.
- Made LLVM and TinyVM consume the same captured backend-neutral lowering file.
- Closed the provider-free scalar surface and admitted exact governed host-call
  tuples; every remaining current LLVM tuple is explicitly inventoried as
  unsupported instead of falsely accepted.
- Added canonical target-policy artifacts and name-only `llvm-host` versus
  `tinyvm-portable` builds with no implicit fallback or partial wrong-backend
  output.
- Added callable lowering-plan v2 and executed one ordinary shared
  tokenizer/document scalar classifier through LLVM and both TinyVM engines.
- Captured the Stage 0 host seed and published a validated dependency graph for
  every remaining Stage 1/2/3 bootstrap requirement.

## Verification

- GCC superbuild: successful.
- Canonical CTest suite: 75/75 passed.
- Focused Clang 18 ASan/UBSan suites: passed with the documented
  `detect_leaks=0` environmental exclusion.
- Callable artifact: deterministic across repeated lowering.
- Callable observable result: LLVM = TinyVM switch = TinyVM computed = 1.
- Worktree: clean before this closure record.

## Explicit non-claims

Flowmini is not yet self-hosted, Stage 1 has not begun, and FlowOpenOffice has
not been ported. UTF-8 source reading, recursive tagged arenas, generic
collections, recovering parsing, canonical artifact I/O in Flow, package
linking and ownership/cleanup remain open with exact owners, dependencies and
acceptance evidence in `docs/bootstrap/remaining-bootstrap-inventory-v1.json`.

The mission proved the governed architecture and first shared vertical slice;
it did not rename future work as completion.
