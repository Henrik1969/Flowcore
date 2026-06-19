#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
parser="${1:-$root/build/flowparse}"

out1="$(mktemp)"
out2="$(mktemp)"
trap 'rm -f "$out1" "$out2"' EXIT

"$parser" < "$root/examples/demo.tokens.jsonl" > "$out1"
"$parser" < "$root/examples/multiline.tokens.jsonl" > "$out2"

grep -q '"schema":"langlab.ast.json"' "$out1"
grep -q '"kind":"DeclarationStatement"' "$out1"
grep -q '"kind":"PipelineExpression"' "$out1"
grep -q '"kind":"FlowDeclaration"' "$out1"
grep -q '"kind":"GraphConnection"' "$out1"
grep -q '"kind":"BinaryExpression"' "$out2"

echo "flowparse smoke test passed"
