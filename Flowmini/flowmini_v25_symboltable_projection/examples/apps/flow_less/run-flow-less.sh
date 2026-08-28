#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/../../../../../../.." && pwd)
flowmini=${FLOWMINI_BIN:?FLOWMINI_BIN is required}
example_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT

"$flowmini" "$example_dir/flow_less.flow" > "$tmpdir/stdout.txt"
cmp -s "$example_dir/expected-stdout.txt" "$tmpdir/stdout.txt"
printf '%s\n' 'flow_less fake-provider example: PASS'
