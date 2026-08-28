# flowmini v10 — compound expressions

This version builds on v9 structured control and adds compound **value expressions** while keeping boolean/control evaluation as a separate semantic layer.

The architecture remains unchanged:

```text
.flow     human source
.flowir   explicit primitive graph IR
runtime   existing flow-policy-envelope graph traversal
```

## New in v10

### Compound value expressions

```flow
n * three + one -> n
```

This lowers to primitive graph mechanics, for example:

```flowir
node __..._op_N : int.mul
policy __..._op_N.lhs = __main_n
policy __..._op_N.rhs = __main_three
policy __..._op_N.out = __..._tmp

node __..._op_M : int.add
policy __..._op_M.lhs = __..._tmp
policy __..._op_M.rhs = __main_one
policy __..._op_M.out = __main_n
```

### Parenthesized expressions

```flow
(a + b) * c -> result
```

### Compound list index expressions

```flow
arr[j + one] -> value
```

### Compound expressions in predicates

Primitive control predicates remain separate from general boolean logic, but a predicate may consume compound **value** expressions on either side:

```flow
while j + one < len {
    ...
}

if arr[j] > arr[j + one] {
    ...
}
```

## Semantic split preserved

```text
value expression:
    produces a value such as int

predicate/control evaluation:
    produces a bool used for routing in if/while
```

In v10 this means:

```text
Supported now:
    n * three + one -> n
    while j + one < len { ... }
    if arr[j] > arr[j + one] { ... }

Still later:
    a && b
    a || b
    compound boolean logic / short-circuiting
```

## Hard semantic rules preserved

```text
- Declaration must initialize.
- Assignment never declares.
- Assignment must satisfy the existing contract.
- Implicit shadowing is forbidden.
- Compound value expressions and predicate/control evaluation are distinct layers.
```

## New examples

```text
examples/compound_expression_demo.flow
examples/collatz_compound.flow
examples/bubblesort_compound.flow
```

## Build

```bash
cmake -S . -B build
cmake --build build -j20
```

## Tests

```bash
echo 5 | ./build/flowmini examples/compound_expression_demo.flow
# 16

echo 5 | ./build/flowmini examples/collatz_compound.flow
# 5
# 16
# 8
# 4
# 2
# 1

echo 0 | ./build/flowmini examples/bubblesort_compound.flow
# 1
# 2
# 2
# 3
# 4
# 5
# 6
# 8
# 9
```

Existing structured examples still work:

```bash
echo 5 | ./build/flowmini examples/countdown_structured.flow
echo 5 | ./build/flowmini examples/collatz_structured.flow
echo 0 | ./build/flowmini examples/bubblesort_indexed_structured.flow
```

## Inspect lowered IR

```bash
./build/flowmini --emit-flowir - examples/collatz_compound.flow
```

## What v10 proves

v10 proves that Flowmini can express real compound value transformations in human source while still lowering to explicit primitive graph mechanics. The runtime architecture is unchanged; the frontend simply became more expressive.
