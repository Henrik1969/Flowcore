# LangLab table-driven lexer 0.1-A

This proof-of-concept lexer separates lexical **data** from scanning **process**.

```text
config/flowcore.rules
    what stream pieces look like
    which token or action each piece produces

src/tablelex.cpp
    generic longest-match scanner
    JSONL emitter
    annotation dispatcher
```

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

## Run

```bash
./build/tablelex \
    --rules config/flowcore.rules \
    --source examples/annotations.flow \
    --source-id 42 \
    < examples/annotations.flow
```

## Rules

Each non-comment line in `config/flowcore.rules` has:

```text
TOKEN_NAME | ECMAScript regex | action
```

Actions:

```text
emit
identifier
newline
annotation
skip
```

The scanner matches only at the current stream position.
Longest match wins. File order breaks equal-length ties.

## Annotations

Source comments may request inspectable notices:

```text
// !!!emit stderr entering @file at line @linenr
// !!!emit stream declared x at @file:@linenr:@column
// !!!emit stdout finished source @sourceid
```

Channels:

```text
stderr
    human-readable notice

stream
stdout
    GHOST_ANNOTATION JSONL record on stdout
```

`stdout` deliberately remains valid JSONL. Arbitrary text would corrupt the token-stream contract.

Macros:

```text
@file
@sourceid
@linenr
@line
@column
```

## Smoke test

```bash
./tests/smoke.sh
```

## Deferred

```text
nested block comments
streaming instead of whole-file buffering
Unicode identifiers
decoded literal values
strict numeric underscore validation
configurable keyword table
separate diagnostics JSONL stream
```
