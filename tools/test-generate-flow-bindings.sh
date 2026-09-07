#!/bin/sh
set -eu

root=${FLOWCORE_ROOT:-$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)}
flowmini=${FLOWMINI_BIN:-$root/build/flowmini/flowmini}
analyst=${FLOWANALYST_BIN:-$root/build/flowanalyst/flowanalyst}
bind=${FLOWBIND_BIN:-$root/build/flowbind/flowbind}
parallel=${FLOWPARALLEL_BIN:-$root/build/flowtools/flowparallel/flowparallel}
optimizer=${FLOWOPTIMIZE_BIN:-$root/build/flowoptimize/flowoptimize}
lowerer=${FLOWLOWER_BIN:-$root/build/flowlower/flowlower}
prepare=${FLOWPREPARE_BIN:-$root/build/flowlower/flowprepare}
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT

# Acceptance must consume the existing compiler tools without replacing them.
sha256sum "$flowmini" "$analyst" "$bind" "$parallel" "$optimizer" "$lowerer" > "$tmpdir/compiler-tools.sha256"

provider=$(ldconfig -p 2>/dev/null | awk '$1 == "libc.so.6" && $NF ~ /^\// { print $NF; exit }')
test -n "$provider"
jq --arg provider "$provider" '.provider.path = $provider' \
    "$root/docs/architecture/schemas/flowcore-native-binding-spec-v1.example.json" > "$tmpdir/spec.json"

"$root/tools/generate-flow-bindings.sh" \
    --spec "$tmpdir/spec.json" \
    --flow-output "$tmpdir/generated.flow" \
    --policy-output "$tmpdir/generated.policy" \
    --manifest-output "$tmpdir/generated.manifest.json" > "$tmpdir/generator.log"

test -s "$tmpdir/generated.flow"
test -s "$tmpdir/generated.policy"
test -s "$tmpdir/generated.manifest.json"
grep -q 'extern fn getpid' "$tmpdir/generated.flow"
grep -Fq 'extern fn getpriority(which : c_int, who : c_int)' "$tmpdir/generated.flow"
grep -Fq 'allow libc.so.6 getpriority c readonly' "$tmpdir/generated.policy"
jq -e '
  .format == "flowcore.generated_binding_manifest" and
  .status == "verified" and
  (.functions | length) == 2 and
  any(.functions[]; .symbol == "getpid" and .symbol_verification == "readelf-export-verified") and
  any(.functions[]; .symbol == "getpriority" and (.parameters | length) == 2)
' "$tmpdir/generated.manifest.json" >/dev/null

"$flowmini" --dump-frontend-bundle "$tmpdir/generated.flow" > "$tmpdir/generated.bundle.json"
jq -e '
  (.diagnostics | length) == 0 and
  any(.symbol_table.symbols[]; .name == "getpid") and
  any(.symbol_table.symbols[]; .name == "getpriority")
' "$tmpdir/generated.bundle.json" >/dev/null

printf '%s\n' \
  'import "generated.flow" as linux' \
  '' \
  'program generated_binding_consumer' \
  '' \
  'main {' \
  '    pid : c_int(0)' \
  '    priority : c_int(0)' \
  '    linux.getpid() -> pid' \
  '    linux.getpriority(0, 0) -> priority' \
  '}' > "$tmpdir/consumer.flow"
"$flowmini" --dump-frontend-bundle "$tmpdir/consumer.flow" > "$tmpdir/consumer.bundle.json"
"$analyst" < "$tmpdir/consumer.bundle.json" > "$tmpdir/generated.semantic.json"
jq -e '.status == "ok" and (.binding_requirements | length) == 2' "$tmpdir/generated.semantic.json" >/dev/null
"$bind" --policy "$tmpdir/generated.policy" < "$tmpdir/generated.semantic.json" > "$tmpdir/generated.binding.json"
jq -e '.status == "ready" and (.capabilities | length) == 2 and all(.capabilities[]; .status == "authorized")' \
    "$tmpdir/generated.binding.json" >/dev/null

jq -n --arg provider "$provider" '
  {format:"flowcore.native_binding_spec",version:1,unit:"generated_login",namespace:"login",
   provider:{soname:"libc.so.6",path:$provider,convention:"c"},
   functions:[
     {name:"getlogin",symbol:"getlogin",effect:"readonly",parameters:[],return_type:"c_string"},
     {name:"puts",symbol:"puts",effect:"io",parameters:[{name:"text",type:"c_string"}],return_type:"c_int"}
   ]}
' > "$tmpdir/login.spec.json"
"$root/tools/generate-flow-bindings.sh" \
    --spec "$tmpdir/login.spec.json" \
    --flow-output "$tmpdir/login.flow" \
    --policy-output "$tmpdir/login.policy" \
    --manifest-output "$tmpdir/login.manifest.json" >/dev/null
printf '%s\n' \
  'import "login.flow" as login' \
  '' \
  'program arbitrary_nullable_identity' \
  '' \
  'main {' \
  '    fallback : c_string("no-login")' \
  '    name : c_string("")' \
  '    status : c_int(0)' \
  '    login.getlogin() -> name' \
  '    if name == "" {' \
  '        login.puts(fallback) -> status' \
  '    } else {' \
  '        login.puts(name) -> status' \
  '    }' \
  '}' > "$tmpdir/login.consumer.flow"
"$flowmini" --dump-frontend-bundle "$tmpdir/login.consumer.flow" > "$tmpdir/login.bundle.json"
"$analyst" < "$tmpdir/login.bundle.json" > "$tmpdir/login.semantic.json"
jq -e '.status == "ok" and any(.lowering_plan.operations[]; .kind == "branch")' "$tmpdir/login.semantic.json" >/dev/null
"$bind" --policy "$tmpdir/login.policy" < "$tmpdir/login.semantic.json" > "$tmpdir/login.binding.json"
sed 's/ c_string / c_long /' "$tmpdir/login.policy" > "$tmpdir/login-hostile.policy"
if "$bind" --policy "$tmpdir/login-hostile.policy" < "$tmpdir/login.semantic.json" > "$tmpdir/login-hostile.binding.json" 2>/dev/null; then
    echo 'hostile generated signature policy unexpectedly accepted' >&2
    exit 1
fi
"$parallel" < "$tmpdir/login.semantic.json" > "$tmpdir/login.parallel.json"
"$optimizer" < "$tmpdir/login.parallel.json" > "$tmpdir/login.optimized.json"
"$lowerer" --emit-llvm "$tmpdir/login.ll" --binding-report "$tmpdir/login.binding.json" < "$tmpdir/login.optimized.json" > "$tmpdir/login.lowering.json"
grep -q '"status": "emitted"' "$tmpdir/login.lowering.json"
grep -Fq 'call ptr @getlogin' "$tmpdir/login.ll"
grep -Fq 'generic structured lowering plan' "$tmpdir/login.ll"
clang "$tmpdir/login.ll" -o "$tmpdir/login"
"$tmpdir/login" > "$tmpdir/login.output"
test -s "$tmpdir/login.output"

jq -n --arg provider "$provider" '
  {format:"flowcore.native_binding_spec",version:1,unit:"generated_tid",namespace:"linux",
   provider:{soname:"libc.so.6",path:$provider,convention:"c"},
   functions:[{name:"gettid",symbol:"gettid",effect:"readonly",parameters:[],return_type:"c_int"}]}
' > "$tmpdir/tid.spec.json"
"$root/tools/generate-flow-bindings.sh" \
    --spec "$tmpdir/tid.spec.json" \
    --flow-output "$tmpdir/tid.flow" \
    --policy-output "$tmpdir/tid.policy" \
    --manifest-output "$tmpdir/tid.manifest.json" >/dev/null
printf '%s\n' \
  'import "tid.flow" as linux' \
  '' \
  'program generated_gettid_main' \
  '' \
  'main {' \
  '    tid : c_int(0)' \
  '    linux.gettid() -> tid' \
  '}' > "$tmpdir/tid.consumer.flow"
"$flowmini" --dump-frontend-bundle "$tmpdir/tid.consumer.flow" |
    "$analyst" > "$tmpdir/tid.semantic.json"
jq -e '.status == "ok" and .lowering_plan.format == "flowcore.lowering_plan"' "$tmpdir/tid.semantic.json" >/dev/null
"$bind" --policy "$tmpdir/tid.policy" < "$tmpdir/tid.semantic.json" > "$tmpdir/tid.binding.json"
"$parallel" < "$tmpdir/tid.semantic.json" | "$optimizer" > "$tmpdir/tid.optimized.json"
"$lowerer" --emit-llvm "$tmpdir/tid.ll" --binding-report "$tmpdir/tid.binding.json" < "$tmpdir/tid.optimized.json" > "$tmpdir/tid.lowering.json"
grep -q '"status": "emitted"' "$tmpdir/tid.lowering.json"
grep -Fq 'call i32 @gettid' "$tmpdir/tid.ll"
clang "$tmpdir/tid.ll" -o "$tmpdir/tid"
"$tmpdir/tid"

# Evidence is independent of source aliases and survives every durable stage.
evidence=$(jq -r '.evidence' "$tmpdir/tid.manifest.json")
for artifact in tid.semantic tid.optimized; do
    jq -e --arg evidence "$evidence" 'all(.lowering_plan.operations[] | select(.kind == "external_call"); .provider.evidence == $evidence)' "$tmpdir/$artifact.json" >/dev/null
done
jq -e --arg evidence "$evidence" 'all(.capabilities[]; .evidence == $evidence)' "$tmpdir/tid.binding.json" >/dev/null
"$prepare" --binding-report "$tmpdir/tid.binding.json" < "$tmpdir/tid.optimized.json" > "$tmpdir/tid.backend.json"
# A policy stripped to the historical signature cannot authorize generated input.
awk '{print $1,$2,$3,$4,$5,$6,$7}' "$tmpdir/tid.policy" > "$tmpdir/no-evidence.policy"
if "$bind" --policy "$tmpdir/no-evidence.policy" < "$tmpdir/tid.semantic.json" > "$tmpdir/no-evidence.binding.json"; then
    echo 'generated binding accepted without evidence grant' >&2; exit 1
fi
for mutation in 'del(.lowering_plan.operations[].provider.evidence)' '.lowering_plan.operations[].provider.evidence = "flowcore.generated_binding.v2:invalid"' '.binding_requirements[].evidence = ""'; do
    jq "$mutation" "$tmpdir/tid.semantic.json" > "$tmpdir/hostile-evidence.semantic.json"
    if "$bind" --policy "$tmpdir/tid.policy" < "$tmpdir/hostile-evidence.semantic.json" > "$tmpdir/hostile-evidence.binding.json" 2>/dev/null; then
        echo 'mutated semantic evidence accepted' >&2; exit 1
    fi
done
for mutation in 'del(.capabilities[].evidence)' '.capabilities[].evidence |= sub("v1:"; "v2:")' '.capabilities[].evidence |= sub("[a-f0-9]{64}:"; "0000000000000000000000000000000000000000000000000000000000000000:")' '.capabilities[].evidence = 42'; do
    jq "$mutation" "$tmpdir/tid.binding.json" > "$tmpdir/hostile-evidence.binding.json"
    if "$lowerer" --emit-llvm "$tmpdir/hostile-evidence.ll" --binding-report "$tmpdir/hostile-evidence.binding.json" < "$tmpdir/tid.optimized.json" > "$tmpdir/hostile-evidence.lowering.json" 2>/dev/null; then
        echo 'mutated authorization evidence accepted by LLVM lowerer' >&2; exit 1
    fi
    test ! -e "$tmpdir/hostile-evidence.ll"
    if "$prepare" --binding-report "$tmpdir/hostile-evidence.binding.json" < "$tmpdir/tid.optimized.json" > "$tmpdir/hostile-evidence.backend.json" 2>/dev/null; then
        echo 'mutated authorization evidence accepted by backend preparation' >&2; exit 1
    fi
done
# Even matching semantic/policy hashes cannot invent evidence for loaded bytes.
wrong_evidence="${evidence%:*}:0000000000000000000000000000000000000000000000000000000000000000"
jq --arg evidence "$wrong_evidence" '.binding_requirements[].evidence = $evidence | (.lowering_plan.operations[] | select(.kind == "external_call") | .provider.evidence) = $evidence' "$tmpdir/tid.semantic.json" > "$tmpdir/drift.semantic.json"
awk -v evidence="$wrong_evidence" '{$8=evidence; print}' "$tmpdir/tid.policy" > "$tmpdir/drift.policy"
if "$bind" --policy "$tmpdir/drift.policy" < "$tmpdir/drift.semantic.json" > "$tmpdir/drift.binding.json"; then
    echo 'invented loaded-provider hash accepted' >&2; exit 1
fi
jq -e '.status == "blocked" and any(.failures[]; contains("loaded provider SHA-256"))' "$tmpdir/drift.binding.json" >/dev/null
jq -e --arg digest "${evidence##*:}" 'any(.provider_evidence[]; .sha256 == $digest and .status == "loaded-bytes-verified")' "$tmpdir/tid.binding.json" >/dev/null

# A replacement library exporting exactly the same name must be regenerated.
printf '%s\n' 'int evidence_probe(void) { return 42; }' > "$tmpdir/provider.c"
clang -shared -fPIC "$tmpdir/provider.c" -o "$tmpdir/libevidence.so"
jq -n --arg provider "$tmpdir/libevidence.so" '{format:"flowcore.native_binding_spec",version:1,unit:"replacement_provider",namespace:"proof",provider:{soname:$provider,path:$provider,convention:"c"},functions:[{name:"probe",symbol:"evidence_probe",effect:"pure",parameters:[],return_type:"c_int"}]}' > "$tmpdir/replacement.spec.json"
"$root/tools/generate-flow-bindings.sh" --spec "$tmpdir/replacement.spec.json" --flow-output "$tmpdir/replacement.flow" --policy-output "$tmpdir/replacement.policy" --manifest-output "$tmpdir/replacement.manifest.json" >/dev/null
printf '%s\n' 'import "replacement.flow" as proof' 'program replaced_bytes' 'main {' '    result : c_int(0)' '    proof.probe() -> result' '    return result' '}' > "$tmpdir/replacement.consumer.flow"
"$flowmini" --dump-frontend-bundle "$tmpdir/replacement.consumer.flow" | "$analyst" > "$tmpdir/replacement.semantic.json"
"$bind" --policy "$tmpdir/replacement.policy" < "$tmpdir/replacement.semantic.json" > "$tmpdir/replacement.binding.json"
printf '%s\n' 'int evidence_probe(void) { return 7; }' > "$tmpdir/provider.c"
clang -shared -fPIC "$tmpdir/provider.c" -o "$tmpdir/libevidence.so"
if "$bind" --policy "$tmpdir/replacement.policy" < "$tmpdir/replacement.semantic.json" > "$tmpdir/replacement-drift.binding.json"; then
    echo 'replaced library accepted with old evidence' >&2; exit 1
fi
jq -e '.status == "blocked" and any(.failures[]; contains("loaded provider SHA-256"))' "$tmpdir/replacement-drift.binding.json" >/dev/null

# Repeated generation has no timestamp-dependent bytes.
"$root/tools/generate-flow-bindings.sh" --spec "$tmpdir/tid.spec.json" --flow-output "$tmpdir/repeated.flow" --policy-output "$tmpdir/repeated.policy" --manifest-output "$tmpdir/repeated.manifest.json" >/dev/null
cmp "$tmpdir/tid.flow" "$tmpdir/repeated.flow"
cmp "$tmpdir/tid.policy" "$tmpdir/repeated.policy"
cmp "$tmpdir/tid.manifest.json" "$tmpdir/repeated.manifest.json"

jq -n --arg provider "$provider" '
  {format:"flowcore.native_binding_spec",version:1,unit:"generated_system_info",namespace:"linux",
   provider:{soname:"libc.so.6",path:$provider,convention:"c"},
   functions:[
     {name:"getpagesize",symbol:"getpagesize",effect:"readonly",parameters:[],return_type:"c_int"},
     {name:"get_nprocs",symbol:"get_nprocs",effect:"readonly",parameters:[],return_type:"c_int"},
     {name:"get_nprocs_conf",symbol:"get_nprocs_conf",effect:"readonly",parameters:[],return_type:"c_int"},
     {name:"get_phys_pages",symbol:"get_phys_pages",effect:"readonly",parameters:[],return_type:"c_long"},
     {name:"get_avphys_pages",symbol:"get_avphys_pages",effect:"readonly",parameters:[],return_type:"c_long"}
   ]}
' > "$tmpdir/system-info.spec.json"
"$root/tools/generate-flow-bindings.sh" \
    --spec "$tmpdir/system-info.spec.json" \
    --flow-output "$tmpdir/system-info.flow" \
    --policy-output "$tmpdir/system-info.policy" \
    --manifest-output "$tmpdir/system-info.manifest.json" >/dev/null
printf '%s\n' \
  'import "system-info.flow" as linux' \
  '' \
  'program arbitrary_machine_facts' \
  '' \
  'main {' \
  '    pagesize : c_int(0)' \
  '    nprocs : c_int(0)' \
  '    nprocs_conf : c_int(0)' \
  '    phys : c_long(0)' \
  '    avphys : c_long(0)' \
  '    linux.getpagesize() -> pagesize' \
  '    linux.get_nprocs() -> nprocs' \
  '    linux.get_nprocs_conf() -> nprocs_conf' \
  '    linux.get_phys_pages() -> phys' \
  '    linux.get_avphys_pages() -> avphys' \
  '}' > "$tmpdir/system-info.consumer.flow"
"$flowmini" --dump-frontend-bundle "$tmpdir/system-info.consumer.flow" |
    "$analyst" > "$tmpdir/system-info.semantic.json"
jq -e '.status == "ok" and ([.lowering_plan.operations[] | select(.kind == "external_call")] | length) == 5' "$tmpdir/system-info.semantic.json" >/dev/null
"$bind" --policy "$tmpdir/system-info.policy" < "$tmpdir/system-info.semantic.json" > "$tmpdir/system-info.binding.json"
"$parallel" < "$tmpdir/system-info.semantic.json" | "$optimizer" > "$tmpdir/system-info.optimized.json"
"$lowerer" --emit-llvm "$tmpdir/system-info.ll" --binding-report "$tmpdir/system-info.binding.json" < "$tmpdir/system-info.optimized.json" > "$tmpdir/system-info.lowering.json"
grep -q '"status": "emitted"' "$tmpdir/system-info.lowering.json"
grep -Fq 'call i32 @getpagesize' "$tmpdir/system-info.ll"
grep -Fq 'call i32 @get_nprocs' "$tmpdir/system-info.ll"
grep -Fq 'call i32 @get_nprocs_conf' "$tmpdir/system-info.ll"
grep -Fq 'call i64 @get_phys_pages' "$tmpdir/system-info.ll"
grep -Fq 'call i64 @get_avphys_pages' "$tmpdir/system-info.ll"
grep -Fq 'generic structured lowering plan' "$tmpdir/system-info.ll"
clang "$tmpdir/system-info.ll" -o "$tmpdir/system-info"
"$tmpdir/system-info"

jq -n --arg provider "$provider" '
  {format:"flowcore.native_binding_spec",version:1,unit:"generated_sysconf",namespace:"linux",
   provider:{soname:"libc.so.6",path:$provider,convention:"c"},
   functions:[{name:"sysconf",symbol:"sysconf",effect:"readonly",parameters:[{name:"selector",type:"c_int"}],return_type:"c_long"}]}
' > "$tmpdir/sysconf.spec.json"
"$root/tools/generate-flow-bindings.sh" \
    --spec "$tmpdir/sysconf.spec.json" \
    --flow-output "$tmpdir/sysconf.flow" \
    --policy-output "$tmpdir/sysconf.policy" \
    --manifest-output "$tmpdir/sysconf.manifest.json" >/dev/null
printf '%s\n' \
  'import "sysconf.flow" as linux' \
  '' \
  'program generated_sysconf_main' \
  '' \
  'main {' \
  '    selector : c_int(30)' \
  '    pagesize : c_long(0)' \
  '    linux.sysconf(selector) -> pagesize' \
  '}' > "$tmpdir/sysconf.consumer.flow"
"$flowmini" --dump-frontend-bundle "$tmpdir/sysconf.consumer.flow" |
    "$analyst" > "$tmpdir/sysconf.semantic.json"
jq -e '.status == "ok" and .lowering_plan.format == "flowcore.lowering_plan"' "$tmpdir/sysconf.semantic.json" >/dev/null
"$bind" --policy "$tmpdir/sysconf.policy" < "$tmpdir/sysconf.semantic.json" > "$tmpdir/sysconf.binding.json"
"$parallel" < "$tmpdir/sysconf.semantic.json" | "$optimizer" > "$tmpdir/sysconf.optimized.json"
"$lowerer" --emit-llvm "$tmpdir/sysconf.ll" --binding-report "$tmpdir/sysconf.binding.json" < "$tmpdir/sysconf.optimized.json" > "$tmpdir/sysconf.lowering.json"
grep -q '"status": "emitted"' "$tmpdir/sysconf.lowering.json"
grep -Fq 'call i64 @sysconf(i32 %flow_load_' "$tmpdir/sysconf.ll"
clang "$tmpdir/sysconf.ll" -o "$tmpdir/sysconf"
"$tmpdir/sysconf"

jq -n --arg provider "$provider" '
  {format:"flowcore.native_binding_spec",version:1,unit:"generated_getauxval",namespace:"linux",
   provider:{soname:"libc.so.6",path:$provider,convention:"c"},
   functions:[{name:"getauxval",symbol:"getauxval",effect:"readonly",parameters:[{name:"type",type:"c_ulong"}],return_type:"c_ulong"}]}
' > "$tmpdir/getauxval.spec.json"
"$root/tools/generate-flow-bindings.sh" \
    --spec "$tmpdir/getauxval.spec.json" \
    --flow-output "$tmpdir/getauxval.flow" \
    --policy-output "$tmpdir/getauxval.policy" \
    --manifest-output "$tmpdir/getauxval.manifest.json" >/dev/null
printf '%s\n' \
  'import "getauxval.flow" as linux' \
  '' \
  'program generated_getauxval_main' \
  '' \
  'main {' \
  '    type : c_ulong(6)' \
  '    pagesize : c_ulong(0)' \
  '    linux.getauxval(type) -> pagesize' \
  '}' > "$tmpdir/getauxval.consumer.flow"
"$flowmini" --dump-frontend-bundle "$tmpdir/getauxval.consumer.flow" |
    "$analyst" > "$tmpdir/getauxval.semantic.json"
jq -e '.status == "ok" and .lowering_plan.format == "flowcore.lowering_plan"' "$tmpdir/getauxval.semantic.json" >/dev/null
"$bind" --policy "$tmpdir/getauxval.policy" < "$tmpdir/getauxval.semantic.json" > "$tmpdir/getauxval.binding.json"
"$parallel" < "$tmpdir/getauxval.semantic.json" | "$optimizer" > "$tmpdir/getauxval.optimized.json"
"$lowerer" --emit-llvm "$tmpdir/getauxval.ll" --binding-report "$tmpdir/getauxval.binding.json" < "$tmpdir/getauxval.optimized.json" > "$tmpdir/getauxval.lowering.json"
grep -q '"status": "emitted"' "$tmpdir/getauxval.lowering.json"
grep -Fq 'call i64 @getauxval(i64 %flow_load_' "$tmpdir/getauxval.ll"
clang "$tmpdir/getauxval.ll" -o "$tmpdir/getauxval"
"$tmpdir/getauxval"

sha256sum --check --status "$tmpdir/compiler-tools.sha256"
printf '%s\n' 'Generated native binding artifacts: PASS (compiler tool hashes unchanged)'
