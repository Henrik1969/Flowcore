# flowmini v4 - indexed memory / list layer

This version adds the next primitive layer discovered by the stress tests: addressable indexed memory.

The v3 record machine proved scalar state, arithmetic, comparison, routing, looping, copying, swapping, and derived compare-swap. The v4 list layer adds enough structure to express algorithms that need collection traversal and dynamic indexing.

## Build

```bash
cmake -S . -B build
cmake --build build -j20
```

## New list atoms

Need-to-have indexed-memory atoms:

```text
list.from_ints
list.length
list.get
list.set
list.swap
```

Nice-to-have derived/library atoms:

```text
list.compare_swap_asc
list.compare_swap_desc
list.print_int_lines
```

`list.compare_swap_asc` is a convenience atom. Semantically, it lowers to indexed get, comparison, boolean route, and indexed swap.

## Important policies

```text
policy node.list = "arr"
policy node.out = "field"
policy node.index = "i"
policy node.index_const = 0
policy node.a = "i"
policy node.b = "next"
policy node.values = "1,9,3,5,8,2,2,6,4"
```

## New tests

```bash
echo 0 | ./build/flowmini examples/list_print.flow
# 1
# 9
# 3
# 5
# 8
# 2
# 2
# 6
# 4

echo 0 | ./build/flowmini examples/sum_list.flow
# 40

echo 0 | ./build/flowmini examples/min_list.flow
# 1

echo 0 | ./build/flowmini examples/bubblesort_list.flow
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

## Existing general tests retained

```bash
echo 5 | ./build/flowmini examples/countdown_general.flow
# 5 4 3 2 1, one per line

echo 5 | ./build/flowmini examples/collatz_general.flow
# 5 16 8 4 2 1, one per line

echo 5 | ./build/flowmini examples/factorial_general.flow
# 120

echo 10 | ./build/flowmini examples/fibonacci_general.flow
# 55

echo 462 | ./build/flowmini examples/gcd_general.flow
# 21

echo 100 | ./build/flowmini examples/sum_1_to_n_general.flow
# 5050

echo 10 | ./build/flowmini examples/power2_general.flow
# 1024
```

## What v4 proves

```text
source -> lexer -> parser -> graph schema -> contract validator -> runtime traversal
```

With:

```text
RecordPayload
List<Value>
dynamic index fields
collection traversal
indexed read/write
indexed swap
nested loop sorting
```

This moves the model from scalar computation to addressable memory computation.
