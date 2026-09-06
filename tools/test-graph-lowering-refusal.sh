#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT

# Graph interpretation is supported, but exporting only the marker in main
# would erase the behavior. A fresh consumer must reject that captured bundle.
source="$root/Flowmini/flowmini_v25_symboltable_projection/examples/apps/flow_less/flow_less.flow"
sed 's/program flow_less/program unrelated_graph_name/' "$source" > "$tmpdir/renamed.flow"
for input in "$source" "$tmpdir/renamed.flow"; do
    "$FLOWMINI_BIN" --dump-frontend-bundle "$input" > "$tmpdir/bundle.json"
    jq -e '.source_map.files[0].path as $source | any(.diagnostics[];
        .code == "FLOWMINI_GRAPH_LOWERING_UNSUPPORTED" and
        .provenance.source == $source and .provenance.line == 3 and
        .provenance.column == 1)' "$tmpdir/bundle.json" >/dev/null
    if "$FLOWANALYST_BIN" < "$tmpdir/bundle.json" > "$tmpdir/semantic.json"; then
        echo 'graph projection unexpectedly admitted' >&2; exit 1
    fi
    jq -e '.status == "error" and .lowering_plan.status == "blocked" and
        any(.diagnostics[]; .code == "FLOWMINI_GRAPH_LOWERING_UNSUPPORTED" and
            .provenance.line == 3 and .provenance.column == 1)' "$tmpdir/semantic.json" >/dev/null
    if "$FLOWPARALLEL_BIN" < "$tmpdir/semantic.json" > "$tmpdir/parallel.json"; then
        echo 'blocked graph projection unexpectedly scheduled' >&2; exit 1
    fi
done

# Lexer-derived refusal must not confuse comments or strings with graph syntax.
cat > "$tmpdir/scalar.flow" <<'EOF'
program scalar_words
// producer source : example; wire a.out => b.in
main {
    text : string("node wire policy =>")
    return 0
}
EOF
"$FLOWMINI_BIN" --dump-frontend-bundle "$tmpdir/scalar.flow" > "$tmpdir/scalar.json"
jq -e '.diagnostics == []' "$tmpdir/scalar.json" >/dev/null
"$FLOWANALYST_BIN" < "$tmpdir/scalar.json" > "$tmpdir/scalar.semantic.json"
jq -e '.status == "ok" and .lowering_plan.status == "ready"' "$tmpdir/scalar.semantic.json" >/dev/null
echo 'Graph projection refusal with source provenance: PASS'
