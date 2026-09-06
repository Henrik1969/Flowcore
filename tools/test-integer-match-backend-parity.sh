#!/bin/sh
set -eu

root=${FLOWCORE_ROOT:?}
flowmini=${FLOWMINI_BIN:?}
analyst=${FLOWANALYST_BIN:?}
parallel=${FLOWPARALLEL_BIN:?}
optimize=${FLOWOPTIMIZE_BIN:?}
prepare=${FLOWPREPARE_BIN:?}
llvm=${FLOWLOWER_BIN:?}
tiny=${FLOWTINYLOWER_BIN:?}
tiny_run=${FLOWTINYRUN_BIN:?}
source=${MATCH_SOURCE:-$root/Flowmini/flowmini_v25_symboltable_projection/examples/bootstrap/when_backend_parity_probe.flow}
expected=${MATCH_EXPECTED_RESULT:-37}
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT

"$flowmini" --dump-frontend-bundle "$source" > "$tmpdir/bundle.json"
"$analyst" --lowering-plan-version 1 "$tmpdir/bundle.json" > "$tmpdir/semantic.json"
"$parallel" "$tmpdir/semantic.json" > "$tmpdir/parallel.json"
"$optimize" "$tmpdir/parallel.json" > "$tmpdir/optimization.json"
"$prepare" "$tmpdir/optimization.json" > "$tmpdir/backend.json"

jq -e '([.match_operations[]?] | length == 1) and ([.lowering_plan.operations[] | select(.kind == "match")] | length == 1)' "$tmpdir/backend.json" >/dev/null

"$llvm" --emit-llvm "$tmpdir/program.ll" "$tmpdir/backend.json" > "$tmpdir/llvm-report.json"
clang "$tmpdir/program.ll" -o "$tmpdir/program.llvm"
set +e
"$tmpdir/program.llvm"
llvm_status=$?
set -e
test "$llvm_status" -eq "$expected"

"$tiny" "$tmpdir/backend.json" "$tmpdir/program.tvm" > "$tmpdir/tiny-report.json"
tiny_result=$("$tiny_run" "$tmpdir/program.tvm" | jq -r '.result')
test "$tiny_result" -eq "$expected"

echo 'integer match backend parity: PASS'
