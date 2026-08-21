#!/bin/sh
set -eu

if [ "$#" -ne 1 ]; then
    printf 'usage: %s TEXTFILE\n' "$0" >&2
    exit 2
fi
input=$1
if [ ! -f "$input" ]; then
    printf 'flow_less: not a regular file: %s\n' "$input" >&2
    exit 1
fi

root=$(CDPATH= cd -- "$(dirname -- "$0")/../../../../../../.." && pwd)
flowmini=${FLOWMINI_BIN:-$root/build/flowmini/flowmini}
source_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT

sed "s|__FLOW_LESS_PATH__|$input|g" "$source_dir/flow_less_ncurses.flow" > "$tmpdir/flow_less.flow"
exec "$flowmini" "$tmpdir/flow_less.flow"
