#!/usr/bin/env bash
set -euo pipefail

revision_bin=${REVISION_BIN:?REVISION_BIN is required}

if ! command -v jq >/dev/null 2>&1; then
    echo 'mutation provenance contract check: jq is required' >&2
    exit 2
fi

report="$($revision_bin --old-revision 7 --new-revision 8 --old-value stale --new-value current)"
jq empty <<<"$report"
test "$(jq -r '.format' <<<"$report")" = "frankencore.mutation_record"
test "$(jq -r '.version' <<<"$report")" = "1"
test "$(jq -r '.status' <<<"$report")" = "committed"
for identity in event_id attempt_id correlation_id; do
    value=$(jq -r ".$identity" <<<"$report")
    test "${#value}" -eq 26
    [[ "$value" =~ ^[0-7][0-9ABCDEFGHJKMNPQRSTVWXYZ]{25}$ ]]
done
test "$(jq -r '.entity_identity' <<<"$report")" = "reference:revision-probe"
test "$(jq -r '.old_revision' <<<"$report")" = "7"
test "$(jq -r '.new_revision' <<<"$report")" = "8"
test "$(jq -r '.actor_identity' <<<"$report")" = "reference.probe"
test "$(jq -r '.provider_identity' <<<"$report")" = "reference.revision-probe"
test "$(jq -r '.authorizing_policy' <<<"$report")" = "reference-default"
test "$(jq -r '.before_state_reference' <<<"$report")" = "reference:revision-probe@7"
test "$(jq -r '.after_state_reference' <<<"$report")" = "reference:revision-probe@8"
test "$(jq -r '.operation' <<<"$report")" = "replace-observation"
test "$(jq -r '.atomicity' <<<"$report")" = "single-publication"
test "$(jq -r '.recoverability' <<<"$report")" = "old-revision-addressable"
test "$(jq -r '.rollback_reference' <<<"$report")" = "null"
test "$(jq -r '.evidence.before.value' <<<"$report")" = "stale"
test "$(jq -r '.evidence.after.value' <<<"$report")" = "current"
test "$(jq -r '.causes | length' <<<"$report")" = "1"
test "$(jq -r '.derived_entities | length' <<<"$report")" = "0"

echo 'Frankencore mutation-provenance contract: PASS'
