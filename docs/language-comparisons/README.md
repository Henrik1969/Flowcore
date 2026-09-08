# Flowmini language comparisons

Flowmini is one tool among many. Choose according to the problem. These notes
are engineering comparisons, not scoreboards or advocacy. They use the same
questions: domain and philosophy; overlap; architecture, execution, types,
memory, mutation, concurrency and errors; FFI; compiler/IR/optimization;
hardware and portability; observability, tooling, ecosystem, performance and
deployment; Flowcore semantics; and a problem-oriented recommendation.

Status labels in every document are **IMPLEMENTED**, **TESTED**, **DESIGNED**,
**PLANNED**, **EXPERIMENTAL**, **SPECULATIVE**, **NOT SUPPORTED**, and
**UNKNOWN**. For absent features, use **NOT SUPPORTED — MISSING**,
**NOT SUPPORTED — DEFERRED**, or **NOT SUPPORTED — INTENTIONALLY EXCLUDED**.
Flowmini claims are limited to the v0.32 checkout and linked probes. External
claims point to current language documentation.

| Comparison | Languages | Main decision pressure |
|---|---|---|
| [C and C++](flowmini-vs-c-cpp.md) | C, C++ | machine control, ABI, abstraction cost |
| [Rust and Zig](flowmini-vs-rust-zig.md) | Rust, Zig | ownership and explicit low-level work |
| [Mojo](flowmini-vs-mojo.md) | Mojo | heterogeneous hardware and MLIR |
| [Python and Julia](flowmini-vs-python-julia.md) | Python, Julia | productivity and numerical ecosystems |
| [Go](flowmini-vs-go.md) | Go | simple services and deployment |
| [JVM and .NET](flowmini-vs-jvm-dotnet.md) | Java, Kotlin, C#, Swift | managed platforms and application ecosystems |
| [JavaScript and TypeScript](flowmini-vs-javascript-typescript.md) | JavaScript, TypeScript | web and event-driven applications |
| [Haskell and OCaml](flowmini-vs-haskell-ocaml.md) | Haskell, OCaml | types, effects, and compiler research |
| [Erlang and Elixir](flowmini-vs-erlang-elixir.md) | Erlang, Elixir | supervision, distribution, failure semantics |

The comparisons intentionally identify where Flowmini loses. A mature
Flowcore system can compose an excellent external implementation behind an
explicit provider boundary instead of reimplementing it.
