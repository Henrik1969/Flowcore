#!/usr/bin/env bash

set -euo pipefail

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
project_dir=$(CDPATH= cd -- "$script_dir/.." && pwd)
release_url=https://www.linuxfromscratch.org/lfs/downloads/13.0-systemd

mkdir -p -- "$project_dir/book" "$project_dir/manifests" "$project_dir/sources"

curl --fail --location --proto '=https' --tlsv1.2 \
  --output "$project_dir/book/LFS-BOOK-13.0-SYSD.pdf" \
  "$release_url/LFS-BOOK-13.0-SYSD.pdf"
curl --fail --location --proto '=https' --tlsv1.2 \
  --output "$project_dir/book/LFS-BOOK-13.0-NOCHUNKS.html" \
  "$release_url/LFS-BOOK-13.0-NOCHUNKS.html"
curl --fail --location --proto '=https' --tlsv1.2 \
  --output "$project_dir/book/LFS-BOOK-13.0.tar.xz" \
  "$release_url/LFS-BOOK-13.0.tar.xz"
curl --fail --location --proto '=https' --tlsv1.2 \
  --output "$project_dir/manifests/wget-list" \
  "$release_url/wget-list"
curl --fail --location --proto '=https' --tlsv1.2 \
  --output "$project_dir/manifests/md5sums" \
  "$release_url/md5sums"

sha256sum "$project_dir"/book/* "$project_dir"/manifests/* \
  > "$project_dir/manifests/retrieval.sha256"

printf '%s\n' 'Canonical LFS book and manifests retrieved.'
printf '%s\n' 'Review manifests/wget-list before retrieving package sources.'
