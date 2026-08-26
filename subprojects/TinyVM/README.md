# TinyVM recovered prototype

This directory recovers Henrik's 2025 TinyVM learning experiment as a small,
buildable baseline for future mutation. It is experimental prior art, not part
of Flowcore's canonical Graph IR and not a supported runtime.

The original `~/Development/tinyvm` source tree is no longer present. This
edition was reconstructed on 2026-08-26 from two surviving backtraces and the
imported ChatGPT archive. It therefore preserves the demonstrated architecture
and core dispatch loop, but does not claim byte-for-byte identity with the lost
tree.

## Preserved ideas

- C17 with the GNU labels-as-values extension.
- Direct-threaded `goto *dispatch[opcode]` execution.
- Four signed 64-bit fields per `InstrWord`: `opcode`, `a`, `b`, and `pad`.
- Eight general registers, a program counter, flags, and a running state.
- The recovered arithmetic, bitwise, comparison, control, memory, stack,
  transaction-context, and halt opcode identities.
- Instruction handlers kept separate from the dispatch loop.

The transaction-context operations are retained as explicit unsupported stubs
until their original storage and rollback semantics can be recovered or newly
specified. The `pad` field is reserved; the historical plan was to divide part
of it into instrumentation flags, an expansion index, and source/debug identity.

`tinyvm_isa_conformance` covers all 27 recovered opcode identities and the
defined fault boundaries. Each case runs through computed goto, `switch`, and
function-pointer dispatch and compares registers, memory, stack, program
counter, flags, running state, result and fault text. The five `CTX_*` opcodes
currently conform by producing the same explicit unsupported fault; passing
their tests is not a claim that their historical semantics were recovered.

## Build the isolated prototype

```sh
cmake -S subprojects/TinyVM -B build/tinyvm-recovered
cmake --build build/tinyvm-recovered
ctest --test-dir build/tinyvm-recovered --output-on-failure
```

## Dispatch benchmark

`tinyvm_dispatch_benchmark` executes identical instruction arrays through the
recovered computed-goto loop, an ordinary `switch`, and a function-pointer
table. It reports a NOP-heavy dispatch workload plus periodic and deterministically
shuffled mixed-handler workloads. Results are measurements of one
host/compiler/configuration, not a portable performance promise.

```sh
cmake -S subprojects/TinyVM -B build/tinyvm-bench \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc
cmake --build build/tinyvm-bench --target tinyvm_dispatch_benchmark
build/tinyvm-bench/tinyvm_dispatch_benchmark
```

The first recovered-baseline observations are recorded in
[`benchmarks/RESULTS-2026-08-26.md`](benchmarks/RESULTS-2026-08-26.md).

## Provenance

- `/home/henrik/Dokumenter/tinyWM_backtraced.md`
- `/home/henrik/Dokumenter/tinyWM_backtraced2.md`
- Chat archive conversation `68dd3d80-fc2c-832c-bb22-bbf76265f30f`,
  "Binary executor implementation", created 2025-10-01.
- Chat archive conversation `6905dbb5-97b8-8327-b4c6-6352af7b2234`,
  "GCC computed goto explanation", created 2025-11-01.

Future Flowmini work should lower a validated canonical artifact into a
versioned TinyVM executable artifact. GNU dispatch is an execution mechanism;
it is not semantic authority.

The staged integration and LLVM-parity plan is documented in
[`docs/architecture/tinyvm-flowcore-backend-plan.md`](../../docs/architecture/tinyvm-flowcore-backend-plan.md).
The first binary boundary is specified in [`ARTIFACT-V1.md`](ARTIFACT-V1.md).
The sectioned Flow-capable successor contract is specified in
[`ARTIFACT-V2.md`](ARTIFACT-V2.md).
The first typed execution semantics are specified in [`ISA-V1.md`](ISA-V1.md).
