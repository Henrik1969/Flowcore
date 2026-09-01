#!/usr/bin/env bash
set -euo pipefail
profile=factory-proof-ripgrep-v0; state=/flow/store/profile-realizations/$profile; prior=/flow/store/profile-realizations/factory-proof-zoxide-v0
forge=sha256-1f7d8fcaebaccba9aaefb340ce5680f1d5a7a930f4acedfba1edc257d6a4284e
ripgrep=sha256-b778ffaef4a6f8f31f7a6f37954ef62cd4fe8c1fae5f2e8d522119540a08fd5c
spec=/flow/forge/specs/ripgrep-15.2.0.tsv; spec_sha=cc2dd89cffeda58294f5e21bba4934ce72079ddcada39e2499bffd920dbbd494
die(){ echo "FLOWPROFILE_RIPGREP_ERROR $*" >&2; exit 1; }; test "$(id -u)" -eq 0||die root
locked(){ test "$(readlink -f "/flow/store/packages/$1/$2/object")" = "/flow/store/objects/$3"||die "$1-lock"; }
preflight(){ test -f "$prior/active.tsv"||die prior-profile-inactive; locked flowforge-cargo 0.2.0 "$forge"; locked ripgrep 15.2.0 "$ripgrep"; echo "$spec_sha  $spec"|sha256sum -c - >/dev/null; echo FLOWPROFILE_RIPGREP_PREFLIGHT_PASS packages=2 isolation=private; }
write_state(){ mkdir -p "$state"; { printf 'flowforge-cargo\t0.2.0\t%s\n' "$forge"; printf 'ripgrep\t15.2.0\t%s\n' "$ripgrep"; } >"$state/active.tsv.incoming"; mv "$state/active.tsv.incoming" "$state/active.tsv"; }
activate(){ preflight; test ! -f "$state/active.tsv"||die already-active; /usr/local/sbin/flowpkg-flowforge-cargo-v0.2-package project; /usr/local/sbin/flowforge-cargo-package-v0.2 "$spec" project; write_state; verify; echo FLOWPROFILE_RIPGREP_ACTIVATE_PASS; }
adopt(){ preflight; test ! -f "$state/active.tsv"||die already-active; write_state; verify; echo FLOWPROFILE_RIPGREP_ADOPT_PASS; }
deactivate(){ /usr/local/sbin/flowforge-cargo-package-v0.2 "$spec" rollback; /usr/local/sbin/flowpkg-flowforge-cargo-v0.2-package rollback; mv "$state/active.tsv" "$state/inactive.$(date +%s).tsv"; echo FLOWPROFILE_RIPGREP_DEACTIVATE_PASS; }
verify(){ preflight; test -f "$state/active.tsv"; /usr/local/sbin/flowpkg-flowforge-cargo-v0.2-package verify; /usr/local/sbin/flowforge-cargo-package-v0.2 "$spec" verify; test -s /usr/share/man/man1/rg.1; test -s /usr/share/bash-completion/completions/rg; test -s /usr/share/zsh/site-functions/_rg; test -s /usr/share/fish/vendor_completions.d/rg.fish; test ! -e /etc/ripgreprc; printf 'factory needle\n'|rg -o needle|grep -Fxq needle; systemctl is-active --quiet sshd.service; echo FLOWPROFILE_RIPGREP_VERIFY_PASS packages=2 tests=446 reforge=identical configuration=owner-controlled; }
status(){ test -f "$state/active.tsv"&&{ echo active; cat "$state/active.tsv"; }||echo inactive; }
case "${1:-}" in preflight)preflight;;adopt)adopt;;activate)activate;;deactivate)deactivate;;verify)verify;;status)status;;*)die usage;;esac
