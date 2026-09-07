#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
flowmini="${FLOWMINI_BIN:-${root}/../../build/flowmini/flowmini}"
analyst="${FLOWANALYST_BIN:-${root}/../../build/flowanalyst/flowanalyst}"
optimize="${FLOWOPTIMIZE_BIN:-${root}/../../build/flowoptimize/flowoptimize}"
prepare="${FLOWPREPARE_BIN:-${root}/../../build/flowlower/flowprepare}"
lower="${FLOWLOWER_BIN:-${root}/../../build/flowlower/flowlower}"
tmpdir="$(mktemp -d)"
trap 'rm -rf "${tmpdir}"' EXIT

run_probe() {
    local fixture="$1" expected_tag="$2" expected_payload_type="$3" name="$4"
    "${flowmini}" --dump-frontend-bundle "${fixture}" > "${tmpdir}/${name}.bundle.json"
    jq -e --arg type "${expected_payload_type}" --arg member "${expected_tag}" \
        'any(.ast.declaration_pool[]?; .kind == "variant" and any(.members[]?; .name == $member and any(.fields[]?; .type == $type)))' \
        "${tmpdir}/${name}.bundle.json" >/dev/null
    "${flowmini}" --dump-frontend-bundle "${fixture}" |
        "${analyst}" --lowering-plan-version 2 > "${tmpdir}/${name}.semantic.json"
    jq -e --arg type "${expected_payload_type}" --arg member "${expected_tag}" \
        'any(.variant_carriers[]?.members[]?; .member == $member and any(.payload_fields[]?; .type == $type)) and .status == "ok"' \
        "${tmpdir}/${name}.semantic.json" >/dev/null
    "${optimize}" < "${tmpdir}/${name}.semantic.json" > "${tmpdir}/${name}.optimization.json"
    "${prepare}" < "${tmpdir}/${name}.optimization.json" > "${tmpdir}/${name}.backend.json"
    "${lower}" --emit-llvm "${tmpdir}/${name}.ll" < "${tmpdir}/${name}.backend.json" > "${tmpdir}/${name}.lowering.json"
    lli "${tmpdir}/${name}.ll"
}

run_probe "${root}/examples/bootstrap/variant_when_probe.flow" scalar int integer
run_probe "${root}/examples/bootstrap/variant_enum_payload_probe.flow" chosen Mode enum
run_probe "${root}/examples/bootstrap/variant_result_probe.flow" value int result

# A multi-field member is retained canonically but is outside the first
# one-slot backend representation. Verify explicit rejection rather than
# accepting a guessed layout.
"${flowmini}" --dump-frontend-bundle "${root}/examples/bootstrap/variant_multi_field_payload_probe.flow" |
    "${analyst}" --lowering-plan-version 2 > "${tmpdir}/multi.semantic.json"
"${optimize}" < "${tmpdir}/multi.semantic.json" > "${tmpdir}/multi.optimization.json"
"${prepare}" < "${tmpdir}/multi.optimization.json" > "${tmpdir}/multi.backend.json"
if "${lower}" --emit-llvm "${tmpdir}/multi.ll" < "${tmpdir}/multi.backend.json" > "${tmpdir}/multi.lowering.json" 2> "${tmpdir}/multi.lowering.err"; then
    echo "multi-field variant unexpectedly lowered without a carrier layout" >&2
    exit 1
fi
grep -q "unsupported variant carrier payload layout" "${tmpdir}/multi.lowering.err"

# Nested carriers remain canonical and analyzable, but the first LLVM carrier
# intentionally admits only a single scalar payload slot.
"${flowmini}" --dump-frontend-bundle "${root}/examples/bootstrap/variant_nested_payload_probe.flow" > "${tmpdir}/nested.bundle.json"
jq -e 'any(.ast.declaration_pool[]?; .kind == "variant" and .name == "Outer" and any(.members[]?; .name == "inner" and any(.fields[]?; .type == "Inner")))' "${tmpdir}/nested.bundle.json" >/dev/null
"${flowmini}" --dump-frontend-bundle "${root}/examples/bootstrap/variant_nested_payload_probe.flow" |
    "${analyst}" --lowering-plan-version 2 > "${tmpdir}/nested.semantic.json"
"${optimize}" < "${tmpdir}/nested.semantic.json" > "${tmpdir}/nested.optimization.json"
"${prepare}" < "${tmpdir}/nested.optimization.json" > "${tmpdir}/nested.backend.json"
if "${lower}" --emit-llvm "${tmpdir}/nested.ll" < "${tmpdir}/nested.backend.json" > "${tmpdir}/nested.lowering.json" 2> "${tmpdir}/nested.lowering.err"; then
    echo "nested variant unexpectedly lowered without a nested carrier layout" >&2
    exit 1
fi
grep -q "unsupported variant carrier payload layout" "${tmpdir}/nested.lowering.err"
echo "Flowmini variant backend tests: PASS"
echo "  integer payload: LLVM lowering and lli execution"
echo "  enum payload: LLVM lowering and lli execution"
echo "  result-flow probe: variant payload + match + guard"
echo "  multi-field payload: explicit backend rejection"
echo "  nested payload: explicit backend rejection"
