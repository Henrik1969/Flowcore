# Flowcore autonomous maturation ledger

## Completed in this continuation

- Baseline audited: current v25 imports already preserve explicit aliases such
  as `curses`, `libc`, and `linux` in AST/SymbolTable projections; the existing
  compatibility path for unaliased legacy imports remains to be tightened.
- Generated binding policies now carry exact declared parameter and return
  carriers. Flowbind accepts the shorter four-field policy as an explicit
  legacy wildcard and binds generated grants exactly.
- Added a hostile generated-signature-policy test; it is rejected.
- Flowbind now parses semantic reports and ABI manifests as structured JSON,
  rejects duplicate JSON keys, and compares aggregate size, alignment, field
  order, names, types, and offsets against semantic facts.
- Runtime graph connections now preserve destination ports and stable per-build
  wire identities in envelope routing metadata.
- Added deterministic graph-routing coverage for destination-port delivery and
  wire identity traces.
- Added the first `flow_less` application slice. `pager.fake` is a deterministic
  provider with policy-controlled page size and navigation; `pager.render` is a
  provider-neutral plain projection. Invalid page sizes fail closed.
- Added `pager.ncurses` as a separate dynamically loaded provider using the same
  page-record contract. Its pseudo-terminal test verifies `q` handling and the
  final plain projection without linking Flowmini directly to ncurses.
- Flowanalyst now exports `abi_type_contracts`, preserving provider-declared ABI
  carrier representation, ownership, access, lifetime, nullability, and
  opacity facts. The ncurses pipeline asserts the typed opaque-window carrier.
- `flow_less` now reads a real temporary text file through `pager.file.ncurses`,
  renders its first page in a pseudo-terminal, and rejects a missing path.
- Flowanalyst now emits an additive `flowcore.lowering_plan` v1 containing
  report-local call operations and exact provider/signature facts.
- Flowbind validates every external operation in that plan against semantic
  requirements before authorizing the binding.

## Evidence

- Focused binding checkpoint: `flowbind_provider`, `flowcore_stdlib_boundary`,
  and `native_binding_generation` passed.
- Focused graph checkpoint: `flowcore_graph_routing` passed.
- Current complete checkpoint: **52/52 CTest tests passed**.
- Focused ncurses checkpoint: `ncurses_flow_pipeline`, `flow_less_pager`, and
  `flow_less_ncurses_pager` passed.

## Remaining work

- Complete namespace law by replacing or explicitly fencing legacy unqualified
  import flattening and add selective-opening semantics only where unambiguous.
- Replace profile/source-name lowering dispatch with reusable lowering plans.
- Make Flowparallel, Flowoptimize, and Flowlower consume the generic lowering
  plan while retaining legacy profile compatibility during migration.
- Expand effects into external effect, argument-memory effect, determinism,
  resource, ownership, nullability, and lifetime facts.
- Model typed ncurses session/window resources.
- Extend the ncurses provider with explicit typed session/window resource facts
  and failure cleanup diagnostics; its current dynamic provider is executable
  evidence, but the resource model is not yet represented in semantic reports.
- Add call-site resource propagation and cleanup diagnostics using the exported
  ABI type facts.
- Add graph fan-out, multiple destination ports, required/optional/terminal
  connectivity, contract rejection, provenance, and failure-routing tests.
- Reconcile documentation and test counts after the implementation stabilizes.

## Exact next action

Next action: add generic scalar external-call emission in Flowlower from the
authorized lowering plan, with a new source program that has no recognized
profile name.
