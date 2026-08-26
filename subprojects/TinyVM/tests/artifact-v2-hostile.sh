#!/bin/sh
set -eu
pack=$1 mutate=$2 validate=$3 build=$4
valid="$build/tinyvm-v2-hostile-valid.tvm"
bad="$build/tinyvm-v2-hostile-bad.tvm"
"$pack" "$valid" --keep >/dev/null
"$validate" "$valid" | grep -q '"status":"valid"'
for attack in identity-padding duplicate-section constant-bool import-reserved provenance-source padding
do
    "$mutate" "$valid" "$bad" "$attack"
    if "$validate" "$bad" >/dev/null 2>&1; then
        echo "v2 validator accepted $attack with a valid recomputed digest" >&2
        exit 1
    fi
done
rm -f "$valid" "$bad"
