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

identity="${root}/examples/ast/generic_identity_probe.flow"
"${flowmini}" --dump-frontend-bundle "${identity}" > "${tmpdir}/identity.bundle.json"
jq -e '
  any(.ast.declaration_pool[]?; .kind == "function" and .name == "identity" and [.type_parameters[]?.name] == ["T"]) and
  any(.ast.expression_pool[]?; .kind == "call" and [.payload.type_arguments[]?.text] == ["int"])
' "${tmpdir}/identity.bundle.json" >/dev/null
"${analyst}" --lowering-plan-version 2 < "${tmpdir}/identity.bundle.json" > "${tmpdir}/identity.semantic.json"
jq -e '
  .status == "ok" and
  any(.lowering_plan.generic_declarations[]?; .name == "identity" and .type_parameters == ["T"]) and
  any(.lowering_plan.operations[]?; .kind == "generic_call" and .instantiation_id == "identity<int>" and .substitutions == [{parameter:"T",type:"int"}])
' "${tmpdir}/identity.semantic.json" >/dev/null
"${optimize}" < "${tmpdir}/identity.semantic.json" > "${tmpdir}/identity.optimization.json"
"${prepare}" < "${tmpdir}/identity.optimization.json" > "${tmpdir}/identity.backend.json"
"${lower}" --emit-llvm "${tmpdir}/identity.ll" < "${tmpdir}/identity.backend.json" > "${tmpdir}/identity.lowering.json"
lli "${tmpdir}/identity.ll"

# The legacy runtime compatibility parser is intentionally pre-generics. Keep
# its divergence explicit rather than treating a parser failure as canonical
# language acceptance.
if "${flowmini}" --runtime-compat "${identity}" > "${tmpdir}/compat.out" 2> "${tmpdir}/compat.err"; then
  echo "runtime compatibility parser unexpectedly accepted generic syntax" >&2
  exit 1
fi
grep -q "expected '(' after function name" "${tmpdir}/compat.err"

inference="${root}/examples/ast/generic_inference_probe.flow"
"${flowmini}" --dump-frontend-bundle "${inference}" | "${analyst}" --lowering-plan-version 2 > "${tmpdir}/inference.semantic.json"
jq -e 'any(.lowering_plan.operations[]?; .kind == "generic_call" and .instantiation_id == "identity<int>" and .type_arguments == ["int"]) and .status == "ok"' "${tmpdir}/inference.semantic.json" >/dev/null

record="${root}/examples/ast/generic_record_probe.flow"
"${flowmini}" --dump-frontend-bundle "${record}" > "${tmpdir}/record.bundle.json"
"${analyst}" --lowering-plan-version 2 < "${tmpdir}/record.bundle.json" > "${tmpdir}/record.semantic.json"
jq -e 'any(.lowering_plan.generic_records[]?; .name == "Pair" and .type_parameters == ["A","B"] and [.fields[].type] == ["A","B"]) and .status == "ok"' "${tmpdir}/record.semantic.json" >/dev/null

for fixture in bad_generic_unknown_type bad_generic_arity; do
  set +e
  "${flowmini}" --dump-frontend-bundle "${root}/examples/fail/${fixture}.flow" | "${analyst}" --lowering-plan-version 2 > "${tmpdir}/${fixture}.semantic.json"
  exit_code=$?
  set -e
  test "${exit_code}" -ne 0
done
jq -e 'any(.diagnostics[]?; .code == "FLOWANALYST_GENERIC_UNKNOWN_TYPE")' "${tmpdir}/bad_generic_unknown_type.semantic.json" >/dev/null
jq -e 'any(.diagnostics[]?; .code == "FLOWANALYST_GENERIC_ARITY")' "${tmpdir}/bad_generic_arity.semantic.json" >/dev/null

echo "Flowmini restrained generic tests: PASS"
echo "  explicit identity<int> and LLVM execution"
echo "  deterministic identity<int> inference"
echo "  generic Pair<A,B> artifact preservation"
echo "  unknown type and wrong arity rejection"
