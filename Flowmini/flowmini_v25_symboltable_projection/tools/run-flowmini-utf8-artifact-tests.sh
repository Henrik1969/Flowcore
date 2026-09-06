#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
bin="${FLOWMINI_UTF8_ARTIFACT_BIN:-${root}/cmake-build-debug/flowmini_utf8_artifact}"
fixtures="${root}/../../docs/bootstrap/fixtures"
tmpdir="$(mktemp -d)"
trap 'rm -rf "${tmpdir}"' EXIT

printf '\101\303\246' | "${bin}" > "${tmpdir}/valid.json"
cmp "${tmpdir}/valid.json" "${fixtures}/utf8-source-valid-v1.json"

printf '\101\300\257\102' | "${bin}" > "${tmpdir}/malformed.json"
cmp "${tmpdir}/malformed.json" "${fixtures}/utf8-source-malformed-v1.json"

echo "UTF-8 artifact fixtures: PASS"
