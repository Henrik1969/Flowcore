#!/usr/bin/env bash

set -euo pipefail

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
project_dir=$(CDPATH= cd -- "$script_dir/.." && pwd)
builder_dir="$project_dir/builder"
release_url=https://releases.ubuntu.com/24.04.4
medium=ubuntu-24.04.4-live-server-amd64.iso

mkdir -p -- "$builder_dir/manifests" "$builder_dir/media"
curl --fail --location --proto '=https' --tlsv1.2 \
  --output "$builder_dir/manifests/SHA256SUMS" "$release_url/SHA256SUMS"
curl --fail --location --proto '=https' --tlsv1.2 \
  --output "$builder_dir/manifests/SHA256SUMS.gpg" "$release_url/SHA256SUMS.gpg"
printf '%s\n' "$medium" > "$builder_dir/manifests/medium"
date --utc +'%Y-%m-%dT%H:%M:%SZ' > "$builder_dir/manifests/retrieved-at"

curl --fail --location --proto '=https' --tlsv1.2 --continue-at - \
  --output "$builder_dir/media/$medium" "$release_url/$medium"
"$script_dir/verify-builder-media.sh"
