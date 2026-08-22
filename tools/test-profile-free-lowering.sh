#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
flowmini=${FLOWMINI_BIN:?FLOWMINI_BIN is required}
analyst=${FLOWANALYST_BIN:?FLOWANALYST_BIN is required}
parallel=${FLOWPARALLEL_BIN:?FLOWPARALLEL_BIN is required}
optimizer=${FLOWOPTIMIZE_BIN:?FLOWOPTIMIZE_BIN is required}
bind=${FLOWBIND_BIN:?FLOWBIND_BIN is required}
lower=${FLOWLOWER_BIN:?FLOWLOWER_BIN is required}
source="$root/Flowmini/flowmini_v25_symboltable_projection/examples/pass/profile_free_getpid.flow"
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT

printf '%s\n' 'allow libc.so.6 getpid c readonly - c_int' > "$tmpdir/policy"
"$flowmini" --dump-frontend-bundle "$source" > "$tmpdir/frontend.json"
"$analyst" < "$tmpdir/frontend.json" > "$tmpdir/semantic.json"
jq -e '.status == "ok" and .lowering_profile == "none" and .lowering_plan.operations[0].kind == "external_call" and .lowering_plan.operations[0].provider.symbol == "getpid"' "$tmpdir/semantic.json" >/dev/null
"$parallel" < "$tmpdir/semantic.json" > "$tmpdir/parallel.json"
jq -e '.lowering_plan.operations[0].provider.symbol == "getpid"' "$tmpdir/parallel.json" >/dev/null
"$optimizer" < "$tmpdir/parallel.json" > "$tmpdir/optimized.json"
jq -e '.lowering_plan.operations[0].provider.symbol == "getpid"' "$tmpdir/optimized.json" >/dev/null
"$bind" --policy "$tmpdir/policy" < "$tmpdir/semantic.json" > "$tmpdir/binding.json"
"$lower" --emit-llvm "$tmpdir/program.ll" --binding-report "$tmpdir/binding.json" < "$tmpdir/optimized.json" > "$tmpdir/lowering.json"
grep -Fq 'declare i32 @getpid()' "$tmpdir/program.ll"
grep -Fq 'call i32 @getpid()' "$tmpdir/program.ll"
clang "$tmpdir/program.ll" -o "$tmpdir/program"
"$tmpdir/program"

jq -n --arg provider "$(ldconfig -p | awk '$1 == "libc.so.6" && $NF ~ /^\// { print $NF; exit }')" '
  {format:"flowcore.native_binding_spec",version:1,unit:"profile_free_getpgid",namespace:"linux",
   provider:{soname:"libc.so.6",path:$provider,convention:"c"},
   functions:[{name:"getpgid",symbol:"getpgid",effect:"readonly",parameters:[{name:"pid",type:"c_int"}],return_type:"c_int"}]}
' > "$tmpdir/spec.json"
"$root/tools/generate-flow-bindings.sh" --spec "$tmpdir/spec.json" \
    --flow-output "$tmpdir/generated.flow" --policy-output "$tmpdir/policy-arg" \
    --manifest-output "$tmpdir/manifest.json" >/dev/null
printf '%s\n' 'import "generated.flow" as linux' '' 'program arbitrary_profile_free_name' '' 'main {' \
    '    group : c_int(0)' '    linux.getpgid(0) -> group' '}' > "$tmpdir/argument.flow"
"$flowmini" --dump-frontend-bundle "$tmpdir/argument.flow" | "$analyst" > "$tmpdir/argument.semantic.json"
jq -e '.status == "ok" and .lowering_profile == "none" and .lowering_plan.operations[0].operands[0].kind == "integer_literal" and .lowering_plan.operations[0].operands[0].type == "c_int"' "$tmpdir/argument.semantic.json" >/dev/null
"$parallel" < "$tmpdir/argument.semantic.json" | "$optimizer" > "$tmpdir/argument.optimized.json"
"$bind" --policy "$tmpdir/policy-arg" < "$tmpdir/argument.semantic.json" > "$tmpdir/argument.binding.json"
"$lower" --emit-llvm "$tmpdir/argument.ll" --binding-report "$tmpdir/argument.binding.json" < "$tmpdir/argument.optimized.json" > "$tmpdir/argument.lowering.json"
grep -Fq 'declare i32 @getpgid(i32)' "$tmpdir/argument.ll"
grep -Fq 'call i32 @getpgid(i32 0)' "$tmpdir/argument.ll"
clang "$tmpdir/argument.ll" -o "$tmpdir/argument"
"$tmpdir/argument"

return_source="$root/Flowmini/flowmini_v25_symboltable_projection/examples/pass/profile_free_return.flow"
"$flowmini" --dump-frontend-bundle "$return_source" > "$tmpdir/return.frontend.json"
"$analyst" < "$tmpdir/return.frontend.json" > "$tmpdir/return.semantic.json"
jq -e '.status == "ok" and .lowering_profile == "none" and .lowering_plan.operations[0].kind == "return_value" and .lowering_plan.operations[0].operands[0].value == "42"' "$tmpdir/return.semantic.json" >/dev/null
"$parallel" < "$tmpdir/return.semantic.json" | "$optimizer" > "$tmpdir/return.optimized.json"
"$lower" --emit-llvm "$tmpdir/return.ll" < "$tmpdir/return.optimized.json" > "$tmpdir/return.lowering.json"
grep -Fq 'ret i32 42' "$tmpdir/return.ll"
clang "$tmpdir/return.ll" -o "$tmpdir/return"
set +e
"$tmpdir/return"
return_rc=$?
set -e
test "$return_rc" -eq 42

expression_source="$root/Flowmini/flowmini_v25_symboltable_projection/examples/pass/profile_free_expression.flow"
"$flowmini" --dump-frontend-bundle "$expression_source" | "$analyst" > "$tmpdir/expression.semantic.json"
jq -e '.status == "ok" and .lowering_profile == "none" and .lowering_plan.operations[0].operands[0].kind == "binary" and .lowering_plan.operations[0].operands[0].operator == "+"' "$tmpdir/expression.semantic.json" >/dev/null
"$parallel" < "$tmpdir/expression.semantic.json" | "$optimizer" > "$tmpdir/expression.optimized.json"
"$lower" --emit-llvm "$tmpdir/expression.ll" < "$tmpdir/expression.optimized.json" > "$tmpdir/expression.lowering.json"
grep -Fq 'add i32 40, 2' "$tmpdir/expression.ll"
clang "$tmpdir/expression.ll" -o "$tmpdir/expression"
set +e
"$tmpdir/expression"
expression_rc=$?
set -e
test "$expression_rc" -eq 42

local_source="$root/Flowmini/flowmini_v25_symboltable_projection/examples/pass/profile_free_local_value.flow"
"$flowmini" --dump-frontend-bundle "$local_source" | "$analyst" > "$tmpdir/local.semantic.json"
jq -e '.status == "ok" and .lowering_profile == "none" and any(.lowering_plan.operations[]; .kind == "value_definition") and any(.lowering_plan.operations[]; .kind == "return_value" and .operands[0].kind == "binary")' "$tmpdir/local.semantic.json" >/dev/null
"$parallel" < "$tmpdir/local.semantic.json" | "$optimizer" > "$tmpdir/local.optimized.json"
"$lower" --emit-llvm "$tmpdir/local.ll" < "$tmpdir/local.optimized.json" > "$tmpdir/local.lowering.json"
grep -Fq 'ret i32' "$tmpdir/local.ll"
clang "$tmpdir/local.ll" -o "$tmpdir/local"
set +e
"$tmpdir/local"
local_rc=$?
set -e
test "$local_rc" -eq 42

branch_source="$root/Flowmini/flowmini_v25_symboltable_projection/examples/pass/profile_free_branch.flow"
"$flowmini" --dump-frontend-bundle "$branch_source" | "$analyst" > "$tmpdir/branch.semantic.json"
jq -e '.status == "ok" and .lowering_profile == "none" and any(.lowering_plan.operations[]; .kind == "branch" and .operands[0].kind == "bool_literal")' "$tmpdir/branch.semantic.json" >/dev/null
"$parallel" < "$tmpdir/branch.semantic.json" | "$optimizer" > "$tmpdir/branch.optimized.json"
"$lower" --emit-llvm "$tmpdir/branch.ll" < "$tmpdir/branch.optimized.json" > "$tmpdir/branch.lowering.json"
grep -Fq 'br i1 true' "$tmpdir/branch.ll"
clang "$tmpdir/branch.ll" -o "$tmpdir/branch"
set +e
"$tmpdir/branch"
branch_rc=$?
set -e
test "$branch_rc" -eq 42

compare_source="$root/Flowmini/flowmini_v25_symboltable_projection/examples/pass/profile_free_branch_compare.flow"
"$flowmini" --dump-frontend-bundle "$compare_source" | "$analyst" > "$tmpdir/compare.semantic.json"
jq -e '.status == "ok" and .lowering_profile == "none" and any(.lowering_plan.operations[]; .kind == "branch" and .operands[0].operator == ">")' "$tmpdir/compare.semantic.json" >/dev/null
"$parallel" < "$tmpdir/compare.semantic.json" | "$optimizer" > "$tmpdir/compare.optimized.json"
"$lower" --emit-llvm "$tmpdir/compare.ll" < "$tmpdir/compare.optimized.json" > "$tmpdir/compare.lowering.json"
grep -Fq 'icmp sgt i32 %flow_value_' "$tmpdir/compare.ll"
clang "$tmpdir/compare.ll" -o "$tmpdir/compare"
set +e
"$tmpdir/compare"
compare_rc=$?
set -e
test "$compare_rc" -eq 42

result_source="$root/Flowmini/flowmini_v25_symboltable_projection/examples/pass/profile_free_external_result.flow"
printf '%s\n' 'allow libc.so.6 getppid c readonly - c_int' > "$tmpdir/result.policy"
"$flowmini" --dump-frontend-bundle "$result_source" | "$analyst" > "$tmpdir/result.semantic.json"
jq -e '.status == "ok" and .lowering_profile == "none" and any(.lowering_plan.operations[]; .kind == "external_call" and .provider.symbol == "getppid" and has("result_symbol_id")) and any(.lowering_plan.operations[]; .kind == "return_value" and .operands[0].kind == "binary" and .operands[0].left.kind == "identifier")' "$tmpdir/result.semantic.json" >/dev/null
"$parallel" < "$tmpdir/result.semantic.json" | "$optimizer" > "$tmpdir/result.optimized.json"
"$bind" --policy "$tmpdir/result.policy" < "$tmpdir/result.semantic.json" > "$tmpdir/result.binding.json"
set +e
"$lower" --emit-llvm "$tmpdir/result-unauthorized.ll" < "$tmpdir/result.optimized.json" > "$tmpdir/result-unauthorized.json" 2>/dev/null
unauthorized_rc=$?
set -e
test "$unauthorized_rc" -ne 0
jq '.capabilities[0].symbol = "getpid"' "$tmpdir/result.binding.json" > "$tmpdir/result-wrong-binding.json"
set +e
"$lower" --emit-llvm "$tmpdir/result-wrong.ll" --binding-report "$tmpdir/result-wrong-binding.json" < "$tmpdir/result.optimized.json" > "$tmpdir/result-wrong.json" 2>/dev/null
wrong_binding_rc=$?
set -e
test "$wrong_binding_rc" -ne 0
"$lower" --emit-llvm "$tmpdir/result.ll" --binding-report "$tmpdir/result.binding.json" < "$tmpdir/result.optimized.json" > "$tmpdir/result.lowering.json"
grep -Fq 'declare i32 @getppid()' "$tmpdir/result.ll"
grep -Fq '%flow_call_' "$tmpdir/result.ll"
grep -Fq 'add i32 %flow_call_' "$tmpdir/result.ll"
grep -Fq 'ret i32 %flow_expr' "$tmpdir/result.ll"
clang "$tmpdir/result.ll" -o "$tmpdir/result"
set +e
"$tmpdir/result"
set -e
printf '%s\n' 'Profile-free generic lowering: PASS'
