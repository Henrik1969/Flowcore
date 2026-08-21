#!/usr/bin/env bash
set -euo pipefail

clock_bin=${CLOCK_BIN:?CLOCK_BIN is required}
report="$($clock_bin --clock monotonic)"

if ! command -v jq >/dev/null 2>&1; then
    echo 'clock contract check: jq is required' >&2
    exit 2
fi

test "$(jq -r '.format' <<<"$report")" = "frankencore.clock_report"
test "$(jq -r '.version' <<<"$report")" = "1"
test "$(jq -r '.status' <<<"$report")" = "ok"
test "$(jq -r '.type' <<<"$report")" = "Clock"
test "$(jq -r '.identity' <<<"$report")" = "clock:monotonic"
test "$(jq -r '.capabilities[0].id' <<<"$report")" = "frankencore.clock.read"
test "$(jq -r '.capabilities[0].version' <<<"$report")" = "1"
test "$(jq -r '.provider.id' <<<"$report")" = "linux.clock_gettime"
test "$(jq -r '.provider.backend' <<<"$report")" = "clock_gettime"
test "$(jq -r '.policy.decision' <<<"$report")" = "allow"
test "$(jq -r '.value_ns | type' <<<"$report")" = "number"
echo 'Frankencore Clock contract: PASS'
