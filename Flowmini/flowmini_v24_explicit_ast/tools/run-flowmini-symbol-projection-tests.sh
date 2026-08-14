#!/usr/bin/env bash
set -euo pipefail

ROOT="${FLOWMINI_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
FLOWMINI_BIN="${FLOWMINI_BIN:-$ROOT/cmake-build-debug/flowmini}"

EXPECTED_DIR="$ROOT/tests/expected/symbols"
EXAMPLES_DIR="$ROOT/examples/ast"

PROBES=(
  "type_reference_probe"
  "statement_else_probe"
)

if [[ ! -x "$FLOWMINI_BIN" ]]; then
    echo "error: FLOWMINI_BIN is not executable: $FLOWMINI_BIN" >&2
    exit 1
fi

mkdir -p "$EXPECTED_DIR"

updated=0
checked=0

for probe in "${PROBES[@]}"; do
    source_file="$EXAMPLES_DIR/$probe.flow"
    expected_file="$EXPECTED_DIR/$probe.symbols.txt"
    actual_file="$(mktemp)"

    trap 'rm -f "$actual_file"' EXIT

    echo "== symbol projection: $probe =="

    "$FLOWMINI_BIN" --dump-ast-symbols - "$source_file" > "$actual_file"

    if [[ "${FLOWMINI_UPDATE_SYMBOL_GOLDENS:-0}" == "1" ]]; then
        cp "$actual_file" "$expected_file"
        echo "updated: $expected_file"
        updated=$((updated + 1))
    else
        if [[ ! -f "$expected_file" ]]; then
            echo "error: missing expected symbol projection file: $expected_file" >&2
            echo "hint: run FLOWMINI_UPDATE_SYMBOL_GOLDENS=1 tools/run-flowmini-symbol-projection-tests.sh" >&2
            exit 1
        fi

        diff -u "$expected_file" "$actual_file"
        checked=$((checked + 1))
    fi

    rm -f "$actual_file"
    trap - EXIT
done

if [[ "${FLOWMINI_UPDATE_SYMBOL_GOLDENS:-0}" == "1" ]]; then
    echo "Symbol projection golden files updated: $updated"
else
    echo "Symbol projection tests: PASS ($checked)"
fi
