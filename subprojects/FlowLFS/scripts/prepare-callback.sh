#!/usr/bin/env bash

set -euo pipefail
test "$(id -u)" -eq 0
LFS=/mnt/lfs
test "$(findmnt -nro SOURCE "$LFS")" = /dev/vdb1
test "$(blkid -s LABEL -o value /dev/vdb1)" = FLOWLFS_ROOT
test -s /tmp/flowlfs-authorized-key.pub

for mountpoint_path in dev proc sys run; do
  mountpoint -q "$LFS/$mountpoint_path" || {
    echo "required chroot mount missing: $LFS/$mountpoint_path" >&2
    exit 1
  }
done

install -Dm0644 /mnt/flowlfs-project/execution/callback/openssh-10.5p1.tar.gz \
  "$LFS/sources/openssh-10.5p1.tar.gz"
install -Dm0600 /tmp/flowlfs-authorized-key.pub \
  "$LFS/root/flowlfs-authorized-key.pub"
install -Dm0755 /mnt/flowlfs-project/scripts/build-callback-chroot.sh \
  "$LFS/root/build-callback-chroot.sh"

chroot "$LFS" /usr/bin/env -i \
  HOME=/root TERM="${TERM:-dumb}" PATH=/usr/bin:/usr/sbin \
  MAKEFLAGS="-j$(nproc)" \
  /bin/bash --login /root/build-callback-chroot.sh
