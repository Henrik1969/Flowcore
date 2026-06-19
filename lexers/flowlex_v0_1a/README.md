# flowlex 0.1-A

A deliberately small hand-written lexer for the first FlowCore frontend milestone.

## Contract

```text
stdin:   FlowCore source text
stdout:  JSON Lines token stream
stderr:  diagnostics
status:  0 success, non-zero lexical failure
```

## Build and run

```bash
cmake -S . -B build
cmake --build build -j
./build/flowlex < examples/smoke.flow
./tests/smoke.sh
```

Pretty-print with `jq`:

```bash
./build/flowlex < examples/smoke.flow | jq
```

## Included in 0.1-A

- significant `NEWLINE` tokens
- optional semicolon separator
- UTF-8 `§` declaration token
- identifiers and reserved keywords
- integer, floating-point, scientific, hexadecimal, binary, and octal literals
- string and character literals
- `//` and `/* ... */` comments
- longest-match operators including `=>`, `->`, `<=`, `>=`, `==`, `!=`, `<<`, `>>`, and `..`
- source line and column diagnostics

## Deliberately deferred

- Unicode identifiers
- multiline strings
- numeric suffixes such as `u16` and `f64`
- escape decoding
- parser-level newline suppression inside incomplete expressions
- semantic type interpretation
- AST construction

The lexer emits `NEWLINE`. The parser will decide when a newline separates statements and when it behaves as whitespace inside an incomplete expression.
