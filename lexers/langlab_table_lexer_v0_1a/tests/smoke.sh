#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
lexer="${1:-$root/build/tablelex}"
out="$(mktemp)"; err="$(mktemp)"
trap 'rm -f "$out" "$err"' EXIT
"$lexer" --rules "$root/config/flowcore.rules" --source "$root/examples/annotations.flow" --source-id 42 \
    < "$root/examples/annotations.flow" > "$out" 2> "$err"
grep -q '"type":"GHOST_STREAM_BEGIN"' "$out"
grep -q '"type":"GHOST_SOURCE_BEGIN"' "$out"
grep -q '"type":"GHOST_ANNOTATION"' "$out"
grep -q '"type":"PIPE","lexeme":"=>"' "$out"
grep -q '"type":"EOF"' "$out"
grep -q '"type":"GHOST_STREAM_END"' "$out"
grep -q 'notice\[' "$err"
echo "tablelex smoke test passed"
