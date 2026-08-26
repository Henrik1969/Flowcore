# Flowcore v0.28 maturation ledger

## Baseline

- Branch: `v25-symboltable-projection`
- Starting commit: `fdbded38bc23cbd2f333df02b1dc0a9192203ac9`
- Upstream at inspection: `origin/v25-symboltable-projection` at the same commit
- Starting worktree: clean
- Previous autonomous mission: complete with `.codex-run-state` equal to `DONE`
- Toolchain: CMake 3.28.3, GNU Makefiles, GCC/G++ 13.3.0, Ninja 1.11.1,
  Clang/LLVM 18.1.3
- Canonical command: `cmake --build build -j4`
- Canonical test command: `ctest --test-dir build --output-on-failure`
- Result: build passed; 54/54 tests passed in 21.41 seconds

## Confirmed starting behavior

- The v0.27 reusable profile-free native chain is present.
- Flowbind and Flowlower already contain stricter structured parsing.
- Flowparallel and Flowoptimize require a complete authority-search inventory
  before contract migration.
- Canonical Graph IR, executable safety profiles, complete boundedness proofs,
  revisioned identity storage, and permanent writable-storage semantics remain
  future work.

## Current phase

Gate 1: inventory every public artifact contract and every text-based authority
decision.

## Exact next action

Produce a checked-in producer/consumer/schema/authority/parser/test inventory,
classify all raw searches in required stages, and identify the smallest first
typed vertical slice.
