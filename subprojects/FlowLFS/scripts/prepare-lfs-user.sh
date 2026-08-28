#!/usr/bin/env bash

set -euo pipefail

project_mount=/mnt/flowlfs-project
LFS=/mnt/lfs

if test "$(id -u)" -ne 0; then
  printf '%s\n' 'Run as root inside the disposable FlowLFS builder VM.' >&2
  exit 1
fi
mountpoint -q "$LFS"
test "$(findmnt -nro SOURCE "$LFS")" = /dev/vdb1
mountpoint -q "$project_mount"

mkdir -pv "$LFS"/{etc,var} "$LFS"/usr/{bin,lib,sbin}
for directory in bin lib sbin; do
  ln -sv "usr/$directory" "$LFS/$directory"
done
case $(uname -m) in
  x86_64) mkdir -pv "$LFS/lib64" ;;
esac
mkdir -pv "$LFS/tools"
test ! -e "$LFS/usr/lib64"

getent group lfs >/dev/null || groupadd lfs
id lfs >/dev/null 2>&1 || useradd -s /bin/bash -g lfs -m -k /dev/null lfs
chown -v lfs "$LFS"/usr "$LFS"/usr/{bin,lib,sbin} \
  "$LFS"/var "$LFS"/etc "$LFS"/tools
case $(uname -m) in
  x86_64) chown -v lfs "$LFS/lib64" ;;
esac

install -o lfs -g lfs -m 0644 \
  "$project_mount/construction/lfs.bash_profile" /home/lfs/.bash_profile
install -o lfs -g lfs -m 0644 \
  "$project_mount/construction/lfs.bashrc" /home/lfs/.bashrc

if test -e /etc/bash.bashrc && ! test -e /etc/bash.bashrc.NOUSE; then
  mv -v /etc/bash.bashrc /etc/bash.bashrc.NOUSE
fi

# Expansion belongs to the lfs-user shell.
# shellcheck disable=SC2016
runuser -u lfs -- env -i HOME=/home/lfs TERM="${TERM:-dumb}" \
  /bin/bash --noprofile --norc -c \
  'source "$HOME/.bashrc"; printf "LFS=%s\nLFS_TGT=%s\nLC_ALL=%s\nPATH=%s\nCONFIG_SITE=%s\nMAKEFLAGS=%s\numask=%s\n" "$LFS" "$LFS_TGT" "$LC_ALL" "$PATH" "$CONFIG_SITE" "$MAKEFLAGS" "$(umask)"'
printf '%s\n' 'Chapter 4 layout and lfs build environment complete.'
