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

Gates 2–4 first vertical checkpoint:

- Added the public header-only `Flowcontracts` component with a strict
  complete-input parser, duplicate-key rejection, signed 64-bit integers,
  finite floating-point handling, Unicode escapes, JSON-path diagnostics, and
  deterministic lexicographically ordered serialization.
- Documented the additive version-1 unknown-field policy: unknown data cannot
  satisfy or select authority, while required authority is validated.
- Added typed artifact headers, semantic report, lowering-plan identity,
  execution plan, analysis/execution matrix, and provider-decision surfaces.
- Migrated Flowparallel's primary semantic-report consumer completely off raw
  JSON searches and substring copying.
- Migrated Flowoptimize's primary semantic/execution-plan and provider-decision
  consumers completely off raw JSON searches and substring copying.
- Flowparallel now validates operation identity uniqueness and semantic matrix
  coordinate uniqueness/ranges before emitting a deterministic execution plan.
- Flowoptimize validates execution matrix coordinates before typed
  deduplication and emits deterministic attributable transform evidence.
- Replaced formatting-sensitive integration assertions with structural `jq`
  assertions where canonical serialization intentionally changed whitespace.
- Focused gate: `flowcontracts_json`, `flowparallel_pipeline`,
  `flowoptimize_pipeline`, CPU provider/execution, and CUDA provider tests all
  passed (6/6 in 0.59 seconds).
- Canonical gate: build passed and 55/55 CTest tests passed in 24.23 seconds.
- Adversarial coverage rejects nested and escaped fake authority, duplicate
  keys, duplicate operation IDs, truncated input, missing lowering plans,
  unsupported provider/representation pairs, and out-of-range matrix entries.
- Deterministic coverage proves semantically identical reordered/whitespace
  input emits byte-identical Flowparallel output.

## Gate 5 shared binding and lowering contracts

- Replaced Flowbind's competing private JSON value and parser with the public
  Flowcontracts parser while preserving its ABI, effect, resource, policy, and
  provider checks.
- Replaced Flowbind's raw top-level envelope authority search with the shared
  typed artifact header contract.
- Moved Flowlower's structured-plan JSON value and parser onto Flowcontracts
  while preserving its existing structured lowering API and behavior.
- Both consumers now share complete-input parsing, duplicate-key refusal,
  Unicode escape handling, exact signed 64-bit integers, and JSON-path errors.
- Added adversarial coverage for duplicate authority, nested fake authority,
  escaped authoritative keys, and integer overflow.
- Focused gate: Flowbind provider/fuzz, Flowlower, reusable profile-free,
  ncurses, and `sel` pipelines passed (6/6 in 8.66 seconds).
- Canonical gate: build passed and 55/55 CTest tests passed in 26.63 seconds.

## Exact next action

### Frontend bundle authority checkpoint

- Replaced Flowanalyst's permissive private JSON parser with Flowcontracts,
  giving the frontend boundary complete-input parsing, duplicate-key refusal,
  Unicode handling, exact signed 64-bit integers, and shared diagnostics.
- Preserved explicit JSON `null` compatibility for optional source-map
  coordinates while rejecting non-integral and out-of-range identities.
- Added fail-closed uniqueness checks for symbols, scopes, symbol origins, and
  AST expression, statement, block, and declaration identities.
- Added adversarial duplicate-key, nested-authority, integer-overflow, and real
  duplicate-symbol-identity coverage.
- Focused Flowanalyst pipeline passed (1/1 in 0.87 seconds).
- Canonical gate: build passed and 55/55 CTest tests passed in 22.68 seconds.

## Exact next action

Complete public typed slices for the remaining binding, optimization,
lowering, ABI, runtime, and adjacent Flowparallel provider/planner artifacts,
then build `flowvalidate` over those public contracts.
