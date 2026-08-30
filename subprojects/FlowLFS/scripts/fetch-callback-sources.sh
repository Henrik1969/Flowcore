#!/usr/bin/env bash

set -euo pipefail
script_dir=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)
project_dir=$(CDPATH='' cd -- "$script_dir/.." && pwd)
destination="$project_dir/execution/callback"
archive="$destination/openssh-10.5p1.tar.gz"
recipe="$destination/openssh-10.5p1.blfs.html"

mkdir -p "$destination"
curl --fail --location --proto '=https' --tlsv1.2 \
  --output "$archive.part" \
  https://ftp.openbsd.org/pub/OpenBSD/OpenSSH/portable/openssh-10.5p1.tar.gz
printf '%s  %s\n' a95119f402dfa0166c9dd1237239085c "$archive.part" | md5sum --check --status
mv "$archive.part" "$archive"

curl --fail --location --proto '=https' --tlsv1.2 \
  --output "$recipe.part" \
  https://www.linuxfromscratch.org/blfs/view/systemd/postlfs/openssh.html
mv "$recipe.part" "$recipe"

printf '%s\n' 'BLFS development systemd recipe: OpenSSH-10.5p1' > "$destination/authority"
date --utc +'%Y-%m-%dT%H:%M:%SZ' > "$destination/retrieved-at"
(
  cd "$destination"
  sha256sum authority openssh-10.5p1.blfs.html openssh-10.5p1.tar.gz retrieved-at \
    > retrieval.sha256
)
