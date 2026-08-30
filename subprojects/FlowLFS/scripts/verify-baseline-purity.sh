#!/usr/bin/env bash

set -euo pipefail
script_dir=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)
project_dir=$(CDPATH='' cd -- "$script_dir/.." && pwd)
expected=14c2baf7801ae33051de6b836b6d71e1edaf5262543e781b92bf4e42b994ef68
artifact="$project_dir/artifacts/FlowLFS-v0.1-x86_64.qcow2"
checkpoint="$project_dir/builder/state/flowlfs-target-baseline-ssh-fixed.qcow2"
twin="$project_dir/artifacts/FlowLFS-v0.1-flowcore-twin.qcow2"

verify_control() {
  local image=$1
  local actual mode attrs info

  test -f "$image"
  actual=$(sha256sum "$image" | cut -d' ' -f1)
  test "$actual" = "$expected" || {
    printf 'FAIL checksum: %s\n' "$image" >&2
    return 1
  }

  mode=$(stat -c '%A' "$image")
  case "$mode" in
    *w*)
      printf 'FAIL writable control: %s (%s)\n' "$image" "$mode" >&2
      return 1
      ;;
  esac

  attrs=$(lsattr -d "$image" | awk '{print $1}')
  case "$attrs" in
    *i*) ;;
    *)
      printf 'FAIL control lacks immutable flag: %s (%s)\n' "$image" "$attrs" >&2
      return 1
      ;;
  esac

  info=$(qemu-img info "$image")
  if grep -q '^backing file:' <<<"$info"; then
    printf 'FAIL control has backing file: %s\n' "$image" >&2
    return 1
  fi
  qemu-img check "$image" >/dev/null
  printf 'PASS pure control: %s\n' "$image"
}

verify_control "$artifact"
verify_control "$checkpoint"

if test -e "$twin"; then
  test "$(stat -c '%i' "$artifact")" != "$(stat -c '%i' "$twin")" || {
    echo 'FAIL twin is the same inode as the control artifact' >&2
    exit 1
  }
  test -w "$twin" || {
    echo 'FAIL Flowcore twin is not writable' >&2
    exit 1
  }
  printf 'PASS independent writable twin: %s\n' "$twin"
fi

printf 'FLOWLFS_BASELINE_PURITY_PASS sha256=%s\n' "$expected"
