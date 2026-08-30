#!/usr/bin/env bash

set -euo pipefail
test "$(id -u)" -eq 0
LFS=/mnt/lfs
test "$(findmnt -nro SOURCE "$LFS")" = /dev/vdb1
test "$(blkid -s LABEL -o value /dev/vdb1)" = FLOWLFS_ROOT
test -x "$LFS/usr/bin/bash"

mkdir -pv "$LFS"/{dev,proc,sys,run}
mountpoint -q "$LFS/dev" || mount -v --bind /dev "$LFS/dev"
mountpoint -q "$LFS/dev/pts" || mount -vt devpts devpts -o gid=5,mode=0620 "$LFS/dev/pts"
mountpoint -q "$LFS/proc" || mount -vt proc proc "$LFS/proc"
mountpoint -q "$LFS/sys" || mount -vt sysfs sysfs "$LFS/sys"
mountpoint -q "$LFS/run" || mount -vt tmpfs tmpfs "$LFS/run"
if test -h "$LFS/dev/shm"; then
  install -v -d -m 1777 "$LFS$(realpath /dev/shm)"
elif ! mountpoint -q "$LFS/dev/shm"; then
  mount -vt tmpfs -o nosuid,nodev tmpfs "$LFS/dev/shm"
fi

install -Dm0755 /mnt/flowlfs-project/scripts/build-chapters09-10-chroot.sh \
  "$LFS/root/build-chapters09-10-chroot.sh"
chroot "$LFS" /usr/bin/env -i \
  HOME=/root TERM="${TERM:-dumb}" PATH=/usr/bin:/usr/sbin \
  MAKEFLAGS="-j$(nproc)" \
  /bin/bash --login /root/build-chapters09-10-chroot.sh
