# TinyVM, cross-target and bootstrap autonomous mission

## Authority

Continue autonomously on the checked-out development branch. Normal commits,
annotated checkpoint tags and pushes are authorized under `AGENTS.md`. This
mission follows the completed v0.28 typed-artifact-contract mission; it does
not rewrite that mission or its evidence.

## Objective

Establish TinyVM as an independently validated Flowcore backend with behavioral
parity for the currently admitted LLVM surface, make target-policy selection a
real cross-compilation boundary, and advance the shared language foundations
needed for product-scale Flow applications and reproducible Flowmini
self-hosting.

All compiler and runtime stages remain independently invocable tools connected
by complete, durable, versioned files. No consumer may require its producer
process, binary, private headers or in-memory state.

## Binding architecture

The detailed requirements are:

- `docs/architecture/tinyvm-flowcore-backend-plan.md`;
- `docs/architecture/product-scale-and-self-hosting-plan.md`;
- `docs/architecture/flowcore-core-promise.md`; and
- `docs/architecture/FLOWCORE-FULL-STACK-IMPLEMENTATION-PLAN.md`.

Where their scopes differ, implement the smallest vertical slice that preserves
all binding laws and moves both backend parity and language closure forward.

## Current baseline

- v0.28 typed artifact contracts are complete at tag
  `v0.28-typed-artifact-contracts-complete`.
- The recovered direct-threaded prototype is tagged
  `tinyvm-recovered-poc-2026-08-26`.
- Dispatch benchmarks are tagged `tinyvm-dispatch-baseline-2026-08-26`.
- Recovered ISA conformance is tagged `tinyvm-isa-conformance`.
- The first recovered-ISA binary envelope is tagged
  `tinyvm-artifact-envelope-v1`.
- The envelope is not the complete Flow-capable artifact or backend.

## Execution order

### Gate 1 — complete TinyVM artifact authority

- Versioned sections for code, typed constants, strings, storage, imports and
  instruction/source provenance.
- Deterministic serialization and digest.
- Strict complete-input validation, identity consistency, bounds and mutation
  rejection.
- Independent emitter/validator/runner processes and captured-file replay.

### Gate 2 — Flow-capable ISA

- Typed values and virtual slots for `i1`, `i32`, `i64` and opaque handles.
- Constants, conversions, comparisons, assignments, blocks, branches, calls,
  returns and explicit traps/outcomes.
- Portable switch reference semantics and computed-goto equivalence.
- No serialized host pointer or label address.

### Gate 3 — backend-neutral lowering file

- Publish the complete validated lowering meaning currently privately consumed
  by Flowlower as a deterministic public artifact.
- Independently invocable LLVM and TinyVM lowerers consume the same file.
- Preserve source, target, operation, block, symbol, provider, effect, resource,
  authorization and provenance identities.

### Gate 4 — provider-free parity

- Empty programs, values, expressions, conversions, assignments, branches,
  loops, arguments, writable-storage compatibility and return.
- Differential execution and diagnostics against LLVM.

### Gate 5 — governed provider parity

- Exact authorized import tuples and typed host-call thunks.
- Pure, readonly, I/O, resource/cleanup, ncurses and TUI fixtures in increasing
  slices.
- Explicit unsupported results for any current LLVM surface not yet admitted.

### Gate 6 — target-policy cross-compilation

- A target name resolves a versioned target-policy artifact covering backend,
  architecture, ABI, capabilities, resources, lifecycle, evidence and fallback.
- Changing the target name changes admitted lowering without source edits.
- Missing or incompatible providers fail with structured diagnostics; no silent
  host or backend substitution.

### Gate 7 — shared language-closure slice

- Inventory Stage 0 host-language requirements.
- Select and implement the smallest explicit language/standard-library facility
  required by both a compiler-construction probe and headless document-model
  probe.
- Preserve the probes as cross-backend acceptance programs.

### Gate 8 — staged bootstrap foundation

- Implement compiler components in Flow one boundary at a time.
- Compare captured inputs and canonical outputs with Stage 0.
- Retain bootstrap escape hatches until Stage 1/2/3 deterministic fixed-point
  evidence exists.

## Checkpoint protocol

At each coherent boundary:

1. run focused positive, negative and adversarial tests;
2. run engine/backend differential tests where relevant;
3. run ASan/UBSan with documented environmental exclusions;
4. run the full canonical suite for shared contract/compiler changes;
5. run `git diff --check` and inspect repository hygiene;
6. update the maturation ledger;
7. commit, add an annotated descriptive tag, push branch and tag;
8. keep `.codex-run-state` as `CONTINUE` and immediately proceed.

## Definition of done

- TinyVM has current LLVM-backend behavioral parity or an exact, documented,
  structured unsupported inventory with no false admission.
- LLVM and TinyVM consume the same public backend-neutral lowering artifact.
- TinyVM artifacts are deterministic, independently validatable and contain no
  process-local authority.
- Target-policy names drive at least two genuinely distinct target/backend
  builds without source changes or silent fallback.
- The portable switch and optimized computed-goto runtimes remain equivalent.
- A language-closure inventory and shared compiler/document vertical slice are
  implemented and tested across admitted backends.
- The staged self-hosting path has executable compiler-construction evidence and
  a precise remaining bootstrap inventory.
- All required tests and sanitizers pass, documentation matches the checkout,
  final commits/tags are pushed, and the worktree is clean.

Full FlowOpenOffice, complete self-hosting, arbitrary architectures, safety
certification and every possible LLVM capability are later horizons, not false
claims required to close this mission.

## Legitimate stopping conditions

Stop only after the definition of done is satisfied or when every safe path is
blocked by a genuine public-language semantic decision, missing authority or
unavailable external dependency. Record exact evidence, attempted reversible
alternatives and the smallest required decision. Workload, context size,
ordinary implementation choice and diagnosable failures are not blockers.
