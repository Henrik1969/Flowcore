#!/bin/sh
set -eu
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT
cat > "$tmpdir/graph.flow" <<'FLOW'
program graph_capture
producer source : stdin.text
node receiver : fn transform
wire source.out => receiver.in
policy source.label = "wire => stays text"
policy source.limit = -7
policy source.enabled = true
fn transform(value : c_string): int {
    return 42
}
main { return 0 }
FLOW
"$FLOWMINI_BIN" --dump-frontend-bundle "$tmpdir/graph.flow" > "$tmpdir/frontend.json"
if "$FLOWANALYST_BIN" --lowering-plan-version 2 < "$tmpdir/frontend.json" > "$tmpdir/semantic.json"; then
    echo 'native graph unexpectedly admitted' >&2; exit 1
fi
jq '.lowering_plan.source_graph' "$tmpdir/semantic.json" > "$tmpdir/graph.json"
jq -e --slurpfile front "$tmpdir/frontend.json" '.syntax == $front[0].graph_syntax and
    .syntax.policies[0].value_kind == "string" and .syntax.policies[0].value_text == "wire => stays text" and
    .syntax.policies[1].value_kind == "integer" and .syntax.policies[1].value_text == "-7" and
    .syntax.policies[2].value_kind == "boolean" and .syntax.policies[2].value_text == "true" and
    .syntax.policies[0].provenance.line == 5 and .receivers[0].node_id == "receiver"' "$tmpdir/graph.json" >/dev/null
# Each invocation reads a captured artifact after the producer has exited.
"$FLOWVALIDATE_BIN" "$tmpdir/graph.json" | jq -e '.classification == "valid"' >/dev/null
"$FLOWVALIDATE_BIN" --canonical "$tmpdir/graph.json" > "$tmpdir/canonical.json"
"$FLOWVALIDATE_BIN" --canonical "$tmpdir/canonical.json" | cmp -s - "$tmpdir/canonical.json"
for mutation in \
    '.version = 99' \
    '.status = "ready"' \
    '.syntax.version = 99' \
    '.syntax.nodes += [.syntax.nodes[0]]' \
    '.syntax.wires += [.syntax.wires[0]]' \
    '.syntax.wires[0].to.port_id = "wrong"' \
    '.syntax.wires[0].from.node_id = "absent"' \
    '.syntax.wires[0].provenance.line = 0' \
    '.syntax.policies += [.syntax.policies[0]]' \
    '.syntax.policies[0].node_id = "absent"' \
    '.syntax.policies[1].value_text = "9223372036854775808"' \
    '.syntax.policies[2].value_text = "1"' \
    'del(.syntax.policies)' \
    '.receivers[0].function_symbol_id = -1' \
    '.receivers[0].activation_contract = "persistent"' \
    '.receivers = []'
do
    jq "$mutation" "$tmpdir/graph.json" > "$tmpdir/mutated.json"
    if "$FLOWVALIDATE_BIN" "$tmpdir/mutated.json" > "$tmpdir/validation.json"; then
        echo "hostile source graph accepted: $mutation" >&2; exit 1
    fi
    jq -e '.classification == "invalid"' "$tmpdir/validation.json" >/dev/null
done
# Generate otherwise valid artifacts and attach the retained non-executable graph.
# Outer status laundering cannot erase the graph at any independent consumer.
printf 'program scalar\nmain { return 0 }\n' > "$tmpdir/scalar.flow"
"$FLOWMINI_BIN" --dump-frontend-bundle "$tmpdir/scalar.flow" > "$tmpdir/scalar.frontend.json"
"$FLOWANALYST_BIN" --lowering-plan-version 2 < "$tmpdir/scalar.frontend.json" > "$tmpdir/scalar.semantic.json"
"$FLOWPARALLEL_BIN" < "$tmpdir/scalar.semantic.json" > "$tmpdir/scalar.execution.json"
"$FLOWOPTIMIZE_BIN" < "$tmpdir/scalar.execution.json" > "$tmpdir/scalar.optimization.json"
for artifact in semantic execution optimization; do
    jq --slurpfile graph "$tmpdir/graph.json" '.lowering_plan.source_graph = $graph[0]' "$tmpdir/scalar.$artifact.json" > "$tmpdir/forged.$artifact.json"
done
refuse() {
    input=$1
    shift
    if "$@" < "$input" > "$tmpdir/result" 2> "$tmpdir/error"; then
        echo "source graph was silently dropped by $*" >&2; exit 1
    fi
    cat "$tmpdir/result" "$tmpdir/error" | grep -Fq 'source graph execution is not admitted'
}
refuse "$tmpdir/forged.semantic.json" "$FLOWPARALLEL_BIN"
refuse "$tmpdir/forged.semantic.json" "$FLOWBIND_BIN"
refuse "$tmpdir/forged.semantic.json" "$FLOWOPTIMIZE_BIN"
refuse "$tmpdir/forged.execution.json" "$FLOWOPTIMIZE_BIN"
refuse "$tmpdir/forged.optimization.json" "$FLOWLOWER_BIN"
refuse "$tmpdir/forged.optimization.json" "$FLOWPREPARE_BIN"
refuse "$tmpdir/forged.optimization.json" "$FLOWLOWER_BIN" --emit-llvm "$tmpdir/forged.ll"
test ! -e "$tmpdir/forged.ll"
"$FLOWPREPARE_BIN" < "$tmpdir/scalar.optimization.json" > "$tmpdir/backend.json"
jq --slurpfile graph "$tmpdir/graph.json" '.lowering_plan.source_graph = $graph[0]' "$tmpdir/backend.json" > "$tmpdir/forged.backend.json"
refuse "$tmpdir/forged.backend.json" "$FLOWLOWER_BIN"
refuse "$tmpdir/forged.backend.json" "$FLOWVALIDATE_BIN"
echo 'Source graph artifact: PASS'
