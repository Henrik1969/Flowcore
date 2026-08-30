#!/usr/bin/env bash

set -euo pipefail
script_dir=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)
project_dir=$(CDPATH='' cd -- "$script_dir/.." && pwd)
twin="$project_dir/artifacts/FlowLFS-v0.1-flowcore-twin.qcow2"
serial_log="$project_dir/artifacts/FlowLFS-v0.1-flowcore-twin-serial.log"

"$script_dir/verify-baseline-purity.sh"
test -w "$twin"

accel=tcg
cpu=max
if test -r /dev/kvm && test -w /dev/kvm; then
  accel=kvm
  cpu=host
fi

printf 'Launching writable Flowcore twin with QEMU acceleration: %s\n' "$accel"
printf '%s\n' 'Callback phone: ssh://127.0.0.1:2222'
printf 'Serial evidence: %s\n' "$serial_log"

exec qemu-system-x86_64 \
  -name flowlfs-v0.1-flowcore-twin \
  -machine "q35,accel=$accel" -cpu "$cpu" -smp 2 -m 2G \
  -display none -monitor none \
  -chardev "file,id=serial,path=$serial_log" -serial chardev:serial \
  -drive "file=$twin,if=virtio,format=qcow2" \
  -nic user,model=virtio-net-pci,hostfwd=tcp:127.0.0.1:2222-:22
