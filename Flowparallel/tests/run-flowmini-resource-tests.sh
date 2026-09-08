#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
flowmini="${FLOWMINI_BIN:?FLOWMINI_BIN is required}"
analyst="${FLOWANALYST_BIN:?FLOWANALYST_BIN is required}"
parallel="${FLOWPARALLEL_BIN:?FLOWPARALLEL_BIN is required}"
fixture="${FLOWMINI_RESOURCE_FIXTURE:-${root}/Flowmini/flowmini_v25_symboltable_projection/examples/ast/parallel_resource_probe.flow}"
tmpdir="$(mktemp -d)"
trap 'rm -rf "${tmpdir}"' EXIT

"${flowmini}" --dump-frontend-bundle "${fixture}" | "${analyst}" --lowering-plan-version 2 > "${tmpdir}/semantic.json"
jq -e '
  .status == "ok" and
  ([.parallel_candidates[]? | select(.proof_status == "proven")] | length) == 3 and
  ([.parallel_candidates[].callee] | sort) == ["cube", "identity", "square"] and
  any(.lowering_plan.operations[]?; .kind == "generic_call" and .instantiation_id == "identity<int>") and
  ([.external_operations[]? | select(.callee == "sendfile") | .resource_uses[]? | .resource_identity] as $ids | ($ids | length) == 3 and $ids[0] == $ids[1] and $ids[2] != $ids[0]) and
  any(.external_operations[]?; .callee == "sendfile" and any(.resource_uses[]?; .access == "read_write" and .alias_status == "symbol-derived" and .concurrency == "unknown")) and
  any(.parallel_rejections[]?; .reason == "resource-write-write-conflict" and .fallback == "serial") and
  any(.parallel_rejections[]?; .reason == "resource-alias-unknown" and .fallback == "serial")
' "${tmpdir}/semantic.json" >/dev/null

"${parallel}" "${tmpdir}/semantic.json" > "${tmpdir}/plan.json"
jq -e '
  .status == "ready" and .dependency_analysis.parallel_candidates == 3 and
  ([.parallel_candidates[].callee] | sort) == ["cube", "identity", "square"] and
  ([.parallel_candidates[]? | select(.proof_status == "proven")] | length) == 3 and
  any(.parallel_rejections[]?; .reason == "resource-write-write-conflict" and .fallback == "serial") and
  any(.parallel_rejections[]?; .reason == "resource-alias-unknown" and .fallback == "serial")
' "${tmpdir}/plan.json" >/dev/null

echo "Flowmini resource/effect evidence: PASS"
echo "  pure computation: proven candidate"
echo "  same read/write resource: serial fallback"
echo "  unresolved resource alias: serial fallback"
