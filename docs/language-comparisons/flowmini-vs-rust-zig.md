# Flowmini versus Rust and Zig

| Area | Flowmini | Rust | Zig |
|---|---|---|---|
| Domain/philosophy | semantic graph and provider accountability; **EXPERIMENTAL** | memory-safe systems programming with ownership/borrowing | explicit low-level programming and compile-time facilities |
| Execution/types/memory | native/runtime providers plus planned TinyVM; no ownership, lifetime, allocator, or generic type contract | ahead-of-time native code, algebraic types, traits, ownership/borrowing checked by compiler | native code, explicit allocators, comptime, manual error unions |
| Mutation/concurrency/errors | placement; concurrency and effect typing **NOT SUPPORTED** | controlled mutation, `Send`/`Sync`, `Result`/`Option`; async ecosystem | explicit mutation, error unions; concurrency libraries and safety are programmer-led |
| FFI/compiler/IR | ABI declarations, split AST/Graph path, incomplete variant lowering | strong C ABI, LLVM-based compiler, mature cargo/rustc tooling | C interop, self-hosted compiler work, explicit build system |
| Hardware/portability/tooling | targets and low-level escape **PLANNED**; small debugger/tooling surface | broad tiered targets, excellent diagnostics, growing embedded support | strong freestanding/embedded story; ecosystem smaller than Rust |
| Ecosystem/performance/deployment | **UNKNOWN** outside probes | production ecosystem and predictable native performance | fast native builds and transparent costs; library maturity varies |

Flowmini’s Flowcore facts, capabilities, providers, policy, provenance,
revision lineage, projections, and evidence are explicit artifact concepts.
Rust and Zig provide stronger local resource and machine semantics but require
Flowcore contracts to be expressed in APIs, wrappers, build metadata, and
external evidence.

Flowmini can be preferable for a provider-neutral, inspectable orchestration
slice. Rust is the stronger candidate for memory-sensitive services, safe
concurrency, and production systems. Zig is stronger when allocator choice,
ABI layout, freestanding startup, and compile-time control must be obvious.
Composing Rust/Zig implementations behind Flowmini providers is often lower
risk than recreating their guarantees.

The comparison falsifies any claim that Flowmini already answers ownership,
borrowing, lifetimes, data-race prevention, allocator policy, or low-level
transparency. These are **NOT SUPPORTED/UNKNOWN** today and are substantial
language-design work. See the [Rust Book](https://doc.rust-lang.org/book/) and
[Zig language reference](https://ziglang.org/documentation/master/) for the
current external contracts.
