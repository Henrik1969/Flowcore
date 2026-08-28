#!/usr/bin/env bash

set -euo pipefail

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
project_dir=$(CDPATH= cd -- "$script_dir/.." && pwd)
control="$project_dir/manifests/md5sums"
execution="$project_dir/execution/manifests/md5sums"

test -s "$control"
test -s "$execution"

control_names=$(mktemp)
execution_names=$(mktemp)
trap 'rm -f -- "$control_names" "$execution_names"' EXIT

awk '{print $2}' "$control" | sort > "$control_names"
awk '{print $2}' "$execution" | sort > "$execution_names"

printf 'control entries:  %s\n' "$(wc -l < "$control_names")"
printf 'execution entries: %s\n' "$(wc -l < "$execution_names")"
printf 'unchanged names:    %s\n' "$(comm -12 "$control_names" "$execution_names" | wc -l)"
printf 'control only:       %s\n' "$(comm -23 "$control_names" "$execution_names" | wc -l)"
printf 'execution only:     %s\n' "$(comm -13 "$control_names" "$execution_names" | wc -l)"

printf '\nControl-only inputs:\n'
comm -23 "$control_names" "$execution_names"
printf '\nExecution-only inputs:\n'
comm -13 "$control_names" "$execution_names"
