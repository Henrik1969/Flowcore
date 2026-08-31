#!/usr/bin/env bash
set -euo pipefail
profile=basis-v2; state_root=/flow/store/profile-realizations/$profile
packages=(zsh flow-basis flow-terminal sel)
versions=(5.9.2 0.1.0 1.0.0 0.2.0)
objects=(
 sha256-35919dcaea12c0c67ea22ef0598feb20148c6be78d331bd84aa117a8bdcd9b73
 sha256-f2cf1b94f4edc7289d92b60a203d659c3a880ddc35ba6081f97bc5d60348a2b9
 sha256-ad1b715e2c2d8e390444ea69a9603901b946a5a2dbb80759fd0d5cdc9f6ddec2
 sha256-597de8dc8a3f883aade4e219b44bfa141f164acbf1e0a398475542750272120b
)
die(){ printf 'FLOWPROFILE_BASIS_V2_ERROR %s\n' "$*" >&2; exit 1; }
test "$(id -u)" -eq 0||die 'owner projection authority requires root'
transaction(){ case "$1" in zsh) /usr/local/sbin/flowpkg-zsh "$2";; flow-basis) /usr/local/sbin/flowpkg-basis "$2";; flow-terminal) /usr/local/sbin/flowpkg-flow-terminal "$2";; sel) /usr/local/sbin/flowpkg-sel-v0.2 "$2";; esac; }
preflight(){ for i in "${!packages[@]}"; do p=${packages[$i]}; v=${versions[$i]}; expected=/flow/store/objects/${objects[$i]}; pointer=/flow/store/packages/$p/$v/object; test -L "$pointer"||die "missing pointer $p@$v"; test "$(readlink -f "$pointer")" = "$expected"||die "object lock mismatch $p@$v"; test -d "$expected/root" -a -f "$expected/manifest.tsv" -a -f "$expected/derivation.txt"||die "incomplete object $expected"; done; printf 'FLOWPROFILE_BASIS_V2_PREFLIGHT_PASS packages=%s\n' "${#packages[@]}"; }
activate(){ preflight; install -d -m0755 "$state_root"; :>"$state_root/active.tsv.incoming"; for i in "${!packages[@]}"; do p=${packages[$i]}; v=${versions[$i]}; test -d "/flow/store/projections/$p-$v"||transaction "$p" project; transaction "$p" verify; printf '%s\t%s\t%s\n' "$p" "$v" "${objects[$i]}">>"$state_root/active.tsv.incoming"; done; mv "$state_root/active.tsv.incoming" "$state_root/active.tsv"; printf 'FLOWPROFILE_BASIS_V2_ACTIVATE_PASS mode=offline\n'; }
deactivate(){ test -f "$state_root/active.tsv"||die 'profile inactive'; for ((i=${#packages[@]}-1;i>=0;i--)); do p=${packages[$i]}; v=${versions[$i]}; test -d "/flow/store/projections/$p-$v"&&transaction "$p" rollback; done; mv "$state_root/active.tsv" "$state_root/inactive.$(date -u +%Y%m%dT%H%M%SZ).tsv"; printf 'FLOWPROFILE_BASIS_V2_DEACTIVATE_PASS\n'; }
verify(){ preflight; test -f "$state_root/active.tsv"||die 'active realization absent'; transaction flow-basis verify; transaction flow-terminal verify; transaction sel verify; test "$(getent passwd root|cut -d: -f7)" = /bin/bash||die 'recovery shell changed'; systemctl is-active --quiet sshd.service||die 'sshd inactive'; printf 'FLOWPROFILE_BASIS_V2_VERIFY_PASS packages=%s terminal_projection=insulated\n' "${#packages[@]}"; }
status(){ if test -f "$state_root/active.tsv"; then printf 'active\n'; cat "$state_root/active.tsv"; else printf 'inactive\n'; fi; }
case "${1:-}" in preflight)preflight;;activate)activate;;deactivate)deactivate;;verify)verify;;status)status;;*)die 'usage: preflight|activate|deactivate|verify|status';;esac
