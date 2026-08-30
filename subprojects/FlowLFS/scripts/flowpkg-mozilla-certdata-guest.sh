#!/usr/bin/env bash
set -euo pipefail

package=mozilla-certdata; version=2026-08-31
source_sha=81b7f2576333a2e360e673f912d7b0b7a765d836c731003e348a46cac5d37198
source_date_epoch=1788078403
input_dir=/var/tmp/flowpkg-input/$package-$version; source_file=$input_dir/certdata.txt
build_root=/var/lib/flowbuild/$package-$version; stage_root=$build_root/stage
evidence_root=/flow/evidence/$package-$version; store_root=/flow/store
object_pointer=$store_root/packages/$package/$version/object
projection=$store_root/projections/$package-$version

die() { printf 'FLOWPKG_CERTDATA_ERROR %s\n' "$*" >&2; exit 1; }
require_root() { test "$(id -u)" -eq 0 || die "$1 requires root"; }
require_builder() { test "$(id -un)" = flowbuilder || die "$1 must run as flowbuilder"; }

prepare() {
  require_root prepare
  test -x /usr/bin/trust -a -x /usr/sbin/make-ca || die 'trust processor dependencies absent'
  install -d -m 0755 "$store_root/packages/$package/$version" "$store_root/projections"
  install -d -o flowbuilder -g flowbuilder -m 0750 "$input_dir" "$build_root" "$evidence_root"
  printf 'FLOWPKG_CERTDATA_PREPARED\n'
}

build() {
  require_builder build
  cd /
  printf '%s  %s\n' "$source_sha" "$source_file" | sha256sum -c -
  grep -Fq 'CKO_NSS_BUILTIN_ROOT_LIST' "$source_file" || die 'NSS root-list marker absent'
  grep -Fq 'BEGINDATA' "$source_file" || die 'certdata data marker absent'
  if test -d "$stage_root"; then chmod -R u+w "$stage_root"; fi
  rm -rf "$stage_root"; install -d -m 0750 "$stage_root"
  export SOURCE_DATE_EPOCH="$source_date_epoch"
  TZ=UTC touch -d "@$source_date_epoch" "$source_file"
  export XDG_CONFIG_HOME="$build_root/p11-config"
  rm -rf "$XDG_CONFIG_HOME"
  install -d -m 0750 "$XDG_CONFIG_HOME/pkcs11/modules"
  printf '%s\n' \
    'module: p11-kit-trust.so' \
    'trust-policy: yes' \
    "x-init-reserved: paths=$stage_root/etc/pki" \
    > "$XDG_CONFIG_HOME/pkcs11/modules/p11-kit-trust.module"
  {
    printf 'package=%s\nversion=%s\nsource_sha256=%s\nsource_date_epoch=%s\n' "$package" "$version" "$source_sha" "$source_date_epoch"
    printf 'processor_make_ca=%s\nprocessor_trust=%s\nbuilder=%s\n' \
      "$(readlink -f /usr/sbin/make-ca)" "$(readlink -f /usr/bin/trust)" "$(id)"
  } > "$evidence_root/environment.txt"
  /usr/sbin/make-ca --force --certdata "$source_file" --destdir "$stage_root" \
    2>&1 | tee "$evidence_root/generate.log"
  rm -rf "$stage_root/etc/ssl/local"
  install -d -m 0755 "$stage_root/usr/share/flow/trust-policy/$version"
  install -m 0644 "$source_file" "$stage_root/usr/share/flow/trust-policy/$version/certdata.txt"
  printf '%s  certdata.txt\n' "$source_sha" > "$stage_root/usr/share/flow/trust-policy/$version/SHA256SUM"
  test -s "$stage_root/etc/pki/tls/certs/ca-bundle.crt" || die 'generated TLS bundle absent'
  anchor_count=$(find "$stage_root/etc/pki/anchors" -type f | wc -l)
  test "$anchor_count" -gt 50 || die "implausible anchor count $anchor_count"
  listed_count=$(trust list --filter=ca-anchors | grep -c '^pkcs11:' || :)
  test "$listed_count" -gt 50 || die "staged provider exposed only $listed_count anchors"
  printf 'anchors=%s\nprovider_listed=%s\n' "$anchor_count" "$listed_count" > "$evidence_root/test.log"
  openssl crl2pkcs7 -nocrl -certfile "$stage_root/etc/pki/tls/certs/ca-bundle.crt" \
    | openssl pkcs7 -print_certs -noout >> "$evidence_root/test.log" 2>&1
  printf 'FLOWPKG_CERTDATA_BUILD_PASS anchors=%s stage=%s\n' "$anchor_count" "$stage_root"
}

admit() {
  require_root admit; test -f "$evidence_root/test.log" -a -d "$stage_root/etc/pki" || die 'completed generation absent'
  test "$(stat -c %U "$stage_root")" = flowbuilder || die 'stage owner is not flowbuilder'; chmod 0755 "$stage_root"
  digest=$(tar --sort=name --mtime=@0 --owner=0 --group=0 --numeric-owner -C "$stage_root" -cf - . | sha256sum | awk '{print $1}')
  object=$store_root/objects/sha256-$digest
  if test -e "$object"; then test -d "$object/root" -a -f "$object/manifest.tsv" -a -f "$object/derivation.txt" || die 'existing object invalid'; else
    incoming=$store_root/objects/.incoming-$package-$digest-$$; install -d -m 0755 "$incoming"; mv "$stage_root" "$incoming/root"; chown -R root:root "$incoming/root"
    (cd "$incoming/root" && find . -mindepth 1 -printf '%y\t%m\t%u\t%g\t%p\t%l\n' | LC_ALL=C sort) > "$incoming/manifest.tsv"
    cp "$evidence_root/environment.txt" "$incoming/derivation.txt"; cat "$evidence_root/test.log" >> "$incoming/derivation.txt"
    printf 'source_sha256=%s\noutput_tree_sha256=%s\n' "$source_sha" "$digest" >> "$incoming/derivation.txt"
    chmod -R a-w "$incoming"; mv "$incoming" "$object"
  fi
  ln -sfn "$object" "$object_pointer"; printf '%s\n' "$object" > "$evidence_root/admitted-object.txt"
  printf 'FLOWPKG_CERTDATA_ADMIT_PASS object=%s\n' "$object"
}

project() {
  require_root project; object=$(readlink -f "$object_pointer"); root=$object/root
  test -s "$root/etc/pki/tls/certs/ca-bundle.crt" || die 'admitted policy absent'; test ! -e "$projection" || die 'projection active'
  while IFS= read -r -d '' entry; do destination=${entry#"$root"}; test ! -e "$destination" -a ! -L "$destination" || die "collision at $destination"; done < <(find "$root" -mindepth 1 \! -type d -print0 | sort -z)
  install -d -m 0755 "$projection"; : > "$projection/links.list"; : > "$projection/dirs.list"
  while IFS= read -r -d '' directory; do destination=${directory#"$root"}; if test ! -d "$destination"; then mkdir "$destination"; printf '%s\n' "$destination" >> "$projection/dirs.list"; fi; done < <(find "$root" -mindepth 1 -type d -print0 | sort -z)
  while IFS= read -r -d '' entry; do destination=${entry#"$root"}; ln -s "$entry" "$destination"; printf '%s\t%s\n' "$destination" "$entry" >> "$projection/links.list"; done < <(find "$root" -mindepth 1 \! -type d -print0 | sort -z)
  printf '%s\n' "$object" > "$projection/object"; printf 'FLOWPKG_CERTDATA_PROJECT_PASS object=%s\n' "$object"
}

verify() {
  require_root verify; test -d "$projection" || die 'projection inactive'
  test -s /etc/pki/tls/certs/ca-bundle.crt || die 'active bundle absent'
  test ! -e /etc/ssl/local || die 'immutable policy claimed local owner state'
  test "$(systemctl is-enabled update-pki.timer 2>/dev/null || true)" != enabled || die 'refresh timer enabled'
  cert_count=$(openssl crl2pkcs7 -nocrl -certfile /etc/pki/tls/certs/ca-bundle.crt \
    | openssl pkcs7 -print_certs -noout | grep -c '^subject=' || :)
  test "$cert_count" -gt 50 || die "active TLS bundle exposes only $cert_count certificates"
  printf 'FLOWPKG_CERTDATA_VERIFY_PASS anchors=%s tls_certs=%s\n' \
    "$(find /etc/pki/anchors -type l | wc -l)" "$cert_count"
}

rollback() {
  require_root rollback; test -d "$projection" || die 'projection inactive'
  while IFS=$'\t' read -r destination target; do test -L "$destination" && test "$(readlink "$destination")" = "$target" || die "changed path $destination"; rm "$destination"; done < "$projection/links.list"
  tac "$projection/dirs.list" | while IFS= read -r directory; do rmdir "$directory" 2>/dev/null || :; done
  mv "$projection" "$projection.rolled-back.$(date -u +%Y%m%dT%H%M%SZ)"; printf 'FLOWPKG_CERTDATA_ROLLBACK_PASS\n'
}

case "${1:-}" in prepare) prepare;; build) build;; admit) admit;; project) project;; verify) verify;; rollback) rollback;; *) die 'usage: prepare|build|admit|project|verify|rollback';; esac
