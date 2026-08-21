#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
bin=${FLOWBIND_BIN:?FLOWBIND_BIN is required}
flowmini=${FLOWMINI_BIN:-$root/Flowmini/flowmini_v25_symboltable_projection/cmake-build-debug/flowmini}
flowanalyst=${FLOWANALYST_BIN:-$root/Flowanalyst/build/flowanalyst}
fixture=$root/Flowmini/flowmini_v25_symboltable_projection/examples/pass/abi_libc_demo.flow
test -x "$flowmini"
test -x "$flowanalyst"
test -x "$bin"
policy=$(mktemp)
trap 'rm -f "$policy"' EXIT
printf '%s\n' \
  'allow libc.so.6 strlen c pure c_string c_size_t' \
  'allow libc.so.6 abs c pure' \
  'allow libc.so.6 labs c pure' \
  'allow libc.so.6 puts c io' \
  'allow libc.so.6 open c io' \
  'allow libc.so.6 read c io' \
  'allow libc.so.6 write c io' \
  'allow libc.so.6 close c io' \
  'allow libc.so.6 flowcore_symbol_that_does_not_exist c pure' > "$policy"

report=$("$flowmini" --dump-frontend-bundle "$fixture" | "$flowanalyst" | "$bin" --policy "$policy")
printf '%s\n' "$report" | grep -q '"status": "ready"'
printf '%s\n' "$report" | grep -q '"execution": "not-performed"'
printf '%s\n' "$report" | grep -q '"carrier_types_supported": true'
printf '%s\n' "$report" | grep -q '"provider_signature_evidence": "not-provided"'

set +e
wrong_signature=$(printf '%s' '{"format":"flowanalyst.semantic_report","version":1,"status":"ok","binding_requirements":[{"contract":"bad","library":"libc.so.6","convention":"c","symbol":"strlen","effect":"pure","parameter_types":"c_string","return_type":"c_int"}]}' | "$bin" --policy "$policy")
wrong_signature_rc=$?
set -e
test "$wrong_signature_rc" -eq 2
printf '%s\n' "$wrong_signature" | grep -q 'denied by capability policy'

file_fixture=$root/Flowmini/flowmini_v25_symboltable_projection/examples/apps/flowcat/flowcat.flow
file_report=$(
  "$flowmini" --dump-frontend-bundle "$file_fixture" |
  "$flowanalyst" |
  "$bin" --policy "$policy"
)
printf '%s\n' "$file_report" | jq -e '.status == "ready" and .lowering_profile == "flowcat_file_main" and .lowering_plan.kind == "capability_sequence" and (.symbols | index("open")) != null and (.symbols | index("read")) != null and (.symbols | index("write")) != null and (.symbols | index("close")) != null' >/dev/null

set +e
bad=$(printf '%s' '{"format":"flowanalyst.semantic_report","version":1,"status":"ok","binding_requirements":[{"contract":"bad","library":"libc.so.6","convention":"c","symbol":"flowcore_symbol_that_does_not_exist","effect":"pure","parameter_types":"","return_type":"c_int"}]}' | "$bin" --policy "$policy")
bad_rc=$?
set -e
test "$bad_rc" -eq 2
printf '%s\n' "$bad" | grep -q 'unavailable'

set +e
bad_type=$(printf '%s' '{"format":"flowanalyst.semantic_report","version":1,"status":"ok","binding_requirements":[{"contract":"bad","library":"libc.so.6","convention":"c","symbol":"abs","effect":"pure","parameter_types":"not_an_abi_type","return_type":"c_int"}]}' | "$bin" --policy "$policy")
bad_type_rc=$?
set -e
test "$bad_type_rc" -eq 2
printf '%s\n' "$bad_type" | grep -q 'unsupported parameter ABI type'

set +e
denied=$(printf '%s' '{"format":"flowanalyst.semantic_report","version":1,"status":"ok","binding_requirements":[{"contract":"bad","library":"libc.so.6","convention":"c","symbol":"abs","effect":"pure","parameter_types":"c_int","return_type":"c_int"}]}' | "$bin")
denied_rc=$?
set -e
test "$denied_rc" -eq 2
printf '%s\n' "$denied" | grep -q 'denied by capability policy'
echo 'Flowbind tests: PASS'
