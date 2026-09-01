#!/usr/bin/env bash
set -euo pipefail
profile=factory-proof-zoxide-v0; state=/flow/store/profile-realizations/$profile; prior=/flow/store/profile-realizations/fabric-floor-v0
rust=sha256-8e327fa0e8c5255a7f41f2212c526ea8f542d265f0fbb6d563caf2abd304609f
forge=sha256-bdf8f850b86560602a2bdf36d0c8bfd4682fe6453f3aba53b097dbbfea17621d
zoxide=sha256-ef684892b18d6698c8994909527c95255404ec7d2a0bef8a18be7ffb6dafdd72
integration=sha256-6c5ff9f9faf7418d3d4f8b2a48d797689a4078e038397f2b562b62bdc433f75a
spec=/flow/forge/specs/zoxide-0.10.0.tsv; spec_sha=1c4d44438d68fa679258bf8991206e235efaedbaf55fddf2255b7431a59c03fd
die(){ echo "FLOWPROFILE_FACTORY_PROOF_ERROR $*" >&2; exit 1; }; test "$(id -u)" -eq 0||die root
locked(){ test "$(readlink -f "/flow/store/packages/$1/$2/object")" = "/flow/store/objects/$3"||die "$1-lock"; }
preflight(){ test -f "$prior/active.tsv"||die prior-profile-inactive; locked rust-bootstrap 1.97.1 "$rust"; locked flowforge-cargo 0.1.0 "$forge"; locked zoxide 0.10.0 "$zoxide"; locked flow-zoxide-integration 0.1.0 "$integration"; echo "$spec_sha  $spec"|sha256sum -c - >/dev/null; echo FLOWPROFILE_FACTORY_PROOF_PREFLIGHT_PASS packages=4 authority=external; }
write_state(){ mkdir -p "$state"; { printf 'rust-bootstrap\t1.97.1\t%s\n' "$rust"; printf 'flowforge-cargo\t0.1.0\t%s\n' "$forge"; printf 'zoxide\t0.10.0\t%s\n' "$zoxide"; printf 'flow-zoxide-integration\t0.1.0\t%s\n' "$integration"; } >"$state/active.tsv.incoming"; mv "$state/active.tsv.incoming" "$state/active.tsv"; }
activate(){ preflight; test ! -f "$state/active.tsv"||die already-active; /usr/local/sbin/flowpkg-rust-bootstrap project; /usr/local/sbin/flowpkg-flowforge-cargo-package project; /usr/local/sbin/flowforge-cargo-package "$spec" project; /usr/local/sbin/flowpkg-flow-zoxide-integration project; write_state; verify; echo FLOWPROFILE_FACTORY_PROOF_ACTIVATE_PASS; }
adopt(){ preflight; test ! -f "$state/active.tsv"||die already-active; write_state; verify; echo FLOWPROFILE_FACTORY_PROOF_ADOPT_PASS; }
deactivate(){ /usr/local/sbin/flowpkg-flow-zoxide-integration rollback; /usr/local/sbin/flowforge-cargo-package "$spec" rollback; /usr/local/sbin/flowpkg-flowforge-cargo-package rollback; /usr/local/sbin/flowpkg-rust-bootstrap rollback; mv "$state/active.tsv" "$state/inactive.$(date +%s).tsv"; echo FLOWPROFILE_FACTORY_PROOF_DEACTIVATE_PASS; }
verify(){ preflight; test -f "$state/active.tsv"; /usr/local/sbin/flowpkg-rust-bootstrap verify; /usr/local/sbin/flowpkg-flowforge-cargo-package verify; /usr/local/sbin/flowforge-cargo-package "$spec" verify; /usr/local/sbin/flowpkg-flow-zoxide-integration verify; ! bash --noprofile --norc -c 'type z' >/dev/null 2>&1; ! zsh -f -c 'type z' >/dev/null 2>&1; bash --noprofile --norc -c 'eval "$(flow-zoxide-init bash)"; type z' >/dev/null; zsh -f -c 'eval "$(flow-zoxide-init zsh)"; type z' >/dev/null; systemctl is-active --quiet sshd.service; echo FLOWPROFILE_FACTORY_PROOF_VERIFY_PASS packages=4 reforge=identical opt_in=explicit; }
status(){ test -f "$state/active.tsv"&&{ echo active; cat "$state/active.tsv"; }||echo inactive; }
case "${1:-}" in preflight)preflight;;adopt)adopt;;activate)activate;;deactivate)deactivate;;verify)verify;;status)status;;*)die usage;;esac
