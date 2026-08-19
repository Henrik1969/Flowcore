#!/bin/sh
set -eu

root=${FLOWCORE_ROOT:-$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)}
flowmini=${FLOWMINI_BIN:-$root/Flowmini/flowmini_v25_symboltable_projection/cmake-build-debug/flowmini}
analyst=${FLOWANALYST_BIN:-$root/Flowanalyst/build/flowanalyst}
optimizer=${FLOWOPTIMIZE_BIN:-$root/Flowoptimize/build/flowoptimize}
lowerer=${FLOWLOWER_BIN:-$root/Flowlower/build/flowlower}
pass_root=$root/Flowmini/flowmini_v25_symboltable_projection/examples/pass
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT

count=0
for source in "$pass_root"/*.flow; do
    name=${source##*/}
    name=${name%.flow}
    bundle=$tmpdir/$name.bundle.json
    semantic=$tmpdir/$name.semantic.json
    optimized=$tmpdir/$name.optimized.json
    lowered=$tmpdir/$name.lowered.json

    "$flowmini" --dump-frontend-bundle "$source" > "$bundle"
    "$analyst" < "$bundle" > "$semantic"
    grep -q '"status": "ok"' "$semantic"
    "$optimizer" < "$semantic" > "$optimized"
    grep -q '"status": "ready"' "$optimized"
    "$lowerer" < "$optimized" > "$lowered"
    grep -q '"status": "ready"' "$lowered"
    count=$((count + 1))
done

echo "Flowcore pass corpus: $count programs passed semantic and lowering boundaries"
