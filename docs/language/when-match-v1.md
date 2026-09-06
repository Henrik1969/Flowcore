# Value-oriented control flow v1

`if` remains the general Boolean branch and `guard` remains the explicit
invariant check. A value-oriented form is reserved for selecting behavior
from a finite state or outcome value:

```flow
when decode_state {
    case ascii {
        65 -> handle_ascii
    }
    case lead_two_byte {
        195 -> handle_lead
    }
    default {
        failure -> return
    }
}
```

The first implementation target is an integer selector with literal cases.
Each case is evaluated at most once, cases are ordered by source, and the
`default` arm is mandatory until the language has an enum or tagged-variant
type that can be checked for exhaustiveness. Duplicate literal cases are a
frontend error. A case body may fall through to the join only when it does not
terminate; a terminating arm does not create a false continuation.

The construct must lower to the same explicit route and join graph as an
equivalent `if`/`else if` chain. The selector is evaluated once and its path
is retained as provenance for every route. No host lookup, libc behavior, or
target-specific dispatch is permitted.

The UTF-8 decoder is the first evidence target. Its C++ reference states are
the ASCII byte, valid two-byte lead, continuation byte, and malformed byte.
The initial Flow probe will compare scalar output, diagnostic count, and
termination behavior against the existing C++ artifact before any broader
pattern or enum syntax is admitted.

The grammar is intentionally deferred until the selector and case semantics
have a differential probe. This keeps `when`/`match` from becoming a second,
less-tested control-flow implementation.
