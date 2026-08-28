# Flowlower

Flowlower is the target-lowering sibling after Flowoptimize. `flowprepare`
publishes the backend-neutral boundary first:

```text
source -> Flowmini -> Flowanalyst -> Flowparallel -> Flowoptimize -> flowprepare -> backend lowerer
```

The first target provider is `llvm`. Profile-free lowering covers scalar and
pointer ABI calls, typed result placement, integer and string values,
expressions, returns, comparisons, checked entry arguments, and selected
structured control-flow plans. The file-copy and terminal examples are chosen
from source-derived plan operations and exact authorized capabilities rather
than application names. Their complex LLVM control-flow emitters remain
transitional until the reusable loop/branch emitter covers the same native
error and cleanup laws.

The lowering report preserves the source path carried by the upstream semantic
and optimization reports. This keeps emitted IR attributable to its source
artifact without making the lowerer depend on Flowmini internals.

`flowcore.backend_lowering_artifact` version 1 is canonical JSON containing the
complete selected target, lowering plan, ABI contracts, external operations,
exact authorization capabilities and optimization provenance. Both
`flowlower` (LLVM) and `flowtinylower` (TinyVM) consume a captured instance from
disk. TinyVM currently admits empty and scalar provider-free plans, including
structured branches and loops, and returns a structured unsupported result for
the remaining surface while Gate 4 is in progress.
Direct optimization-report input to `flowlower` remains a temporary corpus
compatibility path and is not the public backend boundary.

```sh
flowprepare --binding-report binding.json --target cli optimization.json > lowering.json
flowvalidate --canonical lowering.json
flowlower --emit-llvm output.ll lowering.json
flowtinylower lowering.json output.tvm
```

Multiplexed reports are preserved through Flowparallel and Flowoptimize. When
multiple named targets exist, Flowlower requires explicit selection:

```sh
flowlower --target cli < optimization-report.json
```

An absent or unknown target is rejected. A report without named targets keeps
the compatibility default `main`. This establishes target selection as a
separate artifact boundary. For a generic empty lowering plan, independent
target artifact emission is proven with separate
`--emit-llvm` paths. Each output contains an attributable target marker and
the lowering report records `artifact.target_specific: true`. Unsupported
target profiles remain blocked.
