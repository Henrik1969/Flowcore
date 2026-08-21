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
grep -q '"lowering_profile": "abi_ncurses_main"' "$tmpdir/semantic.json"
"$parallel" < "$tmpdir/semantic.json" > "$tmpdir/parallel.json"
"$optimizer" < "$tmpdir/parallel.json" > "$tmpdir/optimized.json"
"$bind" --policy "$tmpdir/policy" < "$tmpdir/semantic.json" > "$tmpdir/binding.json"
jq -e '.status == "ready" and .lowering_profile == "abi_ncurses_main" and .lowering_plan.kind == "external_call" and (.symbols | index("initscr")) != null' "$tmpdir/binding.json" >/dev/null
"$lower" --emit-llvm "$tmpdir/program.ll" --binding-report "$tmpdir/binding.json" < "$tmpdir/optimized.json" > "$tmpdir/lowering.json"
grep -q '"status": "emitted"' "$tmpdir/lowering.json"
grep -q 'call ptr @initscr' "$tmpdir/program.ll"
clang "$tmpdir/program.ll" -lncursesw -o "$tmpdir/program"

TERM=xterm script -qec "$tmpdir/program" "$tmpdir/terminal.log" >/dev/null
grep -q 'Flowcore ncurses binding' "$tmpdir/terminal.log"

printf '%s\n' 'Ncurses Flow pipeline: PASS'
