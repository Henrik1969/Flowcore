#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
flowmini="${FLOWMINI_BIN:-${root}/cmake-build-debug/flowmini}"
fixture="${root}/examples/ast/const_guard_semantics_probe.flow"
const_failure="${root}/examples/fail/bad_constant_mutation.flow"
artifact_failure="${root}/examples/ast/bad_const_mutation_artifact.flow"
analyst="${FLOWANALYST_BIN:-${root}/../../build/flowanalyst/flowanalyst}"
tmpdir="$(mktemp -d)"
trap 'rm -rf "${tmpdir}"' EXIT
"${flowmini}" --dump-frontend-bundle "${fixture}" > "${tmpdir}/bundle.json"
jq -e '
  ([.ast.statement_pool[] | select(.kind == "let" and .payload.is_const == true)] | length) == 1 and
  ([.ast.statement_pool[] | select(.kind == "guard" and (.payload.failure_block | type == "number"))] | length) == 1 and
  ([.symbol_table.symbols[]?.facts[]? | select(.key == "mutability" and .value.value == "const")] | length) == 1 and
  ([.scope_origins[]? | select(.role == "else_block_scope")] | length) >= 1
' "${tmpdir}/bundle.json" >/dev/null
"${analyst}" --lowering-plan-version 2 "${tmpdir}/bundle.json" > "${tmpdir}/const_guard_report.json"
jq -e '.status == "ok"' "${tmpdir}/const_guard_report.json" >/dev/null
"${flowmini}" --dump-frontend-bundle "${artifact_failure}" > "${tmpdir}/bad_const_bundle.json"
if "${analyst}" --lowering-plan-version 2 "${tmpdir}/bad_const_bundle.json" > "${tmpdir}/bad_const_report.json"; then
  echo "artifact const mutation was accepted" >&2
  exit 1
fi
jq -e '[.diagnostics[] | select(.code == "FLOWANALYST_CONST_MUTATION")] | length == 1' "${tmpdir}/bad_const_report.json" >/dev/null
if "${flowmini}" "${const_failure}" >/dev/null 2>&1; then
  echo "const mutation was accepted" >&2
  exit 1
fi
"${flowmini}" "${root}/examples/bootstrap/guard_control_flow_probe.flow" | diff -u - <(printf '7\n7\n')
echo "Flow const/guard semantic identity tests: PASS"
