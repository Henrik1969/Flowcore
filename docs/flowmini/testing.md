# Flowmini Testing

Flowmini uses a categorized integration test suite.

The project-wide [verification-gate policy](../development/verification-gates.md)
defines the binding Tier 1, Tier 2, and Tier 3 Firetest requirements. The
commands below are the current Flowmini realization of those gates.

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
cd Flowmini/flowmini_v25_symboltable_projection
../../tools/run-flowmini-test-suite.sh \
    --root . \
    --build-dir cmake-build-debug \
    --no-build
```

The canonical CMake-visible gates are:

```bash
cmake --build cmake-build-debug --target flowmini_ast_golden_tests
cmake --build cmake-build-debug --target flowmini_symbol_projection_tests
cmake --build cmake-build-debug --target flowmini_frontend_bundle_tests
cmake --build cmake-build-debug --target flowmini_suite
ctest --test-dir cmake-build-debug --output-on-failure
```

Expected current baseline:

```text
AST golden tests:          28 / 28
Symbol projection tests:   14 / 14
Frontend bundle tests:      8 golden / 1 isolated / 19 negative
downstream sibling CTest:   PASS
flowcat ELF example:        PASS
```

These normal gates form the Flowmini Tier 2 integration baseline. Before
declaring a greater architectural border closed, run and record the additional
Tier 3 Firetest pressure checks defined by the project-wide policy.

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

Compiler, sanitizer, Valgrind, support-inclusive, and concurrency results are
checkpoint evidence rather than permanent properties of the branch. A new
greater-border claim requires a new Firetest report tied to the tested commit or
working-tree state.

The raw-frontend checkpoint evidence is recorded in the
[v0.24 frontend Firetest report](v0.24-firetest-report.md). The independent
export boundary has a separate
[frontend-border Firetest report](v0.24-frontend-border-firetest-report.md).

The active v0.25 structural-origin contract and its negative attack surface are
documented in the [origin maturity audit](v0.25-origin-maturity-audit.md).

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

The current struct-by-value ABI probe also shares the focused C declaration in
`subprojects/testabi/include/flowmini_testabi.h` between the test provider and
the runtime's existing hard-coded `Point` compatibility path. This keeps the
function-pointer type and carrier layout exact under UndefinedBehaviorSanitizer.
It is test-provider infrastructure, not a canonical Flowmini ABI type or the
future general ABI call mechanism.
