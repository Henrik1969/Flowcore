#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
flowmini="${FLOWMINI_BIN:-${root}/cmake-build-debug/flowmini}"
analyst="${FLOWANALYST_BIN:-${root}/../../build/flowanalyst/flowanalyst}"
optimizer="${FLOWOPTIMIZE_BIN:-${root}/../../build/flowoptimize/flowoptimize}"
lowerer="${FLOWLOWER_BIN:-${root}/../../build/flowlower/flowlower}"
fixture="${root}/examples/ast/guard_backend_probe.flow"
tmpdir="$(mktemp -d)"
trap 'rm -rf "${tmpdir}"' EXIT

"${flowmini}" --dump-frontend-bundle "${fixture}" > "${tmpdir}/bundle.json"
"${analyst}" --lowering-plan-version 2 "${tmpdir}/bundle.json" > "${tmpdir}/semantic.json"
jq -e 'any(.lowering_plan.operations[]; .kind == "guard" and (.failure_block_id | type == "number"))' "${tmpdir}/semantic.json" >/dev/null
"${optimizer}" "${tmpdir}/semantic.json" > "${tmpdir}/optimization.json"
"${lowerer}" --emit-llvm "${tmpdir}/guard.ll" "${tmpdir}/optimization.json" > "${tmpdir}/lowering.json"
jq -e '.status == "ready" and .backend.name == "llvm"' "${tmpdir}/lowering.json" >/dev/null
clang "${tmpdir}/guard.ll" -o "${tmpdir}/guard"
set +e
"${tmpdir}/guard" >/dev/null
rc=$?
set -e
test "${rc}" -eq 2
echo "Flow canonical guard backend: PASS"
