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

### Gate 6 independent validator

- Added `flowvalidate`, an independently invocable executable built only on
  public Flowcontracts definitions.
- It identifies and validates frontend bundles, semantic reports, lowering
  plans, binding reports, execution plans, optimization reports, lowering
  reports, and graph-provider decisions.
- Stable exits are 0 valid, 1 invalid, 2 blocked, and 3 unsupported; diagnostics
  provide classification, format, version, source provenance where available,
  JSON path, and reason in machine JSON or human form.
- `--canonical` produces deterministic JSON and is idempotent across a second
  independent validation/serialization pass.
- Real producer-captured frontend, semantic, binding, execution, optimization,
  and lowering artifacts validate and round-trip. Duplicate operation identity,
  duplicate keys, blocked status, and unsupported format tests fail into their
  exact classes.
- Focused Flowcontracts and validator gate passed (2/2 in 0.29 seconds).

## Exact next action

### Typed provider and planner authority

- Migrated CUDA selection, CUDA graph execution, runtime planning, and graph
  provider planning from substring/numeric scanning to Flowcontracts.
- Runtime capability variants and calibration artifacts now require structural
  format/version/type validation; device availability and measured speedup are
  read only from their authoritative object paths.
- Graph tools consume the validated semantic dependency matrix, including
  dimensions, unique bounded coordinates, Boolean values, and source path.
- Execution-plan COO duplicates remain valid input to Flowoptimize's explicit
  attributable deduplication transform; the full gate caught and preserved this
  deliberate difference from semantic-report matrix authority.
- Preserved CPU fallback and existing provider-selection policies.
- Updated planner fixtures from permissive fragments to complete versioned
  execution/semantic artifacts and added duplicate-authority attacks.
- Focused CUDA/runtime/graph planner gate passed (3/3 in 0.07 seconds).
- Canonical gate: build passed and 56/56 CTest tests passed in 27.35 seconds.

## Exact next action

### Gate 7 end-to-end identity preservation

- Added public validation for target identities, ABI contract identities,
  lowering operation/block/control identities, provider tuples, effect facts,
  argument-resource facts, and control-reference ranges.
- Added a complete `flowcat` acceptance chain containing values, external
  results, branches, nested loops, admitted resource-bearing operations,
  cleanup calls, explicit returns, source provenance, and the selected `main`
  target.
- The test proves byte-equivalent canonical source, target, ABI, and lowering
  authority across semantic, execution, and optimization stages, then performs
  LLVM lowering under an exact authorized binding report.
- Independent source/provenance, target, operation, block, control, provider,
  ABI, effect, resource, and authorization-evidence mutations are refused by
  the next relevant consumer.
- Preserved the intentional rule that binding authority is consumed at actual
  LLVM emission, not by a report-only lowerer invocation.
- Focused identity gate passed (1/1 in 0.37 seconds).
- Canonical gate: build passed and 57/57 CTest tests passed in 26.84 seconds.

## Exact next action

### Gate 8 repository truth

- Added the current v0.28 status checkpoint and linked it from the root README,
  documentation index, architecture index, and typed-contract progression.
- Updated only present-tense root suite claims to 57/57; historical checkpoint
  counts and subsystem-specific counts remain intact as scoped evidence.
- Distinguished the active Flowcore v0.28 boundary from the inherited Flowmini
  v0.27 language slice.
- Recorded implemented typed/validator/identity behavior separately from future
  Canonical Graph IR, safety admission, certification, self-hosting, permanent
  writable-storage semantics, and generalized ABI/CUDA work.
- Retained the explicit experimental/non-production/non-certified boundary.

## Exact next action

### Gate 9 final hardening

- Extended independent validation to current ABI manifests, both runtime
  capability variants, and matrix/graph calibration evidence.
- Final normal root build passed; 57/57 CTest tests passed in 22.56 seconds.
- A separate GNU 13.3 ASan/UBSan Debug tree at
  `/tmp/flowcore-v028-sanitize` built cleanly; 57/57 tests passed in 55.30
  seconds with `halt_on_error=1`.
- Leak detection was disabled with `ASAN_OPTIONS=detect_leaks=0` because the
  managed traced host cannot run LeakSanitizer; AddressSanitizer and
  UndefinedBehaviorSanitizer remained enabled without suppressions.
- The root suite exercised strict/malformed/duplicate-key attacks,
  deterministic canonical round-trips, identity mutations, native LLVM linking
  and execution, `sel`, `flowcat`, `flow_less`, ncurses, graph routing,
  generated bindings, CPU fallback, and provider planner contracts.
- Required stage and adjacent planner entry points contain no remaining raw
  JSON authority searches. Remaining `find`/`substr` sites in Flowanalyst and
  Flowbind parse ordinary type names, AST-origin paths, or typed carrier lists.
- `git diff --check` passed. The tracked worktree was clean before closure;
  ignored build/IDE outputs were left untouched. `git fsck` reported only
  historical unreachable objects and no repository corruption.
- Branch and all progressive `v0.28-*` tags were synchronized before this final
  closure checkpoint.

## Completion

All v0.28 definition-of-done gates are complete. Canonical Graph IR,
revisioned persistent identity storage, executable safety admission,
certification, self-hosting, permanent writable-storage syntax, and generalized
ABI/CUDA lowering remain explicitly future work rather than hidden blockers.

## Post-v0.28 language maturation checkpoint — enum-driven classifier

- Preserved the C++ UTF-8 artifacts as the comparison authority while extending
  the Flowmini language slice.
- Completed enum values through callable function arguments and return values,
  including enum equality and enum-member visibility while lowering nested
  function calls.
- Refactored the shared scalar classifier probe from magic integer class values
  and nested branching to `ScalarClass` plus a bounded `when` range, with the
  same behavior observed through tokenizer and document call paths.
- Added the classifier to the canonical UTF-8/Flowmini probe chain. Runtime
  output, frontend AST projection, Flowanalyst lowering, enum identity, tagged
  variant declaration, guard behavior, malformed parity, and C++ artifact
  parity all pass.
- The tagged variant syntax and declaration metadata are now carried through
  the language chain; payload construction and payload-aware `when` matching
  remain the next unfinished semantic slice.

## Exact next action

Implement tagged variant construction and payload-aware `when` matching with a
small runtime probe, then carry the same evidence through frontend projection
and Flowanalyst while preserving the C++ artifacts unchanged.

## Enum exhaustiveness checkpoint

- Closed enum `when` statements may omit `default` when every declared member is
  covered exactly once; open integer selectors still require `default`.
- Missing enum members are rejected with a semantic diagnostic, and the
  exhaustive route has an unreachable fallback only to satisfy the graph's
  total wiring contract.
- Added runtime and frontend probe coverage for exhaustive matching and the
  non-exhaustive rejection path.

## Exact next action

Implement the first payload-bearing tagged variant constructor and preserve its
variant identity through a runtime record representation before adding guarded
payload field access.

## Tagged variant construction checkpoint

- Added explicit `Variant.member(payload...)` construction for typed variant
  declarations.
- The runtime representation is a target-neutral record containing a stable
  internal tag and the selected payload fields; construction uses the existing
  record envelope and does not alter the C++ evidence path.
- Variant payload fields are projected into the frontend symbol table so
  Flowanalyst validates construction and field reads through the same semantic
  chain.
- Added runtime, frontend, and Flowanalyst probe coverage for scalar payload
  construction and readback.
- Guarded variant matching remains intentionally unfinished: the current field
  projection is a conservative union of payload fields and does not yet prove
  that a field is accessed only in its matching member arm.

## Exact next action

Add payload-aware variant `when` cases and arm-local field bindings, then reject
cross-member payload access in frontend and Flowanalyst validation.

## Tagged variant matching checkpoint

- `when` now accepts tagged variant selectors and explicit member cases such as
  `case DecodeOutcome.scalar`.
- Routing compares the target-neutral internal variant tag while payload fields
  remain ordinary record values, so runtime behavior stays independent of the
  C++ reference implementation.
- Added runtime, frontend, and Flowanalyst coverage for selecting a scalar
  payload arm and reading its value.
- Variant selectors still require `default`; exhaustive variant matching and
  arm-local payload proof are deferred until the frontend carries selected
  member context.

## Exact next action

Carry selected variant-member context into each `when` arm and reject payload
field reads whose member is not the active case.

## Tagged variant payload isolation checkpoint

- Variant `when` arms now carry selected member context during frontend
  lowering.
- Payload field access is checked against the active member; reading
  `diagnostic.code` from a `scalar` arm is rejected before runtime.
- Added a negative probe for cross-member payload access, while preserving the
  positive scalar construction and matching probes.

## Exact next action

Add arm-local payload bindings and prove diagnostic payload routing, then carry
variant member identity into the semantic report as an explicit fact.

## Arm-local tagged payload bindings checkpoint

- Selected variant payload fields are now introduced as arm-local bindings, so a
  scalar arm may use `value` directly after `case DecodeOutcome.scalar`.
- Nested payload reads lower through the existing record field getter and remain
  target-neutral.
- Cross-member access remains rejected while positive direct-binding,
  construction, routing, and Flowanalyst probes pass.

## Exact next action

Add explicit diagnostic payload routing and semantic-report facts for the
selected variant member, then revisit exhaustive variant matching.

## Diagnostic payload and member-fact checkpoint

- Proved the second payload shape with `DecodeOutcome.diagnostic(code, offset)`
  routed through a variant `when` arm and direct `code` binding.
- Variant member and payload-field symbols now carry explicit
  `variant_member_spelling` facts into the frontend bundle and semantic chain.
- Runtime, frontend, Flowanalyst, positive payload routing, and cross-member
  rejection probes all pass.

## Exact next action

Use the selected-member facts to make variant exhaustiveness explicit and
remove the remaining conservative default requirement for closed variants.

## Tagged variant exhaustiveness checkpoint

- Closed tagged variants may now omit `default` when every declared member is
  covered exactly once.
- Missing variant members are rejected with an explicit exhaustive-match
  diagnostic; open integer selectors still require `default`.
- Added positive and negative exhaustive variant probes alongside diagnostic
  payload routing and member-fact assertions.

## Exact next action

Run the complete canonical repository gate at this language boundary, then
continue with richer semantic-report facts for variant case coverage.

## Canonical gate wiring checkpoint

- Added root CTest entry points for the preserved Flowmini UTF-8 artifact and
  language probes; both previously referenced missing scripts.
- Rebuilt the canonical tree after the variant changes. The artifact fixture and
  Flowmini probe tests now pass through CTest.
- The remaining full-gate failure is isolated to the existing
  `callable_lowering_boundary` contract check, which reports an operation owner
  mismatch in the function catalog and is independent of the variant sources.

## Exact next action

Repair the callable-boundary function-catalog mismatch, then rerun the complete
canonical repository gate before continuing semantic-report maturation.

## Canonical callable gate checkpoint

- Restored the generic callable boundary to its established `fn_demo` fixture
  after confirming that Flowanalyst does not yet lower `when` control blocks into
  backend branch operations.
- The enum/variant classifier remains covered by the dedicated frontend,
  Flowanalyst semantic, runtime, and C++ parity probes; backend lowering of
  `when` is explicitly deferred rather than hidden behind a special dispatch.
- Root CTest wiring and callable boundary now pass in focused verification.

## Exact next action

Run the complete 80-test repository gate, then add generic Flowanalyst lowering
for `when` control blocks before routing the classifier through LLVM/TinyVM.

## Generic `when` IR boundary checkpoint

- Defined the target-neutral `match` lowering operation needed to carry enum
  and tagged-variant case identity beyond Flowmini runtime execution.
- The contract preserves selector type, ordered labels, variant tags,
  arm-local payload bindings, explicit default/join blocks, and exhaustive
  coverage evidence.
- Existing Boolean `branch` remains unchanged for `if` and `guard`; migration
  proceeds through Flowanalyst, Flowparallel, Flowoptimize, Flowlower, and
  TinyVM without application-specific dispatch.

## Exact next action

Emit the new `match` operation from Flowanalyst for the existing `when` AST and
add preservation/validation tests before backend emission.

## Flowanalyst `when` match-facts checkpoint

- Flowanalyst now emits a generic `match_facts` collection for each `when`
  statement, preserving selector expression/symbol identity, ordered case
  labels, ranges, body blocks, and default block.
- Existing Boolean branch operations remain unchanged, so current LLVM/TinyVM
  lowering stays stable while the new control contract is introduced.
- The tagged-variant probe asserts the semantic match fact; Flowmini runtime,
  Flowanalyst, Flowparallel, and focused CTest gates pass.

## Exact next action

Carry `match_facts` through Flowparallel and Flowoptimize without loss, then
promote it to the versioned `match` operation consumed by Flowlower.

## Match-fact preservation checkpoint

- Added optional `match_facts` authority to the shared artifact contract.
- Flowparallel execution plans and Flowoptimize reports now preserve the facts
  losslessly when consuming semantic reports.
- Existing Flowanalyst, Flowparallel, and Flowoptimize pipeline tests pass; the
  tagged-variant Flowmini probe validates the semantic facts directly.
- Variant `when` operations still carry temporary arm scopes outside the old
  Boolean branch model, so backend promotion remains the next explicit step.

## Exact next action

Promote `match_facts` into a versioned `match` operation and teach Flowlower to
consume its arm labels and join structure without application-specific logic.

## Versioned match-operation promotion checkpoint

- Match facts are now operation-shaped records with `kind: "match"`.
- Flowparallel exposes them as `match_operations` while retaining
  `match_facts` compatibility; Flowoptimize carries both fields forward.
- Existing artifact consumers remain backward-compatible and focused pipeline
  tests pass.
- Backend emission is still deferred until arm-scope ownership and join
  structure are consumed by Flowlower.

## Exact next action

Teach Flowlower's structured-plan reader to accept `match_operations` and
validate arm labels, payload bindings, and join blocks before emitting code.

## Flowlower match-operation acceptance checkpoint

- `flowprepare` now validates and preserves `match_operations` as explicit
  backend authority, requiring operation kind, selector, cases, and join
  identity.
- Flowanalyst match records now carry the operation-shaped `kind` and join
  field; existing Flowanalyst, Flowparallel, Flowoptimize, and Flowlower
  pipeline tests remain green.
- Backend emission is intentionally not enabled yet; the acceptance boundary
  prevents malformed match metadata from crossing into code generation.

## Exact next action

Replace the temporary join marker with real lowered join blocks and add
Flowlower structured emission for match arms.

## Concrete match join checkpoint

- Match facts now carry the containing parent block as a concrete join block
  identity instead of the temporary `-1` marker.
- Flowlower rejects match metadata without a non-negative join identity.
- Focused Flowanalyst and Flowlower pipeline tests pass; arm emission remains
  deferred until the structured backend can consume the multi-arm labels.

## Exact next action

Add arm-label validation against selector type and begin structured match-arm
emission in Flowlower using the concrete join block.

## Match arm contract checkpoint

- Flowanalyst match records now carry selector type identity.
- Flowlower validates selector type presence, non-descending arm intervals,
  concrete arm body blocks, and concrete joins before backend preparation.
- Focused Flowanalyst and Flowlower tests pass; emission remains isolated behind
  this validated contract.

## Exact next action

Add selector-kind-aware label validation for enum and variant member tags, then
lower the first integer `match` into structured backend branches.

## Match selector-kind checkpoint

- Match records now distinguish integer selectors from named enum/variant
  selectors.
- Flowlower rejects missing or unsupported selector kinds before preparation;
  integer and named selectors remain accepted for the staged migration.
- Focused Flowanalyst and Flowlower tests pass.

## Exact next action

Carry explicit enum/variant member label identity in match arms, then lower
integer matches into structured branch chains without application dispatch.

## Explicit named match labels checkpoint

- The Flow AST now preserves enum and variant case source identity as
  `label_type` and `label_member` alongside numeric tags.
- Flowanalyst carries those labels into match records; Flowlower requires them
  for named selectors while retaining numeric ranges for integer selectors.
- Full Flowmini language probes and focused frontend/backend preparation builds
  pass.

## Exact next action

Add label-member consistency checks against the selector declaration, then
introduce the first backend branch-chain lowering for integer match arms.

## Match label consistency checkpoint

- Flowanalyst now rejects named match labels whose declaration type differs from
  the selector type.
- Integer selectors carrying named member labels are rejected explicitly.
- Existing enum, variant, runtime, frontend, and Flowanalyst probes remain
  green.

## Exact next action

Introduce backend lowering for integer match arms using the validated selector,
range, body, and join metadata.

## Backend artifact match metadata checkpoint

- The prepared backend artifact now carries the validated `match_operations`
  collection, including the empty collection for artifacts without matches.
- The canonical empty-artifact fixture tracks that schema explicitly; the
  backend artifact gate is green again.
- No backend emission behavior changed yet, preserving the existing C++
  lowering evidence while the match branch contract is completed.

## Exact next action

Trace the existing structured backend plan and add a target-neutral integer
match branch representation before emitting any LLVM or TinyVM instructions.

## Structured match intake checkpoint

- Flowlower now parses the prepared `match_operations` contract into explicit
  target-neutral match and arm records, validating selector identity, kind,
  ranges, arm blocks, and joins at the structured backend boundary.
- Match metadata is deliberately preserved without being mistaken for an
  ordinary branch operation; LLVM/TinyVM emission remains gated until the
  branch-chain representation is present.
- Focused backend, pipeline, and TinyVM boundary tests pass.

## Exact next action

Add a target-neutral integer match branch chain to the lowering plan, with
selector evaluation and explicit case/default/join edges, then teach the LLVM
emitter to consume that representation.

## Integer match branch-chain checkpoint

- Flowanalyst now emits executable integer `match` operations in the lowering
  plan, alongside the preserved semantic `match_operations` facts.
- Flowlower consumes those operations as inclusive comparison chains with
  explicit case, default, and join edges; the generated LLVM is accepted by
  `llvm-as` for the canonical integer `when` probe.
- Nested arm assignments now retain their uniquely resolved target symbols;
  ambiguous fallback names remain unresolved rather than being guessed.
- Named enum and tagged-variant matches remain semantically represented but are
  rejected by the LLVM emitter until their target-neutral tag/payload layout is
  defined.

## Exact next action

Route the same integer match operation through TinyVM and compare its execution
with the LLVM branch-chain result before admitting named matches.

## TinyVM integer match checkpoint

- TinyVM now admits the shared integer `match` operation and lowers inclusive
  ranges into ISA-v1 comparison/branch chains with explicit default and join
  flow.
- The canonical integer `when` artifact lowers and executes to completion in
  TinyVM; named selectors remain rejected with a structured unsupported result.
- Existing TinyVM backend, scalar parity, governed-provider, and cross-target
  tests remain green.

## Exact next action

Add a parity fixture that records the integer match path in both LLVM and
TinyVM artifacts, then use that evidence to define the target-neutral tag and
payload contract for enum and variant matches.

## Integer match parity fixture checkpoint

- Added a canonical `when_backend_parity_probe.flow` program that records one
  integer match operation in the shared backend artifact.
- The parity gate compiles and executes the same artifact through LLVM and
  TinyVM; both return `37`, and the artifact contains both semantic match facts
  and the executable lowering operation.
- The new gate is registered in the root CTest suite and passes independently.

## Exact next action

Use the preserved enum and tagged-variant match facts to specify a shared tag
and payload representation, beginning with enum-only routing and rejecting
variant payload access until its layout is explicit.

## Enum match parity checkpoint

- Enum selectors now carry an explicit `enum` selector kind through the
  lowering plan; enum member construction is represented as its numeric tag.
- LLVM and TinyVM both lower and execute the canonical enum match probe with
  result `11`; variants remain rejected until payload layout is admitted.
- The parity gate covers both integer and enum matches, and the full suite
  remains the next required validation boundary.

## Exact next action

Run the complete suite, then define the shared tagged-variant runtime layout
for tag-only routing before admitting payload bindings to either backend.

## Tagged-variant layout checkpoint

- The shared design now fixes the next representation boundary as a
  discriminant integer plus an arm-local payload record.
- Backend admission remains closed for variant payload construction and
  bindings; no backend guesses a payload slot or silently drops a field.
- The existing semantic/runtime variant probes remain the evidence source while
  the artifact carries the explicit tag and field map.

## Exact next action

Promote variant tag identity and arm field maps into the backend artifact, then
admit tag-only variant routing with a negative payload-access gate.

## Open work inventory

This is the remaining work after the current integer and enum checkpoints. It
is ordered by the next safe vertical slices and keeps confirmed evidence
separate from proposals.

### Tagged variants and match closure

- Add explicit variant declaration metadata to the backend-neutral artifact:
  declaration identity, member identity, numeric tag, payload field names,
  payload types, and field order.
- Emit variant construction as `{tag, payload}` only when the artifact carries
  that layout; admit tag-only routing first and retain a negative gate for any
  payload read without an arm-local field map.
- Preserve arm-local payload bindings through Flowanalyst, Flowparallel,
  Flowoptimize, Flowlower, TinyVM, and provenance records. Reject cross-member
  access in both backends.
- Add a frontend/AST acceptance probe for payload-free variant arms. The
  runtime accepts this shape, but the current frontend projection still needs
  to preserve it as a `when` statement instead of an incomplete statement.
- Add positive, malformed, cross-member, missing-member, duplicate-tag, and
  payload-layout parity fixtures for LLVM and TinyVM.

### Backend and artifact completion

- Keep the shared public lowering artifact authoritative for all match forms;
  remove any remaining top-level metadata versus executable-operation drift.
- Extend TinyVM/LLVM parity coverage to every currently admitted provider-free
  form and maintain an explicit structured unsupported inventory for the rest.
- Add differential diagnostics and provenance checks for match failures,
  malformed ranges, invalid joins, unsupported selector kinds, and payload
  access.
- Run the required ASan/UBSan gates at each major backend checkpoint and record
  environmental exclusions.
- Keep target-policy selection independent of source and preserve at least two
  genuinely distinct backend/target builds.

### Language closure

- Complete the remaining record/collection, tuple, generic-container, text,
  bytes, outcome, diagnostic, serialization, and hashing facilities needed by
  compiler-shaped programs.
- Finish ownership, borrowing/observation, lifetime, cleanup, fallible
  allocation, slices/views, and opaque resource-handle semantics.
- Add functions/closures or callback contracts, bounded iteration, graph/stream
  processing, concurrency, cancellation, async I/O, and scheduler-visible
  effects with deterministic reference behavior.
- Close modules, visibility, imports, namespaces, separate compilation,
  package manifests, dependency identities, lock evidence, incremental builds,
  compatibility, and deprecation contracts.

### Compiler construction and self-hosting

- Maintain the C++ implementation as Stage 0 evidence and capture every
  boundary artifact needed for comparison.
- Build the first compiler-construction vertical slice in Flow, compare its
  outputs with Stage 0, and retain escape hatches until deterministic
  fixed-point evidence exists.
- Expand that slice to a staged Flowmini rebuild, independent replay after
  producer exit, and reproducible next-stage artifacts.
- Prove the same language/library contracts on a bare-metal-oriented target;
  this is the system-development test before claiming language closure.
- Keep FlowOpenOffice and full product-scale platform work behind the language,
  package, provider, ownership, effect, and self-hosting gates above.

### Evidence and repository discipline

- Update this ledger before every major checkpoint, commit and annotated-tag
  each green slice, push branch and tag, and keep `.codex-run-state` at
  `CONTINUE` until the mission definition of done is actually met.
- Preserve the C++ canon and mutate it only when new evidence requires it;
  carry the reasoning and proof into Flowmini rather than treating a Flow
  implementation as authority by itself.
- The unrelated pre-existing `Flowselection/README.md` and
  `Flowselection/sel-ui/` changes remain outside this maturation chain and are
  intentionally preserved.

## 2026-09-06 review freeze

The published evidence boundary is commit `af769d403a2daf41c2f279a0bb40b7d06c09fa51`
(`v0.29.0`) on `flowlfs-v0.1-alive`. No language or FlowLFS implementation work
is admitted under the completed TinyVM bootstrap mission until a new bounded
mission is approved.

Verification recorded at this boundary:

- Normal build configuration and `flowvalidate` build completed successfully.
- `ctest --test-dir build --output-on-failure` passed 82/82 tests.
- An ASan/UBSan Debug configure and build completed successfully with
  `-fsanitize=address,undefined -fno-omit-frame-pointer`.
- The sanitizer CTest run passed 81/82. `terminal_sel_pipeline` failed before
  its assertions because the environment reported `ASan runtime does not come
  first in initial library list`; no code diagnostic was emitted. This remains
  an environmental exclusion to reproduce with a correctly preloaded runtime.

The review identifies the finite decisions required before continuation:

1. Choose whether tagged-variant payload parity is in the v0.29 closure or is
   explicitly deferred to v0.30.
2. Choose whether FlowLFS and Flowcore/Flowmini v0.29 become separate histories
   now or remain a mixed recovery branch until integration.
3. Choose the durable home for large FlowLFS artifacts and the manifest/digest
   contract.
4. Resolve the public name and authority boundary for the durable policy
   decision versus the mutable execution context.
5. Ratify proportional architecture as a repository-level design rule.

The open work inventory above is retained as backlog context, not as an
autonomous definition of done. `.codex-run-state` is therefore set to
`BLOCKED` pending these concrete scope and authority decisions. The unrelated
pre-existing Flowselection changes remain untouched.

## Active scope clarification

The active development scope is the Flowcore/Flowmini language chain only:
language semantics, frontend facts, lowering, backend parity, evidence, and
the bounded self-hosting path. Flowselection and `Flowselection/sel-ui/` are
explicitly excluded from this workstream and must remain untouched unless a
later instruction reopens them.

## 2026-09-07 Flowmini documentation and comparison evidence pass

- Rewrote the Programmer’s Guide around the v0.29 checkout, separating the
  structural artifact path from the experimental runtime parser and marking
  unsupported or uncertain features explicitly.
- Added the language-comparison index and nine problem-oriented comparison
  documents covering C/C++, Rust/Zig, Mojo, Python/Julia, Go, Java/Kotlin/C#/
  Swift, JavaScript/TypeScript, Haskell/OCaml, and Erlang/Elixir.
- Added `docs/FUTURE_WORK_EVIDENCE.md` with repository evidence, comparison
  lessons, gates, dependencies, scope/risk, and architecture-cost decisions.
- Corrected stale version and test-count claims in the root, Flowmini, and
  documentation indexes. Flowselection changes remain untouched.
- Guide example executed successfully and AST, symbol, and frontend-bundle
  focused targets passed. The current canonical CTest run is 81/82: the one
  baseline failure is `flowmini_utf8_flow_probe`, where variant-construction
  semantic JSON is malformed (`jq: Unmatched ']'`) after earlier probes pass.
- This documentation checkpoint changes no compiler or runtime behavior.

## 2026-09-07 evidence-backed remedy checkpoint

- Fixed Flowanalyst’s variant-construction operand serializer: a variant field
  call returned before closing its operand object, producing malformed JSON.
  `flowmini_utf8_flow_probe` now passes and validates the producer output.
- Preserved `const` in the structural AST (`is_const`) and as a symbol
  `mutability=const` fact. Added a dedicated structural `GuardStatement` with
  failure-block provenance; runtime behavior remains compatible.
- Added `--runtime-compat` as the explicit name for the legacy runtime parser
  and a differential corpus covering four equivalent fixtures plus one
  declared backend boundary.
- Preserved variant `label_type`/`label_member` through AST payloads and
  Flowanalyst lowering operations. Flowanalyst now rejects variant backend
  admission early with `FLOWANALYST_VARIANT_MATCH_UNSUPPORTED`; no backend
  claims payload parity.
- Added const/guard identity, parser differential, and provider-paperwork CTest
  gates, plus `tools/report-flowmini-test-status.sh` for reproducible machine
  and Markdown evidence.
- Normal root build and CTest pass: 85/85. Focused Flowmini CTest passes 8/8.
  Categorized suite remains 87/136 with 49 known parser/ABI/profile gaps.
- ASan/UBSan build passes. Sanitizer CTest passes 84/85; `terminal_sel_pipeline`
  remains an environment-only failure because ASan runtime does not come first
  in the initial library list. Re-run with the project’s preload setup.
- Unsafe regions, inline assembly, embedded foreign source, and opaque source
  escapes are explicitly **NOT SUPPORTED — INTENTIONALLY EXCLUDED**. External
  unsafe functionality remains provider/ABI artifact work.

## 2026-09-07 Flowmini language-growth checkpoint — constants and guards

- Set `.codex-run-state` to `CONTINUE` for the active language-growth mission.
- Compile-time constant evaluation now covers deterministic integer and Boolean
  literals, transitive references, unary operators, arithmetic, and comparisons
  in the structural AST and runtime compatibility parser. Checked overflow,
  divide-by-zero, provider calls, runtime state, and unsupported expressions are
  rejected with evidence-producing tests.
- `compile_time_value` is preserved in AST, symbol facts, and Flowanalyst
  lowering operations. The dedicated const gate passes.
- Canonical guard lowering now preserves failure-block provenance through
  Flowanalyst, Flowoptimize, and Flowlower LLVM emission. Nested guard failure
  blocks retain enclosing function ownership; a compiled failing guard exits via
  its declared failure path. The dedicated backend gate passes.
- New tests: `flowmini_const_evaluation` and `flowmini_guard_backend`; focused
  const/guard gates pass 3/3. Variant backend parity remains explicitly
  deferred pending a target-neutral payload contract; no backend claim was
  widened by this checkpoint.

## 2026-09-07 guard backend contract correction

- Guard failure blocks with ordinary lowering operations remain admitted.
  Failure blocks containing runtime-only expression forms (for example
  `print`) are no longer rejected by Flowanalyst's semantic stage; Flowlower
  now rejects an empty failure block explicitly instead of silently erasing
  the failure path. This preserves the distinction between semantic analysis
  and backend capability while preventing false backend success.
- Added nested `when` case/default block ownership propagation alongside guard
  blocks, so all nested operations retain their enclosing function identity.
- Focused UTF-8, const, guard semantics, and guard backend gates pass 4/4.

## 2026-09-07 std.math v0 checkpoint

- Added the first intentionally narrow pure Flowmini standard-library slice in
  `std/math.flow`: integer `min`, `max`, and `clamp` alongside existing
  arithmetic helpers.
- Added `stdlib_math_v0.flow` and `flowmini_stdlib_math`; the runtime result and
  structural declaration inventory are both verified. Floating-point
  transcendental functions remain deferred until the ABI carrier/provider
  contract supports them.
- Updated the Programmer's Guide, Flowmini current status, and evidence ledger
  to distinguish this tested slice from a complete standard library.

## 2026-09-07 binding resolution policy checkpoint

- Flowbind now accepts `--resolution dynamic|linked` and records the selected
  deployment mode in `provider.resolution` while retaining host-loader
  verification for both modes. This keeps one semantic binding contract for
  runtime loading and conventional linking; final linking remains the build
  provider's responsibility.
- Flowbind provider tests cover both modes and continue to reject missing
  symbols, policy mismatches, and unsupported ABI declarations.

## 2026-09-07 canonical evidence report checkpoint

- Full root CTest passes 88/88, including the new constants, guard backend,
  std.math, and binding-mode gates. Focused Flowmini CTest passes 11/11.
- The evidence reporter now runs focused CTest from the root superbuild's
  `build/flowmini` directory and invokes the categorized runner with explicit
  root/build paths. It reports the current categorized inventory mechanically:
  91 pass / 140 total / 49 known gaps.
- No historical checkpoint counts were rewritten; current indexes now point to
  the verified 88/88 and 91/140 evidence.

## 2026-09-07 sanitizer verification checkpoint

- Fresh Debug AddressSanitizer/UndefinedBehaviorSanitizer configuration and
  build completed in `/tmp/flowcore-growth-asan`.
- `ASAN_OPTIONS=detect_leaks=0 LSAN_OPTIONS=detect_leaks=0 ctest --test-dir
  /tmp/flowcore-growth-asan --output-on-failure` passes 88/88. This closes the
  sanitizer check for the current implementation; leak checks remain disabled
  consistently with the existing repository workflow.

## 2026-09-07 remaining-boundary evidence

- Variant backend parity remains NOT SUPPORTED — DEFERRED: current runtime and
  artifact paths preserve tags and payload labels, but no target-neutral
  discriminant/payload carrier is implemented. The backend continues to reject
  variant matches explicitly.
- User-defined generics remain NOT SUPPORTED — DEFERRED. Existing type
  validation recognizes only bounded built-in generic constructors; no partial
  generic syntax was introduced because the canonical AST, substitution facts,
  and backend carrier contract are not yet present.
- These are recorded as bounded architectural dependencies in
  `docs/FUTURE_WORK_EVIDENCE.md`, with gates and cost rationale rather than
  inflated readiness claims.
