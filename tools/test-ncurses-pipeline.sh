#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
flowmini=${FLOWMINI_BIN:?FLOWMINI_BIN is required}
analyst=${FLOWANALYST_BIN:?FLOWANALYST_BIN is required}
parallel=${FLOWPARALLEL_BIN:?FLOWPARALLEL_BIN is required}
optimizer=${FLOWOPTIMIZE_BIN:?FLOWOPTIMIZE_BIN is required}
bind=${FLOWBIND_BIN:?FLOWBIND_BIN is required}
lower=${FLOWLOWER_BIN:?FLOWLOWER_BIN is required}
fixture="$root/Flowmini/flowmini_v25_symboltable_projection/examples/pass/abi_ncurses_main.flow"
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT

test -x "$flowmini" -a -x "$analyst" -a -x "$parallel" -a -x "$optimizer" -a -x "$bind" -a -x "$lower"

printf '%s\n' \
    'allow libncursesw.so.6 initscr c terminal' \
    'allow libncursesw.so.6 endwin c terminal' \
    'allow libncursesw.so.6 noecho c terminal' \
    'allow libncursesw.so.6 cbreak c terminal' \
    'allow libncursesw.so.6 waddnstr c terminal' \
    'allow libncursesw.so.6 wrefresh c terminal' > "$tmpdir/policy"

"$flowmini" --dump-frontend-bundle "$fixture" > "$tmpdir/frontend.json"
"$analyst" < "$tmpdir/frontend.json" > "$tmpdir/semantic.json"
grep -q '"lowering_profile": "none"' "$tmpdir/semantic.json"
jq -e '.abi_type_contracts | any(.contract == "ncurses" and .name == "c_pointer" and .ownership == "external" and .access == "opaque" and .lifetime == "external" and .nullable == "true" and .opaque == "true" and .cleanup == "endwin")' "$tmpdir/semantic.json" >/dev/null
jq -e '.lowering_plan.operations | any(.kind == "external_call" and .provider.contract == "ncurses" and .provider.symbol == "initscr" and .result_resource.ownership == "external" and .result_resource.nullable == "true" and .result_resource.cleanup_capability == "endwin")' "$tmpdir/semantic.json" >/dev/null
jq -e '.lowering_plan.format == "flowcore.lowering_plan" and .lowering_plan.version == 1 and (.lowering_plan.operations | any(.kind == "external_call" and .provider.symbol == "initscr"))' "$tmpdir/semantic.json" >/dev/null
"$parallel" < "$tmpdir/semantic.json" > "$tmpdir/parallel.json"
"$optimizer" < "$tmpdir/parallel.json" > "$tmpdir/optimized.json"
"$bind" --policy "$tmpdir/policy" < "$tmpdir/semantic.json" > "$tmpdir/binding.json"
jq -e '.status == "ready" and .lowering_profile == "none" and (.symbols | index("initscr")) != null' "$tmpdir/binding.json" >/dev/null
jq -e '.lowering_plan.contract == "flowcore.lowering_plan" and .lowering_plan.operation_count > 0' "$tmpdir/binding.json" >/dev/null

jq '(.lowering_plan.operations[] | select(.provider.symbol == "initscr") | .result_resource.cleanup_capability) = "invented_cleanup"' \
    "$tmpdir/semantic.json" > "$tmpdir/hostile-resource.json"
if "$bind" --policy "$tmpdir/policy" < "$tmpdir/hostile-resource.json" >/dev/null 2>&1; then
    echo 'hostile ncurses cleanup identity unexpectedly authorized' >&2
    exit 1
fi
jq '.lowering_plan.operations |= map(select(.provider.symbol != "endwin"))' \
    "$tmpdir/semantic.json" > "$tmpdir/missing-cleanup.json"
if "$bind" --policy "$tmpdir/policy" < "$tmpdir/missing-cleanup.json" >/dev/null 2>&1; then
    echo 'ncurses resource without cleanup unexpectedly authorized' >&2
    exit 1
fi
"$lower" --emit-llvm "$tmpdir/program.ll" --binding-report "$tmpdir/binding.json" < "$tmpdir/optimized.json" > "$tmpdir/lowering.json"
grep -q '"status": "emitted"' "$tmpdir/lowering.json"
grep -q 'call ptr @initscr' "$tmpdir/program.ll"
grep -q 'Flowcore generic lowering plan: ordered mixed-carrier capability sequence' "$tmpdir/program.ll"
clang "$tmpdir/program.ll" -lncursesw -o "$tmpdir/program"

TERM=xterm script -qec "$tmpdir/program" "$tmpdir/terminal.log" >/dev/null
grep -q 'Flowcore ncurses binding' "$tmpdir/terminal.log"

printf '%s\n' 'Ncurses Flow pipeline: PASS'
