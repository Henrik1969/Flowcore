#!/usr/bin/env bash
set -euo pipefail
profile=selection-providers-v0; state=/flow/store/profile-realizations/$profile
packages=(go fzf flow-selection); versions=(1.26.5 0.74.1 0.1.0)
objects=(sha256-31c8d748b54d460c4d114fcd9e94e551164cd109cdee52ddfd74bd3b1b503652 sha256-205dea12964e9664feaeb2987c815d15fbf9ace1fad3bd49ccb34708c8eebf7f sha256-a6b7bf1c6b5546bb7985230d4f2a8c91a21b33d03849789e455ba783786e055a)
die(){ echo "FLOWPROFILE_SELECTION_ERROR $*" >&2; exit 1; }; test "$(id -u)" -eq 0||die root
tx(){ case "$1" in go) /usr/local/sbin/flowpkg-go "$2";; fzf) /usr/local/sbin/flowpkg-fzf "$2";; flow-selection) /usr/local/sbin/flowpkg-flow-selection "$2";; esac; }
preflight(){ for i in "${!packages[@]}"; do p=${packages[$i]}; v=${versions[$i]}; expected=/flow/store/objects/${objects[$i]}; test "$(readlink -f /flow/store/packages/$p/$v/object)" = "$expected"||die "lock $p"; done; echo 'FLOWPROFILE_SELECTION_PREFLIGHT_PASS packages=3'; }
activate(){ preflight; mkdir -p "$state"; :>"$state/active.tsv.incoming"; for i in "${!packages[@]}"; do p=${packages[$i]}; v=${versions[$i]}; test -d "/flow/store/projections/$p-$v"||tx "$p" project; tx "$p" verify; printf '%s\t%s\t%s\n' "$p" "$v" "${objects[$i]}">>"$state/active.tsv.incoming"; done; mv "$state/active.tsv.incoming" "$state/active.tsv"; echo 'FLOWPROFILE_SELECTION_ACTIVATE_PASS default=native'; }
deactivate(){ test ! -e /etc/flowcore/selection/provider.local||die 'owner provider state must be reset or preserved explicitly before profile rollback'; for ((i=2;i>=0;i--)); do p=${packages[$i]}; v=${versions[$i]}; tx "$p" rollback; done; mv "$state/active.tsv" "$state/inactive.$(date +%s).tsv"; echo 'FLOWPROFILE_SELECTION_DEACTIVATE_PASS'; }
verify(){ preflight; test -f "$state/active.tsv"; /usr/local/sbin/flowpkg-go verify; /usr/local/sbin/flowpkg-fzf verify; /usr/local/sbin/flowpkg-flow-selection verify; systemctl is-active --quiet sshd.service; echo 'FLOWPROFILE_SELECTION_VERIFY_PASS providers=native,fzf default=native'; }
status(){ test -f "$state/active.tsv"&&{ echo active; cat "$state/active.tsv"; }||echo inactive; }
case "${1:-}" in preflight)preflight;;activate)activate;;deactivate)deactivate;;verify)verify;;status)status;;*)die usage;;esac
