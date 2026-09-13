#!/bin/sh
set -eu
example_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
root=${FLOWCORE_ROOT:-$(CDPATH= cd -- "$example_dir/../../../../.." && pwd)}
output_dir=${1:?usage: build-flow-less.sh OUTPUT_DIRECTORY}
mkdir -p "$output_dir"
if [ -n "${FLOWCORE_PREFIX:-}" ]; then
    FLOWMINI_BIN=${FLOWMINI_BIN:-$FLOWCORE_PREFIX/bin/flowmini}
    FLOWANALYST_BIN=${FLOWANALYST_BIN:-$FLOWCORE_PREFIX/bin/flowanalyst}
    FLOWBIND_BIN=${FLOWBIND_BIN:-$FLOWCORE_PREFIX/bin/flowbind}
    FLOWPARALLEL_BIN=${FLOWPARALLEL_BIN:-$FLOWCORE_PREFIX/bin/flowparallel}
    FLOWOPTIMIZE_BIN=${FLOWOPTIMIZE_BIN:-$FLOWCORE_PREFIX/bin/flowoptimize}
    FLOWPREPARE_BIN=${FLOWPREPARE_BIN:-$FLOWCORE_PREFIX/bin/flowprepare}
    FLOWLOWER_BIN=${FLOWLOWER_BIN:-$FLOWCORE_PREFIX/bin/flowlower}
    FLOWGRAPH_RUNTIME=${FLOWGRAPH_RUNTIME:-$FLOWCORE_PREFIX/lib/flowcore/libflowgraph_runtime.so}
    FLOWPAGER_INPUT=${FLOWPAGER_INPUT:-$FLOWCORE_PREFIX/lib/flowcore/providers/libflowpager_input.so}
    FLOWPAGER_OUTPUT=${FLOWPAGER_OUTPUT:-$FLOWCORE_PREFIX/lib/flowcore/providers/libflowpager_output.so}
    FLOWBIND_GENERATOR=${FLOWBIND_GENERATOR:-$FLOWCORE_PREFIX/bin/generate-flow-bindings}
fi
if [ -n "${FLOWCORE_BUILD:-}" ]; then
    FLOWMINI_BIN=${FLOWMINI_BIN:-$FLOWCORE_BUILD/flowmini/flowmini}
    FLOWANALYST_BIN=${FLOWANALYST_BIN:-$FLOWCORE_BUILD/flowanalyst/flowanalyst}
    FLOWBIND_BIN=${FLOWBIND_BIN:-$FLOWCORE_BUILD/flowbind/flowbind}
    FLOWPARALLEL_BIN=${FLOWPARALLEL_BIN:-$FLOWCORE_BUILD/flowtools/flowparallel/flowparallel}
    FLOWOPTIMIZE_BIN=${FLOWOPTIMIZE_BIN:-$FLOWCORE_BUILD/flowoptimize/flowoptimize}
    FLOWPREPARE_BIN=${FLOWPREPARE_BIN:-$FLOWCORE_BUILD/flowlower/flowprepare}
    FLOWLOWER_BIN=${FLOWLOWER_BIN:-$FLOWCORE_BUILD/flowlower/flowlower}
    FLOWGRAPH_RUNTIME=${FLOWGRAPH_RUNTIME:-$FLOWCORE_BUILD/flowlower/libflowgraph_runtime.so}
    FLOWPAGER_INPUT=${FLOWPAGER_INPUT:-$FLOWCORE_BUILD/flow_less_providers/libflowpager_input.so}
    FLOWPAGER_OUTPUT=${FLOWPAGER_OUTPUT:-$FLOWCORE_BUILD/flow_less_providers/libflowpager_output.so}
fi
: "${FLOWMINI_BIN:?}" "${FLOWANALYST_BIN:?}" "${FLOWBIND_BIN:?}" "${FLOWPARALLEL_BIN:?}" "${FLOWOPTIMIZE_BIN:?}" "${FLOWPREPARE_BIN:?}" "${FLOWLOWER_BIN:?}" "${FLOWPAGER_INPUT:?}" "${FLOWPAGER_OUTPUT:?}" "${FLOWGRAPH_RUNTIME:?}"
sha256sum "$FLOWMINI_BIN" "$FLOWANALYST_BIN" "$FLOWBIND_BIN" "$FLOWPARALLEL_BIN" "$FLOWOPTIMIZE_BIN" "$FLOWLOWER_BIN" "$FLOWGRAPH_RUNTIME" "$FLOWPAGER_INPUT" "$FLOWPAGER_OUTPUT" > "$output_dir/tools.sha256"
cp "${FLOWPAGER_SOURCE:-$example_dir/flow_less.flow}" "$output_dir/flow_less.flow"
libc=$(ldconfig -p | awk '$1 == "libc.so.6" && $NF ~ /^\// {print $NF; exit}')
python3 - "$output_dir" "$FLOWPAGER_INPUT" "$FLOWPAGER_OUTPUT" "$FLOWGRAPH_RUNTIME" "$libc" <<'PY'
import json, pathlib, sys
out = pathlib.Path(sys.argv[1])
def fn(name, symbol, effect, result, *parameters):
    return dict(name=name, symbol=symbol, effect=effect, return_type=result,
                parameters=[dict(name=f'arg{i}', type=t) for i,t in enumerate(parameters)])
specs = {
 'input': (sys.argv[2], [fn('start','pager_input_start','input','c_int'),
    fn('line_count','pager_line_count','readonly','c_int'),fn('page_size','pager_page_size','readonly','c_int'),
    fn('command_count','pager_command_count','readonly','c_int'),
    fn('line_at','pager_line_at','readonly','c_string','c_int'),fn('command_at','pager_command_at','readonly','c_string','c_int')]),
 'output': (sys.argv[3], [fn('text','pager_write_text','io','c_int','c_string'),
    fn('line','pager_write_line','io','c_int','c_string'),fn('integer','pager_write_integer','io','c_int','c_int'),
    fn('diagnostic','pager_diagnostic','io','c_int','c_string'),fn('error_text','pager_error_text','io','c_int','c_string')]),
 'runtime': (sys.argv[4], [fn('raise','flow_graph_raise','failure','c_int','c_int')]),
 'strings': (sys.argv[5], [fn('compare','strcmp','pure','c_int','c_string','c_string')])}
for namespace,(path,functions) in specs.items():
    soname = 'libc.so.6' if namespace == 'strings' else path
    (out / f'{namespace}.spec.json').write_text(json.dumps(dict(format='flowcore.native_binding_spec',version=1,
        unit=f'generated_{namespace}',namespace=namespace,provider=dict(path=path,soname=soname,convention='c'),functions=functions),sort_keys=True)+'\n')
PY
for module in input output runtime strings; do
    "${FLOWBIND_GENERATOR:-$root/tools/generate-flow-bindings.sh}" --spec "$output_dir/$module.spec.json" --flow-output "$output_dir/$module.flow" --policy-output "$output_dir/$module.policy" --manifest-output "$output_dir/$module.manifest.json" > "$output_dir/$module.generation.log"
done
cat "$output_dir/input.policy" "$output_dir/output.policy" "$output_dir/runtime.policy" "$output_dir/strings.policy" > "$output_dir/policy"
printf '%s\n' '{"format":"flowcore.graph_provider_map","version":1,"providers":[{"implementation":"pager.input","source_callable":"input.start","activation":"startup_once","output_port":"out"}]}' > "$output_dir/providers.json"
"$FLOWMINI_BIN" --dump-frontend-bundle "$output_dir/flow_less.flow" > "$output_dir/frontend.json"
"$FLOWANALYST_BIN" --lowering-plan-version 2 --graph-plan-version 2 --graph-providers "$output_dir/providers.json" < "$output_dir/frontend.json" > "$output_dir/semantic.json"
"$FLOWBIND_BIN" --policy "$output_dir/policy" < "$output_dir/semantic.json" > "$output_dir/binding.json"
"$FLOWPARALLEL_BIN" < "$output_dir/semantic.json" > "$output_dir/execution.json"
"$FLOWOPTIMIZE_BIN" < "$output_dir/execution.json" > "$output_dir/optimization.json"
"$FLOWPREPARE_BIN" --binding-report "$output_dir/binding.json" < "$output_dir/optimization.json" > "$output_dir/backend.json"
"$FLOWLOWER_BIN" --emit-llvm "$output_dir/flow_less.ll" < "$output_dir/backend.json" > "$output_dir/lowering.json"
clang -c "$output_dir/flow_less.ll" -o "$output_dir/flow_less.o"
"${FLOWGRAPH_CXX:-c++}" ${FLOWGRAPH_LINK_FLAGS:-} "$output_dir/flow_less.o" "$FLOWGRAPH_RUNTIME" "$FLOWPAGER_INPUT" "$FLOWPAGER_OUTPUT" \
    "-Wl,-rpath,$(dirname "$FLOWGRAPH_RUNTIME")" "-Wl,-rpath,$(dirname "$FLOWPAGER_INPUT")" "-Wl,-rpath,$(dirname "$FLOWPAGER_OUTPUT")" -o "$output_dir/flow_less"
sha256sum --check --status "$output_dir/tools.sha256"
printf 'Native Flow pager: %s\n' "$output_dir/flow_less"
