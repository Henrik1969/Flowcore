#!/bin/sh
set -eu

root=${FLOWCORE_ROOT:?}
flowmini=${FLOWMINI_BIN:?}
analyst=${FLOWANALYST_BIN:?}
parallel=${FLOWPARALLEL_BIN:?}
optimize=${FLOWOPTIMIZE_BIN:?}
prepare=${FLOWPREPARE_BIN:?}
validate=${FLOWVALIDATE_BIN:?}
lower=${FLOWLOWER_BIN:?}
source="$root/Flowmini/flowmini_v25_symboltable_projection/examples/pass/fn_demo.flow"
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT

"$flowmini" --dump-frontend-bundle "$source" > "$tmpdir/frontend.json"
"$analyst" --lowering-plan-version 2 < "$tmpdir/frontend.json" > "$tmpdir/semantic.json"
"$parallel" < "$tmpdir/semantic.json" > "$tmpdir/execution.json"
"$optimize" < "$tmpdir/execution.json" > "$tmpdir/optimization.json"
"$prepare" "$tmpdir/optimization.json" > "$tmpdir/lowering.json"
"$validate" "$tmpdir/lowering.json" | grep -q '"classification":"valid"'

jq -e '.lowering_plan.version == 2 and
       ([.lowering_plan.functions[] | select(.entry)] | length) == 1 and
       ([.lowering_plan.functions[] | select(.name == "square") | .parameters[0].symbol_id] == [3]) and
       ([.lowering_plan.operations[] | select(.kind == "call") | .callee_symbol_id] | sort == [2,4]) and
       ([.lowering_plan.operations[] | select(.kind == "return_value") | .operands[0] | select(.kind == "call_result") | .callee_symbol_id] == [2])' \
    "$tmpdir/lowering.json" >/dev/null

for mutation in \
    '.lowering_plan.functions[0].symbol_id = .lowering_plan.functions[1].symbol_id' \
    '.lowering_plan.operations[0].function_symbol_id = 999' \
    '.lowering_plan.operations[0].callee_symbol_id = 999'
do
    jq "$mutation" "$tmpdir/lowering.json" > "$tmpdir/mutated.json"
    if "$validate" "$tmpdir/mutated.json" >/dev/null 2>&1; then
        echo "flowvalidate accepted callable lowering mutation: $mutation" >&2; exit 1
    fi
done

classifier="$root/Flowmini/flowmini_v25_symboltable_projection/examples/bootstrap/shared_scalar_classifier.flow"
"$flowmini" --dump-frontend-bundle "$classifier" > "$tmpdir/classifier.frontend.json"
"$analyst" --lowering-plan-version 2 < "$tmpdir/classifier.frontend.json" > "$tmpdir/classifier.semantic.json"
"$parallel" < "$tmpdir/classifier.semantic.json" > "$tmpdir/classifier.execution.json"
"$optimize" < "$tmpdir/classifier.execution.json" > "$tmpdir/classifier.optimization.json"
"$prepare" --target-policy "$root/Flowlower/target-policies/llvm-host.json" "$tmpdir/classifier.optimization.json" > "$tmpdir/classifier.lowering.json"
"$lower" --emit-llvm "$tmpdir/classifier.ll" "$tmpdir/classifier.lowering.json" > "$tmpdir/classifier.report.json"
clang "$tmpdir/classifier.ll" -o "$tmpdir/classifier"
set +e
"$tmpdir/classifier"
classifier_status=$?
set -e
test "$classifier_status" -eq 1
jq -e '.status == "ready" and .backend.name == "llvm"' "$tmpdir/classifier.report.json" >/dev/null

echo 'callable lowering boundary: PASS'
