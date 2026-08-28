# flowmini

Minimal experiment for the Flow Policy Envelope Pattern as a graph-language runtime.

This deliberately tiny subset proves:

- source text -> lexer -> parser -> graph schema
- graph contract validation against native atom contracts
- envelope traversal through a runtime graph
- conditional routing
- loop/backedge
- mutation of state carried in payload
- effectful node that writes to stdout while passing the envelope onward
- halt as a terminal sink

## Layout

```text
include/    public/internal headers
src/        implementations
examples/   tiny `.flow` source files
```

The shared definitions from the earlier refactor are kept as:

```text
include/flow_common.h
src/flow_common.cpp
```

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

## Run

```bash
echo 5 | ./build/flowmini examples/countdown.flow
```

Expected output:

```text
5
4
3
2
1
```

With tracing:

```bash
echo 3 | ./build/flowmini --trace true examples/countdown.flow
```

Trace diagnostics go to stderr.

## Minimal language

```text
module <name>
producer <id> : <atom.kind>
node <id> : <atom.kind>
sink <id> : <atom.kind>
wire <from_node>.<from_port> => <to_node>.<to_port>
```

Comments may start with `#` or `//`.

## Built-in atoms in v0

| Atom | Input | Outputs | Role |
|---|---:|---:|---|
| `stdin.text` | `in: Unit` | `out: Text` | reads stdin |
| `parse.int` | `in: Text` | `out: Int` | parses integer |
| `int.gt_zero` | `in: Int` | `true: Int`, `false: Int` | conditional router |
| `stdout.int_line` | `in: Int` | `out: Int` | writes integer, passes payload onward |
| `int.dec` | `in: Int` | `out: Int` | decrements integer payload |
| `halt` | `in: Int` | none | stops flow |
```

