# Reusable native Flow chain: verified result

Implementation and Firetest checkpoint: `8c2b19d32c877c16d581fde14ed2f56598110e6a`,
pushed on `v29-language-maturation`. This session continued from `ba61221`.
The [mission](../tasks/reusable-flow-chain-autonomous.md) is implemented for its
approved bounded surface. Exact commands, intermediate findings and evidence are
in the [maturation ledger](2026-08-21-autonomous-maturation-ledger.md).

## Definition-of-done inventory

| Gate | Verified result and executable evidence |
| --- | --- |
| 1: generic plan and exact authority | Typed/versioned plans; provider, symbol, convention, carriers, effects, resource facts and generated/live-provider evidence checked. `native_binding_generation`, `provider_call_identity`, `flowbind_provider`, `flowbind_fuzz`, `flowvalidate_artifacts`. |
| 2: middle-stage preservation | Stable operations, source provenance, graph endpoints and schedule identity preserved and independently validated. `flowcontracts_identity_preservation`, `source_graph_artifact`, `native_source_graph`. |
| 3: reusable lowering | Source-driven scalar literals, assignment, calls/results, function returns, branches, loops and supported ABI carriers. Typed LLVM emission and structured refusal, without application/profile selectors. `profile_free_generic_lowering`, `callable_lowering_boundary`, native pipeline gates. |
| 4: profile independence | Installed tools compile newly named source with newly generated `gettid` binding and unchanged compiler hashes. Renamed/source-order/capability-set adversarial checks pass. Legacy application LLVM emitters are removed. |
| 5: correctness repairs | One through four colliding imports, stable qualified calls, diagnosed single-import compatibility; `sel` positive/EOF/error branches, safe terminator placement, explicit initialization and cleanup. `namespace_ambiguity`, `sel_tui_pipeline`, `native_ncurses_binding`. |
| 6: Flow pager and graph semantics | Navigation, bounds, key interpretation, page extraction, rendering and failures are Flow-owned. Native graph tests prove fresh activations, full ports, wires/signals/deliveries, order, fan-out, drops, failures and input selection. C++ pager algorithm removed. `native_source_graph`, `flow_less_pager`, `flow_less_ncurses_pager`. |
| 7: final hardening | Fresh GCC/Clang, sanitizers, malformed-artifact gates, Valgrind, native install/link/run, support fragments, concurrent determinism and documentation reconciliation pass. |

No application/source-name/profile dispatch remains in the required compiler
stages. A source search across their implementation directories confirms this;
fixture names in tests identify cases and do not control compilation.

## Verification

- Canonical GCC build: 81/81 CTests (8.46 seconds).
- Fresh GCC 13.3.0 build: 81/81 (8.42 seconds).
- Fresh Clang 18.1.3 build: 81/81 (7.78 seconds).
- ASan/UBSan: 81/81 (26.09 seconds); `detect_leaks=0` is the existing documented
  environmental exclusion, supplemented by independent leak checks below.
- Categorized native/support run: 91 pass programs through the native boundary;
  37 negative fixtures plus 16 support fragments passed under Valgrind.
- Valgrind 3.22.0: all six installed compiler stages and the native pager report
  zero errors and all heap blocks freed.
- Concurrent capture: 64 processes, eight workers, byte-identical frontend JSON.
- Changed local Markdown links and `git diff --check`: pass. No configured
  Markdown linter or in-repository PR title/body metadata was present.

The fresh builds found and repaired a first-configure CMake cache defect; the
corrected categorized driver explicitly selects the native pass boundary rather
than asking the compatibility interpreter to execute migrated native syntax.

## Installed acceptance artifacts

`/tmp/flowcore-final-install/bin` contains the installed compiler binaries.
The newly generated binding and `september_unregistered_consumer` compile without
editing C++ or rebuilding tools. All six installed tool SHA-256 checks pass;
the linked x86-64 ELF exits 42. Retained artifacts are in
`/tmp/flowcore-reusable-acceptance`; executable SHA-256:

```text
86ac3acba71f522aa13b5d58e733486737c1b4b9ffc19ed5224ab1c75470f400
```

The installed pager build uses installed source/scripts, generator, runtime and
providers through `FLOWCORE_PREFIX`. `/tmp/flowcore-installed-pager/flow_less`
exits 0 and prints `-- page 3/3 --` followed by `epsilon`. Executable SHA-256:

```text
6ae423e0a2f26a48bed8cc13a63ebf11275d5b99cfe6e85c8f029b79ff8e60e4
```

These retained binaries are local verification artifacts; generated outputs and
logs are not committed. Reproduction uses the checked-in tests and pager driver,
not the continued availability of `/tmp`.

## Explicit scope and compatibility

Native demonstrations target Linux x86-64 with LLVM/native linking and
`libncursesw.so.6`. Graph v2 admits scalar payloads, selected zero-argument startup
providers, acyclic FIFO expansion (maximum 65,536 activations) and fresh receivers.
Aggregate graphs, streams, persistent node state, asynchronous/reentrant delivery,
provider policies and TinyVM native graph execution remain outside that surface.
TinyVM retains its separately tested scalar and governed-host-call boundary.

The pager consumes immutable bounded input batches and renders after the batch;
it is not a continuously redrawing terminal application. The default terminal
batch is one raw key, configurable up to 4,096. Source functions own application
semantics, and terminal lifetime is closed by the selected input transport.

Generated evidence authenticates selected provider bytes at binding time; it does
not pin future loads or transitive dependencies. ABI prototypes remain explicit
specification authority rather than something discovered from symbol existence.
Handwritten no-evidence contracts cannot authorize evidence-bearing operations.

The explicitly permitted positive `c_pointer(N)` migration representation awaits
a permanent [writable-storage choice](../architecture/writable-storage-design.md).
A unique unqualified imported name remains diagnosed compatibility, with qualified
calls as the removal path. Neither compatibility mechanism selects an application
emitter. No unresolved mission blocker is being deferred through these limits.
