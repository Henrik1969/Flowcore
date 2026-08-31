#!/usr/bin/env bash
set -euo pipefail
profile=basis-v1; state_root=/flow/store/profile-realizations/$profile
packages=(zsh flow-basis sel)
versions=(5.9.2 0.1.0 0.1.0)
objects=(
 sha256-35919dcaea12c0c67ea22ef0598feb20148c6be78d331bd84aa117a8bdcd9b73
 sha256-f2cf1b94f4edc7289d92b60a203d659c3a880ddc35ba6081f97bc5d60348a2b9
 sha256-fe9a6f9eaedef7679e71948692857e1cafcd285a8f3de9de650f1b22746ded6e
)
die(){ printf 'FLOWPROFILE_BASIS_ERROR %s\n' "$*" >&2; exit 1; }
test "$(id -u)" -eq 0||die 'owner projection authority requires root'
transaction(){ case "$1" in zsh) /usr/local/sbin/flowpkg-zsh "$2";; flow-basis) /usr/local/sbin/flowpkg-basis "$2";; sel) /usr/local/sbin/flowpkg-sel "$2";; esac; }
preflight(){ for i in "${!packages[@]}"; do p=${packages[$i]}; v=${versions[$i]}; expected=/flow/store/objects/${objects[$i]}; pointer=/flow/store/packages/$p/$v/object; test -L "$pointer"||die "missing pointer $p@$v"; test "$(readlink -f "$pointer")" = "$expected"||die "object lock mismatch $p@$v"; test -d "$expected/root" -a -f "$expected/manifest.tsv" -a -f "$expected/derivation.txt"||die "incomplete object $expected"; done; printf 'FLOWPROFILE_BASIS_PREFLIGHT_PASS packages=%s\n' "${#packages[@]}"; }
activate(){ preflight; install -d -m0755 "$state_root"; :>"$state_root/active.tsv.incoming"; for i in "${!packages[@]}"; do p=${packages[$i]}; v=${versions[$i]}; test -d "/flow/store/projections/$p-$v"||transaction "$p" project; transaction "$p" verify; printf '%s\t%s\t%s\n' "$p" "$v" "${objects[$i]}">>"$state_root/active.tsv.incoming"; done; mv "$state_root/active.tsv.incoming" "$state_root/active.tsv"; printf 'FLOWPROFILE_BASIS_ACTIVATE_PASS mode=offline\n'; }
deactivate(){ test -f "$state_root/active.tsv"||die 'profile inactive'; for ((i=${#packages[@]}-1;i>=0;i--)); do p=${packages[$i]}; v=${versions[$i]}; test -d "/flow/store/projections/$p-$v"&&transaction "$p" rollback; done; mv "$state_root/active.tsv" "$state_root/inactive.$(date -u +%Y%m%dT%H%M%SZ).tsv"; printf 'FLOWPROFILE_BASIS_DEACTIVATE_PASS\n'; }
verify(){ preflight; test -f "$state_root/active.tsv"||die 'active realization absent'; transaction flow-basis verify; transaction sel verify; test "$(getent passwd root|cut -d: -f7)" = /bin/bash||die 'recovery shell changed'; systemctl is-active --quiet sshd.service||die 'sshd inactive'; printf 'FLOWPROFILE_BASIS_VERIFY_PASS packages=%s\n' "${#packages[@]}"; }
status(){ if test -f "$state_root/active.tsv"; then printf 'active\n'; cat "$state_root/active.tsv"; else printf 'inactive\n'; fi; }
case "${1:-}" in preflight)preflight;;activate)activate;;deactivate)deactivate;;verify)verify;;status)status;;*)die 'usage: preflight|activate|deactivate|verify|status';;esac
