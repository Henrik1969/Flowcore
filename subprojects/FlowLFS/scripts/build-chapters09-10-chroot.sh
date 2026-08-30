#!/usr/bin/env bash

set -euo pipefail
test "$(id -u)" -eq 0
test -r /proc/self/mounts
test "$(blkid -s LABEL -o value /dev/vdb1)" = FLOWLFS_ROOT

log_dir=/var/log/flowlfs/ch09-10
mkdir -p "$log_dir"
exec > >(tee -a "$log_dir/chapters09-10.log") 2>&1

run_step()
{
  local name=$1 marker="$log_dir/$1.done"
  shift
  if test -e "$marker"; then
    printf 'SKIP completed step: %s\n' "$name"
    return
  fi
  printf '\n===== START %s %s =====\n' "$name" "$(date --utc +'%Y-%m-%dT%H:%M:%SZ')"
  "$@"
  printf 'completed=%s\n' "$(date --utc +'%Y-%m-%dT%H:%M:%SZ')" > "$marker"
  printf '===== PASS %s =====\n' "$name"
}

configure_system()
{
  printf '%s\n' flowlfs-control > /etc/hostname
  cat > /etc/hosts <<'EOF'
127.0.0.1 localhost flowlfs-control
::1       localhost ip6-localhost ip6-loopback
ff02::1   ip6-allnodes
ff02::2   ip6-allrouters
EOF
  mkdir -p /etc/systemd/network
  cat > /etc/systemd/network/10-flowlfs-dhcp.network <<'EOF'
[Match]
Name=enp0s2

[Network]
DHCP=ipv4

[DHCPv4]
UseDomains=true
EOF
  systemctl enable systemd-networkd systemd-resolved
  ln -sfn /run/systemd/resolve/stub-resolv.conf /etc/resolv.conf
  printf '%s\n' 'LANG=en_US.UTF-8' > /etc/locale.conf
  cat > /etc/profile <<'EOF'
for i in $(locale); do unset "${i%=*}"; done
if [[ "$TERM" = linux ]]; then
  export LANG=C.UTF-8
else
  source /etc/locale.conf
  for i in $(locale); do
    key=${i%=*}
    if [[ -v $key ]]; then export "$key"; fi
  done
fi
EOF
  cat > /etc/vconsole.conf <<'EOF'
KEYMAP=dk
FONT=Lat2-Terminus16
EOF
  cat > /etc/adjtime <<'EOF'
0.0 0 0.0
0
UTC
EOF
  cat > /etc/inputrc <<'EOF'
set horizontal-scroll-mode Off
set meta-flag On
set input-meta On
set convert-meta Off
set output-meta On
set bell-style none
"\eOd": backward-word
"\eOc": forward-word
"\e[1~": beginning-of-line
"\e[4~": end-of-line
"\e[5~": beginning-of-history
"\e[6~": end-of-history
"\e[3~": delete-char
"\e[2~": quoted-insert
"\eOH": beginning-of-line
"\eOF": end-of-line
"\e[H": beginning-of-line
"\e[F": end-of-line
EOF
  mkdir -p /etc/systemd/system/getty@tty1.service.d
  cat > /etc/systemd/system/getty@tty1.service.d/noclear.conf <<'EOF'
[Service]
TTYVTDisallocate=no
EOF
  systemctl mask tmp.mount
  cat > /etc/fstab <<'EOF'
# file system        mount-point type options  dump fsck
LABEL=FLOWLFS_ROOT   /           ext4 defaults 1    1
EOF
}

build_kernel()
{
  cd /sources
  rm -rf linux-7.1.8
  tar -xf linux-7.1.8.tar.xz
  cd linux-7.1.8
  make mrproper
  make defconfig
  scripts/config --enable DEVTMPFS
  scripts/config --enable DEVTMPFS_MOUNT
  scripts/config --enable PCI
  scripts/config --enable VIRTIO
  scripts/config --enable VIRTIO_PCI
  scripts/config --enable VIRTIO_BLK
  scripts/config --enable VIRTIO_NET
  scripts/config --enable EXT4_FS
  scripts/config --enable SERIAL_8250
  scripts/config --enable SERIAL_8250_CONSOLE
  scripts/config --enable TMPFS
  scripts/config --enable CGROUPS
  scripts/config --enable FHANDLE
  scripts/config --enable INET
  scripts/config --enable UNIX
  make olddefconfig
  make
  make modules_install
  cp -v arch/x86/boot/bzImage /boot/vmlinuz-7.1.8-lfs-r13.0-201-systemd
  cp -v System.map /boot/System.map-7.1.8
  cp -v .config /boot/config-7.1.8
  cp -r Documentation -T /usr/share/doc/linux-7.1.8
  rm -rf /sources/linux-7.1.8
}

install_bootloader()
{
  local root_partuuid
  root_partuuid=$(blkid -s PARTUUID -o value /dev/vdb1)
  test -n "$root_partuuid"
  grub-install /dev/vdb --target=i386-pc
  cat > /boot/grub/grub.cfg <<EOF
set default=0
set timeout=5

insmod part_msdos
insmod ext2
search --no-floppy --label FLOWLFS_ROOT --set=root

menuentry "FlowLFS control, Linux 7.1.8" {
  linux /boot/vmlinuz-7.1.8-lfs-r13.0-201-systemd root=PARTUUID=$root_partuuid ro console=ttyS0,115200n8
}
EOF
}

run_step configuration configure_system
run_step kernel build_kernel
run_step grub install_bootloader

test -s /boot/vmlinuz-7.1.8-lfs-r13.0-201-systemd
test -s /boot/grub/grub.cfg
root_partuuid=$(blkid -s PARTUUID -o value /dev/vdb1)
grep -q "root=PARTUUID=$root_partuuid" /boot/grub/grub.cfg
printf '\nCHAPTERS 9-10 COMPLETE %s\n' "$(date --utc +'%Y-%m-%dT%H:%M:%SZ')"
