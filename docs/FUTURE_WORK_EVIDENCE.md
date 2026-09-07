# Flowmini future work evidence

This ledger records work exposed by inspecting the v0.29 language chain. It is
not a wish list. “Do nothing” or “use a provider” is retained where the
architecture cost of a new mechanism is not justified.

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

### LANGUAGE/COMPILER — preserve constants and guard identity

- **Finding:** runtime constants and guard syntax lose meaning in artifacts.
- **Evidence:** `LetStatement` has no const flag; AST builder turns `guard` into
  `IfStatement`; bundle probes report const as an ordinary variable.
- **Why it matters:** immutability and control intent cannot be audited.
- **Status:** runtime IMPLEMENTED; artifact preservation NOT SUPPORTED.
- **Direction:** add semantic fields only if a concrete consumer needs them;
  otherwise document compatibility lowering.
- **Scope/risk:** medium AST/schema migration.
- **Gate:** positive/negative const mutation and guard provenance goldens.
- **Priority:** high because the feature was introduced to remove magic values
  and deep nesting.

### COMPILER/GRAPH IR/LOWERING — variant match parity

- **Finding:** variant payload labels do not survive analysis and lowering.
- **Evidence:** variant AST contains `label_type`/`label_member`, but
  Flowanalyst’s match payload contains only cases; `flowprepare` reports the
  labels missing and Flowlower rejects variant selectors.
- **Why it matters:** runtime success is not backend evidence.
- **Status:** runtime TESTED; backend-neutral contract NOT SUPPORTED.
- **Direction:** define a versioned variant operation with payload layout,
  exhaustiveness, and provider target rules, or retain explicit unsupported.
- **Scope/risk:** medium/high IR and backend work.
- **Dependencies:** representation ownership and target-neutral layout.
- **Gate:** captured artifact replay across LLVM/TinyVM with negative malformed
  labels.
- **Priority:** high for tagged data; do not silently admit it.

### TESTING/DOCUMENTATION — reconcile maturity counts

- **Finding:** stale docs claim v0.25/v0.27 and 78/78 while current fixture
  inventory is 87/136.
- **Evidence:** `Flowmini/CURRENT.md`, `docs/flowmini/README.md`, and manual
  disagree with v0.29 and current `flowmini_suite`; root CTest is 82/82.
- **Why it matters:** readiness claims need reproducible evidence.
- **Status:** documentation correction in this mission; fixture gaps remain.
- **Direction:** generate counts from the test runner and label expected-pass
  versus known-unsupported fixtures.
- **Scope/risk:** low documentation, medium harness.
- **Gate:** one CI report with root, focused, and categorized totals.
- **Priority:** high for developer trust.

The current canonical run also reproduces one baseline failure after the
preceding probes pass: `flowmini_utf8_flow_probe` reaches the variant
construction step and `jq` reports malformed semantic JSON (`Unmatched ']'`).
This is retained as an implementation/test finding, not hidden behind a
documentation-only green claim.

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

### INTEROPERABILITY/SAFETY — explicit opaque escape boundary

- **Finding:** ABI/provider calls exist, but no general unsafe or
  provider-specific opaque region exists.
- **Evidence:** ABI declarations carry symbol/effect identity; no `unsafe` or
  inline assembly construct is parsed.
- **Why it matters:** low-level work is either unavailable or risks being
  presented as normally accountable.
- **Status:** ABI IMPLEMENTED; unsafe boundary NOT SUPPORTED; proposal PLANNED.
- **Direction:** specify declared inputs, outputs, effects, target/provider
  identity, and boundary provenance; leave internals opaque.
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
