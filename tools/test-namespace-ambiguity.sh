#!/bin/sh
set -eu

flowmini=${FLOWMINI_BIN:?FLOWMINI_BIN is required}
analyst=${FLOWANALYST_BIN:?FLOWANALYST_BIN is required}
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT

for namespace in first second third; do
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

cat > "$tmpdir/ambiguous.flow" <<EOF
import "$tmpdir/first.flow"
import "$tmpdir/second.flow"
import "$tmpdir/third.flow"

program triple_provider_ambiguity

main {
    status : c_int(0)
    shared("ambiguous") -> status
}
EOF

set +e
"$flowmini" --dump-frontend-bundle "$tmpdir/ambiguous.flow" |
    "$analyst" > "$tmpdir/ambiguous.json"
status=$?
set -e
test "$status" -eq 2
jq -e '.status == "error" and any(.diagnostics[]; .code == "FLOWANALYST_AMBIGUOUS_NAME" and (.message | contains("multiple imported contracts")))' "$tmpdir/ambiguous.json" >/dev/null

cat > "$tmpdir/qualified.flow" <<EOF
import "$tmpdir/first.flow" as first
import "$tmpdir/second.flow" as second
import "$tmpdir/third.flow" as third

program triple_provider_qualified

main {
    status : c_int(0)
    first.shared("first") -> status
    second.shared("second") -> status
    third.shared("third") -> status
}
EOF
"$flowmini" --dump-frontend-bundle "$tmpdir/qualified.flow" |
    "$analyst" > "$tmpdir/qualified.json"
jq -e '.status == "ok" and (.diagnostics | length) == 0 and ((.lowering_plan.operations | map(select(.callee == "first.shared" or .callee == "second.shared" or .callee == "third.shared")) | length) == 3)' "$tmpdir/qualified.json" >/dev/null

printf '%s\n' 'Namespace ambiguity (three providers): PASS'
