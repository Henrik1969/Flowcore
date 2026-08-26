#!/bin/sh
set -eu

root=${FLOWCORE_ROOT:?}
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT
policy="$tmpdir/policy"
printf '%s\n' \
  'allow libc.so.6 abs c pure c_int c_int' \
  'allow libc.so.6 strlen c pure c_string c_size_t' > "$policy"
printf '%s\n' 'allow libc.so.6 abs c readonly c_int c_int' > "$tmpdir/wrong-policy"

for name in abi_abs_main abi_strlen_main
do
  echo "pure-provider parity fixture: $name"
  source="$root/Flowmini/flowmini_v25_symboltable_projection/examples/pass/$name.flow"
  "$FLOWMINI_BIN" --dump-frontend-bundle "$source" > "$tmpdir/$name.frontend.json"
  "$FLOWANALYST_BIN" < "$tmpdir/$name.frontend.json" > "$tmpdir/$name.semantic.json"
  "$FLOWBIND_BIN" --policy "$policy" < "$tmpdir/$name.semantic.json" > "$tmpdir/$name.binding.json"
  "$FLOWPARALLEL_BIN" < "$tmpdir/$name.semantic.json" > "$tmpdir/$name.execution.json"
  "$FLOWOPTIMIZE_BIN" < "$tmpdir/$name.execution.json" > "$tmpdir/$name.optimization.json"
  "$FLOWPREPARE_BIN" --binding-report "$tmpdir/$name.binding.json" "$tmpdir/$name.optimization.json" > "$tmpdir/$name.lowering.json"
  "$FLOWLOWER_BIN" --emit-llvm "$tmpdir/$name.ll" "$tmpdir/$name.lowering.json" >/dev/null
  clang "$tmpdir/$name.ll" -o "$tmpdir/$name.llvm"
  "$FLOWTINYLOWER_BIN" "$tmpdir/$name.lowering.json" "$tmpdir/$name.tvm" >/dev/null
  "$FLOWTINYVALIDATE_BIN" "$tmpdir/$name.tvm" | grep -q '"status":"valid"'

  set +e
  "$tmpdir/$name.llvm"
  llvm_status=$?
  set -e
  tiny_result=$("$FLOWTINYRUN_BIN" --policy "$policy" "$tmpdir/$name.tvm" | jq -r '.result')
  test "$tiny_result" -eq "$llvm_status"

  if "$FLOWTINYRUN_BIN" "$tmpdir/$name.tvm" >/dev/null 2>&1; then
    echo 'TinyVM ran an import artifact without active policy' >&2
    exit 1
  fi
  if "$FLOWTINYRUN_BIN" --policy "$tmpdir/wrong-policy" "$tmpdir/$name.tvm" >/dev/null 2>&1; then
    echo 'TinyVM ran an import under a non-matching policy' >&2
    exit 1
  fi
done

echo 'pure governed LLVM/TinyVM provider parity: PASS'
