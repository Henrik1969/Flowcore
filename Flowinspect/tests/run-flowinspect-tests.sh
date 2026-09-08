#!/usr/bin/env bash
set -euo pipefail

inspect="${FLOWINSPECT_BIN:?FLOWINSPECT_BIN is required}"
flowmini="${FLOWMINI_BIN:?FLOWMINI_BIN is required}"
analyst="${FLOWANALYST_BIN:?FLOWANALYST_BIN is required}"
parallel="${FLOWPARALLEL_BIN:?FLOWPARALLEL_BIN is required}"
optimize="${FLOWOPTIMIZE_BIN:?FLOWOPTIMIZE_BIN is required}"
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
fixture="${root}/Flowmini/flowmini_v25_symboltable_projection/examples/ast/parallel_resource_probe.flow"
tmpdir="$(mktemp -d)"
trap 'rm -rf "${tmpdir}"' EXIT

"${flowmini}" --dump-frontend-bundle "${fixture}" > "${tmpdir}/bundle.json"
"${analyst}" --lowering-plan-version 2 < "${tmpdir}/bundle.json" > "${tmpdir}/semantic.json"
"${inspect}" "${tmpdir}/semantic.json" > "${tmpdir}/semantic.txt"
grep -F 'format:   flowanalyst.semantic_report' "${tmpdir}/semantic.txt" >/dev/null
grep -F 'parallel candidates:   3' "${tmpdir}/semantic.txt" >/dev/null
grep -F 'external-effect: 7' "${tmpdir}/semantic.txt" >/dev/null
cmp -s "${tmpdir}/semantic.txt" <("${inspect}" "${tmpdir}/semantic.json")

"${parallel}" "${tmpdir}/semantic.json" > "${tmpdir}/plan.json"
"${inspect}" "${tmpdir}/plan.json" > "${tmpdir}/plan.txt"
grep -F 'format:   flowparallel.execution_plan' "${tmpdir}/plan.txt" >/dev/null
grep -F 'fallback:              cpu.serial' "${tmpdir}/plan.txt" >/dev/null

"${optimize}" < "${tmpdir}/semantic.json" > "${tmpdir}/optimized.json"
"${inspect}" "${tmpdir}/optimized.json" | grep -F 'format:   flowoptimize.optimization_report' >/dev/null

printf '%s\n' '{"format":"wrong","version":1,"status":"ok"}' > "${tmpdir}/unsupported.json"
set +e
"${inspect}" "${tmpdir}/unsupported.json" > /dev/null 2> "${tmpdir}/unsupported.err"
rc=$?
set -e
test "${rc}" -eq 3
grep -F "unsupported artifact format 'wrong'" "${tmpdir}/unsupported.err" >/dev/null

printf '%s\n' '{"format":"flowanalyst.semantic_report","version":7,"status":"ok"}' > "${tmpdir}/version.json"
set +e
"${inspect}" "${tmpdir}/version.json" > /dev/null 2> "${tmpdir}/version.err"
rc=$?
set -e
test "${rc}" -eq 3
grep -F 'unsupported flowanalyst.semantic_report version 7' "${tmpdir}/version.err" >/dev/null

printf '%s\n' '{"format":"flowanalyst.semantic_report","version":1,"status":"ok"' > "${tmpdir}/malformed.json"
set +e
"${inspect}" "${tmpdir}/malformed.json" > /dev/null 2> "${tmpdir}/malformed.err"
rc=$?
set -e
test "${rc}" -eq 2

: > "${tmpdir}/empty.json"
set +e
"${inspect}" "${tmpdir}/empty.json" > /dev/null 2> "${tmpdir}/empty.err"
rc=$?
set -e
test "${rc}" -eq 2

printf '%s\n' '{"format":"flowanalyst.semantic_report","version":1,"status":true}' > "${tmpdir}/wrong-type.json"
set +e
"${inspect}" "${tmpdir}/wrong-type.json" > /dev/null 2> "${tmpdir}/wrong-type.err"
rc=$?
set -e
test "${rc}" -eq 2

printf '%s\n' '{"format":"flowanalyst.semantic_report","version":1}' > "${tmpdir}/missing-status.json"
set +e
"${inspect}" "${tmpdir}/missing-status.json" > /dev/null 2> "${tmpdir}/missing-status.err"
rc=$?
set -e
test "${rc}" -eq 2

printf '%s\n' '{"format":"flowmini.frontend_bundle","version":2,"source":{"path":"fixture.flow"},"ast":{"declaration_pool":[],"expression_pool":[]},"symbol_table":{"symbols":[],"scopes":[]}}' > "${tmpdir}/frontend.json"
"${inspect}" "${tmpdir}/frontend.json" | grep -F 'format:   flowmini.frontend_bundle' >/dev/null

printf '%s\n' '{"format":"flowcore.lowering_plan","version":1,"operations":[{"id":1}]}' > "${tmpdir}/lowering.json"
"${inspect}" "${tmpdir}/lowering.json" | grep -F 'format:   flowcore.lowering_plan' >/dev/null

echo "flowinspect tests: PASS"
echo "  semantic reports, execution plans, frontend bundles, malformed input, and version rejection"
