# flowmini v9 — break / continue

This version builds on v8 polished.

The language split remains:

```text
.flow   = human source with sugar
.flowir = explicit primitive graph IR / lowered language
```

The primitive `.flowir` graph/runtime model is still the execution target. The structured frontend lowers into it.

## New in v9

Primitive loop control:

```flow
break
continue
```

Semantics:

```text
break    -> exits the nearest enclosing while loop
continue -> jumps to the nearest enclosing while loop's next condition check
```

Both are only legal inside loops.

The lowerer keeps a loop-context stack:

```text
LoopContext {
    continueTarget = current loop condition entry
    breakTarget    = current loop exit join
}
```

A `break` or `continue` lowers to a `record.nop` node wired to the appropriate hidden loop target.

## Hard rules preserved

```text
no uninitialized declarations
no undeclared assignment
no implicit shadowing
no break outside loops
no continue outside loops
no unreachable statements after unconditional break/continue in the same block
```

## Example: break

```flow
module first_even_break

main {
    arr   : list<int>([1,3,5,8,9])
    len   : int(length(arr))
    zero  : int(0)
    one   : int(1)
    two   : int(2)
    i     : int(0)
    value : int(0)
    rem   : int(0)

    while i < len {
        arr[i] -> value
        value % two -> rem

        if rem == zero {
            print value
            break
        }

        i + one -> i
    }
}
```

Expected output:

```text
8
```

## Example: continue

```flow
module odd_values_continue

main {
    arr   : list<int>([1,2,3,4,5])
    len   : int(length(arr))
    zero  : int(0)
    one   : int(1)
    two   : int(2)
    i     : int(0)
    value : int(0)
    rem   : int(0)

    while i < len {
        arr[i] -> value
        value % two -> rem

        if rem == zero {
            i + one -> i
            continue
        }

        print value
        i + one -> i
    }
}
```

Expected output:

```text
1
3
5
```

## Build

```bash
cmake -S . -B build
cmake --build build -j20
```

## Smoke tests

```bash
echo 5 | ./build/flowmini examples/countdown_structured.flow
echo 5 | ./build/flowmini examples/collatz_structured.flow
echo 0 | ./build/flowmini examples/first_even_break.flow
echo 0 | ./build/flowmini examples/odd_values_continue.flow
echo 0 | ./build/flowmini examples/bubblesort_indexed_structured.flow
```

## Diagnostic tests

```bash
echo 0 | ./build/flowmini examples/bad_break_outside_loop.flow
echo 0 | ./build/flowmini examples/bad_continue_outside_loop.flow
echo 0 | ./build/flowmini examples/bad_unreachable_after_break.flow
```

## Inspect lowered graph

```bash
./build/flowmini --emit-flowir - examples/first_even_break.flow
./build/flowmini --emit-flowir - examples/odd_values_continue.flow
```

