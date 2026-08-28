#!/usr/bin/env bash

set -euo pipefail

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
project_dir=$(CDPATH= cd -- "$script_dir/.." && pwd)
builder_dir="$project_dir/builder"
medium=$(cat "$builder_dir/manifests/medium")
iso="$builder_dir/media/$medium"
builder_disk="$builder_dir/state/builder-os.qcow2"
target_disk="$builder_dir/state/flowlfs-target.qcow2"
kernel="$builder_dir/state/installer-vmlinuz"
initrd="$builder_dir/state/installer-initrd"
seed_dir="$builder_dir/seed"

"$script_dir/verify-builder-media.sh"
test -s "$builder_disk"
test -s "$target_disk"
test -s "$seed_dir/user-data"
test -s "$seed_dir/meta-data"

if ! test -s "$kernel" || ! test -s "$initrd"; then
  xorriso -osirrox on -indev "$iso" \
    -extract /casper/vmlinuz "$kernel" \
    -extract /casper/initrd "$initrd"
fi

accel=tcg
cpu=max
if test -r /dev/kvm && test -w /dev/kvm; then
  accel=kvm
  cpu=host
fi

printf 'Launching with QEMU acceleration: %s\n' "$accel"
python3 -m http.server 3003 --bind 127.0.0.1 --directory "$builder_dir" \
  >"$builder_dir/evidence/autoinstall-http.log" 2>&1 &
seed_server=$!
trap 'kill "$seed_server" 2>/dev/null || true' EXIT

qemu-system-x86_64 \
  -name flowlfs-builder-install \
  -machine "q35,accel=$accel" -cpu "$cpu" -smp 4 -m 8G \
  -no-reboot -nographic \
  -drive "file=$builder_disk,if=virtio,format=qcow2" \
  -drive "file=$iso,media=cdrom,readonly=on" \
  -nic user,model=virtio-net-pci \
  -kernel "$kernel" -initrd "$initrd" \
  -append 'autoinstall ds=nocloud-net;s=http://10.0.2.2:3003/seed/ console=ttyS0,115200n8'
