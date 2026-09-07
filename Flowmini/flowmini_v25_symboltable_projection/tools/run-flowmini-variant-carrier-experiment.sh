#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
flowmini="${FLOWMINI_BIN:-${root}/../../build/flowmini/flowmini}"
analyst="${FLOWANALYST_BIN:-${root}/../../build/flowanalyst/flowanalyst}"
fixture="${root}/examples/bootstrap/variant_when_probe.flow"
tmpdir="$(mktemp -d)"
trap 'rm -rf "${tmpdir}"' EXIT

"${flowmini}" --dump-frontend-bundle "${fixture}" > "${tmpdir}/bundle.json"
jq -e 'any(.ast.statement_pool[]?; .kind == "when" and any(.payload.cases[]?; .label_type == "DecodeOutcome" and .label_member == "scalar"))' "${tmpdir}/bundle.json" >/dev/null

if "${analyst}" --lowering-plan-version 2 "${tmpdir}/bundle.json" > "${tmpdir}/semantic.json"; then
    echo "variant carrier experiment unexpectedly admitted backend lowering" >&2
    exit 1
fi
jq -e 'any(.diagnostics[]?; .code == "FLOWANALYST_VARIANT_MATCH_UNSUPPORTED")' "${tmpdir}/semantic.json" >/dev/null

# The smallest useful carrier is still a target-neutral design question: the
# canonical facts retain identity and labels, while payload layout is not yet
# owned by the backend-neutral contract. Keep that result machine-checkable.
cat > "${tmpdir}/carrier-report.json" <<'EOF'
{
  "format": "flowmini.variant_carrier_experiment",
  "status": "EXPERIMENTAL",
  "canonical_identity": "preserved",
  "discriminant": "label_member",
  "payload_identity": "preserved_in_ast_and_facts",
  "payload_layout": "NOT_SUPPORTED — DEFERRED",
  "llvm_lowering": "explicit_reject",
  "tinyvm": "explicit_unsupported",
  "next_gate": "admit_one_payload_layout_without_changing_canonical_facts"
}
EOF
jq -e '.canonical_identity == "preserved" and .payload_layout == "NOT_SUPPORTED — DEFERRED" and .llvm_lowering == "explicit_reject"' "${tmpdir}/carrier-report.json" >/dev/null
echo "Flowmini variant carrier experiment: PASS"
echo "  canonical tag/label identity is preserved"
echo "  payload layout remains an explicit backend contract blocker"
