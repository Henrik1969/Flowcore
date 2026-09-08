# Flowmini versus Java, Kotlin, C#, and Swift

These languages serve mature application platforms: the JVM (. Java/Kotlin),
.NET (C#), and Apple/system ecosystems (Swift).

| Area | Flowmini | Java/Kotlin/C# | Swift |
|---|---|---|---|
| Domain | small accountable provider graphs; **EXPERIMENTAL** | managed enterprise/mobile/cloud applications, libraries, IDEs | safe native applications and Apple platforms |
| Types/memory | scalar/record subset; ownership and GC **UNKNOWN** | rich generics, libraries, GC/runtime services (Kotlin/C# also value types) | static types, ARC, protocols, optionals, native compilation |
| Concurrency/errors | incomplete; provider effects | mature threads/async/coroutines, exceptions/results and tooling | structured concurrency, typed errors, platform integration |
| Compiler/IR | bespoke stages, incomplete backend parity | mature JVM/JIT or Roslyn/.NET pipelines | mature LLVM-based toolchain and ABI within its ecosystem |
| Deployment/ecosystem | no comparable IDE/package/debugger surface | extensive production ecosystems and operations | strong Apple ecosystem, narrower outside it |

Flowmini’s distinctive contribution is explicit facts, capabilities, providers,
policy, provenance, state lineage, projections, and evidence. These platforms
are much stronger for applications, UI, libraries, debugging, deployment, and
team onboarding. Flowmini is only a stronger candidate when that explicit
artifact/evidence boundary is the primary problem. Calling Java/Kotlin/C# or
Swift through a provider is preferable to rebuilding their ecosystems.

This comparison exposes Flowmini's deliberately restrained generics (including
generic variants but no constraints or broad inference), explicit typed
outcomes without exceptions, runtime services, IDE debugging, package
management, and platform libraries. It also warns against adding a managed
runtime without a demonstrated Flowcore problem. Sources: [Java SE 24 specifications](https://docs.oracle.com/en/java/javase/24/docs/specs/index.html),
[C# documentation](https://learn.microsoft.com/en-us/dotnet/csharp/),
[Kotlin documentation](https://kotlinlang.org/docs/home.html), and
[Swift book](https://docs.swift.org/swift-book/documentation/the-swift-programming-language/).
