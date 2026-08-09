# flowmini v7 — primitive if/else frontend

This version keeps the original Flow Policy Envelope runtime and the explicit `.flowir`
primitive graph language intact, while adding the next structured frontend layer.

## Source layers

```text
.flow    human source with sugar
.flowir  explicit primitive graph IR / lowered form
```

The runtime still executes the lowered graph. The frontend only translates human
source into explicit nodes, policies, and wires.

## New in v7

- `if { ... } else { ... }` inside `main` and `while` blocks.
- `if { ... }` without `else`; the false route goes directly to the join.
- Branch-local scopes.
- Hard no-shadowing rule still enforced inside branches.
- Branches rejoin through a primitive `record.nop` atom.
- New structured Collatz example.

This version deliberately keeps `if` primitive:

- no `else if` sugar yet
- no `&&` or `||` compound boolean expressions yet
- no switch / match yet
- no list path expressions yet

## Example

```flow
module collatz_structured

main {
    n     : int(stdin.int())
    zero  : int(0)
    one   : int(1)
    two   : int(2)
    three : int(3)
    rem   : int(0)
    tmp   : int(0)

    while n > one {
        print n
        n % two -> rem

        if rem == zero {
            n / two -> n
        } else {
            n * three -> tmp
            tmp + one -> n
        }
    }

    print n
}
```

## Build

```bash
cmake -S . -B build
cmake --build build -j20
```

## Run examples

```bash
echo 5 | ./build/flowmini examples/countdown_structured.flow
echo 5 | ./build/flowmini examples/collatz_structured.flow
echo 5 | ./build/flowmini examples/factorial_structured.flow
echo 10 | ./build/flowmini examples/fibonacci_structured.flow
```

Expected highlights:

```text
countdown 5: 5 4 3 2 1
collatz 5: 5 16 8 4 2 1
factorial 5: 120
fibonacci 10: 55
```

## Inspect lowered graph

```bash
./build/flowmini --emit-flowir - examples/collatz_structured.flow
```

You will see the `if` lower to:

```text
int.eq / int.gt / int.lt
route.bool
true branch
false branch
record.nop join
continuation
```

## Hard semantic rules preserved

These remain semantic errors:

```flow
one : int              # declaration without initializer
1 -> one               # assignment to undeclared target
person : int(30)
if person > zero {
    person : int(12)   # implicit shadowing
}
```

The principle remains:

```text
Declaration creates a contract and initialized state.
Assignment obeys an existing contract.
Implicit shadowing is forbidden.
Human source lowers to explicit graph mechanics.
```

## Next planned layer

v8 should address list path expressions:

```flow
arr[i] -> current
current -> arr[i]
print arr[i]
```

That will make the list algorithms much more human-readable without changing the
runtime pattern.
