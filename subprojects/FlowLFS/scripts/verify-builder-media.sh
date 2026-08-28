#!/usr/bin/env bash

set -euo pipefail

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
project_dir=$(CDPATH= cd -- "$script_dir/.." && pwd)
builder_dir="$project_dir/builder"
keyring=/usr/share/keyrings/ubuntu-archive-keyring.gpg
medium=$(cat "$builder_dir/manifests/medium")

test -r "$keyring"
test -s "$builder_dir/manifests/SHA256SUMS"
test -s "$builder_dir/manifests/SHA256SUMS.gpg"
test -s "$builder_dir/media/$medium"

gpgv --keyring "$keyring" \
  "$builder_dir/manifests/SHA256SUMS.gpg" \
  "$builder_dir/manifests/SHA256SUMS"

expected=$(awk -v medium="$medium" '$2 == "*" medium {print $1}' \
  "$builder_dir/manifests/SHA256SUMS")
test -n "$expected"
printf '%s  %s\n' "$expected" "$builder_dir/media/$medium" | sha256sum --check
