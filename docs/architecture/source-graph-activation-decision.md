# Source-defined graph activation: decision required

Status: proposal, not confirmed language semantics (2026-09-06).

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

## Smallest proposed decision

Admit a stateless, single-input/single-output source receiver as the first slice:

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

Surface spelling and public artifact representation would be specified and
reviewed against these laws before implementation. The choice does not declare
the existing compatibility ModuleSpec to be canonical Graph IR.

Henrik's decision: accept this bounded receiver contract, or require the broader
node/plug activation design first. The material choice is whether one delivered
input is sufficient to activate a source function, and whether its local state
is fresh or persistent. Ordinary function lowering cannot answer that choice.

## Alternatives checked

- Renaming the built-in pager or moving its C++ into a provider leaves application
  semantics outside Flow and fails Gate 6.
- Calling a function sequentially from main does not preserve `=>` delivery,
  input selection or wire/signal identity and is prohibited by the mission.
- Existing callable v2 does not connect functions to AtomRegistry or ports.
- Implementing a general stateful/join-capable receiver now would select broader
  public semantics without a documented activation contract.

After approval, implement the selected contract through durable frontend,
semantic, execution and lowering artifacts; add independently replayed boundary
tests; then express pager navigation in Flow and remove the built-in algorithm
after equivalent positive, negative, order and native execution coverage passes.
Keep graph projection refusal until that full route is admitted.
