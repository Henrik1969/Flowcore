#!/usr/bin/env bash
set -euo pipefail
root=$(CDPATH='' cd -- "$(dirname -- "$0")/../../.." && pwd)
image=$root/subprojects/FlowLFS/artifacts/FlowLFS-v0.1-wayland-minimal-v0.qcow2
test -r "$image"||{ printf 'missing image: %s\n' "$image" >&2; exit 1; }
accel=tcg; cpu=max
if test -r /dev/kvm -a -w /dev/kvm; then accel=kvm; cpu=host; fi
known=/tmp/flowlfs-wayland-minimal-v0-known-hosts
qemu_system_pid=
cleanup(){
  local rc=$?
  if test -n "${qemu_system_pid:-}" && kill -0 "$qemu_system_pid" 2>/dev/null; then
    kill "$qemu_system_pid" 2>/dev/null||:
    wait "$qemu_system_pid" 2>/dev/null||:
  fi
  exit "$rc"
}
trap cleanup EXIT INT TERM
qemu-system-x86_64 \
  -name flowlfs-wayland-minimal-v0 \
  -machine "q35,accel=$accel" -cpu "$cpu" -smp 1 -m 2G \
  -display gtk,gl=off -monitor none \
  -serial file:"$root/subprojects/FlowLFS/artifacts/FlowLFS-v0.1-wayland-minimal-v0-runtime-serial.log" \
  -snapshot -drive "file=$image,if=virtio,format=qcow2" \
  -device virtio-vga,xres=1280,yres=800 -device virtio-keyboard-pci -device virtio-mouse-pci \
  -nic user,model=virtio-net-pci,hostfwd=tcp:127.0.0.1:2300-:22 &
qemu_system_pid=$!
ssh_vm(){ ssh -n -p 2300 -o BatchMode=yes -o ConnectTimeout=2 -o StrictHostKeyChecking=no -o UserKnownHostsFile="$known" root@127.0.0.1 "$@"; }
ready=no
for n in $(seq 1 90); do
  ssh_vm true >/dev/null 2>&1&&{ ready=yes; break; }
  kill -0 "$qemu_system_pid" 2>/dev/null||{ printf 'VM exited before SSH became ready\n' >&2; exit 1; }
  sleep 1
done
test "$ready" = yes||{ printf 'VM SSH callback did not become ready\n' >&2; exit 1; }
if ! ssh_vm 'set -e; install -d -m0700 /run/user/0; test ! -e /run/flow-wayland-session.pid || ! kill -0 "$(cat /run/flow-wayland-session.pid)" 2>/dev/null; export XDG_RUNTIME_DIR=/run/user/0 SEATD_VTBOUND=0; nohup seatd-launch -- weston -Bdrm --renderer=pixman --idle-time=0 >/var/log/flow-wayland-session.log 2>&1 </dev/null & echo $! >/run/flow-wayland-session.pid; for n in $(seq 1 20); do socket=$(find /run/user/0 -maxdepth 1 -type s -name "wayland-*" -print -quit); test -n "$socket"&&break; sleep 1; done; test -n "${socket:-}"; basename "$socket" >/run/flow-wayland-display'; then
  ssh_vm 'cat /var/log/flow-wayland-session.log 2>/dev/null || true' >&2||:
  printf 'Wayland session failed to become ready\n' >&2
  exit 1
fi
printf 'FLOWLFS_WAYLAND_READY ssh=root@127.0.0.1:2300\n'
wait "$qemu_system_pid"
qemu_system_pid=
trap - EXIT INT TERM
