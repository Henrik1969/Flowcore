#!/bin/sh
set -eu
pack=$1 validate=$2 run=$3 build=$4
artifact="$build/tinyvm-boundary-test.tvm"
artifact2="$build/tinyvm-boundary-test-2.tvm"
corrupt="$build/tinyvm-boundary-corrupt.tvm"
"$pack" "$artifact"
"$pack" "$artifact2"
cmp "$artifact" "$artifact2"
"$validate" "$artifact" | grep -q '"status":"valid"'
"$run" "$artifact" | grep -q '"status":"completed".*"result":42'
cp "$artifact" "$corrupt"
printf '\377' | dd of="$corrupt" bs=1 seek=520 conv=notrunc status=none
if "$validate" "$corrupt" >/dev/null 2>&1; then
    echo 'validator accepted a corrupted instruction' >&2
    exit 1
fi
head -c 500 "$artifact" > "$corrupt"
if "$validate" "$corrupt" >/dev/null 2>&1; then
    echo 'validator accepted a truncated artifact' >&2
    exit 1
fi
cp "$artifact" "$corrupt"
printf '\000' >> "$corrupt"
if "$validate" "$corrupt" >/dev/null 2>&1; then
    echo 'validator accepted trailing bytes' >&2
    exit 1
fi
rm -f "$artifact" "$artifact2" "$corrupt"
