#!/usr/bin/env bash

set -euo pipefail

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
project_dir=$(CDPATH= cd -- "$script_dir/.." && pwd)
state_dir="$project_dir/builder/state"
builder_disk="$state_dir/builder-os.qcow2"
builder_base="$state_dir/builder-base.qcow2"

test -s "$builder_disk"
if test -e "$builder_base"; then
  printf '%s\n' 'Refusing to replace an existing sealed builder base.' >&2
  exit 1
fi

qemu-img check "$builder_disk"
mv -- "$builder_disk" "$builder_base"
chmod 0444 "$builder_base"
(
  cd "$state_dir"
  qemu-img create -f qcow2 -F qcow2 -b builder-base.qcow2 builder-os.qcow2
)
sha256sum "$builder_base"
qemu-img info --backing-chain "$builder_disk"
