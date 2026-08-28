# Flowcore v0.28 typed artifact-contract mission

## Authority

Execute this mission autonomously to completion on the currently checked-out
development branch. Normal commits, annotated checkpoint tags, and pushes are
authorized subject to the repository `AGENTS.md` limits.

This mission supersedes the completed reusable-chain task as the active task.
The previous task and its ledger remain historical evidence and must not be
rewritten.

## Objective

Create one strict, typed, versioned, deterministic, independently checkable
artifact-contract boundary across every required compiler stage.

Every required stage must either:

1. parse and validate the complete authoritative artifact it consumes; or
2. reject it with a structured diagnostic identifying the contract, version,
   field or identity, source provenance, and reason.

No required stage may determine authority through raw substring searches,
formatting assumptions, application names, source names, profile names, or
capability-set recognition.

This mission prepares Canonical Graph IR. It does not implement full Graph IR,
safety profiles, WCET analysis, certification, or self-hosting.

## Binding laws

- Semantic meaning is authoritative; JSON, matrices, LLVM and native artifacts
  are representations, projections, or providers.
- Provider discovery and symbol existence are evidence, not authorization.
- Fallible operations preserve explicit outcome and disposition semantics.
- Published identities and provenance are preserved or explicitly derived.
- Every stage consumes a public, versioned contract and fails closed on
  malformed or unsupported authority.
- Graph connection and value placement remain distinct; matrices remain
  derived views rather than graph authority.
- Flowcore remains experimental. Passing this mission is not a production,
  security-critical, or safety-critical certification claim.

## Execution gates

### Gate 0 — establish the real baseline

- Reconcile branch, upstream, worktree, run state, instructions, and recent
  history with the recorded v0.27 checkpoint.
- Reproduce the canonical build and complete CTest suite.
- Record exact toolchain versions, commands, counts, duration, and failures.

### Gate 1 — inventory artifact contracts

Inventory every current public pipeline artifact, its producer and consumers,
format/version, required and optional fields, authority-bearing fields,
provenance and identity fields, compatibility and rejection behavior, parser,
tests, and uncovered attacks.

Classify every `find`, `substr`, string-view search, JSON slice, textual
format/status/version match, default empty object/array substitution, and
application/source/profile/capability dispatch. Distinguish ordinary string
processing from authority decisions.

### Gate 2 — public typed contract foundation

Build the smallest reusable public component needed by the current chain:

- strict complete-input JSON parsing and duplicate-key rejection;
- required/optional field and integer-range validation;
- exact format/version handling and explicit unknown-field policy;
- typed authoritative enums, variants, identities, and provenance;
- deterministic serialization;
- structured diagnostics;
- no private frontend/backend or application-specific dependency.

Implement this as vertical artifact slices rather than a speculative generic
serialization framework.

### Gate 3 — migrate Flowparallel

Replace raw authority searching and slicing with typed consumption. Validate
format/version/status, targets, operations, ABI/effect/resource facts,
dependencies, matrices, provenance, and identities. Preserve CPU fallback and
emit deterministic execution plans. Add nested-fake-field, duplicate-key,
whitespace/order, escaped-text, truncation, identity-conflict, malformed-matrix,
and semantic-equivalence attacks.

### Gate 4 — migrate Flowoptimize

Consume supported semantic reports or execution plans structurally. Validate
provider decisions and matrices, preserve all authoritative identities, emit a
new attributable artifact for every transform, and serialize deterministically.
Reject representation conflicts, out-of-range coordinates, transform/provenance
mismatches, operation loss/reordering, and mutation without derivation.

### Gate 5 — reconcile Flowbind and Flowlower

Move existing strict parsing onto shared public contract semantics where this
removes competing authority logic. Preserve exact ABI/effect/resource
authorization, cleanup laws, reachability checks, structured unsupported
carrier/operation refusal, and the profile-free application-independent lowerer.

### Gate 6 — independent validator

Add an independently invocable `flowvalidate` that identifies and validates
public artifacts, optionally emits canonical JSON, provides human and machine
diagnostics, avoids private stage implementations, and returns stable valid,
invalid, unsupported, and blocked statuses. Cover every captured stage artifact
with positive, negative, mutation, and round-trip tests.

### Gate 7 — end-to-end identity preservation

Use an acceptance program containing values, an external result, conditional,
loop/repetition, admitted fallible or resource-bearing behavior, cleanup,
explicit return, provenance, and a named target. At every stage assert preserved
or documented derivation of source, target, operation, block/control, provider,
ABI contract, effect, resource, authorization-evidence, and provenance identity.
Independently mutate each category and prove refusal by the next consumer.

### Gate 8 — repository truth

Reconcile active documentation with executable evidence. Clearly distinguish
implemented, narrow, compatibility, provisional, future, and historical facts.
Do not replace legitimate suite-specific counts with one global number.

### Gate 9 — hardening evidence

Run focused contract tests, canonical build and CTest, ASan/UBSan, malformed and
duplicate-key attacks, deterministic and round-trip checks, native LLVM linking
and execution, `sel`, `flowcat`, `flow_less`, ncurses, graph routing, generated
bindings, `git diff --check`, and repository hygiene inspection. Record every
environmental exclusion or skipped provider gate precisely.

## Non-goals

Do not add unrelated syntax; implement full Graph IR or revisioned storage;
claim real-time/safety certification; generalize aggregate ABI, CUDA, or
arbitrary native lowering; redesign writable-storage language semantics; add
new UI/package/runtime products; merge PR #4; or change GitHub settings.

Positive `c_pointer(N)` remains tested compatibility behavior. Do not select a
permanent writable-storage syntax without Henrik's explicit ruling.

## Checkpoint protocol

At each coherent public boundary:

1. run focused tests and `git diff --check`;
2. run the complete canonical suite for public contract or stage changes;
3. update `docs/checkpoints/2026-08-26-v028-maturation-ledger.md`;
4. inspect for unrelated/generated changes;
5. commit, add an annotated `v0.28-*` checkpoint tag, and push branch and tag;
6. leave `.codex-run-state` as `CONTINUE` and proceed.

Do not create empty checkpoints or tag failing implementation states.

## Definition of done

- Flowparallel and Flowoptimize contain no raw-text authority decisions.
- Flowbind and Flowlower agree on shared public contract semantics.
- Every required current artifact has typed public representation and strict
  validation.
- Duplicate keys, malformed authority, unsupported versions, missing fields,
  and conflicting identities fail closed.
- Serialization is deterministic and independently round-trippable.
- Authoritative identity is preserved or explicitly derived across all stages.
- `flowvalidate` independently validates captured artifacts and rejects every
  required adversarial mutation.
- The profile-free reusable native chain and existing graph/resource laws remain
  green without application-specific compiler dispatch.
- Canonical and sanitizer suites pass with exact recorded evidence.
- Active documentation matches the checkout.
- The final checkpoint and tags are pushed; the worktree is clean and synced.

An intermediate build, migrated stage, commit, tag, or documentation update is
not completion.

## Legitimate stopping conditions

Stop only when the complete definition of done passes, or all safe progress is
blocked by a genuine public-language decision, missing credential/authority, or
incompatible external dependency. Record exact evidence, attempted reversible
alternatives, preserved passing state, and the smallest required decision.

Do not treat workload, context size, an ordinary design choice, or a diagnosable
test failure as a blocker.
