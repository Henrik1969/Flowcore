# Enum and tagged-variant values v1

Enums are named finite values, not integer aliases:

```flow
enum DecodeState {
    ascii
    lead_two_byte
    continuation
    malformed
}
```

An enum member carries the enum type identity. It may be compared only with a
member of the same enum, passed through typed bindings, and selected by
`when`. Member ordinals are an implementation detail and are not the
source-level contract.

Tagged variants extend the same identity model with a payload:

```flow
variant DecodeOutcome {
    scalar(value : int)
    diagnostic(code : int, offset : int)
}
```

Construction names the variant explicitly. `when` over a tagged value binds
only the payload fields declared by the selected variant. Accessing a payload
from another variant is a semantic error.

For a closed enum or closed variant set, `when` may become exhaustive without
`default`. Missing members, duplicate members, duplicate variant names, and
payload type mismatches are frontend errors. Open or externally supplied state
still requires `default`.

The implementation order is:

1. enum declaration and member identity in the AST, symbol projection, and
   semantic bundle;
2. typed enum bindings and member construction;
3. exhaustive `when` cases over enums;
4. tagged variants with payload records and guarded field access;
5. differential C++/Flow probes for UTF-8 decoder states and outcomes.

No enum member is lowered by dispatching on an application or source-unit
name. The lowering preserves value identity and explicit target policy for
freestanding execution.
