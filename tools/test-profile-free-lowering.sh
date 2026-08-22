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
printf '%s\n' 'Profile-free generic lowering: PASS'
