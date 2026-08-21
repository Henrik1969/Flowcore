#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
flowmini=${FLOWMINI_BIN:?FLOWMINI_BIN is required}
source="$root/Flowmini/flowmini_v25_symboltable_projection/examples/apps/flow_less/flow_less_ncurses.flow"
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT

test -x "$flowmini"
printf '%s\n' alpha beta gamma delta epsilon > "$tmpdir/input.txt"
sed "s|__FLOW_LESS_PATH__|$tmpdir/input.txt|g" "$source" > "$tmpdir/flow_less.flow"
printf 'q' | TERM=xterm script -qec "$flowmini '$tmpdir/flow_less.flow'" "$tmpdir/terminal.log" >/dev/null
grep -Fq -- '-- page 1/3 --' "$tmpdir/terminal.log"
grep -Fq 'alpha' "$tmpdir/terminal.log"
grep -Fq 'beta' "$tmpdir/terminal.log"
sed 's|__FLOW_LESS_PATH__|/definitely/missing/flow_less.txt|g' "$source" > "$tmpdir/missing.flow"
if "$flowmini" "$tmpdir/missing.flow" >"$tmpdir/missing.out" 2>"$tmpdir/missing.err"; then
    printf '%s\n' 'flow_less ncurses-provider test: missing file was accepted' >&2
    exit 1
fi
grep -Fq 'unable to open text file' "$tmpdir/missing.err"
printf '%s\n' 'flow_less ncurses-provider example: PASS'
