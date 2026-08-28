#!/usr/bin/env bash

set -euo pipefail

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
project_dir=$(CDPATH= cd -- "$script_dir/.." && pwd)
builder_dir="$project_dir/builder"
builder_disk="$builder_dir/state/builder-os.qcow2"
target_disk="$builder_dir/state/flowlfs-target.qcow2"

test -s "$builder_disk"
test -s "$target_disk"

accel=tcg
cpu=max
if test -r /dev/kvm && test -w /dev/kvm; then
  accel=kvm
  cpu=host
fi

printf 'Launching with QEMU acceleration: %s\n' "$accel"
printf '%s\n' 'SSH is available at 127.0.0.1:2222 after the guest boots.'
exec qemu-system-x86_64 \
  -name flowlfs-builder \
  -machine "q35,accel=$accel" -cpu "$cpu" -smp 4 -m 8G \
  -nographic \
  -drive "file=$builder_disk,if=virtio,format=qcow2" \
  -drive "file=$target_disk,if=virtio,format=qcow2" \
  -nic user,model=virtio-net-pci,hostfwd=tcp:127.0.0.1:2222-:22 \
  -virtfs "local,path=$project_dir,mount_tag=flowlfs_project,security_model=none,readonly=on"
