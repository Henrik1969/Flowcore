#!/usr/bin/env bash

set -euo pipefail

if test "$(id -u)" -ne 0; then
  printf '%s\n' 'Run this script as root inside the disposable builder VM.' >&2
  exit 1
fi

export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install --yes \
  bash binutils bison build-essential coreutils diffutils findutils gawk gcc g++ \
  grep gzip m4 make patch perl python3 sed tar texinfo xz-utils \
  ca-certificates curl wget rsync parted e2fsprogs dosfstools grub-pc-bin

ln -sfn bash /bin/sh
printf '%s\n' 'Builder prerequisites installed; /bin/sh now resolves to Bash.'
