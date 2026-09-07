#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
flowmini="${FLOWMINI_BIN:-${root}/cmake-build-debug/flowmini}"
analyst="${FLOWANALYST_BIN:-${root}/../../build/flowanalyst/flowanalyst}"
fixture="${root}/examples/ast/const_compile_time_probe.flow"
tmpdir="$(mktemp -d)"
trap 'rm -rf "${tmpdir}"' EXIT

test "$("${flowmini}" --runtime-compat "${fixture}")" = "30"
"${flowmini}" --dump-frontend-bundle "${fixture}" > "${tmpdir}/bundle.json"
jq -e '
  ([.ast.statement_pool[] | select(.kind == "let" and .payload.is_const == true) | .payload.compile_time_value] == ["10", "15", "30"]) and
  ([.symbol_table.symbols[]?.facts[]? | select(.key == "compile_time_value") | .value.value] | sort) == ["10", "15", "30"]
' "${tmpdir}/bundle.json" >/dev/null
"${analyst}" --lowering-plan-version 2 "${tmpdir}/bundle.json" > "${tmpdir}/semantic.json"
jq -e '[.lowering_plan.operations[] | select(.kind == "value_definition") | .compile_time_value] == ["10", "15", "30"]' "${tmpdir}/semantic.json" >/dev/null

for failure in bad_const_runtime_dependency bad_const_overflow bad_const_provider; do
  if "${flowmini}" --runtime-compat "${root}/examples/fail/${failure}.flow" >/dev/null 2>&1; then
    echo "${failure} was accepted" >&2
    exit 1
  fi
done
echo "Flow compile-time constant evaluation: PASS"
