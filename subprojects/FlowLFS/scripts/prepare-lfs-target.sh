#!/usr/bin/env bash

set -euo pipefail

target=/dev/vdb
partition=/dev/vdb1
expected_size=42949672960
project_mount=/mnt/flowlfs-project
LFS=/mnt/lfs

if test "${1-}" != "--confirm-destroy=$target"; then
  printf 'Refusing target preparation without --confirm-destroy=%s\n' "$target" >&2
  exit 1
fi
if test "$(id -u)" -ne 0; then
  printf '%s\n' 'Run as root inside the disposable FlowLFS builder VM.' >&2
  exit 1
fi

test "$(lsblk -dnro TYPE "$target")" = disk
test "$(blockdev --getsize64 "$target")" -eq "$expected_size"
test "$(lsblk -nrpo NAME "$target" | wc -l)" -eq 1
test -z "$(lsblk -dnro FSTYPE,MOUNTPOINTS "$target" | tr -d ' ')"
test -z "$(wipefs --noheadings --output TYPE "$target")"

printf '%s\n' 'Pre-write target proof:'
lsblk -o NAME,SIZE,TYPE,FSTYPE,MOUNTPOINTS "$target"
qemu_disk_serial=$(udevadm info --query=property --name="$target" |
  sed -n 's/^ID_SERIAL=//p')
printf 'target=%s bytes=%s serial=%s\n' \
  "$target" "$expected_size" "${qemu_disk_serial:-unreported}"

parted --script --align optimal "$target" \
  mklabel msdos \
  mkpart primary ext4 1MiB 100% \
  set 1 boot on
partprobe "$target"
udevadm settle
test -b "$partition"
mkfs -v -t ext4 -L FLOWLFS_ROOT "$partition"

mkdir -pv "$LFS"
mount -v -t ext4 "$partition" "$LFS"
chown root:root "$LFS"
chmod 755 "$LFS"
mountpoint -q "$LFS"
mount_options=$(findmnt -nro OPTIONS "$LFS")
case ",$mount_options," in
  *,nosuid,*|*,nodev,*)
    printf 'Refusing restrictive target mount options: %s\n' "$mount_options" >&2
    exit 1
    ;;
esac

mkdir -p "$project_mount"
if ! mountpoint -q "$project_mount"; then
  mount -t 9p -o trans=virtio,version=9p2000.L,ro \
    flowlfs_project "$project_mount"
fi

mkdir -v "$LFS/sources"
chmod -v a+wt "$LFS/sources"
manifest="$project_mount/execution/manifests/md5sums"
source_cache="$project_mount/execution/sources"
cp -- "$manifest" "$LFS/sources/md5sums"
while read -r _ filename; do
  test -f "$source_cache/$filename"
  cp -- "$source_cache/$filename" "$LFS/sources/$filename"
done < "$manifest"
(
  cd "$LFS/sources"
  md5sum --check md5sums
)
chown root:root "$LFS/sources"/*

printf '%s\n' 'Target preparation and authorized source staging complete.'
