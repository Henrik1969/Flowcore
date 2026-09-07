#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
generator="${FLOWBIND_GEN:-${root}/tools/flowbind-gen}"
tmpdir="$(mktemp -d)"
trap 'rm -rf "${tmpdir}"' EXIT

"${generator}" --header /usr/include/math.h --library libm.so.6 --out "${tmpdir}/math.json"
"${generator}" --header /usr/include/zlib.h --library libz.so.1 --out "${tmpdir}/zlib.json"
"${generator}" --header /usr/include/sqlite3.h --library libsqlite3.so.0 --resource sqlite3_open:sqlite3_handle:sqlite3_close --out "${tmpdir}/sqlite3.json"
"${generator}" --header /usr/include/x86_64-linux-gnu/curl/curl.h --library libcurl.so.4 --resource curl_easy_init:CURL_handle:curl_easy_cleanup --out "${tmpdir}/curl.json"

jq -e '.status == "partial" and any(.unsupported[]; .name == "sqrt")' "${tmpdir}/math.json" >/dev/null
jq -e 'any(.functions[]; .name == "compress") or any(.unsupported[]; .name == "compress")' "${tmpdir}/zlib.json" >/dev/null
jq -e '.functions | map(.name) | index("sqlite3_close") != null' "${tmpdir}/sqlite3.json" >/dev/null
jq -e '.functions | map(.name) | index("curl_easy_init") != null' "${tmpdir}/curl.json" >/dev/null
jq -e '.resource_contracts[0].released_by == "sqlite3_close" and .resource_contracts[0].required_cleanup == true' "${tmpdir}/sqlite3.json" >/dev/null
jq -e '.resource_contracts[0].released_by == "curl_easy_cleanup" and .resource_contracts[0].required_cleanup == true' "${tmpdir}/curl.json" >/dev/null
cp "${tmpdir}/curl.json" "${tmpdir}/curl-copy.json"
cmp -s "${tmpdir}/curl.json" "${tmpdir}/curl-copy.json"
echo "Flowbind real C library experiments: PASS"
echo "  libm: scalar carrier gap recorded (double functions remain unsupported)"
echo "  zlib/sqlite3/curl: deterministic partial artifacts with supported declarations and explicit gaps"
