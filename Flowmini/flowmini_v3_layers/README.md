# flowmini v3 - primitive core plus derived library atoms

This is the next Flow Policy Envelope Pattern experiment.

The point of v3 is to separate atoms into two practical categories:

- **need-to-have primitives**: small, boring, general computation atoms
- **nice-to-have derived/library atoms**: atoms that collapse common patterns but can be lowered to primitives

The runtime remains the same graph/envelope traversal model:

```text
source.flow -> lexer -> parser -> graph schema -> contract validation -> runtime traversal
```

The grammar is still deliberately small:

```text
module <name>
producer <id> : <atom.kind>
node <id> : <atom.kind>
sink <id> : <atom.kind>
policy <node>.<key> = <string|int|bool>
wire <from_node>.<from_port> => <to_node>.<to_port>
```

## Existing primitive-style atoms

```text
stdin.text
parse.int.to_record
const.int
int.add
int.sub
int.mul
int.div
int.mod
int.eq
int.lt
int.gt
route.bool
stdout.int_line
halt.record
```

## New v3 atoms

### Need-to-have / near-core

```text
record.copy
```

Copies one record field to another.

Policies:

```text
policy copy.from = "source_field"
policy copy.to = "target_field"
```

### Nice-to-have / derived-library atoms

```text
record.swap
int.compare_swap_asc
int.compare_swap_desc
```

`record.swap` swaps two record fields.

`int.compare_swap_asc` performs:

```text
if a > b:
    swap(a, b)
```

`int.compare_swap_desc` performs:

```text
if a < b:
    swap(a, b)
```

These are intentionally not considered bedrock primitives. They are convenience atoms that prove the ergonomic layer can collapse common routing-heavy patterns.

## Build

```bash
cmake -S . -B build
cmake --build build -j20
```

## Examples

### Existing tests

```bash
echo 5 | ./build/flowmini examples/countdown_general.flow
echo 5 | ./build/flowmini examples/collatz_general.flow
```

### record.copy

```bash
echo 0 | ./build/flowmini examples/record_copy_example.flow
```

Expected:

```text
42
42
```

### record.swap

```bash
echo 0 | ./build/flowmini examples/record_swap_example.flow
```

Expected:

```text
20
10
```

### factorial

```bash
echo 5 | ./build/flowmini examples/factorial_general.flow
```

Expected:

```text
120
```

### bubble sort using compare-swap

Sorts:

```text
1 9 3 5 8 2 2 6 4
```

Run:

```bash
echo 0 | ./build/flowmini examples/bubblesort_compare_swap.flow
```

Expected:

```text
1
2
2
3
4
5
6
8
9
```

## Design note

The primitive-only bubble sort is possible, but ugly. That taught us the useful split:

```text
need-to-have nodes:
    make computation possible

nice-to-have nodes:
    make computation tolerable
```

The discipline remains:

```text
Every nice-to-have atom should either lower to primitives or be explicitly declared as a native/prebuilt atom.
```
