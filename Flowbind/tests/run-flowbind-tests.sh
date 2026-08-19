#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
bin=${FLOWBIND_BIN:?FLOWBIND_BIN is required}
flowmini=${FLOWMINI_BIN:?FLOWMINI_BIN is required}
flowanalyst=${FLOWANALYST_BIN:?FLOWANALYST_BIN is required}
fixture=$root/Flowmini/flowmini_v25_symboltable_projection/examples/pass/abi_libc_demo.flow

report=$("$flowmini" --dump-frontend-bundle "$fixture" | "$flowanalyst" | "$bin")
printf '%s\n' "$report" | grep -q '"status": "ready"'
printf '%s\n' "$report" | grep -q '"execution": "not-performed"'

set +e
bad=$(printf '%s' '{"format":"flowanalyst.semantic_report","version":1,"status":"ok","binding_requirements":[{"contract":"bad","library":"libc.so.6","convention":"c","symbol":"flowcore_symbol_that_does_not_exist","effect":"pure"}]}' | "$bin")
bad_rc=$?
set -e
test "$bad_rc" -eq 2
printf '%s\n' "$bad" | grep -q 'unavailable'
echo 'Flowbind tests: PASS'
