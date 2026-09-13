# Current Lyraform version

```text
project:        Lyraform
implementation: Lyraform/compiler
toolchain:      Igor
authority:      main
lineage:        v0.29 reusable native language chain
status:         experimental / unstable / not production-ready
```

The current verified boundary includes generic scalar/control-flow lowering,
exact generated ABI evidence, durable source graphs, fresh receiver frames,
and Flow-owned paging. The root clean build and 81-test CTest result are
recorded in the [verification ledger](../docs/checkpoints/2026-09-07-reusable-flow-chain-result.md).

The `flowmini` executable and `flowmini.*` artifact namespace remain as
compatibility surfaces for the historical prototype and existing serialized
fixtures. They do not define the current project identity.

Future work remains explicitly scoped: broader aggregate and streaming graph
activation, additional targets, permanent writable-storage syntax, broader
standard-library coverage, optimizer expansion, and self-hosting.
