#!/usr/bin/env bash
set -euo pipefail
script_dir=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)
project_dir=$(CDPATH='' cd -- "$script_dir/.." && pwd)
image="$project_dir/artifacts/FlowLFS-v0.1-basis-v1.qcow2"
serial_log="$project_dir/artifacts/FlowLFS-v0.1-basis-v1-serial.log"
callback_port=${FLOWLFS_CALLBACK_PORT:-2228}
case "$callback_port" in ''|*[!0-9]*) printf 'FLOWLFS_CALLBACK_PORT must be numeric\n' >&2; exit 2;; esac
test "$callback_port" -ge 1024 -a "$callback_port" -le 65535; test -f "$image"
accel=tcg; cpu=max
if test -r /dev/kvm && test -w /dev/kvm; then accel=kvm; cpu=host; fi
printf 'Launching FlowLFS basis v1 with QEMU acceleration: %s\n' "$accel"
printf 'Callback phone: ssh://127.0.0.1:%s\n' "$callback_port"
printf 'Serial evidence: %s\n' "$serial_log"
exec qemu-system-x86_64 \
  -name flowlfs-basis-v1 \
  -machine "q35,accel=$accel" -cpu "$cpu" -smp 2 -m 2G \
  -display none -monitor none \
  -chardev "file,id=serial,path=$serial_log" -serial chardev:serial \
  -snapshot \
  -drive "file=$image,if=virtio,format=qcow2" \
  -nic "user,model=virtio-net-pci,hostfwd=tcp:127.0.0.1:${callback_port}-:22"
