#!/usr/bin/env bash
set -euo pipefail
root=$(CDPATH='' cd -- "$(dirname -- "$0")/../../.." && pwd); cd "$root"
base=subprojects/FlowLFS/artifacts/FlowLFS-v0.1-desktop-foundation-v0.qcow2
working=subprojects/FlowLFS/artifacts/FlowLFS-v0.1-desktop-v0-working.qcow2
final=subprojects/FlowLFS/artifacts/FlowLFS-v0.1-desktop-v0.qcow2
serial=subprojects/FlowLFS/artifacts/FlowLFS-v0.1-desktop-v0-build-serial.log
source=subprojects/FlowLFS/flowpkg/sources/flow-desktop-defaults/0.1.0/flow-desktop-defaults-0.1.0.tar.xz
guest=subprojects/FlowLFS/scripts/flowpkg-desktop-defaults-guest.sh
source_sha=9fbd217dfb34fade5dd9a65c641df79ceebeb1f405888773cbaba1dd3db101a1
port=2304; known=/tmp/flowlfs-desktop-v0-build-known-hosts; qemu_pid=
die(){ printf 'FLOWLFS_DESKTOP_BUILD_ERROR %s\n' "$*" >&2; exit 1; }
cleanup(){ local rc=$?; if test -n "${qemu_pid:-}" && kill -0 "$qemu_pid" 2>/dev/null; then kill "$qemu_pid" 2>/dev/null||:; wait "$qemu_pid" 2>/dev/null||:; fi; if test "$rc" -ne 0 -a -e "$working"; then chmod u+w "$working" 2>/dev/null||:; rm -f "$working"; fi; return "$rc"; }
trap cleanup EXIT INT TERM
test -r "$base" -a -r "$source" -a -r "$guest"||die inputs
test ! -e "$working" -a ! -e "$final"||die output-exists
printf '%s  %s\n' "$source_sha" "$source"|sha256sum -c -
cp --reflink=auto "$base" "$working"; chmod u+w "$working"
accel=tcg; cpu=max; if test -r /dev/kvm -a -w /dev/kvm; then accel=kvm; cpu=host; fi
qemu-system-x86_64 -name flowlfs-desktop-v0-build -machine "q35,accel=$accel" -cpu "$cpu" -smp 2 -m 2G -display none -monitor none -serial "file:$serial" -drive "file=$working,if=virtio,format=qcow2" -nic "user,model=virtio-net-pci,hostfwd=tcp:127.0.0.1:$port-:22" & qemu_pid=$!
ssh_vm(){ ssh -n -p "$port" -o BatchMode=yes -o ConnectTimeout=2 -o StrictHostKeyChecking=no -o UserKnownHostsFile="$known" root@127.0.0.1 "$@"; }
ready=no; for n in $(seq 1 90); do ssh_vm true >/dev/null 2>&1&&{ ready=yes; break; }; sleep 1; done; test "$ready" = yes||die vm-timeout
scp -P "$port" -o StrictHostKeyChecking=no -o UserKnownHostsFile="$known" "$source" "$guest" root@127.0.0.1:/root/
ssh_vm "set -e; install -m0755 /root/flowpkg-desktop-defaults-guest.sh /usr/local/sbin/flowpkg-desktop-defaults; FLOWPKG_SOURCE_SHA256=$source_sha /usr/local/sbin/flowpkg-desktop-defaults prepare; install -o flowbuilder -g flowbuilder -m0644 /root/flow-desktop-defaults-0.1.0.tar.xz /var/tmp/flowpkg-input/flow-desktop-defaults-0.1.0/; su -s /bin/bash flowbuilder -c 'FLOWPKG_SOURCE_SHA256=$source_sha /usr/local/sbin/flowpkg-desktop-defaults build'; FLOWPKG_SOURCE_SHA256=$source_sha /usr/local/sbin/flowpkg-desktop-defaults admit; FLOWPKG_SOURCE_SHA256=$source_sha /usr/local/sbin/flowpkg-desktop-defaults project; FLOWPKG_SOURCE_SHA256=$source_sha /usr/local/sbin/flowpkg-desktop-defaults verify; object=\$(readlink -f /flow/store/packages/flow-desktop-defaults/0.1.0/object); state=/flow/store/profile-realizations/desktop-v0; install -d -m0755 \"\$state\"; printf 'flow-desktop-defaults\t0.1.0\t%s\n' \"\${object##*/}\" >\"\$state/active.tsv\"; fc-cache -f; fc-match monospace; foot --version; sync; systemctl poweroff"
wait "$qemu_pid"; qemu_pid=
mv "$working" "$final"; chmod a-w "$final"; qemu-img check "$final"
sha=$(sha256sum "$final"|awk '{print $1}'); printf '%s  %s\n' "$sha" "$(basename "$final")">"$final.sha256"
printf 'FLOWLFS_DESKTOP_BUILD_PASS image=%s sha256=%s\n' "$final" "$sha"
trap - EXIT INT TERM
