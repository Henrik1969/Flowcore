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
    jq -e '.graph_syntax | .format == "flowmini.graph_syntax" and .version == 1 and
        (.nodes | length) == 4 and (.wires | length) == 3 and
        .nodes[1].implementation_name == "pager.navigate" and
        .wires[0].wire_id == "wire:0" and
        .wires[0].from.node_id == "source" and .wires[0].from.port_id == "out" and
        .wires[0].to.node_id == "navigate" and .wires[0].to.port_id == "in" and
        .wires[0].provenance.line == 12' "$tmpdir/bundle.json" >/dev/null
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

# A receiver reference is distinct from a provider atom and a function definition.
cat > "$tmpdir/receiver.flow" <<'EOF'
program arbitrary_receiver
node receiver : fn transform
wire source.out => receiver.in
fn transform(value : int): int {
    return value + 1
}
main { return 0 }
EOF
"$FLOWMINI_BIN" --dump-frontend-bundle "$tmpdir/receiver.flow" > "$tmpdir/receiver.json"
jq -e '.graph_syntax.nodes[0] | .implementation_kind == "source_function" and
    .implementation_name == "transform" and .provenance.line == 2' "$tmpdir/receiver.json" >/dev/null
if "$FLOWANALYST_BIN" --lowering-plan-version 2 < "$tmpdir/receiver.json" > "$tmpdir/receiver.semantic.json"; then
    echo 'syntax-only receiver unexpectedly executable' >&2; exit 1
fi
jq -e '.graph_analysis.receivers[0] as $r |
    $r.node_id == "receiver" and $r.activation_contract == "fresh_single_input_v1" and
    any(.lowering_plan.functions[]; .symbol_id == $r.function_symbol_id and
        .parameters[0].symbol_id == $r.parameter_symbol_id and .name == "transform")' \
    "$tmpdir/receiver.semantic.json" >/dev/null
for mutation in '.diagnostics = []' '.diagnostics = [] | .graph_syntax.version = 99' \
    '.diagnostics = [] | .graph_syntax.nodes += [.graph_syntax.nodes[0]]' \
    '.diagnostics = [] | .graph_syntax.nodes[0].implementation_name = "absent"'; do
    jq "$mutation" "$tmpdir/receiver.json" > "$tmpdir/mutated.json"
    if "$FLOWANALYST_BIN" < "$tmpdir/mutated.json" > "$tmpdir/mutated.semantic.json"; then
        echo "hostile captured graph admitted: $mutation" >&2; exit 1
    fi
    jq -e '.lowering_plan.status == "blocked" and
        any(.diagnostics[]; .code == "FLOWANALYST_GRAPH_EXECUTION_UNSUPPORTED")' \
        "$tmpdir/mutated.semantic.json" >/dev/null
done
for declaration in 'node x : fn' 'wire a.out -> b.in' 'wire a.out => b' 'node x : provider extra'; do
    printf 'program malformed\n%s\nmain { return 0 }\n' "$declaration" > "$tmpdir/malformed.flow"
    if "$FLOWMINI_BIN" --dump-frontend-bundle "$tmpdir/malformed.flow" > /dev/null 2>&1; then
        echo "malformed graph declaration accepted: $declaration" >&2; exit 1
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
