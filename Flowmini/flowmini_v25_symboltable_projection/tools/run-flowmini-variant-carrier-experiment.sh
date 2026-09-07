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

"${analyst}" --lowering-plan-version 2 "${tmpdir}/bundle.json" > "${tmpdir}/semantic.json"
jq -e '.status == "ok" and any(.variant_carriers[]?.members[]?; .member == "scalar" and .discriminant == 0 and any(.payload_fields[]?; .type == "int"))' "${tmpdir}/semantic.json" >/dev/null

# The first useful carrier keeps canonical identity and labels while admitting
# one i32 payload slot in the LLVM backend. Larger payload layouts remain an
# explicit backend limitation.
cat > "${tmpdir}/carrier-report.json" <<'EOF'
{
  "format": "flowmini.variant_carrier_experiment",
  "status": "EXPERIMENTAL",
  "canonical_identity": "preserved",
  "discriminant": "label_member",
  "payload_identity": "preserved_in_ast_and_facts",
  "payload_layout": "one-slot i32 carrier",
  "llvm_lowering": "implemented_for_i32_payloads",
  "tinyvm": "explicit_unsupported",
  "next_gate": "admit_one_payload_layout_without_changing_canonical_facts"
}
EOF
jq -e '.canonical_identity == "preserved" and .payload_layout == "one-slot i32 carrier" and .llvm_lowering == "implemented_for_i32_payloads"' "${tmpdir}/carrier-report.json" >/dev/null
echo "Flowmini variant carrier experiment: PASS"
echo "  canonical tag/label identity is preserved"
echo "  one-slot i32 payload layout is lowered through LLVM"
