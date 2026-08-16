# Flowmini Testing

Flowmini uses a categorized integration test suite.

## Categories

```text
examples/pass/*.flow
    runnable programs expected to succeed

examples/fail/*.flow
    examples expected to fail with diagnostics

examples/support/*.flow
    importable units / support files

examples/docs/*
    documentation
```

## Runner

```bash
cd Flowmini/flowmini_v24_explicit_ast
../../tools/run-flowmini-test-suite.sh \
    --root . \
    --build-dir cmake-build-debug \
    --no-build
```

The canonical CMake-visible gates are:

```bash
cmake --build cmake-build-debug --target flowmini_ast_golden_tests
cmake --build cmake-build-debug --target flowmini_symbol_projection_tests
cmake --build cmake-build-debug --target flowmini_suite
ctest --test-dir cmake-build-debug --output-on-failure
```

Expected current baseline:

```text
AST golden tests:          21 / 21
Symbol projection tests:    7 / 7
categorized suite:          78 / 78
CTest:                       2 / 2
```

## Expected files

```text
tests/expected/stdout/<name>.out
tests/expected/stderr/<name>.err
tests/expected/diagnostics/<name>.contains
```

Diagnostic `.contains` files are substring checks. They should contain stable diagnostic phrases, not full path-sensitive stderr.

## Optional modes

```bash
../../tools/run-flowmini-test-suite.sh --root . --build-dir cmake-build-debug --no-build --valgrind
../../tools/run-flowmini-test-suite.sh --root . --build-dir cmake-build-debug --no-build --gdb-failures
../../tools/run-flowmini-test-suite.sh --root . --build-dir cmake-build-debug --no-build --gdb-all
../../tools/run-flowmini-test-suite.sh --root . --build-dir cmake-build-debug --no-build --run-support
```

`--run-support` executes importable support units as expected failures when used
as root sources. The current support-inclusive firetest result is 94/94.

## Current build-isolation limitation

The legacy ABI example path still loads:

```text
build/libflowmini_testabi.so
```

The `flowmini_prepare_test_dependencies` target copies the provider from the
selected CMake build tree into that source-tree path. Consequently, separate
normal, sanitizer, or compiler build trees can overwrite one another's ABI test
provider. Do not run those preparation steps concurrently. Before testing with
a different build tree, restore its matching provider explicitly:

```bash
cmake --build <build-tree> --target flowmini_prepare_test_dependencies
```

This is a documented compatibility bridge, not the intended future
prerequisite/provider model.
