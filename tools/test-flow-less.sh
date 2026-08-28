#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
flowmini=${FLOWMINI_BIN:?FLOWMINI_BIN is required}
example="$root/Flowmini/flowmini_v25_symboltable_projection/examples/apps/flow_less"

test -x "$flowmini"
FLOWMINI_BIN="$flowmini" "$example/run-flow-less.sh"

tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT

sed 's/policy source.commands = "pgdown,end"/policy source.commands = "end,home"/' \
    "$example/flow_less.flow" > "$tmpdir/home.flow"
"$flowmini" --trace true "$tmpdir/home.flow" > "$tmpdir/home.out" 2> "$tmpdir/home.trace"
grep -Fq -- '-- page 1/3 --' "$tmpdir/home.out"
grep -Fq 'alpha' "$tmpdir/home.out"
grep -Fq 'route source.out => navigate.in' "$tmpdir/home.trace"
grep -Fq 'route navigate.out => display.in' "$tmpdir/home.trace"

cat > "$tmpdir/bad.flow" <<'EOF'
program bad_pager

producer source : pager.input.fake
node navigate : pager.navigate
node display : pager.render
sink halt : halt.record

policy source.lines = "a|b"
policy source.page_size = 0
wire source.out => navigate.in
wire navigate.out => display.in
wire display.out => halt.in
main { marker : int(1) }
EOF
if "$flowmini" "$tmpdir/bad.flow" >"$tmpdir/out" 2>"$tmpdir/err"; then
    printf '%s\n' 'flow_less negative test: invalid page size was accepted' >&2
    exit 1
fi
grep -Fq 'page_size must be positive' "$tmpdir/err"

sed 's/policy source.commands = "pgdown,end"/policy source.commands = "invented"/' \
    "$example/flow_less.flow" > "$tmpdir/bad-command.flow"
if "$flowmini" "$tmpdir/bad-command.flow" >/dev/null 2> "$tmpdir/bad-command.err"; then
    printf '%s\n' 'flow_less negative test: unknown navigation was accepted' >&2
    exit 1
fi
grep -Fq 'unknown command: invented' "$tmpdir/bad-command.err"
grep -Eq 'failure at navigate\.in via wire:0 signal:[0-9]+' "$tmpdir/bad-command.err"
printf '%s\n' 'flow_less pager contract: PASS'
