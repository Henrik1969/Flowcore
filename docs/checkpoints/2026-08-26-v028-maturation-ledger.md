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

## Gate 1 inventory

- Added `docs/architecture/v028-artifact-contract-inventory.md` with the
  producer/consumer/version/authority/parser/test map for every required
  pipeline artifact and directly consumed provider/runtime evidence.
- Classified raw searches in required stage entry points and adjacent
  Flowparallel provider tools.
- Confirmed Flowparallel and Flowoptimize are the largest unsafe authority
  surfaces; both copy malformed or absent authority as empty JSON values.
- Confirmed Flowbind and Flowlower already reject duplicate JSON keys in their
  local parsers, while Flowbind still checks its top-level envelope by raw text.
- Confirmed Flowanalyst parses structurally but silently ignores duplicate JSON
  keys and represents integer identities as `double`.
- Selected the first vertical slice: public strict JSON and common envelope,
  followed by the complete semantic-report subset consumed by Flowparallel.

## Current phase

Gate 2: establish the public strict JSON/diagnostic/serialization foundation and
the first complete typed semantic-report consumption slice.

## Exact next action

Create `Flowcontracts`, wire it into the root build, add strict parser and
deterministic serialization tests, then migrate Flowparallel's semantic-report
consumer without changing its emitted semantics.
