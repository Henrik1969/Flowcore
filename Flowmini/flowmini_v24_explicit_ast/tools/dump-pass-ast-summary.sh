#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="${ROOT}/cmake-build-debug/flowmini"
OUT_DIR="${ROOT}/test-report/ast-dumps"
SUMMARY="${OUT_DIR}/all-pass-summary.txt"

mkdir -p "$OUT_DIR"
/usr/bin/rm -f -- "$SUMMARY"

find "${ROOT}/examples/pass" -name '*.flow' | sort | while IFS= read -r file; do
    base="$(basename "$file" .flow)"
    out="${OUT_DIR}/${base}.ast.json"

    echo "===== ${file#"$ROOT"/} =====" >> "$SUMMARY"

    if "$BIN" --dump-ast "$file" > "$out" 2>"$out.err"; then
        grep -E '"name": |"declaration_count": |"kind": "function"|"kind": "main_block"|"parameter_count": |"return_type": ' "$out" \
            >> "$SUMMARY" || true
    else
        echo "AST DUMP FAILED" >> "$SUMMARY"
        cat "$out.err" >> "$SUMMARY"
    fi

    echo >> "$SUMMARY"
done

echo "Wrote $SUMMARY"
