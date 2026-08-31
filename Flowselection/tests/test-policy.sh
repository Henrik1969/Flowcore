#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
mkdir -p "$tmp/policy" "$tmp/root/usr/libexec/flowselection"
cp "$root/src/flowselect" "$tmp/flowselect"
if FLOW_SELECTION_POLICY_ROOT="$tmp/policy" "$tmp/flowselect" alpha beta >/dev/null 2>&1; then exit 1; fi
printf '%s\n' invented > "$tmp/policy/provider"
if FLOW_SELECTION_POLICY_ROOT="$tmp/policy" "$tmp/flowselect" alpha beta >/dev/null 2>&1; then exit 1; fi
printf '%s\n' fzf > "$tmp/policy/provider"
printf '%s\n' invented > "$tmp/policy/fallback"
if FLOW_SELECTION_POLICY_ROOT="$tmp/policy" "$tmp/flowselect" alpha beta >/dev/null 2>&1; then exit 1; fi
grep -q 'FZF_DEFAULT_OPTS=' "$root/src/flowselect-fzf"
grep -q 'SHELL=/bin/false' "$root/src/flowselect-fzf"
! grep -Eq -- '--preview |--bind |execute|reload|transform' "$root/src/flowselect-fzf"
echo 'FLOWSELECTION_POLICY_PASS implicit=denied fallback=explicit fzf_shell_authority=denied'
