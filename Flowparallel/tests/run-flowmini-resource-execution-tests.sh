#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
flowmini="${FLOWMINI_BIN:?FLOWMINI_BIN is required}"
analyst="${FLOWANALYST_BIN:?FLOWANALYST_BIN is required}"
parallel="${FLOWPARALLEL_BIN:?FLOWPARALLEL_BIN is required}"
optimize="${FLOWOPTIMIZE_BIN:?FLOWOPTIMIZE_BIN is required}"
prepare="${FLOWPREPARE_BIN:?FLOWPREPARE_BIN is required}"
lower="${FLOWLOWER_BIN:?FLOWLOWER_BIN is required}"
bind="${FLOWBIND_BIN:?FLOWBIND_BIN is required}"
probe="${FLOWPARALLEL_FLOWMINI_PROBE_BIN:?FLOWPARALLEL_FLOWMINI_PROBE_BIN is required}"
fixture="${FLOWMINI_RESOURCE_FIXTURE:-${root}/Flowmini/flowmini_v25_symboltable_projection/examples/ast/parallel_resource_probe.flow}"
tmpdir="$(mktemp -d)"
trap 'rm -rf "${tmpdir}"' EXIT

"${flowmini}" --dump-frontend-bundle "${fixture}" > "${tmpdir}/frontend.json"
"${analyst}" --lowering-plan-version 2 < "${tmpdir}/frontend.json" > "${tmpdir}/semantic.json"
"${parallel}" "${tmpdir}/semantic.json" > "${tmpdir}/plan.json"
jq -e '.status == "ready" and .dependency_analysis.parallel_candidates == 3 and any(.parallel_rejections[]?; .reason == "resource-write-write-conflict" and .fallback == "serial")' "${tmpdir}/plan.json" >/dev/null

jq -r '.binding_requirements[] | "allow \(.library) \(.symbol) \(.convention) \(.effect) " + (if .parameter_types == "" then "-" else .parameter_types end) + " \(.return_type)"' "${tmpdir}/semantic.json" > "${tmpdir}/policy"
"${bind}" --policy "${tmpdir}/policy" < "${tmpdir}/semantic.json" > "${tmpdir}/binding.json"
"${optimize}" < "${tmpdir}/plan.json" > "${tmpdir}/optimization.json"
"${prepare}" --binding-report "${tmpdir}/binding.json" < "${tmpdir}/optimization.json" > "${tmpdir}/backend.json"
"${lower}" --emit-llvm "${tmpdir}/program.ll" < "${tmpdir}/backend.json" > "${tmpdir}/lowering.json"
clang -shared -fPIC "${tmpdir}/program.ll" -lc -o "${tmpdir}/program.so"

probe_report="$(${probe} "${tmpdir}/plan.json" "${tmpdir}/program.so")"
printf '%s\n' "${probe_report}" | jq -e '.status == "ok" and .matching_observable_results == true and .serial_results == [9,64] and .parallel_results == [9,64]' >/dev/null
printf '%s\n' "${probe_report}"
echo "Flowmini mixed resource/computation execution: PASS"
