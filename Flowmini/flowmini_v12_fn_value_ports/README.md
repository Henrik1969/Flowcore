# flowmini v12 — `fn` value-bound ports

This version builds on v11 fixed-shape arrays and adds the first function layer.

The architecture remains unchanged:

```text
.flow     human source
.flowir   explicit primitive graph IR
runtime   existing flow-policy-envelope graph traversal
```

## New in v12

### Function declarations

```flow
fn square(n : int): int {
    n * n -> return
}
```

For this version, a function is modeled as a one-shot reusable node template:

```text
fn name(arg : int, ...): int

arguments = value-bound input ports
return    = value-bound output port
call      = instantiate/inline function body and route return to target
```

### Function calls

```flow
square(x) -> y
```

The call is lowered by creating a fresh invocation scope, copying argument values into local argument slots, executing the body, then copying the local `return` slot to the target.

Semantically:

```text
copy x into square.n
run square body in isolated invocation scope
copy square.return into y
```

Assignments inside the function affect only the invocation-local symbols. Caller bindings are not mutated by argument passing.

### Calls from other functions

```flow
fn square(n : int): int {
    n * n -> return
}

fn add_then_square(a : int, b : int): int {
    sum : int(0)
    a + b -> sum
    square(sum) -> return
}
```

### Iteration remains the supported repetition mechanism

Recursion is deliberately rejected in v12.

```text
recursive function cycle detected: monster -> monster; recursion is not supported in flowmini v12
```

The function dependency graph must be acyclic. Use `while` for repetition.

## Supported v12 subset

```text
Supported:
    fn name(arg : int, ...): int { ... }
    value-bound argument ports
    initialized local declarations inside fn
    expr -> return
    function call placement: name(args...) -> target
    calls from main
    calls from other functions when non-recursive

Still supported from earlier versions:
    main
    int
    list<int>
    array<int>[shape...]
    arr[i]
    arr[i, j, ...]
    compound arithmetic expressions
    if / else
    while
    break / continue
    print

Rejected / not yet implemented:
    recursion
    references
    void functions
    multiple output ports
    overloads
    generics
    function values
    early-return-as-control-flow
```

## Examples

### Function demo

```flow
module fn_demo

fn square(n : int): int {
    n * n -> return
}

fn add_then_square(a : int, b : int): int {
    sum : int(0)
    a + b -> sum
    square(sum) -> return
}

main {
    x : int(3)
    y : int(4)
    result : int(0)

    add_then_square(x, y) -> result
    print result
}
```

Expected output:

```text
49
```

### Iterative factorial in a function

```flow
module fn_iterative_factorial

fn factorial(n : int): int {
    one : int(1)
    acc : int(1)

    while n > one {
        acc * n -> acc
        n - one -> n
    }

    acc -> return
}

main {
    n      : int(stdin.int())
    result : int(0)

    factorial(n) -> result
    print result
}
```

`echo 5 | ./build/flowmini examples/fn_iterative_factorial.flow` prints:

```text
120
```

### Function-based Collatz step

```flow
fn collatz_next(n : int): int {
    zero  : int(0)
    two   : int(2)
    three : int(3)
    rem   : int(0)

    n % two -> rem

    if rem == zero {
        n / two -> return
    } else {
        n * three + one -> return
    }
}
```

## Build and run

```bash
cmake -S . -B build
cmake --build build -j20

echo 0 | ./build/flowmini examples/fn_demo.flow
echo 5 | ./build/flowmini examples/fn_collatz_next.flow
echo 5 | ./build/flowmini examples/fn_iterative_factorial.flow
echo 0 | ./build/flowmini examples/matrix_multiply_2x2.flow
```

Observed outputs:

```text
49

5
16
8
4
2
1

120

19 22
43 50
```

## Design note

`fn` is intentionally interpreted as a node-contract-friendly construct, not as a hidden C-style call stack model.

For v12 it lowers by fresh instantiation/inlining, but the semantic model is future-safe:

```text
function declaration = reusable node contract
arguments            = input ports
return               = default output port
function call        = input binding + output routing
```

Later Flowcore can grow explicit reference/capability modes, long-lived node instances, ordinary port emission, orchestration policies, timing contracts, and parallel scheduling without contradicting this first `fn` layer.
