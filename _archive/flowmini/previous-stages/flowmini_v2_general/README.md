# flowmini v2 - generalized primitive graph model

`flowmini` is a small experiment in the Flow Policy Envelope Pattern.

Version 1 proved that a tiny graph language could execute countdown and Collatz by adding task-shaped atoms.

Version 2 starts moving toward a generalized primitive model:

- source is lexed and parsed into a graph schema
- graph nodes are validated by atom contracts
- node policies configure behavior
- runtime traverses the graph mechanically
- payload can now be a small inspectable/mutable `Record`
- generic integer operations work over named record fields

The important shift is that algorithms can be expressed by composing primitive atoms instead of adding a special node for every task.

## Layout

```text
flowmini_v2_general/
├── CMakeLists.txt
├── README.md
├── examples/
│   ├── countdown.flow
│   ├── countdown_general.flow
│   ├── collatz.flow
│   └── collatz_general.flow
├── include/
│   ├── flow_common.h
│   ├── flowmini_lexer.h
│   ├── flowmini_parser.h
│   ├── flowmini_payload.h
│   ├── flowmini_runtime.h
│   └── flowmini_schema.h
└── src/
    ├── flow_common.cpp
    ├── flowmini_lexer.cpp
    ├── flowmini_parser.cpp
    ├── flowmini_payload.cpp
    ├── flowmini_runtime.cpp
    └── main.cpp
```

## Build

```bash
cmake -S . -B build
cmake --build build -j20
```

## Run

```bash
echo 5 | ./build/flowmini examples/countdown_general.flow
```

Expected output:

```text
5
4
3
2
1
```

```bash
echo 5 | ./build/flowmini examples/collatz_general.flow
```

Expected output:

```text
5
16
8
4
2
1
```

Trace graph traversal:

```bash
echo 3 | ./build/flowmini --trace true examples/countdown_general.flow
```

## Minimal source language

```text
module <name>
producer <id> : <atom.kind>
node <id> : <atom.kind>
sink <id> : <atom.kind>
policy <node>.<key> = <string|int|bool>
wire <from_node>.<from_port> => <to_node>.<to_port>
```

Comments use `#` or `//`.

## General primitive atoms

The new generalized atoms operate on `Record` payloads.

```text
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

The older task-shaped atoms are retained for comparison:

```text
parse.int
int.gt_zero
int.require_positive
int.eq_one
int.is_even
int.dec
int.half
int.three_n_plus_one
stdout.int_line.legacy
halt
```

## Example: countdown using generic atoms

`examples/countdown_general.flow` does not use `int.gt_zero` or `int.dec`.

Instead it uses:

```text
const.int 0
int.gt
route.bool
const.int 1
int.sub
```

So this:

```text
while n > 0:
    print n
    n = n - 1
```

is represented as graph traversal and record-field mutation.

## Example: Collatz using generic atoms

`examples/collatz_general.flow` does not use `int.eq_one`, `int.is_even`, `int.half`, or `int.three_n_plus_one`.

Instead it uses:

```text
int.eq
int.mod
int.div
int.mul
int.add
route.bool
```

So `3n + 1` is no longer hidden in a special node. It is described by primitive graph composition.

## Current limitation

Record paths are deliberately tiny in v2: only simple top-level names such as `n`, `one`, `cond`, `remainder`.

Nested paths and lists are represented in the `Value` model, but not yet exposed through path syntax or primitive nodes.
