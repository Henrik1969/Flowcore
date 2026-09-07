# Flowmini versus Mojo

Mojo targets AI and heterogeneous compute, with Python interoperability and an
MLIR-based compiler stack. Its current manual documents value ownership,
traits, structs, and target-oriented compilation.

| Area | Flowmini | Mojo |
|---|---|---|
| Domain | accountable Flowcore computations and providers; **EXPERIMENTAL** | AI/ML systems spanning CPU, GPU, and accelerators |
| Types/memory | scalar/record/collection subset; ownership and lifetimes **UNKNOWN** | typed values, structs/traits, ownership conventions and lifecycle controls |
| Runtime/concurrency | provider runtime; concurrency **NOT SUPPORTED** | native heterogeneous runtime and accelerator programming |
| Compiler/IR | bespoke AST/fact/Graph/lowering files; variant payload gap | MLIR multi-level lowering and Modular toolchain |
| Interop/ecosystem | C ABI/provider boundary; tiny library surface | Python interop plus rapidly growing AI ecosystem |
| Performance/deployment | no measured optimizer or accelerator parity | designed for compiled performance on supported targets; exact coverage is version-dependent |

Flowmini contributes explicit facts, capabilities, provider identity, policy,
provenance, state lineage, projections, and evidence. Mojo contributes a
concrete answer to value ownership and heterogeneous lowering that Flowmini
currently lacks. Flowmini is a candidate for a small governed orchestration
boundary; Mojo is stronger for accelerator kernels and Python-adjacent ML
code. A Mojo provider behind a declared Flowcore boundary is a plausible
composition.

The adversarial finding is architectural: Flowmini should study MLIR’s
multi-level representation and Mojo’s ownership/lifecycle rules before adding
new bespoke layers. Reimplementing accelerator lowering would fail the
architecture-cost test. Flowmini currently has no accelerator backend, no
ownership contract, no mature package ecosystem, and no evidence for
performance claims (**NOT SUPPORTED/UNKNOWN**).

Sources: [Mojo manual](https://docs.modular.com/mojo/manual/),
[Mojo ownership](https://docs.modular.com/mojo/manual/values/ownership), and
[Mojo lifecycle](https://docs.modular.com/mojo/manual/lifecycle/).
