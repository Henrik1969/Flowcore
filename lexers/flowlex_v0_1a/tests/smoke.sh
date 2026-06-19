#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
lexer="${1:-$root/build/flowlex}"
output="$("$lexer" < "$root/examples/smoke.flow")"
grep -q '"type":"DECLARE","lexeme":"§"' <<<"$output"
grep -q '"type":"PIPE","lexeme":"=>"' <<<"$output"
grep -q '"type":"GRAPH_ARROW","lexeme":"->"' <<<"$output"
grep -q '"type":"LOGICAL_OR","lexeme":"||"' <<<"$output"
grep -q '"type":"SHIFT_LEFT","lexeme":"<<"' <<<"$output"
grep -q '"type":"INTEGER_LITERAL","lexeme":"0x03"' <<<"$output"
grep -q '"type":"EOF"' <<<"$output"
echo "smoke test passed"
