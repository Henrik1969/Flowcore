#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
flowmini="${FLOWMINI_BIN:-${root}/Flowmini/flowmini_v25_symboltable_projection/cmake-build-debug/flowmini}"
analyst="${FLOWANALYST_BIN:-${root}/build/flowanalyst/flowanalyst}"
artifact="${FLOWMINI_UTF8_ARTIFACT_BIN:-${root}/Flowmini/flowmini_v25_symboltable_projection/cmake-build-debug/flowmini_utf8_artifact}"
source="${root}/Flowmini/flowmini_v25_symboltable_projection/examples/bootstrap/utf8_source_reader_probe.flow"
provider_source="${root}/Flowmini/flowmini_v25_symboltable_projection/examples/bootstrap/utf8_hosted_provider_probe.flow"
runtime_source="${root}/Flowmini/flowmini_v25_symboltable_projection/examples/bootstrap/utf8_reader_runtime_probe.flow"
malformed_runtime_source="${root}/Flowmini/flowmini_v25_symboltable_projection/examples/bootstrap/utf8_malformed_runtime_probe.flow"
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

runtime_output="$(printf '\101\303\246' | "${flowmini}" "${runtime_source}")"
cpp_scalar="$(printf '\101\303\246' | "${artifact}" | jq -r '.scalars[1].value')"
test "${runtime_output}" = "${cpp_scalar}"
echo "Flow UTF-8 runtime decode probe: PASS"

malformed_output="$(printf '\101\300\257\102' | "${flowmini}" "${malformed_runtime_source}")"
cpp_diagnostics="$(printf '\101\300\257\102' | "${artifact}" | jq -r '.diagnostics | length')"
test "${malformed_output}" = "${cpp_diagnostics}"
echo "Flow UTF-8 malformed-input probe: PASS"
