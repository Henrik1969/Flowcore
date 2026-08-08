#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd -- "${SCRIPT_DIR}/.." && pwd)"

BIN="${FLOWMINI_BIN:-${ROOT}/cmake-build-debug/flowmini}"
EXAMPLES_DIR="${ROOT}/examples/ast"
EXPECTED_DIR="${ROOT}/tests/expected/ast"
UPDATE="${FLOWMINI_UPDATE_AST_GOLDENS:-0}"

if [[ ! -x "$BIN" ]]; then
    echo "error: flowmini binary not executable: $BIN" >&2
    echo "hint: build first, for example:" >&2
    echo "  cmake --build ${ROOT}/cmake-build-debug -j20" >&2
    exit 2
fi

if [[ ! -d "$EXAMPLES_DIR" ]]; then
    echo "error: examples directory not found: $EXAMPLES_DIR" >&2
    exit 2
fi

mkdir -p "$EXPECTED_DIR"

tmpdir="$(mktemp -d)"
cleanup() {
    /usr/bin/rm -rf -- "$tmpdir"
}
trap cleanup EXIT

bad=0
count=0

shopt -s nullglob

if [[ "$UPDATE" == "1" ]]; then
    # Update mode intentionally uses all canonical example files present.
    for source in "$EXAMPLES_DIR"/*_probe.flow; do
        base="$(basename "$source" .flow)"
        expected="${EXPECTED_DIR}/${base}.ast.json"
        actual="${tmpdir}/${base}.ast.json"

        echo "== ast golden update: ${base} =="

        "$BIN" --dump-ast "$source" \
            | python3 -m json.tool \
            > "$actual"

        cp "$actual" "$expected"
        echo "updated: ${expected}"

        count=$((count + 1))
    done
else
    # Normal test mode uses checked-in expected files as the canonical test set.
    # This avoids experimental examples/ast/*.flow files accidentally joining the gate.
    for expected in "$EXPECTED_DIR"/*.ast.json; do
        base="$(basename "$expected" .ast.json)"
        source="${EXAMPLES_DIR}/${base}.flow"
        actual="${tmpdir}/${base}.ast.json"

        echo "== ast golden: ${base} =="

        if [[ ! -f "$source" ]]; then
            echo "missing source example: ${source}" >&2
            bad=1
            count=$((count + 1))
            continue
        fi

        "$BIN" --dump-ast "$source" \
            | python3 -m json.tool \
            > "$actual"

        if ! diff -u "$expected" "$actual"; then
            bad=1
        fi

        count=$((count + 1))
    done
fi

shopt -u nullglob

if [[ "$count" -eq 0 ]]; then
    echo "error: no AST golden tests found" >&2
    echo "expected files in: ${EXPECTED_DIR}" >&2
    exit 2
fi

if [[ "$bad" -ne 0 ]]; then
    echo "AST golden tests: FAIL"
    exit 1
fi

if [[ "$UPDATE" == "1" ]]; then
    echo "AST golden files updated: ${count}"
else
    echo "AST golden tests: PASS (${count})"
fi
