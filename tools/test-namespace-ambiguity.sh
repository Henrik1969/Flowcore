#!/bin/sh
set -eu

flowmini=${FLOWMINI_BIN:?FLOWMINI_BIN is required}
analyst=${FLOWANALYST_BIN:?FLOWANALYST_BIN is required}
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT

for namespace in first second third fourth; do
    cat > "$tmpdir/$namespace.flow" <<EOF
unit $namespace

abi $namespace {
    library "libc.so.6"
    convention c
    type ${namespace}_string {
        repr "const char*"
        ownership borrowed
        access read
        lifetime call
        nullable false
        terminator nul
    }
    extern fn shared(text : ${namespace}_string): c_int {
        symbol "puts"
        effect io
    }
}
EOF
done

cat > "$tmpdir/single.flow" <<EOF
import "$tmpdir/first.flow"

program single_provider_compatibility

main {
    status : c_int(0)
    shared("single") -> status
}
EOF
"$flowmini" --dump-frontend-bundle "$tmpdir/single.flow" |
    "$analyst" > "$tmpdir/single.json"
jq -e '.status == "ok" and any(.lowering_plan.operations[]; .kind == "call" and .callee == "shared")' "$tmpdir/single.json" >/dev/null

for count in 2 3 4; do
    imports="import \"$tmpdir/first.flow\"\nimport \"$tmpdir/second.flow\""
    test "$count" -lt 3 || imports="$imports\nimport \"$tmpdir/third.flow\""
    test "$count" -lt 4 || imports="$imports\nimport \"$tmpdir/fourth.flow\""
    printf '%b\n' "$imports" > "$tmpdir/ambiguous-$count.flow"
    cat >> "$tmpdir/ambiguous-$count.flow" <<EOF

program provider_ambiguity_$count

main {
    status : c_int(0)
    shared("ambiguous") -> status
}
EOF

    set +e
    "$flowmini" --dump-frontend-bundle "$tmpdir/ambiguous-$count.flow" |
        "$analyst" > "$tmpdir/ambiguous-$count.json"
    status=$?
    set -e
    test "$status" -eq 2
    jq -e '.status == "error" and any(.diagnostics[]; .code == "FLOWANALYST_AMBIGUOUS_NAME" and (.message | contains("multiple imported contracts")))' "$tmpdir/ambiguous-$count.json" >/dev/null
done

cat > "$tmpdir/qualified.flow" <<EOF
import "$tmpdir/first.flow" as first
import "$tmpdir/second.flow" as second
import "$tmpdir/third.flow" as third
import "$tmpdir/fourth.flow" as fourth

program four_provider_qualified

main {
    status : c_int(0)
    first.shared("first") -> status
    second.shared("second") -> status
    third.shared("third") -> status
    fourth.shared("fourth") -> status
}
EOF
"$flowmini" --dump-frontend-bundle "$tmpdir/qualified.flow" |
    "$analyst" > "$tmpdir/qualified.json"
jq -e '.status == "ok" and (.diagnostics | length) == 0 and ((.lowering_plan.operations | map(select(.callee == "first.shared" or .callee == "second.shared" or .callee == "third.shared" or .callee == "fourth.shared")) | length) == 4)' "$tmpdir/qualified.json" >/dev/null

printf '%s\n' 'Namespace ambiguity (one through four providers): PASS'
