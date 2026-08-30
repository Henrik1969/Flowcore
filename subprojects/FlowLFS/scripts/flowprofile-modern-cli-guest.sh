#!/usr/bin/env bash
set -euo pipefail
profile=modern-cli-v0; state_root=/flow/store/profile-realizations/$profile
packages=(libtasn1 p11-kit make-ca mozilla-certdata libunistring libidn2 nghttp2 libpsl curl git)
versions=(4.21.0 0.26.5 1.16.1 2026-08-31 1.4.2 2.3.8 1.70.0 0.23.3 8.21.0 2.55.0)
objects=(
 sha256-7f9517391aa6d7b2ce0459e9a5b6bed7f0d9a8cba9eb462b7afe6cd1470a500c
 sha256-127645fdf24a6adc2c6c54cfcca8fd4440ee5714e550f4749ff743b4a7cfa65a
 sha256-c00f252ba8a7290451a9ef839535492b8bf8e2ee8442710fffa6310bb80baf73
 sha256-618b86afac9f80b39b2c49d4f5b84ad44253dde8dd55baffe591e8d0e432eeee
 sha256-15f6c9b9134313be6dcdb44220aa88ffab6c6847d7720ea4c02241c66b1abe5d
 sha256-c5be992984715d83a1e9a00e836de18c2470d0f71af7bd8743d4ff6beca30ff1
 sha256-c328bd636352f153bef205eff692172222d05ef86d272ac262d21eec4f27548e
 sha256-82970094b553455d47b71d68c2cc567bc5b287ae6ed732e5d2322f3083626c63
 sha256-08b6f02aab5f00cb530078b3eef5bad6de8d70fee8abb0585dc2ffd4e63188b0
 sha256-63564714a11cf9ad9a92447d8477ad8719c57636437548c58541cc876e039109
)
die(){ printf 'FLOWPROFILE_MODERN_CLI_ERROR %s\n' "$*" >&2; exit 1; }
test "$(id -u)" -eq 0 || die 'owner projection authority requires root'
transaction(){ case "$1:$2" in libtasn1:*) /usr/local/sbin/flowpkg-libtasn1 "$2";; p11-kit:*) /usr/local/sbin/flowpkg-p11-kit "$2";; make-ca:*) /usr/local/sbin/flowpkg-make-ca "$2";; mozilla-certdata:*) /usr/local/sbin/flowpkg-mozilla-certdata "$2";; git:*) /usr/local/sbin/flowpkg-git "$2";; *) /usr/local/sbin/flowpkg-modern-lib "$2" "$1";; esac; }
preflight(){ for i in "${!packages[@]}"; do p=${packages[$i]}; v=${versions[$i]}; expected=/flow/store/objects/${objects[$i]}; pointer=/flow/store/packages/$p/$v/object; test -L "$pointer" || die "missing object pointer $p@$v"; test "$(readlink -f "$pointer")" = "$expected" || die "object lock mismatch $p@$v"; test -d "$expected/root" -a -f "$expected/manifest.tsv" -a -f "$expected/derivation.txt" || die "incomplete object $expected"; done; printf 'FLOWPROFILE_MODERN_CLI_PREFLIGHT_PASS packages=%s\n' "${#packages[@]}"; }
activate(){ preflight; install -d -m0755 "$state_root"; :>"$state_root/active.tsv.incoming"; for i in "${!packages[@]}"; do p=${packages[$i]}; v=${versions[$i]}; if test ! -d "/flow/store/projections/$p-$v"; then transaction "$p" project; fi; FLOWPKG_OFFLINE=1 FLOWPKG_COMPOSED=1 transaction "$p" verify; printf '%s\t%s\t%s\n' "$p" "$v" "${objects[$i]}" >>"$state_root/active.tsv.incoming"; done; mv "$state_root/active.tsv.incoming" "$state_root/active.tsv"; printf 'FLOWPROFILE_MODERN_CLI_ACTIVATE_PASS mode=offline\n'; }
deactivate(){ test -f "$state_root/active.tsv" || die 'profile is not active'; for ((i=${#packages[@]}-1;i>=0;i--)); do p=${packages[$i]}; v=${versions[$i]}; test -d "/flow/store/projections/$p-$v" && transaction "$p" rollback; done; mv "$state_root/active.tsv" "$state_root/inactive.$(date -u +%Y%m%dT%H%M%SZ).tsv"; printf 'FLOWPROFILE_MODERN_CLI_DEACTIVATE_PASS\n'; }
verify(){ preflight; test -f "$state_root/active.tsv" || die 'active realization record absent'; /usr/bin/curl --fail --silent --show-error --proto '=https' --tlsv1.2 https://www.example.com/ >/dev/null; /usr/bin/git --version >/dev/null; test "$(systemctl is-enabled update-pki.timer 2>/dev/null || true)" != enabled || die 'automatic trust refresh enabled'; test "$(getent passwd root|cut -d: -f7)" = /bin/bash || die 'recovery shell changed'; systemctl is-active --quiet sshd.service || die 'sshd inactive'; printf 'FLOWPROFILE_MODERN_CLI_VERIFY_PASS packages=%s\n' "${#packages[@]}"; }
status(){ if test -f "$state_root/active.tsv"; then printf 'active\n'; cat "$state_root/active.tsv"; else printf 'inactive\n'; fi; }
case "${1:-}" in preflight)preflight;;activate)activate;;deactivate)deactivate;;verify)verify;;status)status;;*)die 'usage: preflight|activate|deactivate|verify|status';;esac
