#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
generator="${FLOWBIND_GEN:-${root}/tools/flowbind-gen}"
tmpdir="$(mktemp -d)"
trap 'rm -rf "${tmpdir}"' EXIT
cat > "${tmpdir}/probe.h" <<'EOF'
typedef struct sqlite3 sqlite3;
typedef enum ProbeMode { PROBE_OFF = 0, PROBE_ON = 1 } ProbeMode;
int probe_add(int a, int b);
sqlite3 *probe_open(const char *path);
int probe_close(sqlite3 *db);
double probe_unsupported(double value);
EOF
"${generator}" --header "${tmpdir}/probe.h" --library sqlite3 --resource probe_open:sqlite3_handle:probe_close --out "${tmpdir}/probe.flowbind"
jq -e '
  .format == "flowbind.c_binding" and .version == 1 and .status == "partial" and
  .provider.kind == "c_abi" and
  ([.functions[].name] | sort) == ["probe_add", "probe_close", "probe_open"] and
  (.functions[] | select(.name == "probe_open") |
    .return_type == "sqlite3_handle" and .parameters[0].type == "c_string")
' "${tmpdir}/probe.flowbind" >/dev/null
jq -e '([.enums[0].members[].name] | sort) == ["PROBE_OFF", "PROBE_ON"]' "${tmpdir}/probe.flowbind" >/dev/null
jq -e '.resource_contracts == [{"created_by":"probe_open","released_by":"probe_close","required_cleanup":true,"resource_type":"sqlite3_handle"}]' "${tmpdir}/probe.flowbind" >/dev/null
echo "Flowbind C generator: PASS"
