#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
flowmini="${FLOWMINI_BIN:-${root}/cmake-build-debug/flowmini}"
analyst="${FLOWANALYST_BIN:-${root}/../../build/flowanalyst/flowanalyst}"
tmpdir="$(mktemp -d)"
trap 'rm -rf "${tmpdir}"' EXIT
shared=(
  "${root}/examples/bootstrap/enum_state_probe.flow"
  "${root}/examples/bootstrap/variant_construction_probe.flow"
  "${root}/examples/bootstrap/guard_control_flow_probe.flow"
  "${root}/examples/bootstrap/when_state_probe.flow"
)
for source in "${shared[@]}"; do
  "${flowmini}" --dump-frontend-bundle "${source}" > "${tmpdir}/bundle.json"
  jq -e '([.ast.statement_pool[]? | select(.kind == "unknown")] | length) == 0' "${tmpdir}/bundle.json" >/dev/null
  "${analyst}" --lowering-plan-version 2 "${tmpdir}/bundle.json" > "${tmpdir}/semantic.json"
  jq -e '.status == "ok"' "${tmpdir}/semantic.json" >/dev/null
  "${flowmini}" --runtime-compat "${source}" >/dev/null
 done
# Variant matching is now represented in facts and the lowering plan, while
# backend execution remains a separate admission gate.
variant="${root}/examples/bootstrap/variant_when_probe.flow"
"${flowmini}" --dump-frontend-bundle "${variant}" > "${tmpdir}/variant.json"
jq -e 'any(.ast.statement_pool[]?; .kind == "when" and any(.payload.cases[]?; .label_type == "DecodeOutcome" and .label_member == "scalar"))' "${tmpdir}/variant.json" >/dev/null
if "${analyst}" --lowering-plan-version 2 "${tmpdir}/variant.json" > "${tmpdir}/variant-semantic.json"; then
  echo "variant backend divergence was not declared" >&2
  exit 1
fi
jq -e 'any(.diagnostics[]?; .code == "FLOWANALYST_VARIANT_MATCH_UNSUPPORTED")' "${tmpdir}/variant-semantic.json" >/dev/null
"${flowmini}" --runtime-compat "${variant}" >/dev/null
echo "Flow parser differential corpus: PASS (4 equivalent fixtures, 1 declared compatibility divergence)"
