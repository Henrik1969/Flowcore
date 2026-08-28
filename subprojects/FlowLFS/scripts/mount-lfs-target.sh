#!/usr/bin/env bash

set -euo pipefail

test "$(id -u)" -eq 0
test -b /dev/vdb1
test "$(blkid -s LABEL -o value /dev/vdb1)" = FLOWLFS_ROOT
mkdir -p /mnt/lfs /mnt/flowlfs-project
mountpoint -q /mnt/lfs || mount -t ext4 /dev/vdb1 /mnt/lfs
mountpoint -q /mnt/flowlfs-project || \
  mount -t 9p -o trans=virtio,version=9p2000.L,ro \
    flowlfs_project /mnt/flowlfs-project
findmnt /mnt/lfs
findmnt /mnt/flowlfs-project
