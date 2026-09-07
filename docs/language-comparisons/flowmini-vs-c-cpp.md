# Flowmini versus C and C++

## Common matrix

| Area | Flowmini v0.31 | C | C++ |
|---|---|---|---|
| Domain/philosophy | **EXPERIMENTAL** accountable data-flow language | portable systems language with direct machine model | systems language with zero-cost abstractions and large legacy |
| Runtime/types/memory | typed scalars, records, lists; two parser paths; no ownership or allocator contract (**UNKNOWN**) | native compilation, explicit storage and pointer arithmetic | native compilation, RAII, templates, value/reference types |
| Mutation/concurrency/errors | placement and provider effects; concurrency **NOT SUPPORTED**; diagnostics evolving | explicit mutation; threads/libraries; error conventions vary | explicit mutation, RAII, exceptions or status values; broad thread libraries |
| FFI/ABI | ABI/provider declarations **IMPLEMENTED**, parity incomplete | native ABI is the baseline | strongest practical C and platform ABI ecosystem |
| Compiler/IR/optimization | AST→facts→Graph/lowering split; integer/enum match tested; variant payload lowering rejected | mature optimizing compilers and established IRs | mature LLVM/GCC/MSVC ecosystems and profilers |
| Hardware/portability/deployment | target policy and TinyVM work **PLANNED/EXPERIMENTAL** | widest embedded/OS reach | broad desktop/server/embedded reach, toolchain variance |
| Tooling/ecosystem/performance | small tests and probes; no mature debugger/library ecosystem; performance **UNKNOWN** | decades of debuggers, libraries, predictable overhead | extensive libraries, sanitizers, profilers; complexity is real |

## Flowcore-specific semantics

Flowmini makes facts, capabilities, providers, policy, provenance, revisions,
projections, and evidence explicit in its artifacts (**IMPLEMENTED** in the
language-chain slice; coverage varies). C/C++ usually express the same
concerns through types, APIs, build policy, comments, and external tooling.
Neither language automatically supplies Flowcore’s fact lineage or evidence
gates. A C/C++ provider can nevertheless be the right authority for a mature
algorithm or device driver.

## Selection pressure

Flowmini is a stronger candidate when a small computation must expose its
inputs, effects, provider identity, and captured evidence to Flowcore. C is
stronger for tiny firmware, exact layout, startup code, and stable C ABI. C++
is stronger for production libraries, generic data structures, vendor SDKs,
profilers, and mature heterogeneous backends. Composition is usually best:
keep a proven C/C++ implementation behind a declared provider and record the
boundary provenance.

The adversarial result is clear: Flowmini currently loses on ABI maturity,
debugging, allocator/ownership semantics, concurrency, library breadth, and
raw deployment history. It also has no general low-level escape mechanism;
that is a missing explicit boundary, not a reason to pretend normal guarantees
cover opaque code. C and C++ expose the useful lesson that abstraction cost and
machine escape must remain visible.

## Known unknowns and sources

Flowmini performance, long-term ABI stability, optimizer quality, and target
coverage are **UNKNOWN**. C and C++ claims should be checked against the
[current C language reference](https://en.cppreference.com/w/c/language) and
[current C++ language reference](https://en.cppreference.com/w/cpp/language).
