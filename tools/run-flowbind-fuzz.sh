#!/bin/sh
set -eu

bin=${FLOWBIND_BIN:?FLOWBIND_BIN is required}
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT

case_number=0
while [ "$case_number" -lt 128 ]; do
    input=$tmpdir/input
    output=$tmpdir/output
    case_number=$((case_number + 1))
    case "$case_number" in
        1) printf '' > "$input" ;;
        2) printf '{' > "$input" ;;
        3) printf '{"format":"flowanalyst.semantic_report"' > "$input" ;;
        4) printf '{"format":"flowanalyst.semantic_report","version":1,"status":"ok","binding_requirements":null}' > "$input" ;;
        *) dd if=/dev/urandom of="$input" bs=256 count=1 status=none ;;
    esac
    set +e
    timeout 2 "$bin" "$input" > "$output" 2>&1
    result=$?
    set -e
    case "$result" in
        0|1|2) ;;
        *) echo "Flowbind fuzz failure: case $case_number exited $result" >&2; exit 1 ;;
    esac
done

echo "Flowbind fuzz: 128 malformed/random inputs classified without crash"
