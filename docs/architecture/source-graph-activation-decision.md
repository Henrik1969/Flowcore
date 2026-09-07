# Source-defined graph activation: approved bounded contract

Status: approved by Henrik in the autonomous mission (2026-09-07).
Implementation is in progress; approval does not imply native graph support.

## Recovered gap

The reusable-chain ledger previously equated Flow wiring with Flow-owned pager
behavior. `PagerNavigateNode::run` in `flowmini_runtime.cpp` implements command
interpretation, page state, bounds and page extraction in C++. Both pager Flow
sources select this built-in atom; neither defines those algorithms.

`NodeDecl` currently contains only role, id and built-in kind. `buildCheckedGraph`
requires that kind to exist in `AtomRegistry`. There is no function-body identity,
input-to-parameter mapping or return-to-output mapping. Callable lowering v2
provides ordinary function calls, but defines no graph activation contract.

Before this audit, the frontend bundle silently omitted graph declarations and
Flowanalyst marked the remaining marker initializer ready for native lowering.
The new `FLOWMINI_GRAPH_LOWERING_UNSUPPORTED` diagnostic prevents that projection
and retains original source locations. Interpreter graph demonstrations remain
supported. This refusal is a correctness fix, not a completed graph backend.

## Approved v0.29 contract

Admit a single-input/single-output source receiver with fresh activation-local state:

- A node explicitly references an ordinary Flow function by semantic identity.
- Delivery on its declared input activates that function once with the payload
  as its typed parameter. One result emits on its declared output.
- Each activation has fresh local state; persistence between deliveries requires
  an explicit future state contract. Batched pager commands can be a payload.
- A failure emits no successful result and retains node, port, wire and signal
  provenance. No implicit retry or activation on an absent input is introduced.
- Scheduling chooses delivery order; it does not change the activation rule.
  Existing deterministic fan-out and unconnected-output laws still apply.
- Multi-input joins, repeated outputs, cycles and persistent receiver state
  remain unsupported until their own explicit activation contracts are defined.

One delivered input creates exactly one fresh activation frame and invokes the
receiving function exactly once. A successful return creates one logical output
activation. Fan-out copies deliveries, preserves that output's signal identity,
and never re-executes the function. Failure emits no normal result. `guard`,
`when` and selector evaluation retain their existing intra-activation contracts.
Scheduling policy remains separate from activation semantics.

Multi-input joins, persistent/shared state, streams, repeated outputs, suspension,
reentrant or parallel scheduling, cancellation, retries, backpressure, zero-output
sinks, generalized multi-result functions, distributed execution and durable
signal identity are outside this bounded mission.

The first explicit syntax spelling is `node receiver : fn function_name`; the
function is resolved semantically, never as an AtomRegistry factory. Existing
provider nodes retain their spelling. `flowmini.graph_syntax` v1 captures node
roles, implementation references and full wire endpoints with source provenance.
It is syntax evidence, not authorization or canonical Graph IR. Native export
continues to refuse graphs until semantic validation and execution are admitted.

## Alternatives checked

- Renaming the built-in pager or moving its C++ into a provider leaves application
  semantics outside Flow and fails Gate 6.
- Calling a function sequentially from main does not preserve `=>` delivery,
  input selection or wire/signal identity and is prohibited by the mission.
- Existing callable v2 does not connect functions to AtomRegistry or ports.
- Implementing a general stateful/join-capable receiver now would select broader
  public semantics without a documented activation contract.

Implement the approved contract through durable frontend,
semantic, execution and lowering artifacts; add independently replayed boundary
tests; then express pager navigation in Flow and remove the built-in algorithm
after equivalent positive, negative, order and native execution coverage passes.
Keep graph projection refusal until that full route is admitted.
