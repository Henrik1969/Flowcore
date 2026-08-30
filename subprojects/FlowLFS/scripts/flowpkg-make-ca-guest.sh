#!/usr/bin/env bash
set -euo pipefail

package=make-ca; version=1.16.1
source_sha=98c7e6fded1938b972d0177b5d5e925d318be87f84ee9e4cad680656a8c29414
input_dir=/var/tmp/flowpkg-input/$package-$version
archive=$input_dir/$package-$version.tar.gz
build_root=/var/lib/flowbuild/$package-$version
source_root=$build_root/source; stage_root=$build_root/stage
evidence_root=/flow/evidence/$package-$version; store_root=/flow/store
object_pointer=$store_root/packages/$package/$version/object
projection=$store_root/projections/$package-$version

die() { printf 'FLOWPKG_MAKE_CA_ERROR %s\n' "$*" >&2; exit 1; }
require_root() { test "$(id -u)" -eq 0 || die "$1 requires root"; }
require_builder() { test "$(id -un)" = flowbuilder || die "$1 must run as flowbuilder"; }

prepare() {
  require_root prepare
  test -x /usr/bin/trust || die 'projected p11-kit dependency absent'
  install -d -m 0755 "$store_root/packages/$package/$version" "$store_root/projections"
  install -d -o flowbuilder -g flowbuilder -m 0750 "$input_dir" "$build_root" "$evidence_root"
  printf 'FLOWPKG_MAKE_CA_PREPARED\n'
}

build() {
  require_builder build
  printf '%s  %s\n' "$source_sha" "$archive" | sha256sum -c -
  tar -tf "$archive" | awk '/^\// {bad=1} {n=split($0,a,"/"); for(i=1;i<=n;i++) if(a[i]=="..") bad=1} END{exit bad}' \
    || die 'archive contains unsafe path'
  rm -rf "$source_root" "$stage_root"; install -d -m 0750 "$source_root" "$stage_root"
  tar -xf "$archive" --strip-components=1 -C "$source_root"; cd "$source_root"
  sed '/mktemp/s/-t //' -i make-ca
  {
    printf 'package=%s\nversion=%s\nsource_sha256=%s\n' "$package" "$version" "$source_sha"
    printf 'dependency_p11_kit=%s\nbuilder=%s\nkernel=%s\n' \
      "$(readlink -f /usr/bin/trust)" "$(id)" "$(uname -srvmo)"
    printf 'source_mutation=remove deprecated mktemp -t option\n'
  } > "$evidence_root/environment.txt"
  bash -O extglob -n make-ca
  bash -n copy-trust-modifications help2man
  make 2>&1 | tee "$evidence_root/build.log"
  ./make-ca --help > "$evidence_root/test.log"
  make DESTDIR="$stage_root" install 2>&1 | tee "$evidence_root/install.log"
  test -x "$stage_root/usr/sbin/make-ca" || die 'staged processor missing'
  test -f "$stage_root/usr/lib/systemd/system/update-pki.timer" || die 'timer unit absent'
  printf 'FLOWPKG_MAKE_CA_BUILD_PASS stage=%s\n' "$stage_root"
}

admit() {
  require_root admit
  test -f "$evidence_root/test.log" -a -d "$stage_root/usr" || die 'completed build absent'
  test "$(stat -c %U "$stage_root")" = flowbuilder || die 'stage owner is not flowbuilder'
  chmod 0755 "$stage_root"
  digest=$(tar --sort=name --mtime=@0 --owner=0 --group=0 --numeric-owner -C "$stage_root" -cf - . | sha256sum | awk '{print $1}')
  object=$store_root/objects/sha256-$digest
  if test -e "$object"; then
    test -d "$object/root" -a -f "$object/manifest.tsv" -a -f "$object/derivation.txt" || die 'existing object invalid'
  else
    incoming=$store_root/objects/.incoming-$package-$digest-$$; install -d -m 0755 "$incoming"
    mv "$stage_root" "$incoming/root"; chown -R root:root "$incoming/root"
    (cd "$incoming/root" && find . -mindepth 1 -printf '%y\t%m\t%u\t%g\t%p\t%l\n' | LC_ALL=C sort) > "$incoming/manifest.tsv"
    cp "$evidence_root/environment.txt" "$incoming/derivation.txt"
    printf 'source_sha256=%s\noutput_tree_sha256=%s\n' "$source_sha" "$digest" >> "$incoming/derivation.txt"
    chmod -R a-w "$incoming"; mv "$incoming" "$object"
  fi
  ln -sfn "$object" "$object_pointer"; printf '%s\n' "$object" > "$evidence_root/admitted-object.txt"
  printf 'FLOWPKG_MAKE_CA_ADMIT_PASS object=%s\n' "$object"
}

project() {
  require_root project
  object=$(readlink -f "$object_pointer"); root=$object/root
  test -x "$root/usr/sbin/make-ca" || die 'admitted object absent'; test ! -e "$projection" || die 'projection active'
  while IFS= read -r -d '' entry; do destination=${entry#"$root"}; test ! -e "$destination" -a ! -L "$destination" || die "collision at $destination"; done < <(find "$root" -mindepth 1 \! -type d -print0 | sort -z)
  install -d -m 0755 "$projection"; : > "$projection/links.list"; : > "$projection/dirs.list"
  while IFS= read -r -d '' directory; do destination=${directory#"$root"}; if test ! -d "$destination"; then mkdir "$destination"; printf '%s\n' "$destination" >> "$projection/dirs.list"; fi; done < <(find "$root" -mindepth 1 -type d -print0 | sort -z)
  while IFS= read -r -d '' entry; do destination=${entry#"$root"}; ln -s "$entry" "$destination"; printf '%s\t%s\n' "$destination" "$entry" >> "$projection/links.list"; done < <(find "$root" -mindepth 1 \! -type d -print0 | sort -z)
  printf '%s\n' "$object" > "$projection/object"; systemctl daemon-reload
  printf 'FLOWPKG_MAKE_CA_PROJECT_PASS object=%s\n' "$object"
}

verify() {
  require_root verify; test -d "$projection" || die 'projection inactive'
  /usr/sbin/make-ca --help >/dev/null
  test "$(systemctl is-enabled update-pki.timer 2>/dev/null || true)" != enabled || die 'network refresh timer unexpectedly enabled'
  if test -e /etc/pki/anchors; then
    test "${FLOWPKG_COMPOSED:-0}" = 1 || die 'processor projection silently created trust policy'
    test -d /flow/store/projections/mozilla-certdata-2026-08-31 || die 'trust policy has no active policy owner'
  fi
  su -s /bin/bash flowbuilder -c '/usr/sbin/make-ca --help >/dev/null' || die 'ordinary user cannot inspect processor'
  systemctl is-active --quiet sshd.service || die 'sshd inactive'
  printf 'FLOWPKG_MAKE_CA_VERIFY_PASS\n'
}

rollback() {
  require_root rollback; test -d "$projection" || die 'projection inactive'
  while IFS=$'\t' read -r destination target; do test -L "$destination" && test "$(readlink "$destination")" = "$target" || die "changed path $destination"; rm "$destination"; done < "$projection/links.list"
  tac "$projection/dirs.list" | while IFS= read -r directory; do rmdir "$directory" 2>/dev/null || :; done
  systemctl daemon-reload; mv "$projection" "$projection.rolled-back.$(date -u +%Y%m%dT%H%M%SZ)"
  printf 'FLOWPKG_MAKE_CA_ROLLBACK_PASS\n'
}

case "${1:-}" in prepare) prepare;; build) build;; admit) admit;; project) project;; verify) verify;; rollback) rollback;; *) die 'usage: prepare|build|admit|project|verify|rollback';; esac
