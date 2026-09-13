#!/bin/sh
set -eu
root=${FLOWCORE_ROOT:?}
flowmini=${FLOWMINI_BIN:?}
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT
count=0
for source in "$root"/Flowmini/flowmini_v29_reusable_native_chain/examples/support/*.flow; do
    if "$flowmini" "$source" > "$tmpdir/out" 2> "$tmpdir/err"; then
        printf 'Support fragment unexpectedly executable: %s\n' "$source" >&2
        exit 1
    fi
    test -s "$tmpdir/err"
    count=$((count + 1))
done
test "$count" -gt 0
printf 'Support fragments: %s correctly refused standalone execution\n' "$count"
