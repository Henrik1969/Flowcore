#!/usr/bin/env bash

set -euo pipefail
LFS=/mnt/lfs
test "$(id -u)" -eq 0
if mountpoint -q "$LFS/dev/shm"; then umount "$LFS/dev/shm"; fi
if mountpoint -q "$LFS/dev/pts"; then umount "$LFS/dev/pts"; fi
for path in sys proc run dev; do
  if mountpoint -q "$LFS/$path"; then umount "$LFS/$path"; fi
done
