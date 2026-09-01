#!/usr/bin/env bash
set -euo pipefail
root=$(CDPATH='' cd -- "$(dirname -- "$0")/../../.." && pwd)
image=$root/subprojects/FlowLFS/artifacts/FlowLFS-v0.1-desktop-foundation-v0.qcow2
test -r "$image"||{ printf 'missing image: %s\n' "$image" >&2; exit 1; }
accel=tcg; cpu=max
if test -r /dev/kvm -a -w /dev/kvm; then accel=kvm; cpu=host; fi
known=/tmp/flowlfs-desktop-foundation-v0-known-hosts
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
  -name flowlfs-desktop-foundation-v0 \
  -machine "q35,accel=$accel" -cpu "$cpu" -smp 2 -m 2G \
  -display gtk,gl=off,show-tabs=on,show-menubar=on,zoom-to-fit=on,grab-on-hover=on -monitor none \
  -serial file:"$root/subprojects/FlowLFS/artifacts/FlowLFS-v0.1-desktop-foundation-v0-runtime-serial.log" \
  -snapshot -drive "file=$image,if=virtio,format=qcow2" \
  -vga virtio -device virtio-keyboard-pci -device virtio-tablet-pci \
  -nic user,model=virtio-net-pci,hostfwd=tcp:127.0.0.1:2302-:22 &
qemu_system_pid=$!
ssh_vm(){ ssh -n -p 2302 -o BatchMode=yes -o ConnectTimeout=2 -o StrictHostKeyChecking=no -o UserKnownHostsFile="$known" root@127.0.0.1 "$@"; }
ready=no
for n in $(seq 1 90); do
  ssh_vm true >/dev/null 2>&1&&{ ready=yes; break; }
  kill -0 "$qemu_system_pid" 2>/dev/null||{ printf 'VM exited before SSH became ready\n' >&2; exit 1; }
  sleep 1
done
test "$ready" = yes||{ printf 'VM SSH callback did not become ready\n' >&2; exit 1; }
if ! ssh_vm 'set -e; install -d -m0700 /run/user/0; fc-cache -f; export XDG_RUNTIME_DIR=/run/user/0 SEATD_VTBOUND=0; nohup seatd-launch -- weston -Bdrm --renderer=pixman --idle-time=0 >/var/log/flow-desktop-session.log 2>&1 </dev/null & echo $! >/run/flow-desktop-session.pid; for n in $(seq 1 20); do socket=$(find /run/user/0 -maxdepth 1 -type s -name "wayland-*" -print -quit); test -n "$socket"&&break; sleep 1; done; test -n "${socket:-}"; display=$(basename "$socket"); printf "%s\n" "$display" >/run/flow-wayland-display; WAYLAND_DISPLAY="$display" nohup foot --log-level=info >/var/log/flow-foot.log 2>&1 </dev/null & echo $! >/run/flow-foot.pid; for n in $(seq 1 20); do kill -0 "$(cat /run/flow-foot.pid)" 2>/dev/null&&break; sleep 1; done; kill -0 "$(cat /run/flow-foot.pid)"'; then
  ssh_vm 'cat /var/log/flow-desktop-session.log /var/log/flow-foot.log 2>/dev/null || true' >&2||:
  printf 'Desktop foundation failed to become ready\n' >&2
  exit 1
fi
printf 'FLOWLFS_DESKTOP_FOUNDATION_READY ssh=root@127.0.0.1:2302 terminal=foot\n'
wait "$qemu_system_pid"
qemu_system_pid=
trap - EXIT INT TERM
