# Value-oriented control flow v1

`if` remains the general Boolean branch and `guard` remains the explicit
invariant check. A value-oriented form is reserved for selecting behavior
from a finite state or outcome value:

```flow
when decode_state {
    case 1..2 {
        11 -> result
    }
    default {
        failure -> return
    }
}
```

The first implementation target is an integer selector with literal and
bounded inclusive range cases.
Each case is evaluated at most once, cases are ordered by source, and the
`default` arm is mandatory until the language has an enum or tagged-variant
type that can be checked for exhaustiveness. Duplicate or overlapping case
values are a frontend error. A case body may fall through to the join only when it does not
terminate; a terminating arm does not create a false continuation.

The construct must lower to the same explicit route and join graph as an
equivalent `if`/`else if` chain. The selector is evaluated once and its path
is retained as provenance for every route. No host lookup, libc behavior, or
target-specific dispatch is permitted.

The UTF-8 decoder is the first evidence target. Its C++ reference states are
the ASCII byte, valid two-byte lead, continuation byte, and malformed byte.
The initial Flow state-trace probe now compares scalar output and diagnostics
against the existing C++ artifact and records the corresponding state codes
using `when` arms, before any broader pattern or enum syntax is admitted.

The first integer `when` grammar is now implemented against this contract and
covered by case, default, duplicate-case, and missing-default probes. Broader
patterns, enum selectors, and tagged variants remain deferred until they have
their own differential evidence. This keeps `when`/`match` from becoming a
second, less-tested control-flow implementation.
