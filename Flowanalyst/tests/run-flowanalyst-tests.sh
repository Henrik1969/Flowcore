#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
bin=${FLOWANALYST_BIN:?FLOWANALYST_BIN is required}
flowmini=${FLOWMINI_BIN:-$root/Flowmini/flowmini_v25_symboltable_projection/cmake-build-debug/flowmini}
fixture="$root/Flowmini/flowmini_v25_symboltable_projection/examples/ast/target_projection_probe.flow"
test -x "$flowmini"
test -x "$bin"

report=$("$flowmini" --dump-frontend-bundle "$fixture" | "$bin")
printf '%s\n' "$report" | grep -q '"status": "ok"'
printf '%s\n' "$report" | grep -q '"targets":2'
printf '%s\n' "$report" | grep -q '"analysis_regions"'
printf '%s\n' "$report" | grep -q '"id":"target:cli"'
printf '%s\n' "$report" | grep -q '"format":"flowanalyst.analysis_graph"'
printf '%s\n' "$report" | grep -q '"name":"region_dependency"'
printf '%s\n' "$report" | jq -e --arg source "$fixture" '.source.path == $source' >/dev/null

call_fixture="$root/Flowmini/flowmini_v25_symboltable_projection/examples/ast/call_expression_probe.flow"
call_report=$("$flowmini" --dump-frontend-bundle "$call_fixture" | "$bin")
printf '%s\n' "$call_report" | grep -q '"resolved_names":12'
printf '%s\n' "$call_report" | grep -q '"name":"square"'
printf '%s\n' "$call_report" | jq -e '[.effect_facts[] | select(.effect == "pure" and .certainty == "proven")] | length == 2' >/dev/null

parallel_fixture="$root/Flowmini/flowmini_v25_symboltable_projection/examples/ast/parallel_independence_probe.flow"
parallel_report=$("$flowmini" --dump-frontend-bundle "$parallel_fixture" | "$bin")
printf '%s\n' "$parallel_report" | jq -e '.parallel_candidates | length == 2 and all(.[]; .status == "deferred" and .proof == "pure-callee-disjoint-inputs")' >/dev/null

refined_fixture="$root/Flowmini/flowmini_v25_symboltable_projection/examples/ast/refined_contract_probe.flow"
refined_report=$("$flowmini" --dump-frontend-bundle "$refined_fixture" | "$bin")
printf '%s\n' "$refined_report" | grep -q '"status": "ok"'
printf '%s\n' "$refined_report" | grep -q '"refined_types":2'

abi_fixture="$root/Flowmini/flowmini_v25_symboltable_projection/examples/pass/abi_libc_demo.flow"
abi_report=$("$flowmini" --dump-frontend-bundle "$abi_fixture" | "$bin")
printf '%s\n' "$abi_report" | grep -q '"binding_requirements"'
printf '%s\n' "$abi_report" | grep -q '"symbol":"strlen"'
printf '%s\n' "$abi_report" | jq -e '.aggregate_abi_layouts == []' >/dev/null
printf '%s\n' "$abi_report" | jq -e '(.external_operations | length) == 4 and ([.external_operations[].callee] | index("strlen")) != null and ([.external_operations[].callee] | index("abs")) != null and ([.external_operations[].callee] | index("puts")) != null and ([.external_operations[].result_symbol_id] | length) == 4' >/dev/null

struct_fixture="$root/Flowmini/flowmini_v25_symboltable_projection/examples/pass/abi_struct_demo.flow"
struct_report=$("$flowmini" --dump-frontend-bundle "$struct_fixture" | "$bin")
printf '%s\n' "$struct_report" | jq -e '
    (.aggregate_abi_layouts | length) == 1 and
    .aggregate_abi_layouts[0].contract == "testabi" and
    .aggregate_abi_layouts[0].name == "Point" and
    .aggregate_abi_layouts[0].status == "declared" and
    .aggregate_abi_layouts[0].layout_policy == "provider_verified_required" and
    [.aggregate_abi_layouts[0].fields[].name] == ["x", "y"]
' >/dev/null

kernel_clock_fixture="$root/Flowmini/flowmini_v25_symboltable_projection/examples/pass/abi_kernel_clock_main.flow"
kernel_clock_report=$("$flowmini" --dump-frontend-bundle "$kernel_clock_fixture" | "$bin")
printf '%s\n' "$kernel_clock_report" | jq -e '
    .lowering_profile == "none" and
    ([.binding_requirements[] | .symbol] == ["clock_gettime"]) and
    any(.lowering_plan.operations[]; .kind == "value_definition" and .operands[0].kind == "writable_storage" and .operands[0].storage.bytes == 16)
' >/dev/null

kernel_random_fixture="$root/Flowmini/flowmini_v25_symboltable_projection/examples/pass/abi_kernel_random_main.flow"
kernel_random_report=$("$flowmini" --dump-frontend-bundle "$kernel_random_fixture" | "$bin")
printf '%s\n' "$kernel_random_report" | jq -e '
    .lowering_profile == "none" and
    ([.binding_requirements[] | .symbol] == ["getrandom"])
' >/dev/null

kernel_uname_fixture="$root/Flowmini/flowmini_v25_symboltable_projection/examples/pass/abi_kernel_uname_main.flow"
kernel_uname_report=$("$flowmini" --dump-frontend-bundle "$kernel_uname_fixture" | "$bin")
printf '%s\n' "$kernel_uname_report" | jq -e '
    .lowering_profile == "none" and
    ([.binding_requirements[] | .symbol] == ["uname"]) and
    any(.lowering_plan.operations[]; .kind == "value_definition" and .operands[0].kind == "writable_storage" and .operands[0].storage.bytes == 390)
' >/dev/null

kernel_openat_fixture="$root/Flowmini/flowmini_v25_symboltable_projection/examples/pass/abi_kernel_openat_main.flow"
kernel_openat_report=$("$flowmini" --dump-frontend-bundle "$kernel_openat_fixture" | "$bin")
printf '%s\n' "$kernel_openat_report" | jq -e '
    .lowering_profile == "none" and
    ([.binding_requirements[] | .symbol] == ["openat"])
' >/dev/null

kernel_read_fixture="$root/Flowmini/flowmini_v25_symboltable_projection/examples/pass/abi_kernel_read_main.flow"
kernel_read_report=$("$flowmini" --dump-frontend-bundle "$kernel_read_fixture" | "$bin")
printf '%s\n' "$kernel_read_report" | jq -e '
    .lowering_profile == "none" and
    ([.binding_requirements[] | .symbol] == ["read"])
' >/dev/null

kernel_write_fixture="$root/Flowmini/flowmini_v25_symboltable_projection/examples/pass/abi_kernel_write_main.flow"
kernel_write_report=$("$flowmini" --dump-frontend-bundle "$kernel_write_fixture" | "$bin")
printf '%s\n' "$kernel_write_report" | jq -e '
    .lowering_profile == "none" and
    ([.binding_requirements[] | .symbol] == ["write"])
' >/dev/null

kernel_lseek_fixture="$root/Flowmini/flowmini_v25_symboltable_projection/examples/pass/abi_kernel_lseek_main.flow"
kernel_lseek_report=$("$flowmini" --dump-frontend-bundle "$kernel_lseek_fixture" | "$bin")
printf '%s\n' "$kernel_lseek_report" | jq -e '
    .lowering_profile == "none" and
    ([.binding_requirements[] | .symbol] == ["lseek"])
' >/dev/null

kernel_unlinkat_fixture="$root/Flowmini/flowmini_v25_symboltable_projection/examples/pass/abi_kernel_unlinkat_main.flow"
kernel_unlinkat_report=$("$flowmini" --dump-frontend-bundle "$kernel_unlinkat_fixture" | "$bin")
printf '%s\n' "$kernel_unlinkat_report" | jq -e '
    .lowering_profile == "none" and
    ([.binding_requirements[] | .symbol] == ["unlinkat"])
' >/dev/null

rmdir_fixture="$root/Flowmini/flowmini_v25_symboltable_projection/examples/pass/abi_kernel_rmdir_main.flow"
rmdir_report=$("$flowmini" --dump-frontend-bundle "$rmdir_fixture" | "$bin")
printf '%s\n' "$rmdir_report" | jq -e '.lowering_profile == "none" and ([.binding_requirements[] | .symbol] == ["rmdir"])' >/dev/null

for kernel_name in fork socket listen unshare sethostname pipe2 waitpid socketpair bind poll accept4 connect gethostname; do
    kernel_fixture="$root/Flowmini/flowmini_v25_symboltable_projection/examples/pass/abi_kernel_${kernel_name}_main.flow"
    kernel_report=$("$flowmini" --dump-frontend-bundle "$kernel_fixture" | "$bin")
    printf '%s\n' "$kernel_report" | jq -e --arg symbol "$kernel_name" '.lowering_profile == "none" and ([.binding_requirements[] | .symbol] == [$symbol])' >/dev/null
done

flowcat_fixture="$root/Flowmini/flowmini_v25_symboltable_projection/examples/apps/flowcat/flowcat.flow"
flowcat_report=$("$flowmini" --dump-frontend-bundle "$flowcat_fixture" | "$bin")
printf '%s\n' "$flowcat_report" | jq -e '
  .lowering_profile == "none" and
  ([.lowering_plan.operations[] | select(.kind == "loop")] | length) == 2 and
  any(.lowering_plan.operations[]; .kind == "assignment")
' >/dev/null
printf '%s\n' "$flowcat_report" | grep -q '"name":"args"'
printf '%s\n' "$flowcat_report" | grep -q '"symbol":"open"'
printf '%s\n' "$flowcat_report" | grep -q '"symbol":"read"'
printf '%s\n' "$flowcat_report" | grep -q '"symbol":"write"'
printf '%s\n' "$flowcat_report" | grep -q '"symbol":"close"'

bad_field_fixture="$root/Flowmini/flowmini_v25_symboltable_projection/examples/fail/bad_record_no_such_field.flow"
set +e
bad_field_report=$("$flowmini" --dump-frontend-bundle "$bad_field_fixture" | "$bin")
bad_field_rc=$?
set -e
test "$bad_field_rc" -eq 2
printf '%s\n' "$bad_field_report" | grep -q 'FLOWANALYST_UNKNOWN_FIELD'

if printf '%s' '{"format":"wrong","version":2}' | "$bin" >/dev/null 2>&1; then
    echo 'invalid bundle unexpectedly accepted' >&2
    exit 1
fi
echo 'Flowanalyst tests: PASS'
