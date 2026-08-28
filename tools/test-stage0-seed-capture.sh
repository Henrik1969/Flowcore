#!/bin/sh
set -eu

root=${FLOWCORE_ROOT:?}
validate=${FLOWVALIDATE_BIN:?}
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT

"$root/tools/capture-stage0-seed.sh" > "$tmpdir/first.json"
"$root/tools/capture-stage0-seed.sh" > "$tmpdir/second.json"
cmp -s "$tmpdir/first.json" "$tmpdir/second.json"
jq -e '.format == "flowcore.bootstrap_seed" and .version == 1 and .status == "captured" and
       (.source.tracked_sha256 | test("^[0-9a-f]{64}$")) and
       .standards.c == "C11" and .standards.cxx == "C++20" and
       (.providers | index("llvm-toolchain")) != null' "$tmpdir/first.json" >/dev/null
"$validate" "$tmpdir/first.json" | grep -q '"classification":"valid"'
jq '.source.tracked_sha256 = "not-a-digest"' "$tmpdir/first.json" > "$tmpdir/mutated.json"
if "$validate" "$tmpdir/mutated.json" >/dev/null 2>&1; then
    echo 'flowvalidate accepted a malformed bootstrap source digest' >&2; exit 1
fi

echo 'Stage 0 seed capture: PASS'
