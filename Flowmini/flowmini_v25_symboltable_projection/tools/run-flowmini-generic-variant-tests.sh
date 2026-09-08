#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
flowmini="${FLOWMINI_BIN:?FLOWMINI_BIN is required}"
analyst="${FLOWANALYST_BIN:?FLOWANALYST_BIN is required}"
optimize="${FLOWOPTIMIZE_BIN:?FLOWOPTIMIZE_BIN is required}"
prepare="${FLOWPREPARE_BIN:?FLOWPREPARE_BIN is required}"
lower="${FLOWLOWER_BIN:?FLOWLOWER_BIN is required}"
tmpdir="$(mktemp -d)"
trap 'rm -rf "${tmpdir}"' EXIT

fixture="${root}/examples/ast/generic_variant_probe.flow"
"${flowmini}" --dump-frontend-bundle "${fixture}" > "${tmpdir}/bundle.json"
jq -e '
  any(.ast.declaration_pool[]?; .kind == "variant" and .name == "Result" and [.type_parameters[]?.name] == ["T","E"] and .members[0].name == "ok" and .members[1].name == "error") and
  any(.ast.declaration_pool[]?; .kind == "variant" and .name == "Either" and [.type_parameters[]?.name] == ["A","B"]) and
  any(.ast.declaration_pool[]?; .kind == "variant" and .name == "Option" and [.type_parameters[]?.name] == ["T"])
' "${tmpdir}/bundle.json" >/dev/null

"${analyst}" --lowering-plan-version 2 < "${tmpdir}/bundle.json" > "${tmpdir}/semantic.json"
"${flowmini}" --dump-frontend-bundle "${fixture}" > "${tmpdir}/bundle-repeat.json"
"${analyst}" --lowering-plan-version 2 < "${tmpdir}/bundle-repeat.json" > "${tmpdir}/semantic-repeat.json"
cmp -s "${tmpdir}/semantic.json" "${tmpdir}/semantic-repeat.json"
jq -e '
  .status == "ok" and
  any(.lowering_plan.generic_variants[]?; .name == "Result" and .type_parameters == ["T","E"] and .members[0].discriminant == 0 and .members[1].discriminant == 1) and
  ([.lowering_plan.operations[]? | select(.kind == "variant_construct")] | length) == 3 and
  all(.lowering_plan.operations[]? | select(.kind == "variant_construct" and .variant_type == "Result<int,ParseError>"); .generic_owner == "Result" and .instance_id == "Result<int,ParseError>" and .type_arguments == ["int","ParseError"] and .substitutions == [{parameter:"T",type:"int"},{parameter:"E",type:"ParseError"}]) and
  any(.lowering_plan.operations[]?; .kind == "variant_construct" and .variant_type == "Either<int,Bool>" and .generic_owner == "Either" and .instance_id == "Either<int,Bool>" and .type_arguments == ["int","Bool"] and .substitutions == [{parameter:"A",type:"int"},{parameter:"B",type:"Bool"}]) and
  any(.lowering_plan.operations[]?; .kind == "variant_construct" and .variant_member == "ok" and .variant_discriminant == 0 and .payload_types == ["int"]) and
  any(.lowering_plan.operations[]?; .kind == "variant_construct" and .variant_member == "error" and .variant_discriminant == 1 and .payload_types == ["ParseError"]) and
  any(.lowering_plan.operations[]?; .kind == "match" and .selector_kind == "variant" and .selector_type == "Result<int,ParseError>" and .variant_generic_owner == "Result" and .variant_instance_id == "Result<int,ParseError>" and .cases[0].payload_bindings[0].type == "int")
' "${tmpdir}/semantic.json" >/dev/null

"${optimize}" < "${tmpdir}/semantic.json" > "${tmpdir}/optimization.json"
"${prepare}" < "${tmpdir}/optimization.json" > "${tmpdir}/backend.json"
"${lower}" --emit-llvm "${tmpdir}/generic_variant.ll" < "${tmpdir}/backend.json" > "${tmpdir}/lowering.json"
lli "${tmpdir}/generic_variant.ll"

imported="${root}/examples/ast/result_import_probe.flow"
"${flowmini}" --dump-frontend-bundle "${imported}" | "${analyst}" --lowering-plan-version 2 > "${tmpdir}/import.semantic.json"
jq -e '.status == "ok" and any(.lowering_plan.generic_variants[]?; .name == "Result" and .type_parameters == ["T","E"]) and any(.lowering_plan.operations[]?; .kind == "variant_construct" and .variant_type == "Result<int,ParseError>")' "${tmpdir}/import.semantic.json" >/dev/null

invalid="${root}/examples/fail/bad_generic_variant_unknown_type.flow"
set +e
"${flowmini}" --dump-frontend-bundle "${invalid}" | "${analyst}" --lowering-plan-version 2 > "${tmpdir}/invalid.semantic.json"
exit_code=$?
set -e
test "${exit_code}" -ne 0
jq -e 'any(.diagnostics[]?; .code == "FLOWANALYST_GENERIC_VARIANT_UNKNOWN_TYPE")' "${tmpdir}/invalid.semantic.json" >/dev/null

for invalid in \
    "${root}/examples/fail/bad_generic_variant_payload.flow" \
    "${root}/examples/fail/bad_generic_variant_duplicate_parameter.flow"; do
    set +e
    "${flowmini}" --dump-frontend-bundle "${invalid}" | "${analyst}" --lowering-plan-version 2 > "${tmpdir}/invalid.semantic.json"
    exit_code=$?
    set -e
    test "${exit_code}" -ne 0
    jq -e '(.diagnostics | length) > 0' "${tmpdir}/invalid.semantic.json" >/dev/null
done

echo "Flowmini generic variant tests: PASS"
echo "  generic Result<T,E> construction, substitution, match, and LLVM execution"
echo "  neutral Either<A,B> and payloadless Option<T> declarations"
echo "  std/result.flow import"
echo "  deterministic artifacts and invalid payload/parameter rejection"
