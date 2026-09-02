#!/usr/bin/env bash
set -euo pipefail
root=$(CDPATH='' cd -- "$(dirname -- "$0")/../../.." && pwd)
export FLOWLFS_DESKTOP_IMAGE=$root/subprojects/FlowLFS/artifacts/FlowLFS-v0.1-desktop-v0.1.qcow2
export FLOWLFS_DESKTOP_NAME=flowlfs-desktop-v0.1
export FLOWLFS_DESKTOP_PORT=2308
exec "$root/subprojects/FlowLFS/scripts/launch-desktop-foundation-v0.sh"
