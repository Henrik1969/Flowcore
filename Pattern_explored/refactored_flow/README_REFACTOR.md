# Refactored Flow Policy Envelope examples

This refactor splits the repeated proof-of-concept scaffolding out of the four original single-file programs.

## Common layer

- `flow_common.h/.cpp`
  - `Severity`, `Diagnostic`, `DiagnosticError`
  - `PolicyValue`, `PolicyBag`
  - `PipelineContext`
  - `ByteBuffer`
  - typed `Envelope<T>`
  - `StdinProducer`
  - binary-safe `StdoutSink`
  - `LogSink`
  - `parseBool`, `parseInt`, `requireArgValue`
  - generic `operator>>` pipeline glue

## Tool-specific layers

- `base64_stage.h/.cpp` + `base64_main.cpp`
- `hex_stage.h/.cpp` + `hex_main.cpp`
- `bitart_stage.h/.cpp` + `bitart_main.cpp`
- `collatz_pipeline.h/.cpp` + `collatz_main.cpp`

The Base64, Hex, and BitArt tools now contain mostly their transformation logic plus policy-loading for their own CLI flags.

The Collatz example still has its routed graph runtime in `collatz_pipeline.*` because it is structurally different from the byte-filter tools, but it reuses the same common diagnostics, policy bag, context, logging, and CLI parsing primitives.

## Build

```bash
cmake -S . -B build
cmake --build build -j20
```

## Smoke tests

```bash
printf 'Hello Flowcore\n' | ./build/flowbase64 --mode encode | ./build/flowbase64 --mode decode
printf 'Hello World\n'   | ./build/flowhex --mode encode   | ./build/flowhex --mode decode
echo 27 | ./build/flowexec
```
