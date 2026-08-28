# Flowmini test expectations

This directory pins observable behavior for the categorized Flowmini suite.

```text
examples/pass/*.flow     must exit 0
examples/fail/*.flow     must exit nonzero
examples/support/*.flow  ignored unless explicitly requested
```

Expected-output files are optional, but when present the suite runner checks them.

```text
tests/expected/stdout/<test-name>.out
    Exact stdout expected from a passing program.

tests/expected/stderr/<test-name>.err
    Exact stderr expected from a test. Use sparingly because diagnostics may include paths.

tests/expected/diagnostics/<test-name>.contains
    One required diagnostic substring per non-empty line. Preferred for negative tests.
```

The current baseline was generated from `flowmini_v22_unit_kinds` with stdin value `5`.
The suite passed 75/75 with these expectations.

Useful commands:

```bash
$TOP/tools/run-flowmini-test-suite.sh
FLOWMINI_VALGRIND=1 $TOP/tools/run-flowmini-test-suite.sh
FLOWMINI_GDB_FAILURES=1 $TOP/tools/run-flowmini-test-suite.sh
```
