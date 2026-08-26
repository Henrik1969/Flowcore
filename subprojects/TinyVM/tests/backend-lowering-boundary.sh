#!/bin/sh
set -eu

lower=$1
validate=$2
run=$3
fixture=$4
build=$5
artifact="$build/backend-empty.tvm"
artifact_again="$build/backend-empty-again.tvm"
report="$build/backend-empty-report.json"

"$lower" "$fixture" "$artifact" > "$report"
"$lower" "$fixture" "$artifact_again" >/dev/null
cmp -s "$artifact" "$artifact_again"
grep -q '"status":"emitted"' "$report"
"$validate" "$artifact" | grep -q '"status":"valid"'
"$run" "$artifact" | grep -q '"carrier":2,"result":0'

jq '.lowering_plan.operations = [{"id":0,"kind":"switch","block_id":0,"statement_id":0,"operands":[]}]' "$fixture" > "$build/nonempty.json"
rm -f "$build/nonempty.tvm"
set +e
"$lower" "$build/nonempty.json" "$build/nonempty.tvm" > "$build/unsupported.json"
status=$?
set -e
test "$status" -eq 2
grep -q '"status":"unsupported"' "$build/unsupported.json"
test ! -e "$build/nonempty.tvm"

for value in \
  '{"kind":"string_literal","type":"c_string","value":"captured"}' \
  '{"kind":"writable_storage","type":"c_pointer","storage":{"bytes":16,"access":"read_write","lifetime":"call"}}'
do
  jq --argjson value "$value" '.lowering_plan.operations = [{"id":0,"kind":"value_definition","block_id":0,"statement_id":0,"result_symbol_id":1,"operands":[$value]}]' "$fixture" > "$build/opaque-value.json"
  "$lower" "$build/opaque-value.json" "$build/opaque-value.tvm" >/dev/null
  "$validate" "$build/opaque-value.tvm" | grep -q '"status":"valid"'
  "$run" "$build/opaque-value.tvm" | grep -q '"carrier":2,"result":0'
done

echo 'TinyVM backend lowering boundary: PASS'
