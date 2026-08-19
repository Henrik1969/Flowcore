#!/usr/bin/env bash
set -euo pipefail

ROOT="${FLOWMINI_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
FLOWMINI_BIN="${FLOWMINI_BIN:-$ROOT/cmake-build-debug/flowmini}"
CONSUMER="$ROOT/tools/consume-flowmini-frontend-bundle.py"
ATTACKER="$ROOT/tools/attack-flowmini-frontend-bundle.py"
EXPECTED_DIR="$ROOT/tests/expected/frontend"
UPDATE="${FLOWMINI_UPDATE_FRONTEND_BUNDLE_GOLDENS:-0}"

SOURCES=(
  "examples/ast/type_reference_probe.flow"
  "examples/ast/placement_statement_probe.flow"
  "examples/ast/statement_assignment_probe.flow"
  "examples/ast/statement_else_probe.flow"
  "examples/ast/control_flow_unary_probe.flow"
  "examples/ast/abi_contract_probe.flow"
  "examples/ast/target_projection_probe.flow"
  "examples/pass/import_demo.flow"
)

if [[ ! -x "$FLOWMINI_BIN" ]]; then
    echo "error: FLOWMINI_BIN is not executable: $FLOWMINI_BIN" >&2
    exit 1
fi

mkdir -p "$EXPECTED_DIR"
tmpdir="$(mktemp -d)"
cleanup() {
    /usr/bin/rm -rf -- "$tmpdir"
}
trap cleanup EXIT

cd "$ROOT"
checked=0
updated=0

for source_file in "${SOURCES[@]}"; do
    probe="$(basename "$source_file" .flow)"
    bundle_file="$tmpdir/$probe.bundle.json"
    actual_file="$tmpdir/$probe.lowering.json"
    expected_file="$EXPECTED_DIR/$probe.lowering.json"

    echo "== frontend bundle: $probe =="
    "$FLOWMINI_BIN" --dump-frontend-bundle "$source_file" > "$bundle_file"

    consumer_args=()
    case "$probe" in
        type_reference_probe)
            consumer_args+=(
                --require-declaration-kind refined_type
                --require-declaration-kind record
                --require-declaration-kind function
                --require-declaration-kind main_block
                --require-symbol-origin-role refined_type_declaration
                --require-symbol-origin-role record_declaration
                --require-symbol-origin-role record_field
                --require-symbol-origin-role function_declaration
                --require-symbol-origin-role function_parameter
                --require-symbol-origin-role main_declaration
                --require-scope-origin-role module_scope
                --require-scope-origin-role record_scope
                --require-scope-origin-role function_scope
                --require-scope-origin-role main_scope
            )
            ;;
        placement_statement_probe)
            consumer_args+=(
                --require-statement-kind placement
                --require-statement-kind return
            )
            ;;
        statement_assignment_probe)
            consumer_args+=(--require-statement-kind assignment)
            ;;
        statement_else_probe)
            consumer_args+=(
                --require-scope-origin-role if_then_scope
                --require-scope-origin-role else_block_scope
            )
            ;;
        control_flow_unary_probe)
            consumer_args+=(
                --require-scope-origin-role while_body_scope
                --require-scope-origin-role if_then_scope
            )
            ;;
        abi_contract_probe)
            consumer_args+=(
                --require-declaration-kind abi
                --require-symbol-origin-role abi_declaration
                --require-symbol-origin-role abi_type
                --require-symbol-origin-role abi_struct
                --require-symbol-origin-role abi_struct_field
                --require-symbol-origin-role extern_function
                --require-symbol-origin-role extern_parameter
                --require-scope-origin-role abi_scope
                --require-scope-origin-role abi_struct_scope
                --require-scope-origin-role extern_function_scope
            )
            ;;
        target_projection_probe)
            consumer_args+=(
                --require-declaration-kind target
                --require-symbol-origin-role target_declaration
                --require-scope-origin-role target_scope
            )
            ;;
        import_demo)
            consumer_args+=(
                --minimum-source-files 3
                --require-declaration-kind import
                --require-declaration-kind function
                --require-declaration-kind main_block
                --require-symbol-origin-role source_unit
                --require-symbol-origin-role import_declaration
                --require-scope-origin-role module_scope
            )
            ;;
    esac

    python3 "$CONSUMER" "${consumer_args[@]}" "$bundle_file" > "$actual_file"

    if [[ "$UPDATE" == "1" ]]; then
        cp "$actual_file" "$expected_file"
        echo "updated: $expected_file"
        updated=$((updated + 1))
    else
        if [[ ! -f "$expected_file" ]]; then
            echo "error: missing frontend bundle golden: $expected_file" >&2
            exit 1
        fi
        diff -u "$expected_file" "$actual_file"
        checked=$((checked + 1))
    fi
done

negative_bundle="$tmpdir/type_reference_probe.bundle.json"
if python3 "$CONSUMER" \
    --require-declaration-kind graph_ir \
    "$negative_bundle" > /dev/null 2>&1; then
    echo "error: independent consumer accepted a missing required construct" >&2
    exit 1
fi

python3 "$ATTACKER" "$CONSUMER" "$tmpdir/import_demo.bundle.json"

if [[ "$UPDATE" == "1" ]]; then
    echo "Frontend bundle golden files updated: $updated"
else
    echo "Frontend bundle tests: PASS ($checked golden, 1 isolated, 19 negative)"
fi
