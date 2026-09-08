#!/usr/bin/env bash
set -euo pipefail

flowmini="${FLOWMINI_BIN:?FLOWMINI_BIN is required}"
source_file="${FLOWWC_SOURCE:?FLOWWC_SOURCE is required}"
test -x "${flowmini}"
test -f "${source_file}"

run_case() {
    local name="$1" input="$2" expected="$3" actual
    actual="$(printf '%s' "${input}" | "${flowmini}" "${source_file}")"
    if [[ "${actual}" != "${expected}" ]]; then
        printf 'flowwc case %s: expected %q, got %q\n' "${name}" "${expected}" "${actual}" >&2
        exit 1
    fi
}

run_case empty '' $'0\n0\n0'
run_case one-word 'hello' $'0\n1\n5'
run_case mixed $'hello world\nsecond\tline' $'1\n4\n23'
run_case whitespace $' \t\n' $'1\n0\n3'
run_case utf8 'æ' $'0\n1\n2'
run_case no-final-newline $'one\ntwo' $'1\n2\n7'

first="$(printf '%s' $'alpha beta\ngamma' | "${flowmini}" "${source_file}")"
second="$(printf '%s' $'alpha beta\ngamma' | "${flowmini}" "${source_file}")"
test "${first}" = "${second}"

echo "flowwc Flowmini application tests: PASS"
