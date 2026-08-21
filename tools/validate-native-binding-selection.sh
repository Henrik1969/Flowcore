#!/bin/sh
set -eu

usage() {
    cat <<'EOF'
validate-native-binding-selection - validate explicit provider/implementation choices

Usage: validate-native-binding-selection INVENTORY.json SELECTION.json [--output RESULT.json]

Validation confirms that selected existing providers are present in the supplied
inventory. It does not authorize, load, execute, or generate bindings.
EOF
}

if [ "$#" -lt 2 ]; then usage >&2; exit 2; fi
inventory=$1
selection=$2
shift 2
output=""
while [ "$#" -gt 0 ]; do
    case "$1" in
        --output) shift; output=${1:?--output requires a path} ;;
        -h|--help) usage; exit 0 ;;
        -a|--about) printf '%s\n' 'Validate explicit native-provider choices against discovery evidence.'; exit 0 ;;
        -v|--version) printf '%s\n' '0.1.0'; exit 0 ;;
        *) printf 'validate-native-binding-selection: unknown option: %s\n' "$1" >&2; exit 2 ;;
    esac
    shift
done

jq -e '
  .format == "flowcore.native_binding_inventory" and .version == 1 and
  .status == "inventory-only" and
  .policy.authorization == "not-performed" and
  .policy.execution == "not-performed" and
  .policy.binding_generation == "not-performed"
' "$inventory" >/dev/null

jq -e '
  .format == "flowcore.native_binding_selection" and .version == 1 and
  .status == "review-required" and
  .inventory_format == "flowcore.native_binding_inventory" and
  (.selections | map(.id) | length == (unique | length)) and
  (.selections | all(.strategy == "bind-existing" or .strategy == "implement-flowcore" or .strategy == "internal-substrate-adapter" or .strategy == "external-toolchain")) and
  (.selections | all(.namespace | test("^[a-z][a-z0-9_.-]*$"))) and
  .policy.authorization == "not-performed" and
  .policy.execution == "not-performed" and
  .policy.binding_generation == "not-performed"
' "$selection" >/dev/null

if jq -e '
  [.selections[] | select(.strategy == "bind-existing" or .strategy == "external-toolchain") |
    .provider as $p |
    any($inventory[0].libraries[]; .soname == $p.soname and .path == $p.path and .architecture == $p.architecture) | not
  ] | any
' --slurpfile inventory "$inventory" "$selection" >/dev/null; then
    printf '%s\n' 'validate-native-binding-selection: selected provider is absent from inventory' >&2
    exit 1
fi

result=$(jq -n \
    --arg inventory_digest "$(sha256sum "$inventory" | awk '{print $1}')" \
    --arg selection_digest "$(sha256sum "$selection" | awk '{print $1}')" \
    '{format:"flowcore.native_binding_selection_result",version:1,status:"valid",inventory_digest:$inventory_digest,selection_digest:$selection_digest,policy:{authorization:"not-performed",execution:"not-performed",binding_generation:"not-performed"}}')

if [ -n "$output" ]; then printf '%s\n' "$result" > "$output"; else printf '%s\n' "$result"; fi
