#!/usr/bin/env bash

set -euo pipefail

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
project_dir=$(CDPATH= cd -- "$script_dir/.." && pwd)
execution_dir="$project_dir/execution"

test -s "$execution_dir/manifests/wget-list"
test -s "$execution_dir/manifests/md5sums"

if ! wget --input-file="$execution_dir/manifests/wget-list" \
    --continue --no-verbose --timeout=30 --tries=2 \
    --directory-prefix="$execution_dir/sources"; then
  printf '%s\n' 'warning: a download-list entry failed; verifying required set' >&2
fi

cp -- "$execution_dir/manifests/md5sums" "$execution_dir/sources/md5sums"
(
  cd "$execution_dir/sources"
  md5sum --check md5sums

  manifest_names=$(mktemp)
  cached_names=$(mktemp)
  trap 'rm -f -- "$manifest_names" "$cached_names"' EXIT
  awk '{print $2}' md5sums | sort > "$manifest_names"
  find . -maxdepth 1 -type f -printf '%f\n' |
    grep -v -E '^(\.gitkeep|md5sums)$' | sort > "$cached_names"
  if comm -13 "$manifest_names" "$cached_names" | grep -q .; then
    printf '%s\n' \
      'notice: downloaded cache files outside the checksum contract:' >&2
    comm -13 "$manifest_names" "$cached_names" >&2
  fi
)

printf '%s\n' 'All execution inputs required by the checksum manifest verify.'
