# Flowmini versus Go

| Area | Flowmini | Go |
|---|---|---|
| Domain | evidence-bearing provider graphs; **EXPERIMENTAL** | simple production services, tooling, and network systems |
| Types/memory | declared subset; no ownership/GC contract | static types, garbage collection, interfaces, slices/maps |
| Concurrency/errors | concurrency **NOT SUPPORTED**, effects evolving | goroutines, channels, context patterns, explicit errors |
| Compiler/runtime | split parser paths and developing lowering | fast mature compiler, runtime, race detector, standard tools |
| Portability/deployment | targets and TinyVM **PLANNED** | strong cross-compilation and simple static deployment |
| Ecosystem/performance | small, unmeasured | broad operational ecosystem and predictable service performance |

Flowmini expresses provider identity, capability policy, provenance, revisions,
projections, and evidence as first-class concerns. Go usually expresses those
through packages and operational conventions. Flowmini may be stronger for a
small auditable boundary or generated plan. Go is the stronger candidate for
HTTP services, distributed tooling, cross-compilation, and teams needing a
stable standard library. Composition is straightforward through a provider or
C ABI.

Go exposes missing Flowmini answers: simple concurrency semantics, deployment
simplicity, mature tooling, garbage-collection/resource policy, and an
operational ecosystem. Adding a scheduler or standard library to Flowmini
should pass the architecture-cost test; using Go behind a provider may be the
cheaper answer.

Source: [A Tour of Go: concurrency](https://go.dev/tour/concurrency/1).
Performance and safety claims beyond that documentation are **UNKNOWN** for
Flowmini.
