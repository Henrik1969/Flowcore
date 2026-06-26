# Compiling Flowcore

This document records the current working compiler pipeline smoke path.

The language design is still moving, but the first three executable stages are now
in place:

```text
flowlex | flowparse | flowcheck
```

Later stages exist as placeholders:

```text
flowoptimize | flowlink | flowemit
```

## Build

From the Flowcore repository root:

```bash
cmake -S . -B build
cmake --build build -j
```

The current smoke tests are:

```bash
ctest --test-dir build --output-on-failure
```

Expected result:

```text
Test #1: AstLiib_smoke_tests ..............   Passed
Test #2: flowparse_smoke ..................   Passed
Test #3: flowcheck_smoke ..................   Passed

100% tests passed, 0 tests failed out of 3
```

## Smoke Source

The current end-to-end sample is:

```text
lexers/flowlex_v0_1a/examples/load_config.flow
```

It exercises:

- `package` and `module`
- `effect`
- `abi`
- `contract ... : wire`
- `node ... : provider/transmuter`
- port declarations such as `in.data` and `out.error`
- `effects {}`
- `impl`, `let`, and `emit`
- explicit `flow` wiring with `wire`, `node`, and `connect`
- flow sugar such as `ast <- ParseConfig(bytes)`

## Stage 1: Lexing

`flowlex` reads one source file and emits JSON Lines tokens.

```bash
build/lexers/flowlex_v0_1a/flowlex \
  lexers/flowlex_v0_1a/examples/load_config.flow
```

To count tokens without the final `EOF` token:

```bash
build/lexers/flowlex_v0_1a/flowlex --no-eof \
  lexers/flowlex_v0_1a/examples/load_config.flow | wc -l
```

Current result:

```text
317
```

Important current token examples:

```text
emit ast <- astValue      -> KEYWORD emit, IDENTIFIER ast, BIND_LEFT <-
connect path -> ...      -> KEYWORD connect, GRAPH_ARROW ->
ast <- ParseConfig(...)   -> IDENTIFIER ast, BIND_LEFT <-
```

## Stage 2: Parsing

`flowparse` consumes the token stream from `flowlex`, builds an AstLiib-backed
token tree, and performs the current syntax checks.

```bash
build/lexers/flowlex_v0_1a/flowlex \
  lexers/flowlex_v0_1a/examples/load_config.flow \
| build/parsers/Handwritten_V1/flowparse \
> /tmp/flowparse-load-config.json
```

Summary:

```bash
jq '{parser, syntax, token_count, node_count, diag_count: (.diagnostics | length), root_kind: .tree.kind}' \
  /tmp/flowparse-load-config.json
```

Current result:

```json
{
  "parser": "flowparse-astliib-cpp",
  "syntax": "ok",
  "token_count": 317,
  "node_count": 318,
  "diag_count": 0,
  "root_kind": "ROOT"
}
```

The node count is one greater than the token count because AstLiib adds the root
node.

## Stage 3: Semantic Checking

`flowcheck` consumes the parser artifact and validates first-pass semantic
coherence.

```bash
build/lexers/flowlex_v0_1a/flowlex \
  lexers/flowlex_v0_1a/examples/load_config.flow \
| build/parsers/Handwritten_V1/flowparse \
| build/stages/flowcheck/flowcheck
```

Current result:

```json
{
  "checker":"flowcheck-cpp",
  "semantics":"ok",
  "diagnostics":[]
}
```

The current semantic checks include:

- contracts contain `payload`, `abi`, `protocol`, and `lane`
- node ports reference declared contracts
- node and flow effect sets reference declared effects
- flow wires reference declared contracts
- flow node instances reference declared node types
- `connect` endpoints resolve to declared wires or `instance.direction.lane.port`
- flow sugar references declared wires and node types
- duplicate contracts, nodes, flows, ports, and flow bindings are reported

## Current Boundary

`flowparse` is still a structural stage. It builds and emits the AstLiib token
tree, and it checks syntax shape.

`flowcheck` owns semantic coherence. That keeps the first pipeline boundary clear:

```text
source file
  -> flowlex token stream
  -> flowparse AstLiib tree artifact
  -> flowcheck semantic result
```
