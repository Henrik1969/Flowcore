#!/bin/sh
set -eu

root=${FLOWCORE_ROOT:-$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)}
flowmini=${FLOWMINI_BIN:-$root/Flowmini/flowmini_v25_symboltable_projection/cmake-build-debug/flowmini}
analyst=${FLOWANALYST_BIN:-$root/Flowanalyst/build/flowanalyst}
optimizer=${FLOWOPTIMIZE_BIN:-$root/Flowoptimize/build/flowoptimize}
lowerer=${FLOWLOWER_BIN:-$root/Flowlower/build/flowlower}
ast_root=$root/Flowmini/flowmini_v25_symboltable_projection/examples/ast
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT

pass_count=0
blocked_count=0

run_accepted() {
    name=$1
    bundle=$tmpdir/$name.bundle.json
    semantic=$tmpdir/$name.semantic.json
    optimized=$tmpdir/$name.optimized.json
    lowered=$tmpdir/$name.lowered.json
    "$flowmini" --dump-frontend-bundle "$ast_root/$name.flow" > "$bundle"
    set +e
    "$analyst" < "$bundle" > "$semantic"
    analyst_rc=$?
    set -e
    test "$analyst_rc" -eq 0
    grep -q '"status": "ok"' "$semantic"
    set +e
    "$optimizer" < "$semantic" > "$optimized"
    optimizer_rc=$?
    set -e
    test "$optimizer_rc" -eq 0
    grep -q '"status": "ready"' "$optimized"
    set +e
    "$lowerer" < "$optimized" > "$lowered"
    lowerer_rc=$?
    set -e
    test "$lowerer_rc" -eq 0
    grep -q '"status": "ready"' "$lowered"
    pass_count=$((pass_count + 1))
    echo "PASS accepted $name"
}

run_blocked() {
    name=$1
    bundle=$tmpdir/$name.bundle.json
    semantic=$tmpdir/$name.semantic.json
    optimized=$tmpdir/$name.optimized.json
    "$flowmini" --dump-frontend-bundle "$ast_root/$name.flow" > "$bundle"
    set +e
    "$analyst" < "$bundle" > "$semantic"
    analyst_rc=$?
    set -e
    test "$analyst_rc" -eq 2
    grep -q '"status": "error"' "$semantic"
    set +e
    "$optimizer" < "$semantic" > "$optimized"
    optimizer_rc=$?
    set -e
    test "$optimizer_rc" -eq 2
    grep -q '"status": "blocked"' "$optimized"
    blocked_count=$((blocked_count + 1))
    echo "PASS blocked $name"
}

run_accepted call_expression_probe
run_accepted control_flow_unary_probe
run_accepted expression_pool_probe
run_accepted refined_contract_probe
run_accepted target_projection_probe
run_blocked abi_contract_probe
run_blocked type_reference_probe

echo "Flowcore pipeline matrix: $pass_count accepted, $blocked_count blocked"
