#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
flowmini="${FLOWMINI_BIN:-${root}/Flowmini/flowmini_v25_symboltable_projection/cmake-build-debug/flowmini}"
analyst="${FLOWANALYST_BIN:-${root}/build/flowanalyst/flowanalyst}"
source="${root}/Flowmini/flowmini_v25_symboltable_projection/examples/bootstrap/utf8_source_reader_probe.flow"
provider_source="${root}/Flowmini/flowmini_v25_symboltable_projection/examples/bootstrap/utf8_hosted_provider_probe.flow"
tmpdir="$(mktemp -d)"
trap 'rm -rf "${tmpdir}"' EXIT

"${flowmini}" --dump-frontend-bundle "${source}" > "${tmpdir}/bundle.json"
"${analyst}" --lowering-plan-version 2 "${tmpdir}/bundle.json" > "${tmpdir}/semantic.json"
jq -e '
    .status == "ok" and
    ([.lowering_plan.functions[]? | select(.name == "main")] | length == 1) and
    ([.lowering_plan.operations[]? | select(.kind == "loop")] | length >= 1)
' "${tmpdir}/semantic.json" >/dev/null

echo "Flow UTF-8 contract probe: PASS"

"${flowmini}" --dump-frontend-bundle "${provider_source}" > "${tmpdir}/provider-bundle.json"
grep -q 'stdin.bytes' "${tmpdir}/provider-bundle.json"
"${analyst}" --lowering-plan-version 2 "${tmpdir}/provider-bundle.json" > "${tmpdir}/provider-semantic.json"
jq -e '
    .status == "ok"
' "${tmpdir}/provider-semantic.json" >/dev/null

echo "Flow UTF-8 hosted provider probe: PASS"
