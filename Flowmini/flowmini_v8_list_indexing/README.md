# flowmini v8 — list indexing frontend

This version builds on v7 primitive `if/else` and adds the first structured list/indexing sugar.

The architecture remains unchanged:

```text
.flow     human source
.flowir   explicit primitive graph IR
runtime   existing flow-policy-envelope graph traversal
```

## New in v8

### List element read

```flow
arr[i] -> current
```

Lowers to a primitive `list.get` node:

```flowir
node __..._list_get_N : list.get
policy __..._list_get_N.list = __main_arr
policy __..._list_get_N.index = __main_i
policy __..._list_get_N.out = __main_current
```

### List element write

```flow
current -> arr[i]
```

Lowers to a primitive `list.set` node:

```flowir
node __..._list_set_N : list.set
policy __..._list_set_N.list = __main_arr
policy __..._list_set_N.index = __main_i
policy __..._list_set_N.value = __main_current
```

### Indexed expressions

```flow
arr[i] + one -> current
```

The frontend lowers `arr[i]` to a temporary via `list.get`, then lowers the arithmetic expression.

### List length expression

```flow
len : int(length(arr))
```

Lowers to `list.length`.

### Print list

```flow
print arr
```

For `list<int>`, this lowers to `list.print_int_lines`.

### Start record producer

Programs that do not read stdin can now start with a generated `start.record` producer. This avoids dummy stdin input for pure list/constant examples.

## Hard semantic rules preserved

```text
- Declaration must initialize.
- Assignment never declares.
- Assignment must satisfy the existing contract.
- Implicit shadowing is forbidden.
- Indexed access requires list<int> and int index.
- Writing to arr[i] requires arr : list<int> and assigned value : int.
```

## New examples

```text
examples/list_index_read_write.flow
examples/sum_list_indexed_structured.flow
examples/bubblesort_indexed_structured.flow
examples/bad_list_wrong_type.flow
examples/bad_list_unknown.flow
```

## Build

```bash
cmake -S . -B build
cmake --build build -j20
```

## Tests

```bash
echo 0 | ./build/flowmini examples/list_index_read_write.flow
# 9
# 42
# 1
# 42
# 3

echo 0 | ./build/flowmini examples/sum_list_indexed_structured.flow
# 40

echo 0 | ./build/flowmini examples/bubblesort_indexed_structured.flow
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
```

## Inspect lowered IR

```bash
./build/flowmini --emit-flowir - examples/bubblesort_indexed_structured.flow
```

## What v8 proves

v8 proves that structured source can now address values inside collections while still lowering to explicit primitive graph mechanics.

The language has moved from scalar structured algorithms into indexed mutable collection algorithms without changing the runtime architecture.

## v8.1 warning cleanup

This package also cleans the GCC `-Wdangling-reference` warnings that appeared in the list runtime code.
The underlying issue was not a failing test case, but the compiler correctly made the code look suspicious: list runtime nodes held references returned from helper calls across later record mutation / output operations. The code now copies the short-lived list/value data where appropriate before mutating the record or printing, keeping ownership/lifetime explicit.

The parser `[[nodiscard]]` warnings for intentionally discarded top-level parse probes were also removed by binding the returned step to an explicitly ignored local.

Representative tests run after cleanup:

```bash
cmake -S . -B build
cmake --build build -j20

echo 5 | ./build/flowmini examples/countdown_structured.flow
echo 5 | ./build/flowmini examples/collatz_structured.flow
echo 0 | ./build/flowmini examples/list_index_read_write.flow
echo 0 | ./build/flowmini examples/sum_list_indexed_structured.flow
echo 0 | ./build/flowmini examples/bubblesort_indexed_structured.flow
```
