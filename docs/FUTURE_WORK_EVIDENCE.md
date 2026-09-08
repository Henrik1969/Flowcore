# Flowmini future work evidence

This ledger records work exposed by inspecting the v0.32 language chain. It is
not a wish list. “Do nothing” or “use a provider” is retained where the
architecture cost of a new mechanism is not justified.

## Resolved or narrowed by the remedy mission

- **Parser authority:** the structural frontend remains canonical for artifacts;
  the runtime parser is explicitly named `--runtime-compat` and a differential
  corpus now covers five equivalent fixtures. Full parser convergence remains
  future work.
- **Const and guard identity:** `is_const`, a `mutability=const` symbol fact,
  and a dedicated structural `guard` node with failure-block provenance are
  now preserved and tested.
- **Variant carrier:** AST and semantic/lowering serialization retain
  `label_type`, `label_member`, payload types, and deterministic discriminants.
  LLVM now lowers one-slot `i32` carriers and extracts payloads in match arms;
  larger layouts and TinyVM remain explicit backend limitations.
- **Semantic JSON:** the malformed variant operand object was fixed at the
  Flowanalyst producer and the UTF-8 flow probe now validates it successfully.
- **Evidence reporting:** `tools/report-flowmini-test-status.sh` emits a
  versioned JSON and Markdown projection of root CTest, focused CTest, and the
  categorized suite. The current report is 99/99 root, 16/16 focused, and
  96/145 categorized (49 known gaps). Tests remain authoritative.
- **Sanitizer verification:** a fresh Debug AddressSanitizer/UndefinedBehavior
  Sanitizer configure and build passed 93/93 tests with leak checks disabled.
  The new Flowmini variant carrier and backend artifact gates pass under
  sanitizers; no variant memory error was observed.
- **Unsafe policy:** unsafe regions, inline assembly, and embedded foreign
  source are **NOT SUPPORTED — INTENTIONALLY EXCLUDED**. External unsafe work
  must arrive as a declared provider/ABI artifact with boundary paperwork.
- **Compile-time constants:** deterministic integer and Boolean constant
  evaluation is now IMPLEMENTED/TESTED, including transitive references and
  checked arithmetic. Runtime/provider dependencies and overflow are rejected.
- **Canonical guards:** guard identity and failure provenance now reach
  Flowlower's LLVM branch emission; unsupported failure blocks are rejected at
  the backend boundary instead of being erased.
- **Pure standard library v0:** `std/math.flow` now has tested integer `abs`,
  `min`, `max`, and `clamp` helpers alongside its existing arithmetic helpers.
  Floating-point transcendental APIs remain deferred until a carrier/provider
  contract exists.
- **C binding generator:** `tools/flowbind-gen` is an EXPERIMENTAL,
  deterministic Clang AST-JSON prototype. It emits partial artifacts for a
  scalar/enum/opaque-handle subset and preserves explicitly declared
  create/use/release metadata. Real installed libm, zlib, sqlite3, and libcurl
  headers are exercised by passing gates; unsupported declarations are
  reported rather than guessed.
- **Mature program probes:** `flowstats` and `flowconfig` are IMPLEMENTED and
  TESTED runtime probes combining lists, loops, guards, constants, enum
  branching, and pure math. They are architectural probes, not an ecosystem
  readiness claim.
- **Variant backend completion:** a target-neutral carrier now preserves
  variant/member identity, discriminants, payload type identity, and arm-local
  extraction facts. LLVM implements a straightforward `{i32 tag, i32 payload}`
  representation for one-slot integer/enum payloads. Multi-field, record,
  nested, and TinyVM payload lowering remain explicitly unsupported at the
  backend boundary with regression gates.
- **Variant result probe:** `variant_result_probe.flow` combines a concrete
  `ParseResult`-style variant, payload extraction, match routing, and a guard;
  its LLVM artifact executes successfully. Repeated concrete result shapes are
  now generic-pressure evidence, but no generic syntax was introduced.
- **Generic sum types:** generic variants now preserve declaration ownership,
  ordered substitutions, deterministic concrete instance identity, member
  discriminants, and concrete payload contracts. `Result<T,E>` and neutral
  `Either<A,B>` constructions pass the LLVM carrier gate; `std/result.flow`
  imports through the ordinary unit path. Invalid undeclared payload types,
  duplicate parameters, and substituted payload mismatches are rejected before
  lowering. Result remains library data rather than a compiler primitive.

## Findings

### LANGUAGE — converge the two parsers

- **Finding:** structural AST/bundle and normal runtime use different parsers.
- **Evidence:** `src/main.cpp` dispatches `build_source_header_ast` for
  `--dump-frontend-bundle` and `flowmini::parseModule` for execution; the same
  source can therefore have different accepted semantics.
- **Why it matters:** facts and executable behavior can disagree, violating
  authority and evidence laws.
- **Status:** IMPLEMENTED paths; convergence PLANNED.
- **Direction:** choose one canonical grammar/semantic core, or publish an
  explicit compatibility contract and differential corpus.
- **Scope/risk:** large compiler slice; grammar and diagnostic compatibility.
- **Dependencies:** parser ownership decision, golden corpus.
- **Gate:** identical AST/facts/runtime outcomes for an admitted corpus and
  structured unsupported diagnostics elsewhere.
- **Priority:** highest; every later language claim depends on it.

### LANGUAGE/COMPILER — preserve constants and guard identity (resolved)

- **Finding:** runtime constants and guard syntax previously lost meaning in
  artifacts.
- **Evidence:** the remedy adds `LetStatement::is_const`, `GuardStatement`,
  symbol mutability facts, and `flowmini_const_guard_semantics`.
- **Why it matters:** immutability and control intent cannot be audited.
- **Status:** IMPLEMENTED and TESTED for the structural artifact path; runtime
  compatibility behavior remains covered.
- **Direction:** add semantic fields only if a concrete consumer needs them;
  otherwise document compatibility lowering.
- **Scope/risk:** medium AST/schema migration.
- **Gate:** positive/negative const mutation and guard provenance goldens.
- **Priority:** high because the feature was introduced to remove magic values
  and deep nesting.

### COMPILER/GRAPH IR/LOWERING — variant match parity (narrowed)

- **Finding:** variant payload labels previously did not survive analysis and
  lowering.
- **Evidence:** `variant_carriers` and `payload_bindings` survive AST,
  semantic, prepared, and lowering artifacts; `flowmini_variant_backend`
  executes integer and enum payload probes through LLVM and rejects larger
  layouts with a structured Flowlower diagnostic.
- **Why it matters:** runtime success is not backend evidence.
- **Status:** carrier and one-slot LLVM payload lowering IMPLEMENTED/TESTED;
  multi-field/record/nested and TinyVM parity NOT SUPPORTED — BACKEND
  LIMITATION.
- **Direction:** add a wider target-neutral payload storage contract only when
  a concrete structured variant program demonstrates that its complexity is
  proportionate. Keep backend layout decisions local to each provider.
- **Scope/risk:** medium/high IR and backend work.
- **Dependencies:** representation ownership and target-neutral layout.
- **Gate:** captured artifact replay across LLVM/TinyVM with negative malformed
  labels.
- **Priority:** high for tagged data; do not silently admit it.

### LANGUAGE/COMPILER — restrained generic declarations (resolved in v0.31)

- **Finding:** user-defined generic functions and records needed an explicit
  owner, substitution environment, and concrete lowering boundary.
- **Evidence:** the `flowmini_generics` gate now preserves `FunctionDecl` and
  `RecordDecl` parameters, validates `Pair<int,Bool>`, records `T -> int`, and
  executes `identity<int>` through LLVM. The runtime compatibility parser
  remains explicitly pre-generics.
- **Why it matters:** reusable algorithms are needed for self-hosting, but an
  accidental type-erasure implementation would hide invalid substitutions and
  violate semantic evidence.
- **Status:** IMPLEMENTED/TESTED for explicit type arguments, deterministic
  single-answer inference, and generic record artifacts. Constraints, generic
  variants, and generic collection carriers remain NOT SUPPORTED — DEFERRED.
- **Possible direction:** gather real collection/provider pressure before
  extending inference or adding constraints; keep concrete carrier resolution
  before backend lowering.
- **Estimated scope/risk:** medium/high frontend, symbol, and lowering schema
  change; high risk of parser divergence if introduced piecemeal.
- **Dependencies:** parser convergence, type identity contract, backend carrier
  strategy.
- **Suggested gate:** generic identity and two-type record fixtures must pass
  AST, facts, invalid-substitution, and deterministic LLVM/TinyVM artifact
  checks before the feature is called IMPLEMENTED.
- **Priority rationale:** strategic for future self-hosting, lower immediate
  priority than preserving current scalar/control/provider contracts.

### TESTING/DOCUMENTATION — reconcile maturity counts

- **Finding:** stale docs claim v0.25/v0.27 and 78/78 while current fixture
  inventory is 91/140.
- **Evidence:** `Flowmini/CURRENT.md`, `docs/flowmini/README.md`, and manual
  disagree with v0.29 and current `flowmini_suite`; the pre-remedy root CTest evidence was 81/82.
- **Why it matters:** readiness claims need reproducible evidence.
- **Status:** documentation correction in this mission; fixture gaps remain.
- **Direction:** generate counts from the test runner and label expected-pass
  versus known-unsupported fixtures.
- **Scope/risk:** low documentation, medium harness.
- **Gate:** one CI report with root, focused, and categorized totals.
- **Priority:** high for developer trust.

The pre-remedy canonical run reproduced one baseline failure after the
preceding probes passed: `flowmini_utf8_flow_probe` reached the variant
construction step and `jq` reported malformed semantic JSON (`Unmatched ']').
The producer fix is now covered by the passing regression gate.

### LANGUAGE — ownership, resources, and effects

- **Finding:** Flowmini has no ownership, borrowing, lifetime, allocator, or
  effect type contract.
- **Evidence:** language implementation has typed scalars/records/providers but
  no ownership checker; Rust/Mojo comparisons identify mature alternatives.
- **Why it matters:** bare-metal and resource claims cannot be made honestly.
- **Status:** UNKNOWN/NOT SUPPORTED.
- **Direction:** first measure a concrete resource bug; prefer provider/library
  contracts before adding a type system.
- **Scope/risk:** very high; could redefine language semantics.
- **Dependencies:** use cases and target policy.
- **Gate:** a resource lifecycle probe and cost comparison with Rust/C.
- **Priority:** high only if self-hosted/bare-metal work requires it.

### INTEROPERABILITY/SAFETY — explicit opaque escape boundary (policy settled)

- **Finding:** ABI/provider calls exist, but no general unsafe or
  provider-specific opaque region exists.
- **Evidence:** ABI declarations carry symbol/effect identity; no `unsafe` or
  inline assembly construct is parsed.
- **Why it matters:** low-level work is either unavailable or risks being
  presented as normally accountable.
- **Status:** ABI IMPLEMENTED; unsafe boundary NOT SUPPORTED — INTENTIONALLY
  EXCLUDED.
- **Direction:** external providers may carry declared inputs, outputs, effects,
  target/provider identity, artifact identity, and boundary provenance; no
  opaque computation is embedded in Flowmini source.
- **Scope/risk:** medium language and contract work.
- **Gate:** malformed-boundary rejection and captured provenance replay.
- **Priority:** medium; use existing providers until a real need appears.

### TOOLING/DEBUGGING — developer feedback loop

- **Finding:** no mature interactive debugger, formatter, language server, or
  source-level provenance viewer is part of the Flowmini toolchain.
- **Evidence:** current verification relies on CTest, dump files, and custom
  golden executables.
- **Why it matters:** semantic languages need inspectable failures, not only
  compiler logs.
- **Status:** TESTED command-line probes; tooling UNKNOWN/PLANNED.
- **Direction:** add the smallest artifact inspector or source-location map
  demanded by a failing workflow; do not build an IDE first.
- **Scope/risk:** low/medium per tool.
- **Gate:** a newcomer can map one diagnostic to source, fact, and lowering op.
- **Priority:** medium.

### ARCHITECTURE — self-hosting and standard library boundary

- **Finding:** compiler remains C++ and the general standard library/generic
  collections are incomplete.
- **Evidence:** all parser/AST/analysis sources are C++; guide marks generic
  maps, iterators, and classes unsupported; self-hosting plan is staged.
- **Why it matters:** bare-metal-from-Flowmini is a long-horizon claim, not a
  current capability.
- **Status:** PLANNED by architecture documents; no complete bootstrap probe.
- **Direction:** inventory one compiler-construction and one document-model
  slice, then implement the smallest shared facility.
- **Scope/risk:** very high; preserve C++ as canon/evidence during migration.
- **Gate:** deterministic Flowmini compiler slice matches C++ captured outputs.
- **Priority:** strategic, after parser and artifact parity.

### ARCHITECTURE/PERFORMANCE — target and IR cost

- **Finding:** target policy, TinyVM parity, and multi-level IR remain partial.
- **Evidence:** task definition lists target-policy and cross-backend gates;
  current match lowering supports integer/enum only; performance is unmeasured.
- **Why it matters:** target names and abstraction layers must change admitted
  behavior, not merely rename it.
- **Status:** PLANNED/EXPERIMENTAL.
- **Direction:** publish the smallest versioned lowering artifact and compare
  existing LLVM/TinyVM/provider capabilities before adding IR layers.
- **Scope/risk:** high.
- **Gate:** deterministic two-target artifact differential and measured cost.
- **Priority:** high for bare-metal goals.

### INTEROPERABILITY/TOOLING — C binding generator and real-library pressure

- **Finding:** the provider contract can now be populated from real C headers,
  but only a deliberately small carrier subset is supported.
- **Evidence:** `flowbind_generator` and `flowbind_real_libraries` pass for
  libm, zlib, sqlite3, and libcurl; libm floating-point declarations remain in
  the explicit `unsupported` list, while sqlite3/curl resource contracts are
  preserved as metadata.
- **Why it matters:** external reuse is inspectable without claiming that
  Flowmini governs foreign internals or can call every ABI shape.
- **Status:** EXPERIMENTAL/TESTED prototype; generated Flowmini declarations,
  callbacks, complex structs, and static lifetime enforcement remain deferred.
- **Possible direction:** add one carrier only when a concrete binding needs it,
  retaining Clang as parser and the provider artifact as authority.
- **Estimated scope/risk:** medium tooling work; high risk if expanded into a
  universal foreign-object framework.
- **Dependencies:** stable carrier and resource contracts.
- **Suggested gate:** deterministic generated artifacts plus a real linked and
  dynamic provider test for each new carrier.
- **Priority rationale:** high for ecosystem usefulness, but bounded growth is
  safer than ABI completeness by enumeration.

### LANGUAGE — restrained generic type relationships

- **Finding:** the language now admits one canonical type-parameter
  representation for user-defined functions and records. Explicit application
  and deterministic single-argument inference work for the initial forwarding
  function subset.
- **Evidence:** `flowmini_generics` preserves `identity<T>`, `Pair<A,B>`,
  `identity<int>` substitutions, deterministic instance identity, and executes
  the concrete LLVM lowering. `bad_generic_unknown_type.flow` and
  `bad_generic_arity.flow` produce structured Flowanalyst diagnostics.
- **Why it matters:** reusable type relationships are now visible in AST,
  facts, lowering plans, and concrete backend input instead of textual
  substitution. This is the minimum foundation for later collection and result
  abstractions.
- **Current status:** IMPLEMENTED/TESTED for explicit generic functions,
  simple inference, generic record declaration/application artifacts, and
  generic variant declaration/instantiation through the concrete LLVM carrier;
  generic record construction/layout and generic collection carriers remain
  deferred.
- **Possible direction:** gather pressure from real collection and provider
  programs before adding constraints, generic collection carriers, or broader
  inference. Keep generic variants concrete before backend lowering.
- **Estimated scope/risk:** medium compiler/schema work; high if expanded into
  value-level metaprogramming or a global constraint solver.
- **Dependencies:** canonical parser ownership, concrete carrier lowering, and
  a separately justified collection representation.
- **Suggested verification/gate:** every future generic construct must preserve
  owner, parameter order, substitutions, and deterministic instance identity,
  then lower only after concrete carrier resolution.
- **Priority rationale:** this resolves the demonstrated identity/record reuse
  gap while keeping generic programming deliberately small.

### LANGUAGE — concrete generic pressure from probes

- **Finding:** a reusable `length(list<T>)` helper cannot currently be expressed
  through the imported function path; the runtime parser rejects a list-valued
  function parameter as a callable value type.
- **Evidence:** the attempted `std/collections.flow` helper was removed after
  that parser error; `flowstats` therefore uses the existing `length` intrinsic
  directly and keeps its list element type concrete.
- **Why it matters:** this is measured pressure for later generics and
  collection APIs, while avoiding speculative generic syntax now.
- **Status:** NOT SUPPORTED — DEFERRED for collection carriers; the first
  canonical type-parameter/substitution path is now implemented for identity
  and record artifacts.
- **Possible direction:** use the generic identity/record evidence to design a
  callable collection carrier, then prove `length<T>` through AST, facts, and
  lowering rather than adding a type-erasure workaround.
- **Estimated scope/risk:** medium/high frontend and backend schema work.
- **Dependencies:** parser convergence and a type-carrier contract.
- **Suggested gate:** generic identity, invalid substitution, and deterministic
  backend artifacts on both admitted parser paths.
- **Priority rationale:** the probe demonstrates real need, but variant and
  binding contracts remain nearer-term correctness constraints.

### INTEROPERABILITY — resource contract v0 evidence

- **Finding:** handle libraries need lifecycle paperwork before Flowmini can
  present them as ordinary values.
- **Evidence:** generator artifacts preserve `created_by`, `resource_type`,
  `released_by`, and `required_cleanup` for sqlite3 and libcurl; the provider
  does not claim static lifetime enforcement.
- **Why it matters:** callers can inspect obligations and tooling can reject
  absent declarations without pretending to prove all paths.
- **Status:** IMPLEMENTED/TESTED descriptive metadata; automatic ownership and
  lifetime checking remain NOT SUPPORTED — DEFERRED.
- **Possible direction:** add path-sensitive enforcement only after a concrete
  missed-cleanup defect and cost comparison with an external wrapper.
- **Estimated scope/risk:** medium metadata, very high if promoted to a type
  system.
- **Dependencies:** provider ABI and effect identity.
- **Suggested gate:** create/use/release artifacts, missing-release diagnostics,
  and linked/dynamic parity where the provider can observe cleanup.
- **Priority rationale:** useful immediately for honesty; stronger enforcement
  is not justified by current evidence.

### LANGUAGE/COMPILER — generic sum types and typed outcomes

- **Finding:** repeated concrete result variants justified a generic variant
  composition slice, but not a compiler-owned Result mechanism.
- **Evidence:** `flowmini_generic_variants` preserves `Result<T,E>`, `Either<A,B>`,
  and `Option<T>` declarations; records ordered substitutions and deterministic
  concrete instances; constructs `Result<int,ParseError>` and `Either<int,Bool>`;
  matches/extracts a concrete payload; imports `std/result.flow`; executes the
  LLVM lowering; and rejects undeclared payload types, duplicate parameters,
  and substituted payload mismatches.
- **Why it matters:** reusable typed outcomes now compose from existing generic
  and variant machinery without making `Result` a privileged compiler type.
- **Status:** IMPLEMENTED/TESTED for the canonical structural path and the
  existing one-slot integer/enum LLVM carrier. Runtime compatibility parsing,
  generic-qualified member spelling, cross-function variant returns, payloadless
  construction, wider record/nested/resource layouts, and TinyVM remain
  NOT SUPPORTED — DEFERRED/BACKEND LIMITATION.
- **Possible direction:** mature module/package resolution and collection
  carriers before adding Result helpers, implicit propagation, constraints, or
  broad payload boxing.
- **Estimated scope/risk:** low/medium additive semantic metadata now; high for
  widening carriers or introducing ownership through outcome types.
- **Dependencies:** parser convergence, concrete carrier contracts, module
  resolution, and resource semantics.
- **Suggested verification/gate:** preserve declaration owner, parameter order,
  substitution, instance identity, member discriminant, payload type, and
  provider effect/resource provenance in every future generic sum type.
- **Priority rationale:** the composition works; remaining friction is module
  ergonomics and carrier breadth, not a missing Result primitive.

### ANALYSIS/FLOWPARALLEL — Result must not launder effects

- **Finding:** a Result return type is orthogonal to the effect of the operation
  that produced it.
- **Evidence:** generic variant lowering classifies the concrete value carrier
  from its declaration and substitution; Flowanalyst's call effect/resource
  evidence is derived from the callee/provider contract rather than return-type
  spelling. The generic variant gate contains no Result-specific purity path.
- **Why it matters:** wrapping an external read or resource operation in
  `Result<T,E>` must not create a false parallel candidate.
- **Status:** semantic rule documented and preserved; a dedicated provider
  wrapper returning a Result remains a follow-up because current cross-function
  variant return carriers are not admitted.
- **Possible direction:** add the negative Flowparallel gate when concrete
  provider-result return lowering is available; retain serial fallback for
  unknown effects/resources.
- **Estimated scope/risk:** medium integration work; no new effect system is
  justified yet.
- **Dependencies:** callable variant return lowering and provider wrapper
  contracts.
- **Suggested verification/gate:** compare pure and provider-bound producers
  with identical `Result` return types and assert distinct effect/proof status.
- **Priority rationale:** correctness invariant is clear; implementation should
  wait for the concrete carrier boundary rather than add special cases.

### LANGUAGE — outcome ergonomics and module pressure

- **Finding:** `Result<T,E>` can be imported as an ordinary unit, but its current
  construction requires the concrete type annotation to establish the instance
  and the member spelling remains `Result.ok`.
- **Evidence:** `result_import_probe.flow` passes through the existing import
  expansion and semantic chain; `Result<int,ParseError>.ok` is not admitted by
  the current expression grammar.
- **Why it matters:** explicit typed outcomes are usable, while concise member
  syntax and helper operations have not yet earned language changes.
- **Status:** import IMPLEMENTED/TESTED; ergonomic helpers and propagation syntax
  NOT SUPPORTED — DEFERRED.
- **Possible direction:** gather real repetition from mature Result programs;
  prefer library helpers or module-resolution improvements before adding syntax.
- **Estimated scope/risk:** low for library helpers, high for implicit control-flow
  operators because they affect provenance and guard composition.
- **Dependencies:** mature application probes and package/module semantics.
- **Suggested verification/gate:** every helper must preserve explicit outcome,
  guard, effect, and provenance facts.
- **Priority rationale:** current architecture is semantically sufficient; do not
  pay syntax cost without measured repetition.

## Lessons from other languages

- **Rust:** ownership and `Send`/`Sync` are a concrete challenge to Flowmini’s
  resource and concurrency assumptions. Study first; do not copy wholesale.
- **Mojo/MLIR:** multi-level IR and heterogeneous lowering are existing answers;
  reuse provider/MLIR boundaries where possible rather than inventing layers.
- **C:** direct layout, startup, and explicit machine escape set the abstraction
  cost test for bare metal. A provider may be enough.
- **C++:** ABI, templates, tooling, and zero-cost abstraction history are the
  current evidence baseline; preserve C++ artifacts during migration.
- **Python/Julia:** productivity and numerical ecosystems can make a new
  accountable language more expensive than the problem. “Use the library” is a
  valid result.
- **Go:** simple concurrency and deployment expose missing operational
  contracts; adding a runtime needs demonstrated demand.
- **Erlang/Elixir:** supervision and failure semantics are a separate runtime
  architecture, not a syntax feature.
- **Haskell/OCaml:** purity, algebraic data, modules, and typed effects expose
  unresolved Flowmini semantics; adopt only against a measured use case.
- **Zig:** explicit allocators, comptime, and transparent low-level behavior
  are useful design references for provider boundaries.

## Rejected or deferred on cost grounds

No general class system, managed runtime, actor scheduler, accelerator backend,
IDE, or duplicate standard library is justified by current repository evidence.
Use existing implementations behind explicit providers until a concrete
failure, benchmark, or required acceptance program demonstrates that the
architectural cost is lower than the problem cost.

## Flowinspect application evidence

The first durable artifact consumer is implemented in `Flowinspect/`. It uses
the shared `flowcontracts` parser/validators and therefore keeps artifact facts
authoritative. The tool currently inspects five versioned artifact families,
rejects unsupported versions and malformed input explicitly, and emits stable
summaries for real Flowanalyst and Flowparallel captures. Detailed findings and
deferred application pressure are recorded in
[`docs/flowmini/flowinspect-pressure-ledger.md`](flowmini/flowinspect-pressure-ledger.md).

This investigation did not justify a Flowmini-native JSON parser, a new Result
carrier, directory mode, or a scheduler extension. Those remain evidence-led
questions rather than planned features.
