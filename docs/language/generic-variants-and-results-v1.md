# Generic variants and typed outcomes

Generic variants are an **IMPLEMENTED / TESTED** slice of the structural
Flowmini language path. They extend the existing generic type-parameter model
to variant declarations; they do not add a special error mechanism.

```flow
variant Result<T, E> {
    ok(value : T)
    error(reason : E)
}
```

The declaration owns its type parameters in declaration order. An annotation
supplies concrete arguments and establishes the instance used by construction:

```flow
enum ParseError { invalid }

main {
    result : Result<int,ParseError>(Result.ok(42))
}
```

The current construction spelling names the generic variant owner and member
(`Result.ok`) while the surrounding declaration supplies the concrete
instance. A fully qualified member spelling such as
`Result<int,ParseError>.ok` is not part of the admitted syntax yet.

Flowanalyst records an ordered substitution (`T -> int`, `E -> ParseError`),
retains the generic origin, and derives the concrete member contracts before
lowering. Member discriminants remain deterministic zero-based declaration
order values (`ok = 0`, `error = 1`). The concrete instance is then passed to
the existing variant carrier; LLVM receives concrete payload types and never
unresolved type parameters.

`Result<T,E>` is an ordinary library convention. The compiler contains no
knowledge of `Result`, `ok`, or `error`. The checked library unit is
[`std/result.flow`](../../Flowmini/flowmini_v25_symboltable_projection/std/result.flow),
which currently declares `Result<T,E>` and the bounded `Option<T>` shape.
The import path is covered by `flowmini_generic_variants`.

`Either<A,B>` is used by the same gate as a neutral, non-error generic variant
to prove that the implementation is general. Payloadless members are retained
in the declaration model through `Option<T>`, but construction and backend
execution of payloadless or wider payload layouts remain outside this slice.

Generic variants currently execute through the LLVM one-slot carrier for
concrete integer and enum payloads. Multi-field records, arbitrary nested
layouts, resource-handle payloads, and TinyVM lowering remain **NOT SUPPORTED —
BACKEND LIMITATION/DEFERRED**. Invalid arity, undeclared payload types,
duplicate parameters, and substituted payload mismatches are rejected by
Flowanalyst before backend lowering.

`Result` values are explicit data. They do not introduce exceptions, implicit
propagation, or ownership semantics. A provider operation returning a Result
retains its provider, effect, resource, and provenance facts; the return type
does not make an effectful operation pure. `guard` remains a separate
required-condition control-flow construct.

The runtime compatibility parser is intentionally pre-generics. Use the
structural bundle path and the canonical semantic/lowering stages for generic
variant programs until parser convergence is complete.
