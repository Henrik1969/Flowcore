#!/bin/sh
set -eu
pack=$1 validate=$2 run=$3 build=$4
artifact="$build/tinyvm-v2-boundary.tvm"
corrupt="$build/tinyvm-v2-corrupt.tvm"
"$pack" "$artifact" --keep --no-import >/dev/null
"$validate" "$artifact" | grep -q '"status":"valid".*"artifact_format":2'
"$run" "$artifact" | grep -q '"status":"completed".*"result":42'

# Digest-protected payload corruption.
cp "$artifact" "$corrupt"
printf '\377' | dd of="$corrupt" bs=1 seek=720 conv=notrunc status=none
if "$validate" "$corrupt" >/dev/null 2>&1; then
    echo 'v2 validator accepted corrupted payload' >&2
    exit 1
fi

# Exact complete-input boundary.
cp "$artifact" "$corrupt"
printf '\000' >> "$corrupt"
if "$validate" "$corrupt" >/dev/null 2>&1; then
    echo 'v2 validator accepted trailing data' >&2
    exit 1
fi

head -c 511 "$artifact" > "$corrupt"
if "$validate" "$corrupt" >/dev/null 2>&1; then
    echo 'v2 validator accepted truncated header' >&2
    exit 1
fi
rm -f "$artifact" "$corrupt"
