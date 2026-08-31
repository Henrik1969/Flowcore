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
LD_LIBRARY_PATH="$FLOWTERMINAL_LIBDIR${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export LD_LIBRARY_PATH
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT

for symbol in flow_terminal_open flow_terminal_close flow_terminal_enter flow_terminal_write flow_terminal_present flow_terminal_read_event; do
    printf 'allow libflowterminal.so.1 %s c terminal\n' "$symbol"
done > "$tmpdir/policy"
printf '%s\n' 'allow libc.so.6 puts c io' >> "$tmpdir/policy"

"$flowmini" --dump-frontend-bundle "$fixture" > "$tmpdir/frontend.json"
"$analyst" < "$tmpdir/frontend.json" > "$tmpdir/semantic.json"
jq -e '.lowering_plan.operations | any(.provider.contract == "terminal" and .provider.symbol == "flow_terminal_open" and .result_resource.cleanup_capability == "flow_terminal_close")' "$tmpdir/semantic.json" >/dev/null
jq -e '[.lowering_plan.operations[] | select(.kind == "external_call") | select(.provider.library == "libncursesw.so.6" or (.provider.symbol != "puts" and (.provider.symbol | startswith("flow_terminal_") | not)))] | length == 0' "$tmpdir/semantic.json" >/dev/null
"$parallel" < "$tmpdir/semantic.json" > "$tmpdir/parallel.json"
"$optimizer" < "$tmpdir/parallel.json" > "$tmpdir/optimized.json"
"$bind" --policy "$tmpdir/policy" < "$tmpdir/semantic.json" > "$tmpdir/binding.json"
"$lower" --emit-llvm "$tmpdir/sel.ll" --binding-report "$tmpdir/binding.json" < "$tmpdir/optimized.json" >/dev/null
clang "$tmpdir/sel.ll" -L"$FLOWTERMINAL_LIBDIR" -Wl,-rpath,"$FLOWTERMINAL_LIBDIR" -lflowterminal -o "$tmpdir/sel"

printf 'q' | TERM=invented-terminal "$tmpdir/sel" > "$tmpdir/output" || status=$?
test "${status:-0}" -eq 1
grep -q 'selection: none' "$tmpdir/output"
printf '\033[B\n' | TERM=xterm-kitty "$tmpdir/sel" > "$tmpdir/down-output"
grep -q 'cursor: beta' "$tmpdir/down-output"
grep -q 'selected: beta' "$tmpdir/down-output"
printf '\033[B\033[B\033[A\n' | TERM=alacritty "$tmpdir/sel" > "$tmpdir/multi-output"
grep -q 'selected: beta' "$tmpdir/multi-output"
! grep -q 'ncurses\|terminfo\|xterm-kitty' "$tmpdir/sel.ll"
echo 'FLOWTERMINAL_SEL_PIPELINE_PASS provider=flowterminal navigation=up,down,enter projection=unknown'
