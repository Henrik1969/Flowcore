#!/usr/bin/env bash
set -euo pipefail
d=$(CDPATH='' cd -- "$(dirname -- "$0")/.." && pwd); image="$d/artifacts/FlowLFS-v0.1-bulk-cargo-small-v0.qcow2"; port=${FLOWLFS_CALLBACK_PORT:-2244}; accel=tcg; cpu=max
test -f "$image"; if test -r /dev/kvm -a -w /dev/kvm; then accel=kvm; cpu=host; fi
printf 'Launching FlowLFS small Cargo bulk (%s)\nSSH: ssh -p %s root@127.0.0.1\n' "$accel" "$port"
exec qemu-system-x86_64 -name flowlfs-bulk-cargo-small-v0 -machine "q35,accel=$accel" -cpu "$cpu" -smp 4 -m 4G -display none -monitor none -chardev "file,id=serial,path=$d/artifacts/FlowLFS-v0.1-bulk-cargo-small-v0-serial.log" -serial chardev:serial -snapshot -drive "file=$image,if=virtio,format=qcow2" -nic "user,model=virtio-net-pci,hostfwd=tcp:127.0.0.1:${port}-:22"
