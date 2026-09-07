#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
probe="${FLOWPARALLEL_FLOWMINI_PROBE_BIN:?FLOWPARALLEL_FLOWMINI_PROBE_BIN is required}"
flowmini="${FLOWMINI_BIN:?FLOWMINI_BIN is required}"
analyst="${FLOWANALYST_BIN:?FLOWANALYST_BIN is required}"
parallel="${FLOWPARALLEL_BIN:?FLOWPARALLEL_BIN is required}"
optimize="${FLOWOPTIMIZE_BIN:?FLOWOPTIMIZE_BIN is required}"
prepare="${FLOWPREPARE_BIN:?FLOWPREPARE_BIN is required}"
lower="${FLOWLOWER_BIN:?FLOWLOWER_BIN is required}"
cpu="${FLOWPARALLEL_CPU_BIN:?FLOWPARALLEL_CPU_BIN is required}"
fixture="${FLOWMINI_PARALLEL_FIXTURE:-${root}/Flowmini/flowmini_v25_symboltable_projection/examples/ast/parallel_independence_probe.flow}"
tmpdir="$(mktemp -d)"
trap 'rm -rf "${tmpdir}"' EXIT

"${flowmini}" --dump-frontend-bundle "${fixture}" | "${analyst}" --lowering-plan-version 2 > "${tmpdir}/semantic.json"
jq -e '.status == "ok" and ([.parallel_candidates[]? | select(.proof_status == "proven" and .evidence.dependency_independent == true and .evidence.effect_compatible == true and .evidence.mutation_conflict == false and (.provenance.ast_path | startswith("/statement_pool/")))] | length) == 2' "${tmpdir}/semantic.json" >/dev/null
"${parallel}" "${tmpdir}/semantic.json" > "${tmpdir}/execution-plan.json"
jq -e '.status == "ready" and .dependency_analysis.parallel_candidates == 2 and ([.parallel_candidates[]? | select(.proof_status == "proven" and .provenance.source != null and .evidence.resource_compatibility == "unknown-resources-not-present")] | length) == 2' "${tmpdir}/execution-plan.json" >/dev/null
serial_selection="$(${cpu} --plan "${tmpdir}/execution-plan.json" --observed-speedup 0.1 --workers 2)"
parallel_selection="$(${cpu} --plan "${tmpdir}/execution-plan.json" --observed-speedup 2.0 --workers 2)"
printf '%s\n' "${serial_selection}" | jq -e '.decision == "serial" and .provider == "cpu.serial"' >/dev/null
printf '%s\n' "${parallel_selection}" | jq -e '.decision == "parallel" and .provider == "cpu.threadpool"' >/dev/null
"${optimize}" < "${tmpdir}/execution-plan.json" > "${tmpdir}/optimization.json"
jq -e '([.parallel_candidates[]? | select(.proof_status == "proven")] | length) == 2 and ([.parallel_rejections[]?] | length) == 0' "${tmpdir}/optimization.json" >/dev/null
"${prepare}" < "${tmpdir}/optimization.json" > "${tmpdir}/backend.json"
"${lower}" --emit-llvm "${tmpdir}/program.ll" < "${tmpdir}/backend.json" > "${tmpdir}/lowering.json"
clang -shared -fPIC "${tmpdir}/program.ll" -o "${tmpdir}/program.so"
probe_report="$(${probe} "${tmpdir}/execution-plan.json" "${tmpdir}/program.so")"
printf '%s\n' "${probe_report}" | jq -e '.status == "ok" and .provider_paths == ["cpu.serial","cpu.threadpool"] and .matching_observable_results == true and .serial_results == [9,49]' >/dev/null
printf '%s\n' "${probe_report}"
echo "Flowmini serial/thread-pool execution: PASS"
