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
jq -e '.lowering_plan.operations | any(.provider.contract == "curses" and .provider.symbol == "initscr" and .result_resource.cleanup_capability == "endwin")' "$tmpdir/semantic.json" >/dev/null
"$parallel" < "$tmpdir/semantic.json" > "$tmpdir/parallel.json"
"$optimizer" < "$tmpdir/parallel.json" > "$tmpdir/optimized.json"
"$bind" --policy "$tmpdir/policy" < "$tmpdir/semantic.json" > "$tmpdir/binding.json"
jq -e '.status == "ready" and .lowering_profile == "sel_main" and (.symbols | index("wgetch")) != null and (.symbols | index("puts")) != null' "$tmpdir/binding.json" >/dev/null
"$lower" --emit-llvm "$tmpdir/program.ll" --binding-report "$tmpdir/binding.json" < "$tmpdir/optimized.json" > "$tmpdir/lowering.json"
grep -q '"status": "emitted"' "$tmpdir/lowering.json"
clang "$tmpdir/program.ll" -lncursesw -o "$tmpdir/program"
grep -q 'icmp eq i32 %key, 113' "$tmpdir/program.ll"
grep -q 'getelementptr ptr, ptr %argv' "$tmpdir/program.ll"
grep -q 'call i64 @read(i32 0, ptr %input_ptr, i64 4095)' "$tmpdir/program.ll"
grep -q 'icmp sgt i64 %input_count, 0' "$tmpdir/program.ll"
grep -q 'icmp eq i64 %input_count, 0' "$tmpdir/program.ll"
grep -q 'input_error:' "$tmpdir/program.ll"
if grep -q 'getelementptr i8, ptr %input_ptr, i64 %input_count' "$tmpdir/program.ll"; then
    terminator_line=$(grep -n 'getelementptr i8, ptr %input_ptr, i64 %input_count' "$tmpdir/program.ll" | cut -d: -f1)
    ready_line=$(grep -n '^input_ready:' "$tmpdir/program.ll" | cut -d: -f1)
    test "$terminator_line" -gt "$ready_line"
fi

TERM=xterm script -qec "printf 'stdin-alpha\\nstdin-beta\\n' | '$tmpdir/program'" "$tmpdir/terminal.log" >/dev/null
grep -q 'alpha' "$tmpdir/terminal.log"
grep -q 'stdin-alpha' "$tmpdir/terminal.log"

set +e
TERM=xterm script -qec "'$tmpdir/program' 0<&-" "$tmpdir/read-error.log" >/dev/null
read_error_status=$?
set -e
test "$read_error_status" -eq 2

TERM=xterm script -qec "'$tmpdir/program' </dev/null" "$tmpdir/eof.log" >/dev/null

printf '%s\n' 'Flowcore sel TUI: PASS'
