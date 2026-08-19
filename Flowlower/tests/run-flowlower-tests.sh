#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
lowerer=${FLOWLOWER_BIN:?FLOWLOWER_BIN is required}
optimizer=${FLOWOPTIMIZE_BIN:?FLOWOPTIMIZE_BIN is required}
analyst=${FLOWANALYST_BIN:?FLOWANALYST_BIN is required}
flowmini="$root/Flowmini/flowmini_v25_symboltable_projection/cmake-build-debug/flowmini"
fixture="$root/Flowmini/flowmini_v25_symboltable_projection/examples/ast/call_expression_probe.flow"

report=$("$flowmini" --dump-frontend-bundle "$fixture" | "$analyst" | "$optimizer" | "$lowerer")
printf '%s\n' "$report" | grep -q '"format": "flowlower.lowering_report"'
printf '%s\n' "$report" | grep -q '"name": "llvm"'
printf '%s\n' "$report" | grep -q '"format": "llvm-ir"'

if printf '%s' '{"format":"flowoptimize.optimization_report","version":1,"status":"blocked"}' | "$lowerer" >/dev/null 2>&1; then
    echo 'blocked optimization report unexpectedly lowered' >&2
    exit 1
fi

tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT
trial="$root/Flowlower/tests/empty_program_main.flow"
"$flowmini" --dump-frontend-bundle "$trial" | "$analyst" | "$optimizer" | "$lowerer" --emit-llvm "$tmpdir/trial.ll" > "$tmpdir/lowering-report.json"
grep -q '"status": "emitted"' "$tmpdir/lowering-report.json"
test -s "$tmpdir/trial.ll"
clang "$tmpdir/trial.ll" -o "$tmpdir/trial"
"$tmpdir/trial"
echo 'Flowlower tests: PASS'
