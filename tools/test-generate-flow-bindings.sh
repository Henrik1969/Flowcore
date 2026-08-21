#!/bin/sh
set -eu

root=${FLOWCORE_ROOT:-$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)}
flowmini=${FLOWMINI_BIN:-$root/build/flowmini/flowmini}
analyst=${FLOWANALYST_BIN:-$root/build/flowanalyst/flowanalyst}
bind=${FLOWBIND_BIN:-$root/build/flowbind/flowbind}
parallel=${FLOWPARALLEL_BIN:-$root/build/flowtools/flowparallel/flowparallel}
optimizer=${FLOWOPTIMIZE_BIN:-$root/build/flowoptimize/flowoptimize}
lowerer=${FLOWLOWER_BIN:-$root/build/flowlower/flowlower}
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT

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
  'program generated_getlogin_main' \
  '' \
  'main {' \
  '    name : c_string("")' \
  '    status : c_int(0)' \
  '    login.getlogin() -> name' \
  '    login.puts(name) -> status' \
  '}' > "$tmpdir/login.consumer.flow"
"$flowmini" --dump-frontend-bundle "$tmpdir/login.consumer.flow" > "$tmpdir/login.bundle.json"
"$analyst" < "$tmpdir/login.bundle.json" > "$tmpdir/login.semantic.json"
jq -e '.status == "ok" and .lowering_profile == "generated_getlogin_main"' "$tmpdir/login.semantic.json" >/dev/null
"$bind" --policy "$tmpdir/login.policy" < "$tmpdir/login.semantic.json" > "$tmpdir/login.binding.json"
sed 's/ c_string$/ c_long/' "$tmpdir/login.policy" > "$tmpdir/login-hostile.policy"
if "$bind" --policy "$tmpdir/login-hostile.policy" < "$tmpdir/login.semantic.json" > "$tmpdir/login-hostile.binding.json" 2>/dev/null; then
    echo 'hostile generated signature policy unexpectedly accepted' >&2
    exit 1
fi
"$parallel" < "$tmpdir/login.semantic.json" > "$tmpdir/login.parallel.json"
"$optimizer" < "$tmpdir/login.parallel.json" > "$tmpdir/login.optimized.json"
"$lowerer" --emit-llvm "$tmpdir/login.ll" --binding-report "$tmpdir/login.binding.json" < "$tmpdir/login.optimized.json" > "$tmpdir/login.lowering.json"
grep -q '"status": "emitted"' "$tmpdir/login.lowering.json"
grep -Fq 'call ptr @getlogin' "$tmpdir/login.ll"
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
jq -e '.status == "ok" and .lowering_profile == "generated_gettid_main"' "$tmpdir/tid.semantic.json" >/dev/null
"$bind" --policy "$tmpdir/tid.policy" < "$tmpdir/tid.semantic.json" > "$tmpdir/tid.binding.json"
"$parallel" < "$tmpdir/tid.semantic.json" | "$optimizer" > "$tmpdir/tid.optimized.json"
"$lowerer" --emit-llvm "$tmpdir/tid.ll" --binding-report "$tmpdir/tid.binding.json" < "$tmpdir/tid.optimized.json" > "$tmpdir/tid.lowering.json"
grep -q '"status": "emitted"' "$tmpdir/tid.lowering.json"
grep -Fq 'call i32 @gettid' "$tmpdir/tid.ll"
clang "$tmpdir/tid.ll" -o "$tmpdir/tid"
"$tmpdir/tid"

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
  'program generated_system_info_main' \
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
jq -e '.status == "ok" and .lowering_profile == "generated_system_info_main"' "$tmpdir/system-info.semantic.json" >/dev/null
"$bind" --policy "$tmpdir/system-info.policy" < "$tmpdir/system-info.semantic.json" > "$tmpdir/system-info.binding.json"
"$parallel" < "$tmpdir/system-info.semantic.json" | "$optimizer" > "$tmpdir/system-info.optimized.json"
"$lowerer" --emit-llvm "$tmpdir/system-info.ll" --binding-report "$tmpdir/system-info.binding.json" < "$tmpdir/system-info.optimized.json" > "$tmpdir/system-info.lowering.json"
grep -q '"status": "emitted"' "$tmpdir/system-info.lowering.json"
grep -Fq 'call i32 @getpagesize' "$tmpdir/system-info.ll"
grep -Fq 'call i32 @get_nprocs' "$tmpdir/system-info.ll"
grep -Fq 'call i32 @get_nprocs_conf' "$tmpdir/system-info.ll"
grep -Fq 'call i64 @get_phys_pages' "$tmpdir/system-info.ll"
grep -Fq 'call i64 @get_avphys_pages' "$tmpdir/system-info.ll"
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
jq -e '.status == "ok" and .lowering_profile == "generated_sysconf_main"' "$tmpdir/sysconf.semantic.json" >/dev/null
"$bind" --policy "$tmpdir/sysconf.policy" < "$tmpdir/sysconf.semantic.json" > "$tmpdir/sysconf.binding.json"
"$parallel" < "$tmpdir/sysconf.semantic.json" | "$optimizer" > "$tmpdir/sysconf.optimized.json"
"$lowerer" --emit-llvm "$tmpdir/sysconf.ll" --binding-report "$tmpdir/sysconf.binding.json" < "$tmpdir/sysconf.optimized.json" > "$tmpdir/sysconf.lowering.json"
grep -q '"status": "emitted"' "$tmpdir/sysconf.lowering.json"
grep -Fq 'call i64 @sysconf(i32 30)' "$tmpdir/sysconf.ll"
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
jq -e '.status == "ok" and .lowering_profile == "generated_getauxval_main"' "$tmpdir/getauxval.semantic.json" >/dev/null
"$bind" --policy "$tmpdir/getauxval.policy" < "$tmpdir/getauxval.semantic.json" > "$tmpdir/getauxval.binding.json"
"$parallel" < "$tmpdir/getauxval.semantic.json" | "$optimizer" > "$tmpdir/getauxval.optimized.json"
"$lowerer" --emit-llvm "$tmpdir/getauxval.ll" --binding-report "$tmpdir/getauxval.binding.json" < "$tmpdir/getauxval.optimized.json" > "$tmpdir/getauxval.lowering.json"
grep -q '"status": "emitted"' "$tmpdir/getauxval.lowering.json"
grep -Fq 'call i64 @getauxval(i64 6)' "$tmpdir/getauxval.ll"
clang "$tmpdir/getauxval.ll" -o "$tmpdir/getauxval"
"$tmpdir/getauxval"

printf '%s\n' 'Generated native binding artifacts: PASS'
