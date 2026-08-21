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
printf '%s\n' 'Profile-free generic lowering: PASS'
