#!/usr/bin/env bash

set -euo pipefail

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
project_dir=$(CDPATH= cd -- "$script_dir/.." && pwd)
state_dir="$project_dir/builder/state"
builder_disk="$state_dir/builder-os.qcow2"
builder_base="$state_dir/builder-base.qcow2"
target_disk="$state_dir/flowlfs-target.qcow2"

mkdir -p -- "$state_dir"
if test -e "$builder_disk" || test -e "$builder_base" || test -e "$target_disk"; then
  printf '%s\n' 'Refusing to replace an existing builder or target disk.' >&2
  exit 1
fi

qemu-img create -f qcow2 "$builder_disk" 32G
qemu-img create -f qcow2 "$target_disk" 40G
qemu-img info "$builder_disk"
qemu-img info "$target_disk"
