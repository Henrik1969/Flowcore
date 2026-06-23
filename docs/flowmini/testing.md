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
$TOP/tools/run-flowmini-test-suite.sh
```

Useful environment:

```bash
export TOP="/home/henrik/Projekter/scratchpad/flow_Policy_envelope_pattern"
export FLOWLAB_TOP="$TOP"
export FLOWMINI_ROOT="$TOP/Flowmini/flowmini_v22_unit_kinds"
export FLOWMINI_STDIN=5
```

Expected current baseline:

```text
total: 75
pass:  75
bad:   0
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
FLOWMINI_VALGRIND=1 $TOP/tools/run-flowmini-test-suite.sh
FLOWMINI_GDB_FAILURES=1 $TOP/tools/run-flowmini-test-suite.sh
FLOWMINI_GDB_ALL=1 $TOP/tools/run-flowmini-test-suite.sh
```
