#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
flowmini=${FLOWMINI_BIN:?FLOWMINI_BIN is required}
example="$root/Flowmini/flowmini_v25_symboltable_projection/examples/apps/flow_less"

test -x "$flowmini"
FLOWMINI_BIN="$flowmini" "$example/run-flow-less.sh"

tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT
cat > "$tmpdir/bad.flow" <<'EOF'
program bad_pager

producer source : pager.fake
node display : pager.render
sink halt : halt.record

policy source.lines = "a|b"
policy source.page_size = 0
wire source.out => display.in
wire display.out => halt.in
main { marker : int(1) }
EOF
if "$flowmini" "$tmpdir/bad.flow" >"$tmpdir/out" 2>"$tmpdir/err"; then
    printf '%s\n' 'flow_less negative test: invalid page size was accepted' >&2
    exit 1
fi
grep -Fq 'page_size must be positive' "$tmpdir/err"
printf '%s\n' 'flow_less pager contract: PASS'
