#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
flowmini="${FLOWMINI_BIN:-${root}/cmake-build-debug/flowmini}"
fixture="${root}/examples/pass/stdlib_math_v0.flow"
test -x "${flowmini}"
test_output="$("${flowmini}" "${fixture}")"
test "${test_output}" = $'3\n7\n10'
bundle="$(mktemp)"
trap 'rm -f "${bundle}"' EXIT
"${flowmini}" --dump-frontend-bundle "${fixture}" > "${bundle}"
jq -e '
  .diagnostics == [] and
  ([.ast.declaration_pool[]? | select(.kind == "function" and (.name == "min" or .name == "max" or .name == "clamp"))] | length) == 3
' "${bundle}" >/dev/null
echo "Flowmini std.math v0: PASS"
