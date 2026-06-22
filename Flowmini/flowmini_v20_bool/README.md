# flowmini v20 — Bool / predicate result

v20 adds `Bool` as a real internal value type. Comparisons are no longer only control-only predicate syntax; they now produce Bool values that can be stored, printed, passed through functions, returned, and used by `if`/`while`.

## Added

- `Bool` built-in value type.
- `true` / `false` literals.
- `Bool(...)` declarations.
- Comparisons produce `Bool`:
  - `a < b`
  - `a > b`
  - `a == b`
- `not <predicate>` produces `Bool`.
- `Bool == Bool` produces `Bool`.
- `print` supports `Bool` and prints `true` / `false`.
- Function arguments and return values may be `Bool`.
- `if` and `while` conditions must be `Bool`.
- No implicit `int -> Bool` truthiness.

## Deliberately not added yet

- No `and` / `or` yet.
- No short-circuit predicate grammar yet.
- No eager bitwise/logical confusion.
- No implicit conversion between `int` and `Bool`.

`and` / `or` are postponed because they are evaluation-order constructs in predicate mode, not ordinary eager binary math/bitwise operations.

## Example

```flow
module bool_demo

main {
    zero : int(0)
    one  : int(1)
    two  : int(2)
    out  : int(0)

    less  : Bool(false)
    equal : Bool(false)
    same  : Bool(false)
    inv   : Bool(false)

    one < two -> less
    print less

    one == two -> equal
    print equal

    not equal -> inv
    print inv

    less == inv -> same
    print same

    if less {
        one -> out
    } else {
        zero -> out
    }

    print out
}
```

Expected:

```text
true
false
true
true
1
```

## Loop-state Bool example

```flow
module bool_loop_demo

main {
    i    : int(0)
    five : int(5)
    one  : int(1)
    keep_going : Bool(true)

    while keep_going {
        print i
        i + one -> i
        i < five -> keep_going
    }
}
```

Expected:

```text
0
1
2
3
4
```

## New examples

- `examples/bool_demo.flow`
- `examples/bool_loop_demo.flow`
- `examples/fn_bool_demo.flow`
- `examples/bad_if_int.flow`
- `examples/bad_bool_init.flow`
- `examples/bad_compare_to_int.flow`

## Smoke tests

```bash
cmake -S . -B build
cmake --build build -j4

./build/flowmini examples/bool_demo.flow
./build/flowmini examples/bool_loop_demo.flow
./build/flowmini examples/fn_bool_demo.flow
./build/flowmini examples/capable.flow
./build/flowmini examples/capable_typed_imported.flow
./build/flowmini examples/abi_struct_demo.flow
./build/flowmini examples/abi_pointer_contracts_demo.flow
./build/flowmini examples/comment_demo.flow

./build/flowmini examples/bad_if_int.flow
./build/flowmini examples/bad_bool_init.flow
./build/flowmini examples/bad_compare_to_int.flow
```

Expected negative diagnostics:

```text
bad_if_int.flow          -> if condition must be Bool
bad_bool_init.flow       -> initializer for Bool is not Bool
bad_compare_to_int.flow  -> type mismatch assigning Bool to int
```

## Current checklist

```text
0. Comments
   done: // line comments
   done: nested /* block comments */
   done: comments ignored outside strings only
   pending with Text: expression-level string concatenation across commented line breaks

1. Internal Bool / Predicate result
   done in v20: Bool, true/false, comparison -> Bool, not, Bool print, Bool fn args/returns
   postponed: and/or short-circuit predicate grammar

2. Internal Text / String value
   next candidate: Text type, Text literals, Text + Text concat, print Text

3. Broader field access inside expressions
   mostly present for record fields; still needs wider stress testing with future Text/Bool records

4. Record printing / debug output
   pending

5. Type-bound functions without method sugar
   pending

6. Public/private
   pending

7. receiver.method() sugar
   pending

Additional backlog remembered from design discussion:
   storage primitives: bit/byte/i16/u16/i32/u32/i64/u64/f32/f64
   Real/f64 numeric family
   checked casts/conversions
   ABI error/failure flow
   ABI output pointer/buffer contracts
   safe opaque handles and lifetimes
   internal/ABI record bridge conversions
   effects enforcement and scheduling policy
   short-circuit predicate lowering
   type-bound functions and method visibility
```
