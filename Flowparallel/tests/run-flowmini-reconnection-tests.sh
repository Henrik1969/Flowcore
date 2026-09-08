#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
flowmini="${FLOWMINI_BIN:?FLOWMINI_BIN is required}"
analyst="${FLOWANALYST_BIN:?FLOWANALYST_BIN is required}"
parallel="${FLOWPARALLEL_BIN:?FLOWPARALLEL_BIN is required}"
tmpdir="$(mktemp -d)"
trap 'rm -rf "${tmpdir}"' EXIT

check_serial_fallback() {
  local fixture="$1" reason="$2" name="$3"
  "${flowmini}" --dump-frontend-bundle "${fixture}" | "${analyst}" --lowering-plan-version 2 > "${tmpdir}/${name}.semantic.json" || true
  jq -e --arg reason "${reason}" '.status == "ok" and (.parallel_candidates | length) == 0 and any(.parallel_rejections[]?; .reason == $reason and .fallback == "serial")' "${tmpdir}/${name}.semantic.json" >/dev/null
  "${parallel}" "${tmpdir}/${name}.semantic.json" > "${tmpdir}/${name}.plan.json"
  jq -e '.status == "ready" and .dependency_analysis.parallel_candidates == 0 and (.parallel_candidates | length) == 0 and any(.parallel_rejections[]?; .fallback == "serial")' "${tmpdir}/${name}.plan.json" >/dev/null
}

check_serial_fallback "${root}/Flowmini/flowmini_v25_symboltable_projection/examples/bootstrap/shared_scalar_classifier.flow" "unknown-effect" unknown
check_serial_fallback "${root}/Flowmini/flowmini_v25_symboltable_projection/examples/ast/parallel_conflicting_output.flow" "conflicting-output" conflicting
check_serial_fallback "${root}/Flowmini/flowmini_v25_symboltable_projection/examples/ast/parallel_dependent.flow" "read-after-write-dependency" dependent

echo "Flowmini parallel legality fallbacks: PASS"
echo "  unknown effect: serial fallback"
echo "  conflicting output: serial fallback"
echo "  dependency: serial fallback"
