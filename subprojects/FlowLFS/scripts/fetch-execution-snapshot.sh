#!/usr/bin/env bash

set -euo pipefail

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
project_dir=$(CDPATH= cd -- "$script_dir/.." && pwd)
execution_dir="$project_dir/execution"
release_url=https://www.linuxfromscratch.org/lfs/downloads/systemd

mkdir -p -- "$execution_dir/book" "$execution_dir/manifests" \
  "$execution_dir/sources"

curl --fail --location --proto '=https' --tlsv1.2 \
  --output "$execution_dir/manifests/download-index.html" \
  "$release_url/"

html_name=$(grep -o 'LFS-BOOK-r[0-9.-]*-systemd-NOCHUNKS.html.xz' \
  "$execution_dir/manifests/download-index.html" | sort -V | tail -n1)
archive_name=$(grep -o 'LFS-BOOK-r[0-9.-]*-systemd.tar.bz2' \
  "$execution_dir/manifests/download-index.html" | sort -V | tail -n1)
test -n "$html_name"
test -n "$archive_name"

curl --fail --location --proto '=https' --tlsv1.2 \
  --output "$execution_dir/book/LFS-SYSTEMD-NOCHUNKS.html.xz" \
  "$release_url/$html_name"
curl --fail --location --proto '=https' --tlsv1.2 \
  --output "$execution_dir/book/LFS-SYSTEMD-BOOK.tar.bz2" \
  "$release_url/$archive_name"
curl --fail --location --proto '=https' --tlsv1.2 \
  --output "$execution_dir/manifests/wget-list" \
  "$release_url/wget-list"
curl --fail --location --proto '=https' --tlsv1.2 \
  --output "$execution_dir/manifests/md5sums" \
  "$release_url/md5sums"

xz --decompress --keep --force \
  "$execution_dir/book/LFS-SYSTEMD-NOCHUNKS.html.xz"

revision=$(grep -m1 -o 'Version r[0-9.-]*-systemd' \
  "$execution_dir/book/LFS-SYSTEMD-NOCHUNKS.html" | tr ' ' '-')
test -n "$revision"
printf '%s\n' "$revision" > "$execution_dir/manifests/book-revision"
date --utc +'%Y-%m-%dT%H:%M:%SZ' > "$execution_dir/manifests/retrieved-at"

(
  cd "$execution_dir"
  sha256sum book/* manifests/book-revision manifests/download-index.html manifests/md5sums \
    manifests/retrieved-at manifests/wget-list > manifests/retrieval.sha256
)

printf 'Retrieved %s\n' "$revision"
