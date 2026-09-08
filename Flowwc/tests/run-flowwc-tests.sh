#!/usr/bin/env bash
set -euo pipefail

flowmini="${FLOWMINI_BIN:?FLOWMINI_BIN is required}"
flowanalyst="${FLOWANALYST_BIN:?FLOWANALYST_BIN is required}"
source_file="${FLOWWC_SOURCE:?FLOWWC_SOURCE is required}"
stdin_source="${FLOWWC_STDIN_SOURCE:?FLOWWC_STDIN_SOURCE is required}"
test -x "${flowmini}"
test -x "${flowanalyst}"
test -f "${source_file}"
test -f "${stdin_source}"

run_path_case() {
    local name="$1" input="$2" expected="$3" actual tmp
    tmp="$(mktemp)"
    trap 'rm -f "${tmp}"' RETURN
    printf '%s' "${input}" >"${tmp}"
    actual="$(${flowmini} "${source_file}" "${tmp}")"
    if [[ "${actual}" != "${expected}" ]]; then
        printf 'flowwc path case %s: expected %q, got %q\n' "${name}" "${expected}" "${actual}" >&2
        exit 1
    fi
}

run_stdin_case() {
    local name="$1" input="$2" expected="$3" actual
    actual="$(printf '%s' "${input}" | "${flowmini}" "${stdin_source}")"
    if [[ "${actual}" != "${expected}" ]]; then
        printf 'flowwc stdin case %s: expected %q, got %q\n' "${name}" "${expected}" "${actual}" >&2
        exit 1
    fi
}

run_binary_case() {
    local name="$1" expected="$2" actual tmp
    tmp="$(mktemp)"
    trap 'rm -f "${tmp}"' RETURN
    printf 'a\0b' >"${tmp}"
    actual="$(${flowmini} "${source_file}" "${tmp}")"
    if [[ "${actual}" != "${expected}" ]]; then
        printf 'flowwc binary case %s: expected %q, got %q\n' "${name}" "${expected}" "${actual}" >&2
        exit 1
    fi
}

run_path_case empty '' $'0\n0\n0'
run_path_case one-word 'hello' $'0\n1\n5'
run_path_case mixed $'hello world\nsecond\tline' $'1\n4\n23'
run_path_case whitespace $' \t\n' $'1\n0\n3'
run_path_case utf8 'æ' $'0\n1\n2'
run_path_case no-final-newline $'one\ntwo' $'1\n2\n7'
run_binary_case embedded-nul $'0\n2\n3'

large="$(mktemp)"
trap 'rm -f "${large}"' RETURN
dd if=/dev/zero of="${large}" bs=256 count=1 status=none
large_actual="$(${flowmini} "${source_file}" "${large}")"
[[ "${large_actual}" == $'0\n0\n256' ]] || {
    printf 'flowwc large-file case: expected %q, got %q\n' $'0\n0\n256' "${large_actual}" >&2
    exit 1
}

missing="$(mktemp)"
rm -f "${missing}"
if "${flowmini}" "${source_file}" "${missing}" >/dev/null 2>/dev/null; then
    echo 'flowwc missing-file case unexpectedly succeeded' >&2
    exit 1
fi

run_stdin_case regression $'alpha beta\ngamma' $'1\n3\n16'

first="$(printf '%s' $'alpha beta\ngamma' | "${flowmini}" "${stdin_source}")"
second="$(printf '%s' $'alpha beta\ngamma' | "${flowmini}" "${stdin_source}")"
test "${first}" = "${second}"

bundle="$(mktemp)"
report="$(mktemp)"
trap 'rm -f "${bundle}" "${report}"' EXIT
"${flowmini}" --dump-frontend-bundle "${source_file}" >"${bundle}"
"${flowanalyst}" "${bundle}" >"${report}"
jq -e '.external_operations[] | select(.callee == "file.bytes" and .provider_contract == "flowcore.filesystem" and .effect_class == "filesystem.read")' "${report}" >/dev/null

echo "flowwc Flowmini application tests: PASS"
