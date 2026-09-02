#!/usr/bin/env bash
set -euo pipefail
root=$(CDPATH='' cd -- "$(dirname -- "$0")/../../.." && pwd); cd "$root"
base=subprojects/FlowLFS/artifacts/FlowLFS-v0.1-desktop-v0.qcow2
working=subprojects/FlowLFS/artifacts/FlowLFS-v0.1-desktop-v0.1-working.qcow2
final=subprojects/FlowLFS/artifacts/FlowLFS-v0.1-desktop-v0.1.qcow2
serial=subprojects/FlowLFS/artifacts/FlowLFS-v0.1-desktop-v0.1-build-serial.log
source=subprojects/FlowLFS/flowpkg/sources/flow-desktop-defaults/0.2.0/flow-desktop-defaults-0.2.0.tar.xz
guest=subprojects/FlowLFS/scripts/flowpkg-desktop-defaults-v0.2-guest.sh
source_sha=36464f2a63ebda8aa419c984b0344693dbf33f9a442207496a9893153403ad63
port=2304; known=/tmp/flowlfs-desktop-v0.1-build-known-hosts; qemu_pid=
die(){ printf 'FLOWLFS_DESKTOP_BUILD_ERROR %s\n' "$*" >&2; exit 1; }
cleanup(){ local rc=$?; if test -n "${qemu_pid:-}" && kill -0 "$qemu_pid" 2>/dev/null; then kill "$qemu_pid" 2>/dev/null||:; wait "$qemu_pid" 2>/dev/null||:; fi; if test "$rc" -ne 0 -a -e "$working"; then chmod u+w "$working" 2>/dev/null||:; rm -f "$working"; fi; return "$rc"; }
trap cleanup EXIT INT TERM
test -r "$base" -a -r "$source" -a -r "$guest"||die inputs
test ! -e "$working" -a ! -e "$final"||die output-exists
printf '%s  %s\n' "$source_sha" "$source"|sha256sum -c -
cp --reflink=auto "$base" "$working"; chmod u+w "$working"
accel=tcg; cpu=max; if test -r /dev/kvm -a -w /dev/kvm; then accel=kvm; cpu=host; fi
qemu-system-x86_64 -name flowlfs-desktop-v0.1-build -machine "q35,accel=$accel" -cpu "$cpu" -smp 2 -m 2G -display none -monitor none -serial "file:$serial" -drive "file=$working,if=virtio,format=qcow2" -nic "user,model=virtio-net-pci,hostfwd=tcp:127.0.0.1:$port-:22" & qemu_pid=$!
ssh_vm(){ ssh -n -p "$port" -o BatchMode=yes -o ConnectTimeout=2 -o StrictHostKeyChecking=no -o UserKnownHostsFile="$known" root@127.0.0.1 "$@"; }
ready=no; for n in $(seq 1 90); do ssh_vm true >/dev/null 2>&1&&{ ready=yes; break; }; sleep 1; done; test "$ready" = yes||die vm-timeout
scp -P "$port" -o StrictHostKeyChecking=no -o UserKnownHostsFile="$known" "$source" "$guest" root@127.0.0.1:/root/
ssh_vm "set -e; install -m0755 /root/flowpkg-desktop-defaults-v0.2-guest.sh /usr/local/sbin/flowpkg-desktop-defaults-v0.2; FLOWPKG_SOURCE_SHA256=$source_sha /usr/local/sbin/flowpkg-desktop-defaults-v0.2 prepare; install -o flowbuilder -g flowbuilder -m0644 /root/flow-desktop-defaults-0.2.0.tar.xz /var/tmp/flowpkg-input/flow-desktop-defaults-0.2.0/; su -s /bin/bash flowbuilder -c 'FLOWPKG_SOURCE_SHA256=$source_sha /usr/local/sbin/flowpkg-desktop-defaults-v0.2 build'; FLOWPKG_SOURCE_SHA256=$source_sha /usr/local/sbin/flowpkg-desktop-defaults-v0.2 admit; FLOWPKG_SOURCE_SHA256=$source_sha /usr/local/sbin/flowpkg-desktop-defaults-v0.2 project; FLOWPKG_SOURCE_SHA256=$source_sha /usr/local/sbin/flowpkg-desktop-defaults-v0.2 verify; object=\$(readlink -f /flow/store/packages/flow-desktop-defaults/0.2.0/object); state=/flow/store/profile-realizations/desktop-v0.1; install -d -m0755 \"\$state\"; printf 'flow-desktop-defaults\t0.2.0\t%s\n' \"\${object##*/}\" >\"\$state/active.tsv\"; fc-cache -f; fc-match monospace; foot --version; sync; systemctl poweroff"
wait "$qemu_pid"; qemu_pid=
mv "$working" "$final"; chmod a-w "$final"; qemu-img check "$final"
sha=$(sha256sum "$final"|awk '{print $1}'); printf '%s  %s\n' "$sha" "$(basename "$final")">"$final.sha256"
printf 'FLOWLFS_DESKTOP_BUILD_PASS image=%s sha256=%s\n' "$final" "$sha"
trap - EXIT INT TERM
