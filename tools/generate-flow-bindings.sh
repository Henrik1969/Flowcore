#!/bin/sh
set -eu

spec=""
flow_output=""
policy_output=""
manifest_output=""

while [ "$#" -gt 0 ]; do
    case "$1" in
        --spec) shift; spec=${1:?--spec requires a path} ;;
        --flow-output) shift; flow_output=${1:?--flow-output requires a path} ;;
        --policy-output) shift; policy_output=${1:?--policy-output requires a path} ;;
        --manifest-output) shift; manifest_output=${1:?--manifest-output requires a path} ;;
        -h|--help)
            cat <<'EOF'
generate-flow-bindings - verify explicit ABI facts and emit Flow binding artifacts

Usage: generate-flow-bindings --spec SPEC.json \
    --flow-output MODULE.flow --policy-output ABI.policy \
    --manifest-output ABI-manifest.json

The specification is authoritative input for signatures and effects. The tool
verifies the selected provider and exported symbols, then emits declarations,
policy grants, and an inspectable manifest. It never calls a foreign symbol.
More help: docs/architecture/native-binding-generation.md
EOF
            exit 0
            ;;
        -a|--about)
            printf '%s\n' 'Generate reviewable Flow ABI declarations and policy artifacts from explicit provider facts.'
            printf '%s\n' 'More help: docs/architecture/native-binding-generation.md'
            exit 0
            ;;
        -v|--version) printf '%s\n' '0.1.0'; exit 0 ;;
        *) printf 'generate-flow-bindings: unknown option: %s\n' "$1" >&2; exit 2 ;;
    esac
    shift
done

if [ -z "$spec" ] || [ -z "$flow_output" ] || [ -z "$policy_output" ] || [ -z "$manifest_output" ]; then
    printf '%s\n' 'generate-flow-bindings: --spec, --flow-output, --policy-output, and --manifest-output are required' >&2
    exit 2
fi

test -r "$spec" || { printf 'generate-flow-bindings: cannot read spec: %s\n' "$spec" >&2; exit 2; }
jq -e '
    .format == "flowcore.native_binding_spec" and .version == 1 and
    (.unit | test("^[A-Za-z_][A-Za-z0-9_]*$")) and
    (.namespace | test("^[A-Za-z_][A-Za-z0-9_]*$")) and
    (.provider.soname | type == "string" and length > 0) and
    (.provider.path | type == "string" and startswith("/")) and
    (.provider.convention == "c") and
    (.functions | type == "array" and length > 0) and
    (all(.functions[];
        (.name | test("^[A-Za-z_][A-Za-z0-9_]*$")) and
        (.symbol | test("^[A-Za-z_][A-Za-z0-9_]*$")) and
        (.effect | test("^[A-Za-z_][A-Za-z0-9_]*$")) and
        (.return_type | test("^[A-Za-z_][A-Za-z0-9_]*$")) and
        (.parameters | type == "array") and
        (all(.parameters[]; (.name | test("^[A-Za-z_][A-Za-z0-9_]*$")) and (.type | test("^[A-Za-z_][A-Za-z0-9_]*$"))))
    )) and
    ([.functions[].name] | unique | length) == (.functions | length) and
    ([.functions[].symbol] | unique | length) == (.functions | length)
' "$spec" >/dev/null

provider=$(jq -r '.provider.path' "$spec")
test -r "$provider" || { printf 'generate-flow-bindings: provider is not readable: %s\n' "$provider" >&2; exit 2; }

for symbol in $(jq -r '.functions[].symbol' "$spec"); do
    if ! readelf -Ws "$provider" 2>/dev/null | awk -v symbol="$symbol" '$8 == symbol || index($8, symbol "@@") == 1 { found=1 } END { exit !found }'; then
        printf 'generate-flow-bindings: provider does not export %s: %s\n' "$symbol" "$provider" >&2
        exit 3
    fi
done

mkdir -p "$(dirname "$flow_output")" "$(dirname "$policy_output")" "$(dirname "$manifest_output")"

{
    printf 'unit %s\n\nabi %s {\n' "$(jq -r '.unit' "$spec")" "$(jq -r '.namespace' "$spec")"
    printf '    library "%s"\n    convention c\n\n' "$(jq -r '.provider.soname' "$spec")"
    jq -r '.functions[] | "    extern fn " + .name + "(" + ([.parameters[] | .name + " : " + .type] | join(", ")) + "): " + .return_type + " {\n        symbol \"" + .symbol + "\"\n        effect " + .effect + "\n    }\n"' "$spec"
    printf '}\n'
} > "$flow_output"

jq -r '. as $root | .functions[] | "allow " + $root.provider.soname + " " + .symbol + " c " + .effect + " " + (if (.parameters | length) == 0 then "-" else ([.parameters[].type] | join(",")) end) + " " + .return_type' "$spec" > "$policy_output"

jq --arg generated_at "$(date -u +%Y-%m-%dT%H:%M:%SZ)" \
   --arg spec_sha256 "$(sha256sum "$spec" | awk '{print $1}')" \
   --arg provider_sha256 "$(sha256sum "$provider" | awk '{print $1}')" \
   '{format:"flowcore.generated_binding_manifest",version:1,status:"verified",
     generated_at:$generated_at,spec_sha256:$spec_sha256,
     provider:(.provider + {sha256:$provider_sha256}),
     unit:.unit,namespace:.namespace,convention:.provider.convention,
     functions:[.functions[] | {name,symbol,effect,parameters,return_type,
       authorization:"policy-grant-emitted",symbol_verification:"readelf-export-verified"}]}' "$spec" > "$manifest_output"

printf 'generated Flow binding: %s\n' "$flow_output"
printf 'generated policy: %s\n' "$policy_output"
printf 'generated manifest: %s\n' "$manifest_output"
