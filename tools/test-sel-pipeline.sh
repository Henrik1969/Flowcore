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
printf '%s\n' 'allow libc.so.6 write c filesystem' >> "$tmpdir/policy"
printf '%s\n' 'allow libc.so.6 memset c io' >> "$tmpdir/policy"

"$flowmini" --dump-frontend-bundle "$fixture" > "$tmpdir/frontend.json"
jq -e '[.ast.declaration_pool[] | select(.kind == "import") | .alias] == ["curses", "libc", "linux", "memory"]' "$tmpdir/frontend.json" >/dev/null
"$analyst" < "$tmpdir/frontend.json" > "$tmpdir/semantic.json"
grep -q '"lowering_profile": "none"' "$tmpdir/semantic.json"
 jq -e '(.external_operations | map(.callee) | index("curses.initscr")) != null and (.external_operations | map(.callee) | index("libc.puts")) != null and (.external_operations | map(.callee) | index("linux.read")) != null' "$tmpdir/semantic.json" >/dev/null
jq -e '.lowering_plan.operations | any(.provider.contract == "curses" and .provider.symbol == "initscr" and .result_resource.cleanup_capability == "endwin")' "$tmpdir/semantic.json" >/dev/null
jq -e '
    ([.lowering_plan.operations[].operands[]? | select(.intrinsic == "list_length")] | length) >= 1 and
    ([.lowering_plan.operations[].operands[]? | select(.intrinsic == "list_index")] | length) >= 1 and
    ([.lowering_plan.operations[] | select(.kind == "branch")] | length) == 4 and
    any(.lowering_plan.operations[]; .kind == "branch" and .operands[0].operator == "<") and
    any(.lowering_plan.operations[]; .kind == "branch" and .operands[0].operator == ">") and
    any(.lowering_plan.operations[]; .kind == "return_value" and .operands[0].value == "2")
' "$tmpdir/semantic.json" >/dev/null
"$parallel" < "$tmpdir/semantic.json" > "$tmpdir/parallel.json"
"$optimizer" < "$tmpdir/parallel.json" > "$tmpdir/optimized.json"
"$bind" --policy "$tmpdir/policy" < "$tmpdir/semantic.json" > "$tmpdir/binding.json"
jq -e '.status == "ready" and .lowering_profile == "none" and (.symbols | index("wgetch")) != null and (.symbols | index("puts")) != null' "$tmpdir/binding.json" >/dev/null

# The backend recognizes the structured capability plan, not this fixture's
# source-unit identity. An arbitrary program name must retain the same path.
sed \
    -e "s|\"../../../std/|\"$root/Flowmini/flowmini_v25_symboltable_projection/std/|" \
    -e 's/^program sel$/program terminal_choice_probe/' \
    "$fixture" > "$tmpdir/renamed.flow"
"$flowmini" --dump-frontend-bundle "$tmpdir/renamed.flow" |
    "$analyst" > "$tmpdir/renamed.semantic.json"
jq -e '.lowering_profile == "none" and (.lowering_plan.operations | any(.provider.symbol == "wgetch"))' "$tmpdir/renamed.semantic.json" >/dev/null
"$parallel" < "$tmpdir/renamed.semantic.json" |
    "$optimizer" > "$tmpdir/renamed.optimized.json"
"$bind" --policy "$tmpdir/policy" < "$tmpdir/renamed.semantic.json" > "$tmpdir/renamed.binding.json"
"$lower" --emit-llvm "$tmpdir/renamed.ll" --binding-report "$tmpdir/renamed.binding.json" < "$tmpdir/renamed.optimized.json" >/dev/null
grep -q 'generic structured lowering plan' "$tmpdir/renamed.ll"

# The same capability set with a different source literal must produce different
# behavior without selecting another backend path.
sed \
    -e "s|\"../../../std/|\"$root/Flowmini/flowmini_v25_symboltable_projection/std/|" \
    -e 's/^program sel$/program terminal_choice_variant/' \
    -e 's/selected: alpha/selected: source-variant/' \
    "$fixture" > "$tmpdir/variant.flow"
"$flowmini" --dump-frontend-bundle "$tmpdir/variant.flow" | "$analyst" > "$tmpdir/variant.semantic.json"
"$parallel" < "$tmpdir/variant.semantic.json" | "$optimizer" > "$tmpdir/variant.optimized.json"
"$bind" --policy "$tmpdir/policy" < "$tmpdir/variant.semantic.json" > "$tmpdir/variant.binding.json"
"$lower" --emit-llvm "$tmpdir/variant.ll" --binding-report "$tmpdir/variant.binding.json" < "$tmpdir/variant.optimized.json" >/dev/null
grep -q 'source-variant' "$tmpdir/variant.ll"
! cmp -s "$tmpdir/renamed.ll" "$tmpdir/variant.ll"

# Source operation order is preserved rather than reconstructed from the
# capability set. Swapping two same-block terminal operations swaps their calls.
sed \
    -e "s|\"../../../std/|\"$root/Flowmini/flowmini_v25_symboltable_projection/std/|" \
    -e 's/^program sel$/program terminal_order_variant/' \
    -e 's/curses.noecho()/curses.__order_placeholder__()/' \
    -e 's/curses.cbreak()/curses.noecho()/' \
    -e 's/curses.__order_placeholder__()/curses.cbreak()/' \
    "$fixture" > "$tmpdir/order.flow"
"$flowmini" --dump-frontend-bundle "$tmpdir/order.flow" | "$analyst" > "$tmpdir/order.semantic.json"
"$parallel" < "$tmpdir/order.semantic.json" | "$optimizer" > "$tmpdir/order.optimized.json"
"$bind" --policy "$tmpdir/policy" < "$tmpdir/order.semantic.json" > "$tmpdir/order.binding.json"
"$lower" --emit-llvm "$tmpdir/order.ll" --binding-report "$tmpdir/order.binding.json" < "$tmpdir/order.optimized.json" >/dev/null
test "$(grep -n 'call i32 @cbreak' "$tmpdir/order.ll" | cut -d: -f1)" -lt "$(grep -n 'call i32 @noecho' "$tmpdir/order.ll" | cut -d: -f1)"
test "$(grep -n 'call i32 @noecho' "$tmpdir/renamed.ll" | cut -d: -f1)" -lt "$(grep -n 'call i32 @cbreak' "$tmpdir/renamed.ll" | cut -d: -f1)"

# An unused policy grant changes neither the typed authorized operation set nor
# the emitted program.
cp "$tmpdir/policy" "$tmpdir/policy-with-unused"
printf '%s\n' 'allow libc.so.6 abs c pure' >> "$tmpdir/policy-with-unused"
"$bind" --policy "$tmpdir/policy-with-unused" < "$tmpdir/renamed.semantic.json" > "$tmpdir/unused.binding.json"
"$lower" --emit-llvm "$tmpdir/unused.ll" --binding-report "$tmpdir/unused.binding.json" < "$tmpdir/renamed.optimized.json" >/dev/null
cmp -s "$tmpdir/renamed.ll" "$tmpdir/unused.ll"

# Removing source-derived control flow must be rejected rather than silently
# flattening operations that still carry child-block identities.
jq '.lowering_plan.operations |= map(select(.kind != "branch"))' \
    "$tmpdir/optimized.json" > "$tmpdir/branchless.optimized.json"
if "$lower" --emit-llvm "$tmpdir/branchless.ll" --binding-report "$tmpdir/binding.json" < "$tmpdir/branchless.optimized.json" >/dev/null 2>&1; then
    echo 'terminal lowering accepted a capability set without source branches' >&2
    exit 1
fi

# Exact operation authorization remains mandatory on the profile-free path.
sed 's/"wgetch"/"invented_wgetch"/g' "$tmpdir/binding.json" > "$tmpdir/hostile.binding.json"
if "$lower" --emit-llvm "$tmpdir/hostile.ll" --binding-report "$tmpdir/hostile.binding.json" < "$tmpdir/optimized.json" >/dev/null 2>&1; then
    echo 'profile-free terminal lowering accepted a mutated binding report' >&2
    exit 1
fi
"$lower" --emit-llvm "$tmpdir/program.ll" --binding-report "$tmpdir/binding.json" < "$tmpdir/optimized.json" > "$tmpdir/lowering.json"
grep -q '"status": "emitted"' "$tmpdir/lowering.json"
clang "$tmpdir/program.ll" -lncursesw -o "$tmpdir/program"
grep -q 'icmp eq i32' "$tmpdir/program.ll"
grep -q 'getelementptr ptr, ptr %argv' "$tmpdir/program.ll"
grep -q 'call i64 @read' "$tmpdir/program.ll"
grep -q 'call ptr @memset' "$tmpdir/program.ll"
grep -q 'call i64 @write' "$tmpdir/program.ll"
grep -q 'icmp sgt i64' "$tmpdir/program.ll"
grep -q 'icmp slt i64' "$tmpdir/program.ll"

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
