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
constant_failure_source="${root}/Flowmini/flowmini_v25_symboltable_projection/examples/fail/bad_constant_mutation.flow"
guard_source="${root}/Flowmini/flowmini_v25_symboltable_projection/examples/bootstrap/guard_control_flow_probe.flow"
state_source="${root}/Flowmini/flowmini_v25_symboltable_projection/examples/bootstrap/utf8_state_trace_probe.flow"
when_source="${root}/Flowmini/flowmini_v25_symboltable_projection/examples/bootstrap/when_state_probe.flow"
when_no_default_source="${root}/Flowmini/flowmini_v25_symboltable_projection/examples/fail/bad_when_no_default.flow"
when_duplicate_source="${root}/Flowmini/flowmini_v25_symboltable_projection/examples/fail/bad_when_duplicate.flow"
when_overlap_source="${root}/Flowmini/flowmini_v25_symboltable_projection/examples/fail/bad_when_overlap.flow"
when_descending_source="${root}/Flowmini/flowmini_v25_symboltable_projection/examples/fail/bad_when_descending.flow"
enum_source="${root}/Flowmini/flowmini_v25_symboltable_projection/examples/bootstrap/enum_state_probe.flow"
enum_exhaustive_source="${root}/Flowmini/flowmini_v25_symboltable_projection/examples/bootstrap/enum_exhaustive_probe.flow"
enum_nonexhaustive_source="${root}/Flowmini/flowmini_v25_symboltable_projection/examples/fail/bad_enum_nonexhaustive.flow"
variant_source="${root}/Flowmini/flowmini_v25_symboltable_projection/examples/bootstrap/variant_declaration_probe.flow"
variant_construction_source="${root}/Flowmini/flowmini_v25_symboltable_projection/examples/bootstrap/variant_construction_probe.flow"
variant_when_source="${root}/Flowmini/flowmini_v25_symboltable_projection/examples/bootstrap/variant_when_probe.flow"
classifier_source="${root}/Flowmini/flowmini_v25_symboltable_projection/examples/bootstrap/shared_scalar_classifier.flow"
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

if "${flowmini}" "${constant_failure_source}" >/dev/null 2>&1; then
    echo "constant mutation was accepted" >&2
    exit 1
fi
echo "Flow constant immutability probe: PASS"

guard_output="$("${flowmini}" "${guard_source}")"
test "${guard_output}" = $'7\n7'
echo "Flow guard true/false branch probe: PASS"

state_artifact="$(printf '\101\303\246\300\257' | "${artifact}")"
echo "${state_artifact}" | jq -e '
    .scalars == [
        {value: 65, byte_offset: 0, byte_length: 1},
        {value: 230, byte_offset: 1, byte_length: 2}
    ] and
    .diagnostics == [
        {code: "invalid-leading-byte", byte_offset: 3},
        {code: "unexpected-continuation", byte_offset: 4}
    ]
' >/dev/null
state_output="$("${flowmini}" "${state_source}")"
test "${state_output}" = $'1\n2\n3\n4\n5'
echo "Flow UTF-8 state trace evidence probe: PASS"

when_output="$("${flowmini}" "${when_source}")"
test "${when_output}" = $'11\n22\n99'
echo "Flow when case/default probe: PASS"

"${flowmini}" --dump-frontend-bundle "${when_source}" > "${tmpdir}/when-bundle.json"
"${analyst}" --lowering-plan-version 2 "${tmpdir}/when-bundle.json" > "${tmpdir}/when-semantic.json"
jq -e '.status == "ok"' "${tmpdir}/when-semantic.json" >/dev/null
jq -e '([.ast.statement_pool[]? | select(.kind == "when")] | length == 3)' "${tmpdir}/when-bundle.json" >/dev/null
echo "Flow when frontend integration probe: PASS"

if "${flowmini}" "${when_no_default_source}" >/dev/null 2>&1; then
    echo "when without default was accepted" >&2
    exit 1
fi
if "${flowmini}" "${when_duplicate_source}" >/dev/null 2>&1; then
    echo "duplicate when case was accepted" >&2
    exit 1
fi
if "${flowmini}" "${when_overlap_source}" >/dev/null 2>&1; then
    echo "overlapping when case was accepted" >&2
    exit 1
fi
if "${flowmini}" "${when_descending_source}" >/dev/null 2>&1; then
    echo "descending when range was accepted" >&2
    exit 1
fi
echo "Flow when validation probe: PASS"

enum_output="$("${flowmini}" "${enum_source}")"
test "${enum_output}" = "11"
echo "Flow enum identity probe: PASS"

"${flowmini}" --dump-frontend-bundle "${enum_source}" > "${tmpdir}/enum-bundle.json"
"${analyst}" --lowering-plan-version 2 "${tmpdir}/enum-bundle.json" > "${tmpdir}/enum-semantic.json"
jq -e '.status == "ok"' "${tmpdir}/enum-semantic.json" >/dev/null
jq -e '([.ast.declaration_pool[]? | select(.kind == "enum")] | length == 1) and ([.ast.statement_pool[]? | select(.kind == "when")] | length == 1)' "${tmpdir}/enum-bundle.json" >/dev/null
echo "Flow enum frontend integration probe: PASS"

enum_exhaustive_output="$(${flowmini} "${enum_exhaustive_source}")"
test "${enum_exhaustive_output}" = "22"
if "${flowmini}" --dump-frontend-bundle "${enum_exhaustive_source}" > "${tmpdir}/enum-exhaustive-bundle.json"; then
    :
else
    echo "exhaustive enum when was rejected" >&2
    exit 1
fi
echo "Flow exhaustive enum when probe: PASS"
if "${flowmini}" "${enum_nonexhaustive_source}" >/dev/null 2>&1; then
    echo "non-exhaustive enum when was accepted" >&2
    exit 1
fi
echo "Flow enum exhaustiveness validation probe: PASS"

variant_output="$("${flowmini}" "${variant_source}")"
test "${variant_output}" = "1"
"${flowmini}" --dump-frontend-bundle "${variant_source}" > "${tmpdir}/variant-bundle.json"
"${analyst}" --lowering-plan-version 2 "${tmpdir}/variant-bundle.json" > "${tmpdir}/variant-semantic.json"
jq -e '.status == "ok"' "${tmpdir}/variant-semantic.json" >/dev/null
jq -e '([.ast.declaration_pool[]? | select(.kind == "variant")] | length == 1)' "${tmpdir}/variant-bundle.json" >/dev/null
echo "Flow tagged variant declaration probe: PASS"
variant_construction_output="$(${flowmini} "${variant_construction_source}")"
test "${variant_construction_output}" = "42"
"${flowmini}" --dump-frontend-bundle "${variant_construction_source}" > "${tmpdir}/variant-construction-bundle.json"
"${analyst}" --lowering-plan-version 2 "${tmpdir}/variant-construction-bundle.json" > "${tmpdir}/variant-construction-semantic.json"
jq -e '.status == "ok"' "${tmpdir}/variant-construction-semantic.json" >/dev/null
echo "Flow tagged variant construction probe: PASS"
variant_when_output="$(${flowmini} "${variant_when_source}")"
test "${variant_when_output}" = "42"
"${flowmini}" --dump-frontend-bundle "${variant_when_source}" > "${tmpdir}/variant-when-bundle.json"
"${analyst}" --lowering-plan-version 2 "${tmpdir}/variant-when-bundle.json" > "${tmpdir}/variant-when-semantic.json"
jq -e '.status == "ok"' "${tmpdir}/variant-when-semantic.json" >/dev/null
echo "Flow tagged variant when probe: PASS"

classifier_output="$(${flowmini} "${classifier_source}")"
test "${classifier_output}" = "53"
"${flowmini}" --dump-frontend-bundle "${classifier_source}" > "${tmpdir}/classifier-bundle.json"
"${analyst}" --lowering-plan-version 2 "${tmpdir}/classifier-bundle.json" > "${tmpdir}/classifier-semantic.json"
jq -e '([.ast.declaration_pool[]? | select(.kind == "enum")] | length == 1) and ([.ast.statement_pool[]? | select(.kind == "when")] | length == 1)' "${tmpdir}/classifier-bundle.json" >/dev/null
echo "Flow shared scalar classifier refactor probe: PASS"
