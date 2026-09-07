#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
flowmini="${FLOWMINI_BIN:-${root}/cmake-build-debug/flowmini}"
flowstats="${root}/examples/apps/flowstats/flowstats.flow"
flowconfig="${root}/examples/apps/flowconfig/flowconfig.flow"
test -x "${flowmini}"
test "$("${flowmini}" "${flowstats}")" = $'5\n1\n9\n25'
test "$("${flowmini}" "${flowconfig}")" = "20"
"${flowmini}" --dump-frontend-bundle "${flowstats}" | jq -e '.diagnostics == [] and any(.ast.statement_pool[]; .kind == "guard")' >/dev/null
"${flowmini}" --dump-frontend-bundle "${flowconfig}" | jq -e '.diagnostics == [] and any(.ast.declaration_pool[]; .kind == "enum")' >/dev/null
echo "Flowmini mature program probes: PASS"
