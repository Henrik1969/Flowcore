#!/bin/sh
set -eu
example_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT
"$example_dir/build-flow-less.sh" "$tmpdir" >&2
"$tmpdir/flow_less"
