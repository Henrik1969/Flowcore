#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
lowerer=${FLOWLOWER_BIN:?FLOWLOWER_BIN is required}
optimizer=${FLOWOPTIMIZE_BIN:?FLOWOPTIMIZE_BIN is required}
analyst=${FLOWANALYST_BIN:?FLOWANALYST_BIN is required}
bind=${FLOWBIND_BIN:?FLOWBIND_BIN is required}
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

policy="$tmpdir/abi.policy"
printf '%s\n' \
    'allow libc.so.6 strlen c pure' \
    'allow libc.so.6 abs c pure' \
    'allow libc.so.6 labs c pure' \
    'allow libc.so.6 puts c io' > "$policy"
abs_source="$root/Flowmini/flowmini_v25_symboltable_projection/examples/pass/abi_abs_main.flow"
"$flowmini" --dump-frontend-bundle "$abs_source" > "$tmpdir/abs.bundle.json"
"$analyst" < "$tmpdir/abs.bundle.json" > "$tmpdir/abs.semantic.json"
"$bind" --policy "$policy" < "$tmpdir/abs.semantic.json" > "$tmpdir/abs.binding.json"
"$optimizer" < "$tmpdir/abs.semantic.json" > "$tmpdir/abs.optimized.json"
"$lowerer" --emit-llvm "$tmpdir/abs.ll" --binding-report "$tmpdir/abs.binding.json" < "$tmpdir/abs.optimized.json" > "$tmpdir/abs.lowering.json"
grep -q '"status": "emitted"' "$tmpdir/abs.lowering.json"
clang "$tmpdir/abs.ll" -o "$tmpdir/abs"
set +e
"$tmpdir/abs"
abs_rc=$?
set -e
test "$abs_rc" -eq 42

strlen_source="$root/Flowmini/flowmini_v25_symboltable_projection/examples/pass/abi_strlen_main.flow"
"$flowmini" --dump-frontend-bundle "$strlen_source" > "$tmpdir/strlen.bundle.json"
"$analyst" < "$tmpdir/strlen.bundle.json" > "$tmpdir/strlen.semantic.json"
"$bind" --policy "$policy" < "$tmpdir/strlen.semantic.json" > "$tmpdir/strlen.binding.json"
"$optimizer" < "$tmpdir/strlen.semantic.json" > "$tmpdir/strlen.optimized.json"
"$lowerer" --emit-llvm "$tmpdir/strlen.ll" --binding-report "$tmpdir/strlen.binding.json" < "$tmpdir/strlen.optimized.json" > "$tmpdir/strlen.lowering.json"
grep -q '"status": "emitted"' "$tmpdir/strlen.lowering.json"
clang "$tmpdir/strlen.ll" -o "$tmpdir/strlen"
set +e
"$tmpdir/strlen"
strlen_rc=$?
set -e
test "$strlen_rc" -eq 8

flowcat_source="$root/Flowmini/flowmini_v25_symboltable_projection/examples/apps/flowcat.flow"
"$flowmini" --dump-frontend-bundle "$flowcat_source" > "$tmpdir/flowcat.bundle.json"
"$analyst" < "$tmpdir/flowcat.bundle.json" > "$tmpdir/flowcat.semantic.json"
grep -q '"lowering_profile": "flowcat_argv_main"' "$tmpdir/flowcat.semantic.json"
"$bind" --policy "$policy" < "$tmpdir/flowcat.semantic.json" > "$tmpdir/flowcat.binding.json"
"$optimizer" < "$tmpdir/flowcat.semantic.json" > "$tmpdir/flowcat.optimized.json"
"$lowerer" --emit-llvm "$tmpdir/flowcat.ll" --binding-report "$tmpdir/flowcat.binding.json" < "$tmpdir/flowcat.optimized.json" > "$tmpdir/flowcat.lowering.json"
grep -q '"status": "emitted"' "$tmpdir/flowcat.lowering.json"
clang "$tmpdir/flowcat.ll" -o "$tmpdir/flowcat"
"$tmpdir/flowcat" alpha beta > "$tmpdir/flowcat.output"
printf '%s\n' alpha beta | cmp -s - "$tmpdir/flowcat.output"
echo 'Flowlower tests: PASS'
