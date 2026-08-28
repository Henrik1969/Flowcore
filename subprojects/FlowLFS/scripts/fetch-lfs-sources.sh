#!/usr/bin/env bash

set -euo pipefail

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
project_dir=$(CDPATH= cd -- "$script_dir/.." && pwd)

test -s "$project_dir/manifests/wget-list"
test -s "$project_dir/manifests/md5sums"

if ! wget --input-file="$project_dir/manifests/wget-list" \
    --continue \
    --no-verbose \
    --directory-prefix="$project_dir/sources"; then
  printf '%s\n' \
    'warning: one or more non-checksum download-list entries were unavailable' >&2
fi

cp -- "$project_dir/manifests/md5sums" "$project_dir/sources/md5sums"
(
  cd "$project_dir/sources"
  md5sum --check md5sums
)

printf '%s\n' 'All inputs required by the edition checksum manifest verify.'
