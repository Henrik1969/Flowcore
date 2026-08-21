#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
flowmini=${FLOWMINI_BIN:?FLOWMINI_BIN is required}
analyst=${FLOWANALYST_BIN:?FLOWANALYST_BIN is required}
parallel=${FLOWPARALLEL_BIN:?FLOWPARALLEL_BIN is required}
optimizer=${FLOWOPTIMIZE_BIN:?FLOWOPTIMIZE_BIN is required}
bind=${FLOWBIND_BIN:?FLOWBIND_BIN is required}
lower=${FLOWLOWER_BIN:?FLOWLOWER_BIN is required}
fixture="$root/Flowmini/flowmini_v25_symboltable_projection/examples/apps/sel/sel.flow"
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT

printf '%s\n' \
    'allow libncursesw.so.6 initscr c terminal' \
    'allow libncursesw.so.6 endwin c terminal' \
    'allow libncursesw.so.6 noecho c terminal' \
    'allow libncursesw.so.6 cbreak c terminal' \
    'allow libncursesw.so.6 keypad c terminal' \
    'allow libncursesw.so.6 waddnstr c terminal' \
    'allow libncursesw.so.6 wrefresh c terminal' \
    'allow libncursesw.so.6 wgetch c terminal' \
    'allow libc.so.6 puts c io' > "$tmpdir/policy"
printf '%s\n' 'allow libc.so.6 read c filesystem' >> "$tmpdir/policy"

"$flowmini" --dump-frontend-bundle "$fixture" > "$tmpdir/frontend.json"
jq -e '[.ast.declaration_pool[] | select(.kind == "import") | .alias] == ["curses", "libc", "linux"]' "$tmpdir/frontend.json" >/dev/null
"$analyst" < "$tmpdir/frontend.json" > "$tmpdir/semantic.json"
grep -q '"lowering_profile": "sel_main"' "$tmpdir/semantic.json"
 jq -e '(.external_operations | map(.callee) | index("curses.initscr")) != null and (.external_operations | map(.callee) | index("libc.puts")) != null and (.external_operations | map(.callee) | index("linux.read")) != null' "$tmpdir/semantic.json" >/dev/null
"$parallel" < "$tmpdir/semantic.json" > "$tmpdir/parallel.json"
"$optimizer" < "$tmpdir/parallel.json" > "$tmpdir/optimized.json"
"$bind" --policy "$tmpdir/policy" < "$tmpdir/semantic.json" > "$tmpdir/binding.json"
jq -e '.status == "ready" and .lowering_profile == "sel_main" and (.symbols | index("wgetch")) != null and (.symbols | index("puts")) != null' "$tmpdir/binding.json" >/dev/null
"$lower" --emit-llvm "$tmpdir/program.ll" --binding-report "$tmpdir/binding.json" < "$tmpdir/optimized.json" > "$tmpdir/lowering.json"
grep -q '"status": "emitted"' "$tmpdir/lowering.json"
clang "$tmpdir/program.ll" -lncursesw -o "$tmpdir/program"
grep -q 'icmp eq i32 %key, 113' "$tmpdir/program.ll"
grep -q 'getelementptr ptr, ptr %argv' "$tmpdir/program.ll"

TERM=xterm script -qec "printf 'stdin-alpha\\nstdin-beta\\n' | '$tmpdir/program'" "$tmpdir/terminal.log" >/dev/null
grep -q 'alpha' "$tmpdir/terminal.log"
grep -q 'stdin-alpha' "$tmpdir/terminal.log"

printf '%s\n' 'Flowcore sel TUI: PASS'
