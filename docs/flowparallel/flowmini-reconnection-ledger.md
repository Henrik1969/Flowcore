# Flowmini / Flowparallel reconnection ledger

This ledger records the contract inventory and the conservative evidence
boundary used to reconnect current Flowmini programs to Flowparallel. It keeps
canonical meaning in Flowmini and Flowanalyst; Flowparallel only consumes
derived proof and runtime policy.

| Producer | Artifact/version | Consumer | Responsibility | Provenance | Status / gap |
|---|---|---|---|---|---|
| Flowmini structural frontend | `flowmini.frontend_bundle` v2 | Flowanalyst | AST, symbols, facts, source identity | source path, symbol origins, AST paths | Current and authoritative for artifact flow |
| Flowanalyst | `flowanalyst.semantic_report` v1 | Flowparallel, Flowoptimize | resolved types, effects, calls, regions, dependency matrix, candidate evidence | source path, symbol/expression/statement identities | Current; candidate evidence is conservative and still narrower than a full alias/resource proof |
| Flowparallel | `flowparallel.execution_plan` v1 | CPU/runtime providers | derived candidates, graph projection, required fallback, runtime policy boundary | source path, copied semantic graph and candidate references | Current; first scalar user-region bridge is proven, broader region scheduling remains a gap |
| CPU provider | `flowparallel.cpu_selection` v1 | deployment/runtime | serial/thread-pool choice from candidates, capacity, calibration, policy | plan identity and selection evidence | Current selection only; execution API accepts approved task closures |
| CPU execution API | C++ `Task` / `ExecutionResult` | Flowparallel callers | execute already-approved independent work and propagate first failure | task index and error text | Current; Flowmini bridge supplies approved closures from lowered call sites |
| Runtime planner | `flowparallel.provider_decision` v1 | provider dispatch | separate legality from capability/calibration/profitability | policy, capability and calibration fields | Current; defaults to `cpu.serial` |

## Current semantic coverage

| Flowmini construct | Parallel-safety classification | Reason |
|---|---|---|
| Pure function over literals/parameters | proven-safe when calls have disjoint inputs and outputs | Flowanalyst `effect_facts` proves `pure`; pair proof is emitted |
| Compile-time constant | not-applicable at runtime | no runtime state or task effect |
| Local mutable variable | proven-conflicting when shared; otherwise isolated by output identity | writes are tracked conservatively |
| Guard/branch/match | unknown for candidate extraction | control ownership is preserved, but no region-level parallel proof is inferred |
| Variant construction/payload read | unknown | carrier identity is preserved; alias/resource separation is not yet proven |
| Generic instantiation | unknown unless reduced to a concrete pure call | generic metadata is resolved before lowering; Flowparallel does not infer generic effects |
| External/provider call | unknown or conflicting unless an explicit pure contract is present | provider effects and resource metadata remain authoritative |
| Opaque/resource handle | conflicting by default when shared; unknown otherwise | descriptive lifecycle metadata is not a lifetime/alias proof |
| Collection access | unknown when carrier aliasing is not explicit | missing alias contract forces serial execution |

Unknown evidence is a serial fallback. No source construct adds a permission
to parallelize, and runtime policy may still choose serial for a proven legal
candidate when calibration does not justify a thread pool.

## First reconnection probe

`shared_scalar_classifier.flow` is a modern Flowmini program with two result
producing calls, but its called functions contain `when` control and therefore
remain `unknown`. It is a useful negative case. The positive probe uses the
same current grammar with two pure functions, disjoint scalar inputs, and
distinct result variables. The semantic report proves the pair; the completed
gate carries that report through Flowparallel and compares serial/thread-pool
execution without introducing source-level thread syntax.

The CPU task API executes closures supplied by an already-approved caller. The
first bounded Flowmini invocation bridge is now provided by
`flowparallel_flowmini_probe`: it consumes the execution plan and lowered
artifact, then invokes approved scalar Flowmini call sites through the serial
and thread-pool providers. This proves the provider boundary for a real
compiled Flowmini program; broader region scheduling, resource alias proof,
and backend-neutral failure semantics remain future work.

## Failure and resource policy

The first failure is retained with its task index by the CPU executor. No
rollback is promised. The Flowmini bridge preserves source operation identity,
rejects missing or conflicting resource evidence, and makes partial completion
visible. Shared mutable state, external effects, resource handles, unknown
calls, dependent calls, and conflicting outputs remain serial or blocked.

## 2026-09-07 checkpoint evidence

The reconnection slice is green at the current repository boundary:

* `flowmini.frontend_bundle` -> `flowanalyst.semantic_report` ->
  `flowparallel.execution_plan` preserves candidate proof, evidence, and
  rejection provenance through `flowoptimize`.
* The compiled `parallel_independence_probe.flow` executes its two approved
  scalar calls through both `cpu.serial` and `cpu.threadpool`; both return
  `[9,49]` and the probe reports matching observable results.
* Runtime policy selects `cpu.serial` at observed speedup `0.1` and
  `cpu.threadpool` at observed speedup `2.0`, keeping legality and
  profitability separate.
* Unknown/effectful calls, conflicting outputs, and read-after-write
  dependencies produce explicit `serial` rejections. A forged candidate missing
  evidence is rejected at the Flowparallel contract boundary.
* Normal root CTest: `96/96`; focused Flowparallel (`flowanalyst_pipeline` plus
  `flowparallel_*`): `11/11`; focused Flowmini (`flowmini_*`): `13/13`;
  ASan/UBSan CTest: `96/96`; categorized Flowmini inventory remains `91/140`
  with 49 known gaps.

Broader region scheduling, resource alias proof, backend-neutral cancellation,
and TinyVM execution of current Flowmini regions remain explicitly unresolved.
