# Flowmini versus Haskell and OCaml

| Area | Flowmini | Haskell | OCaml |
|---|---|---|---|
| Domain/philosophy | explicit facts/effects/providers; **EXPERIMENTAL** | pure functional core, lazy evaluation, rich type/effect idioms | strict functional/imperative mix, concise algebraic types |
| Types/effects | small nominal/refined subset; effect semantics incomplete | type classes, algebraic data, purity and monads | variants, modules, functors, algebraic data and effects |
| Runtime/performance | developing native/VM targets; **UNKNOWN** | mature native/runtime options; laziness has costs | mature native/bytecode toolchains and predictable strict evaluation |
| Compiler/interop/tooling | bespoke Graph chain, sparse tools | GHC ecosystem and profiling | OCaml compiler, dune, editor/tool support |
| Deployment/ecosystem | small | established research/production niches | strong compiler/tooling niche, smaller general ecosystem |

Flowmini makes provenance, provider policy, revisions, projections, and
external evidence explicit. Haskell and OCaml expose design lessons for pure
computation, algebraic data, modules, and typed effects. They are stronger for
language-level abstraction and compiler work; Flowmini is only preferable when
Flowcore artifact governance dominates.

The adversarial finding is that Flowmini currently lacks principled effects,
exhaustive algebraic data lowering, modules/functors/type classes, and a clear
purity story. Adding these without a concrete use case risks excessive
complexity; a provider boundary or an OCaml/Haskell component may be better.

Sources: [Haskell documentation](https://www.haskell.org/documentation/) and
[OCaml documentation](https://ocaml.org/docs). Several performance and effect
claims remain **UNKNOWN** for Flowmini.
