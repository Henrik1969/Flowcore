#!/bin/sh
set -eu

target=${FLOWTARGET_BIN:?}
prepare=${FLOWPREPARE_BIN:?}
llvm=${FLOWLOWER_BIN:?}
tiny=${FLOWTINYLOWER_BIN:?}
fixture_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
policy_root="$fixture_dir/../target-policies"
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT

resolve_and_prepare() {
    name=$1
    "$target" --policy-root "$policy_root" "$name" > "$tmpdir/policy.json"
    "$prepare" --target-policy "$tmpdir/policy.json" "$fixture_dir/captured-empty-optimization.json" > "$tmpdir/$name.lowering.json"
}

resolve_and_prepare llvm-host
jq -e '.version == 2 and .target_policy.name == "llvm-host" and .target_policy.backend.name == "llvm"' "$tmpdir/llvm-host.lowering.json" >/dev/null
"$llvm" --emit-llvm "$tmpdir/llvm.ll" "$tmpdir/llvm-host.lowering.json" > "$tmpdir/llvm.report.json"
grep -q 'define i32 @main()' "$tmpdir/llvm.ll"
if "$tiny" "$tmpdir/llvm-host.lowering.json" "$tmpdir/wrong.tvm" > "$tmpdir/wrong-tiny.json"; then
    echo 'TinyVM accepted an LLVM target policy' >&2; exit 1
else
    test $? -eq 2
fi
test ! -e "$tmpdir/wrong.tvm"
jq -e '.status == "unsupported" and (.reason | contains("incompatible with the TinyVM backend"))' "$tmpdir/wrong-tiny.json" >/dev/null

resolve_and_prepare tinyvm-portable
jq -e '.version == 2 and .target_policy.name == "tinyvm-portable" and .target_policy.backend.name == "tinyvm"' "$tmpdir/tinyvm-portable.lowering.json" >/dev/null
"$tiny" "$tmpdir/tinyvm-portable.lowering.json" "$tmpdir/program.tvm" > "$tmpdir/tiny.report.json"
test -s "$tmpdir/program.tvm"
if "$llvm" --emit-llvm "$tmpdir/wrong.ll" "$tmpdir/tinyvm-portable.lowering.json" > "$tmpdir/wrong-llvm.json"; then
    echo 'LLVM accepted a TinyVM target policy' >&2; exit 1
else
    test $? -eq 2
fi
test ! -e "$tmpdir/wrong.ll"
jq -e '.status == "unsupported" and (.reason | contains("incompatible with the LLVM backend"))' "$tmpdir/wrong-llvm.json" >/dev/null

jq '.capabilities.required += ["imaginary-provider"]' "$policy_root/tinyvm-portable.json" > "$tmpdir/incompatible.json"
"$prepare" --target-policy "$tmpdir/incompatible.json" "$fixture_dir/captured-empty-optimization.json" > "$tmpdir/incompatible.lowering.json"
if "$tiny" "$tmpdir/incompatible.lowering.json" "$tmpdir/incompatible.tvm" > "$tmpdir/incompatible.report.json"; then
    echo 'TinyVM accepted an unavailable required provider capability' >&2; exit 1
else
    test $? -eq 2
fi
test ! -e "$tmpdir/incompatible.tvm"
jq -e '.status == "unsupported" and (.reason | contains("unavailable provider capability"))' "$tmpdir/incompatible.report.json" >/dev/null

jq '.abi.version = 99' "$policy_root/tinyvm-portable.json" > "$tmpdir/wrong-abi.json"
"$prepare" --target-policy "$tmpdir/wrong-abi.json" "$fixture_dir/captured-empty-optimization.json" > "$tmpdir/wrong-abi.lowering.json"
if "$tiny" "$tmpdir/wrong-abi.lowering.json" "$tmpdir/wrong-abi.tvm" > "$tmpdir/wrong-abi.report.json"; then
    echo 'TinyVM accepted an incompatible ABI policy' >&2; exit 1
else
    test $? -eq 2
fi
test ! -e "$tmpdir/wrong-abi.tvm"
jq -e '.status == "unsupported" and (.reason | contains("incompatible with the TinyVM backend"))' "$tmpdir/wrong-abi.report.json" >/dev/null

echo 'target-policy cross compilation: PASS'
