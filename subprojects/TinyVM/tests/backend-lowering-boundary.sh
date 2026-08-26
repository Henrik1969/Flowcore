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

jq '.lowering_plan.operations = [{"id":0,"kind":"call","block_id":0,"statement_id":0,"operands":[]}]' "$fixture" > "$build/nonempty.json"
rm -f "$build/nonempty.tvm"
set +e
"$lower" "$build/nonempty.json" "$build/nonempty.tvm" > "$build/unsupported.json"
status=$?
set -e
test "$status" -eq 2
grep -q '"status":"unsupported"' "$build/unsupported.json"
test ! -e "$build/nonempty.tvm"

echo 'TinyVM backend lowering boundary: PASS'
