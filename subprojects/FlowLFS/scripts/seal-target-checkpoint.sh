#!/usr/bin/env bash

set -euo pipefail

if test "$#" -ne 1 || [[ "$1" != +([a-z0-9-]) ]]; then
  printf '%s\n' 'Usage: seal-target-checkpoint.sh <lowercase-checkpoint-name>' >&2
  exit 1
fi

script_dir=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)
project_dir=$(CDPATH='' cd -- "$script_dir/.." && pwd)
state_dir="$project_dir/builder/state"
target="$state_dir/flowlfs-target.qcow2"
checkpoint="$state_dir/flowlfs-target-$1.qcow2"

test -s "$target"
if test -e "$checkpoint"; then
  printf 'Refusing to replace existing checkpoint: %s\n' "$checkpoint" >&2
  exit 1
fi

qemu-img check "$target"
mv -- "$target" "$checkpoint"
chmod 0444 "$checkpoint"
(
  cd "$state_dir"
  qemu-img create -f qcow2 -F qcow2 \
    -b "$(basename "$checkpoint")" "$(basename "$target")"
)
sha256sum "$checkpoint"
qemu-img info --backing-chain "$target"
