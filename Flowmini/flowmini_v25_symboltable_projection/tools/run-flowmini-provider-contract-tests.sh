#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
flowmini="${FLOWMINI_BIN:-${root}/cmake-build-debug/flowmini}"
analyst="${FLOWANALYST_BIN:-${root}/../../build/flowanalyst/flowanalyst}"
fixture="${root}/examples/pass/abi_libc_demo.flow"
tmpdir="$(mktemp -d)"
trap 'rm -rf "${tmpdir}"' EXIT
"${flowmini}" --dump-frontend-bundle "${fixture}" > "${tmpdir}/bundle.json"
"${analyst}" --lowering-plan-version 2 "${tmpdir}/bundle.json" > "${tmpdir}/semantic.json"
jq -e '
  ([.binding_requirements[]? | select(.contract != "" and .library != "" and .convention != "" and .symbol != "" and .effect != "" and .return_type != "")] | length) >= 1 and
  ([.lowering_plan.operations[]? | select(.kind == "external_call") | select(.provider.contract != "" and .provider.library != "" and .provider.convention != "" and .provider.symbol != "" and .provider.effect != "" and .provider.parameter_types != null and .provider.return_type != "")] | length) >= 1
' "${tmpdir}/semantic.json" >/dev/null
# The root flowbind_provider gate separately rejects missing symbols and
# malformed policy/ABI manifests; this focused gate checks the paperwork at
# the semantic boundary before that consumer runs.
echo "Flow provider contract paperwork: PASS (identity, ABI, effect, and type metadata captured)"
