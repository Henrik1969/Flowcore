#!/usr/bin/env bash
set -euo pipefail

facade=${1:?facade path required}
tmpdir=$(mktemp -d /tmp/frankencore-ls-facade.XXXXXX)
trap 'rm -rf "$tmpdir"' EXIT
touch "$tmpdir/alpha" "$tmpdir/.hidden"
mkdir "$tmpdir/folder"

diff -u <(/usr/bin/ls -a "$tmpdir") <("$facade" -a "$tmpdir")
diff -u <(/usr/bin/ls -l "$tmpdir") <("$facade" -l "$tmpdir")
if "$facade" --Use_call_Policy=project-safe "$tmpdir" >/dev/null 2>&1; then
    echo 'facade unexpectedly accepted unresolved policy extension' >&2
    exit 1
fi
test "$({ "$facade" --Use_call_Policy=project-safe "$tmpdir" >/dev/null 2>&1; echo "$?"; })" -eq 2
echo 'franken-ls compatibility: PASS'
