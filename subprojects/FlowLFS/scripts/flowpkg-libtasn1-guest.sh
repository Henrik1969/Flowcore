#!/usr/bin/env bash
set -euo pipefail

package=libtasn1
version=4.21.0
source_sha=1d8a444a223cc5464240777346e125de51d8e6abf0b8bac742ac84609167dc87
source_date_epoch=1767879696
input_dir=/var/tmp/flowpkg-input/$package-$version
archive=$input_dir/$package-$version.tar.gz
build_root=/var/lib/flowbuild/$package-$version
source_root=$build_root/source
stage_root=$build_root/stage
evidence_root=/flow/evidence/$package-$version
store_root=/flow/store
object_pointer=$store_root/packages/$package/$version/object
projection=$store_root/projections/$package-$version

die() { printf 'FLOWPKG_LIBTASN1_ERROR %s\n' "$*" >&2; exit 1; }
require_root() { test "$(id -u)" -eq 0 || die "$1 requires root"; }
require_builder() { test "$(id -un)" = flowbuilder || die "$1 must run as flowbuilder"; }

prepare() {
  require_root prepare
  getent group flowbuilder >/dev/null || groupadd --system flowbuilder
  getent passwd flowbuilder >/dev/null || useradd --system --gid flowbuilder \
    --home-dir /var/lib/flowbuild --create-home --shell /bin/bash flowbuilder
  install -d -m 0755 /flow "$store_root" "$store_root/objects" \
    "$store_root/packages/$package/$version" "$store_root/projections" /flow/evidence
  install -d -o flowbuilder -g flowbuilder -m 0750 \
    "$input_dir" "$build_root" "$evidence_root"
  printf 'FLOWPKG_LIBTASN1_PREPARED\n'
}

build() {
  require_builder build
  test -f "$archive" || die "missing source archive $archive"
  printf '%s  %s\n' "$source_sha" "$archive" | sha256sum -c -
  tar -tf "$archive" | awk '
    /^\// { bad=1 }
    { n=split($0,a,"/"); for (i=1;i<=n;i++) if (a[i]=="..") bad=1 }
    END { exit bad ? 1 : 0 }
  ' || die 'archive contains an unsafe path'
  rm -rf "$source_root" "$stage_root"
  install -d -m 0750 "$source_root" "$stage_root"
  tar -xf "$archive" --strip-components=1 -C "$source_root"
  cd "$source_root"
  export SOURCE_DATE_EPOCH="$source_date_epoch"
  {
    printf 'package=%s\nversion=%s\nsource_sha256=%s\nsource_date_epoch=%s\n' \
      "$package" "$version" "$source_sha" "$source_date_epoch"
    printf 'builder=%s\nkernel=%s\n' "$(id)" "$(uname -srvmo)"
    gcc --version | sed -n '1p'
    make --version | sed -n '1p'
  } > "$evidence_root/environment.txt"
  ./configure --prefix=/usr --disable-static 2>&1 | tee "$evidence_root/configure.log"
  make -j"$(nproc)" 2>&1 | tee "$evidence_root/build.log"
  make check 2>&1 | tee "$evidence_root/test.log"
  make DESTDIR="$stage_root" install 2>&1 | tee "$evidence_root/install.log"
  rm -f "$stage_root/usr/share/info/dir"
  test -f "$stage_root/usr/lib/libtasn1.so.6" || die 'staged shared library missing'
  printf 'FLOWPKG_LIBTASN1_BUILD_PASS stage=%s\n' "$stage_root"
}

admit() {
  require_root admit
  test -f "$evidence_root/test.log" || die 'test evidence absent'
  test -d "$stage_root/usr" || die 'completed staging root absent'
  test "$(stat -c %U "$stage_root")" = flowbuilder || die 'stage owner is not flowbuilder'
  chmod 0755 "$stage_root"
  digest=$(tar --sort=name --mtime=@0 --owner=0 --group=0 --numeric-owner \
    -C "$stage_root" -cf - . | sha256sum | awk '{print $1}')
  object=$store_root/objects/sha256-$digest
  if test -e "$object"; then
    test -d "$object/root" -a -f "$object/manifest.tsv" -a -f "$object/derivation.txt" \
      || die "invalid existing object $object"
  else
    incoming=$store_root/objects/.incoming-$package-$digest-$$
    test ! -e "$incoming" || die "stale incoming object $incoming"
    install -d -m 0755 "$incoming"
    mv "$stage_root" "$incoming/root"
    chown -R root:root "$incoming/root"
    (cd "$incoming/root" && find . -mindepth 1 \
      -printf '%y\t%m\t%u\t%g\t%p\t%l\n' | LC_ALL=C sort) > "$incoming/manifest.tsv"
    cp "$evidence_root/environment.txt" "$incoming/derivation.txt"
    printf 'source_sha256=%s\noutput_tree_sha256=%s\n' "$source_sha" "$digest" \
      >> "$incoming/derivation.txt"
    chmod -R a-w "$incoming"
    mv "$incoming" "$object"
  fi
  ln -sfn "$object" "$object_pointer"
  printf '%s\n' "$object" > "$evidence_root/admitted-object.txt"
  printf 'FLOWPKG_LIBTASN1_ADMIT_PASS object=%s\n' "$object"
}

project() {
  require_root project
  test -L "$object_pointer" || die 'no admitted object'
  object=$(readlink -f "$object_pointer")
  root=$object/root
  test -f "$root/usr/lib/libtasn1.so.6" || die 'admitted object incomplete'
  test ! -e "$projection" || die "projection already exists: $projection"
  while IFS= read -r -d '' entry; do
    destination=${entry#"$root"}
    test ! -e "$destination" -a ! -L "$destination" || die "collision at $destination"
  done < <(find "$root" -mindepth 1 \! -type d -print0 | sort -z)
  install -d -m 0755 "$projection"
  : > "$projection/links.list"
  : > "$projection/dirs.list"
  while IFS= read -r -d '' directory; do
    destination=${directory#"$root"}
    if test ! -d "$destination"; then
      mkdir "$destination"
      printf '%s\n' "$destination" >> "$projection/dirs.list"
    fi
  done < <(find "$root" -mindepth 1 -type d -print0 | sort -z)
  while IFS= read -r -d '' entry; do
    destination=${entry#"$root"}
    ln -s "$entry" "$destination"
    printf '%s\t%s\n' "$destination" "$entry" >> "$projection/links.list"
  done < <(find "$root" -mindepth 1 \! -type d -print0 | sort -z)
  ldconfig
  printf '%s\n' "$object" > "$projection/object"
  printf 'FLOWPKG_LIBTASN1_PROJECT_PASS object=%s\n' "$object"
}

verify() {
  require_root verify
  test -d "$projection" || die 'projection inactive'
  ldconfig -p | grep -Fq 'libtasn1.so.6' || die 'dynamic linker cannot resolve libtasn1'
  su -s /bin/bash flowbuilder -c '/usr/bin/asn1Parser --version >/dev/null' \
    || die 'ordinary user cannot execute projected tool'
  test "$(getent passwd root | cut -d: -f7)" = /bin/bash || die 'root shell changed'
  systemctl is-active --quiet sshd.service || die 'sshd inactive'
  while IFS=$'\t' read -r destination target; do
    test -L "$destination" && test "$(readlink "$destination")" = "$target" \
      || die "projection changed: $destination"
  done < "$projection/links.list"
  printf 'FLOWPKG_LIBTASN1_VERIFY_PASS\n'
}

rollback() {
  require_root rollback
  test -d "$projection" || die 'projection inactive'
  while IFS=$'\t' read -r destination target; do
    test -L "$destination" && test "$(readlink "$destination")" = "$target" \
      || die "refusing changed path: $destination"
    rm "$destination"
  done < "$projection/links.list"
  tac "$projection/dirs.list" | while IFS= read -r directory; do
    rmdir "$directory" 2>/dev/null || :
  done
  ldconfig
  mv "$projection" "$projection.rolled-back.$(date -u +%Y%m%dT%H%M%SZ)"
  printf 'FLOWPKG_LIBTASN1_ROLLBACK_PASS\n'
}

case "${1:-}" in
  prepare) prepare ;;
  build) build ;;
  admit) admit ;;
  project) project ;;
  verify) verify ;;
  rollback) rollback ;;
  *) die 'usage: flowpkg-libtasn1-guest.sh prepare|build|admit|project|verify|rollback' ;;
esac
