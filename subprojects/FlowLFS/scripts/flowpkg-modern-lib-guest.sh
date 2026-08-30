#!/usr/bin/env bash
set -euo pipefail

package=${2:-}
source_date_epoch=1788131457
case "$package" in
  libunistring)
    version=1.4.2; archive_name=libunistring-1.4.2.tar.xz
    source_sha=5b46e74377ed7409c5b75e7a96f95377b095623b689d8522620927964a41499c
    build_kind=autotools; library_pattern='libunistring.so.'; metadata_name=
    dependencies=()
    configure_args=(--prefix=/usr --disable-static --docdir=/usr/share/doc/libunistring-1.4.2)
    ;;
  libidn2)
    version=2.3.8; archive_name=libidn2-2.3.8.tar.gz
    source_sha=f557911bf6171621e1f72ff35f5b1825bb35b52ed45325dcdee931e5d3c0787a
    build_kind=autotools; library_pattern='libidn2.so.'; metadata_name=libidn2
    dependencies=(/usr/lib/libunistring.so); configure_args=(--prefix=/usr --disable-static)
    ;;
  nghttp2)
    version=1.70.0; archive_name=nghttp2-1.70.0.tar.xz
    source_sha=e05cb1388eaca3830aded4ccf20044b6e1ac1a61411dcca11b0437c4285c8bc2
    build_kind=autotools; library_pattern='libnghttp2.so.'; metadata_name=libnghttp2
    dependencies=(); configure_args=(--prefix=/usr --disable-static --enable-lib-only --docdir=/usr/share/doc/nghttp2-1.70.0)
    ;;
  libpsl)
    version=0.23.3; archive_name=libpsl-0.23.3.tar.gz
    source_sha=93941f85a1e7bd593fa94f299233cb5dfc91cd144fd9a78a6ceb75001c5b03be
    build_kind=meson; library_pattern='libpsl.so.'; metadata_name=libpsl
    dependencies=(/usr/lib/libunistring.so /usr/lib/libidn2.so); configure_args=()
    ;;
  curl)
    version=8.21.0; archive_name=curl-8.21.0.tar.xz
    source_sha=aa1b66a70eace83dc624508745646c08ae561de512ab403adffb93ac87fc72e6
    build_kind=autotools; library_pattern='libcurl.so.'; metadata_name=libcurl; test_target=test
    dependencies=(/usr/lib/libpsl.so /usr/lib/libnghttp2.so /etc/pki/tls/certs/ca-bundle.crt)
    configure_args=(--prefix=/usr --disable-static --with-openssl --with-ca-path=/etc/ssl/certs)
    ;;
  *) printf 'FLOWPKG_MODERN_LIB_ERROR unknown package %s\n' "$package" >&2; exit 1 ;;
esac
test_target=${test_target:-check}

input_dir=/var/tmp/flowpkg-input/$package-$version; archive=$input_dir/$archive_name
build_root=/var/lib/flowbuild/$package-$version; source_root=$build_root/source; stage_root=$build_root/stage
evidence_root=/flow/evidence/$package-$version; store_root=/flow/store
object_pointer=$store_root/packages/$package/$version/object; projection=$store_root/projections/$package-$version

die() { printf 'FLOWPKG_MODERN_LIB_ERROR package=%s %s\n' "$package" "$*" >&2; exit 1; }
require_root() { test "$(id -u)" -eq 0 || die "$1 requires root"; }
require_builder() { test "$(id -un)" = flowbuilder || die "$1 must run as flowbuilder"; }

prepare() {
  require_root prepare
  for dependency in "${dependencies[@]}"; do test -e "$dependency" || die "dependency absent: $dependency"; done
  install -d -m 0755 "$store_root/packages/$package/$version" "$store_root/projections"
  install -d -o flowbuilder -g flowbuilder -m 0750 "$input_dir" "$build_root" "$evidence_root"
  printf 'FLOWPKG_MODERN_LIB_PREPARED package=%s\n' "$package"
}

build() {
  require_builder build; cd /
  printf '%s  %s\n' "$source_sha" "$archive" | sha256sum -c -
  tar -tf "$archive" | awk '/^\//{bad=1}{n=split($0,a,"/");for(i=1;i<=n;i++)if(a[i]=="..")bad=1}END{exit bad}' || die 'unsafe archive path'
  if test -d "$stage_root"; then chmod -R u+w "$stage_root"; fi
  rm -rf "$source_root" "$stage_root"; install -d -m 0750 "$source_root" "$stage_root"
  tar -xf "$archive" --strip-components=1 -C "$source_root"; cd "$source_root"
  export SOURCE_DATE_EPOCH="$source_date_epoch"
  {
    printf 'package=%s\nversion=%s\nsource_sha256=%s\nsource_date_epoch=%s\n' "$package" "$version" "$source_sha" "$source_date_epoch"
    printf 'builder=%s\nkernel=%s\n' "$(id)" "$(uname -srvmo)"
    for dependency in "${dependencies[@]}"; do printf 'dependency=%s\n' "$(readlink -f "$dependency")"; done
  } > "$evidence_root/environment.txt"
  if test "$build_kind" = autotools; then
    ./configure "${configure_args[@]}" 2>&1 | tee "$evidence_root/configure.log"
    make -j"$(nproc)" 2>&1 | tee "$evidence_root/build.log"
    make "$test_target" 2>&1 | tee "$evidence_root/test.log"
    make DESTDIR="$stage_root" install 2>&1 | tee "$evidence_root/install.log"
  else
    meson setup build --prefix=/usr --buildtype=release 2>&1 | tee "$evidence_root/configure.log"
    ninja -C build 2>&1 | tee "$evidence_root/build.log"
    ninja -C build test 2>&1 | tee "$evidence_root/test.log"
    DESTDIR="$stage_root" ninja -C build install 2>&1 | tee "$evidence_root/install.log"
  fi
  rm -f "$stage_root/usr/share/info/dir"
  if test "$package" = curl; then
    rm -rf docs/examples/.deps
    find docs \( -name 'Makefile*' -o -name '*.1' -o -name '*.3' -o -name CMakeLists.txt \) -delete
    install -d -m 0755 "$stage_root/usr/share/doc/curl-$version"
    cp -R docs/. "$stage_root/usr/share/doc/curl-$version/"
  fi
  find "$stage_root/usr/lib" -maxdepth 1 -name "$library_pattern*" | grep -q . || die 'shared library absent'
  printf 'FLOWPKG_MODERN_LIB_BUILD_PASS package=%s stage=%s\n' "$package" "$stage_root"
}

admit() {
  require_root admit; test -f "$evidence_root/test.log" -a -d "$stage_root/usr" || die 'completed build absent'
  test "$(stat -c %U "$stage_root")" = flowbuilder || die 'stage owner mismatch'; chmod 0755 "$stage_root"
  digest=$(tar --sort=name --mtime=@0 --owner=0 --group=0 --numeric-owner -C "$stage_root" -cf - . | sha256sum | awk '{print $1}')
  object=$store_root/objects/sha256-$digest
  if test -e "$object"; then test -d "$object/root" -a -f "$object/manifest.tsv" -a -f "$object/derivation.txt" || die 'existing object invalid'; else
    incoming=$store_root/objects/.incoming-$package-$digest-$$; install -d -m 0755 "$incoming"; mv "$stage_root" "$incoming/root"; chown -R root:root "$incoming/root"
    (cd "$incoming/root" && find . -mindepth 1 -printf '%y\t%m\t%u\t%g\t%p\t%l\n' | LC_ALL=C sort) > "$incoming/manifest.tsv"
    cp "$evidence_root/environment.txt" "$incoming/derivation.txt"; printf 'source_sha256=%s\noutput_tree_sha256=%s\n' "$source_sha" "$digest" >> "$incoming/derivation.txt"
    chmod -R a-w "$incoming"; mv "$incoming" "$object"
  fi
  ln -sfn "$object" "$object_pointer"; printf '%s\n' "$object" > "$evidence_root/admitted-object.txt"
  printf 'FLOWPKG_MODERN_LIB_ADMIT_PASS package=%s object=%s\n' "$package" "$object"
}

project() {
  require_root project; object=$(readlink -f "$object_pointer"); root=$object/root
  test -d "$root/usr/lib" || die 'object absent'; test ! -e "$projection" || die 'projection active'
  while IFS= read -r -d '' entry; do destination=${entry#"$root"}; test ! -e "$destination" -a ! -L "$destination" || die "collision at $destination"; done < <(find "$root" -mindepth 1 \! -type d -print0 | sort -z)
  install -d -m 0755 "$projection"; : > "$projection/links.list"; : > "$projection/dirs.list"
  while IFS= read -r -d '' directory; do destination=${directory#"$root"}; if test ! -d "$destination"; then mkdir "$destination"; printf '%s\n' "$destination" >> "$projection/dirs.list"; fi; done < <(find "$root" -mindepth 1 -type d -print0 | sort -z)
  while IFS= read -r -d '' entry; do destination=${entry#"$root"}; ln -s "$entry" "$destination"; printf '%s\t%s\n' "$destination" "$entry" >> "$projection/links.list"; done < <(find "$root" -mindepth 1 \! -type d -print0 | sort -z)
  ldconfig; printf '%s\n' "$object" > "$projection/object"
  printf 'FLOWPKG_MODERN_LIB_PROJECT_PASS package=%s object=%s\n' "$package" "$object"
}

verify() {
  require_root verify; test -d "$projection" || die 'projection inactive'
  ldconfig -p | grep -Fq "$library_pattern" || die 'dynamic linker resolution absent'
  if test -n "$metadata_name"; then
    su -s /bin/bash flowbuilder -c "pkg-config --exists $metadata_name" || die 'unprivileged metadata unavailable'
  else
    su -s /bin/bash flowbuilder -c 'test -r /usr/include/unistr.h' || die 'unprivileged public header unavailable'
  fi
  if test "$package" = curl && test "${FLOWPKG_OFFLINE:-0}" != 1; then
    su -s /bin/bash flowbuilder -c '/usr/bin/curl --fail --silent --show-error --proto =https --tlsv1.2 https://www.example.com/ >/dev/null' \
      || die 'unprivileged HTTPS capability failed'
  elif test "$package" = curl; then
    printf 'FLOWPKG_MODERN_LIB_VERIFY_OFFLINE package=curl network_check=deferred\n'
  fi
  systemctl is-active --quiet sshd.service || die 'sshd inactive'
  printf 'FLOWPKG_MODERN_LIB_VERIFY_PASS package=%s\n' "$package"
}

rollback() {
  require_root rollback; test -d "$projection" || die 'projection inactive'
  while IFS=$'\t' read -r destination target; do test -L "$destination" && test "$(readlink "$destination")" = "$target" || die "changed path $destination"; rm "$destination"; done < "$projection/links.list"
  tac "$projection/dirs.list" | while IFS= read -r directory; do rmdir "$directory" 2>/dev/null || :; done
  ldconfig; mv "$projection" "$projection.rolled-back.$(date -u +%Y%m%dT%H%M%SZ)"
  printf 'FLOWPKG_MODERN_LIB_ROLLBACK_PASS package=%s\n' "$package"
}

case "${1:-}" in prepare) prepare;; build) build;; admit) admit;; project) project;; verify) verify;; rollback) rollback;; *) die 'usage: prepare|build|admit|project|verify|rollback PACKAGE';; esac
