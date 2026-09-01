#!/usr/bin/env bash
set -euo pipefail
spec=${1:?spec required}; action=${2:?action required}; store=/flow/store
die(){ printf 'FLOWFORGE_SOURCE_V0_ERROR %s\n' "$*" >&2; exit 1; }
field(){ local key=$1; awk -F '\t' -v k="$key" '$1==k {sub(/^[^\t]*\t/,""); print; exit}' "$spec"; }
test "$(field format)" = flowforge.source-package.v0||die format
package=$(field package); version=$(field version); archive=$(field source_archive)
source_sha=$(field source_sha256); class=$(field build_class); verify_pc=$(field verify_pc)
verify_path=$(field verify_path); verify_output=$(field verify_output)
case "$package/$version/$archive/$class/$verify_pc$verify_path" in *[!A-Za-z0-9._+/-]*) die identity;; esac
case "$class" in meson|autotools|font-data) :;; *) die build-class;; esac
input=/var/tmp/flowpkg-input/$package-$version; build=/var/lib/flowbuild/$package-$version
stage=$build/stage; evidence=/flow/evidence/$package-$version
pointer=$store/packages/$package/$version/object; projection=$store/projections/$package-$version
root(){ test "$(id -u)" -eq 0||die root; }
builder(){ test "$(id -un)" = flowbuilder||die builder; }
prepare(){
  root
  command -v meson >/dev/null; command -v ninja >/dev/null
  test -x /usr/local/sbin/flowpkg-projector||die projector
  install -d -m0755 "$store/packages/$package/$version"
  install -d -o flowbuilder -g flowbuilder -m0750 "$input" "$build" "$evidence"
}
build_package(){
  builder
  printf '%s  %s\n' "$source_sha" "$input/$archive"|sha256sum -c -
  rm -rf "$build/source" "$stage" "$build/home" "$build/tmp"
  mkdir "$build/source" "$stage" "$build/home" "$build/tmp"
  tar -xf "$input/$archive" -C "$build/source" --strip-components=1
  export PATH=/usr/bin:/bin HOME="$build/home" TMPDIR="$build/tmp" SOURCE_DATE_EPOCH=0
  export PKG_CONFIG_PATH=/usr/lib/pkgconfig:/usr/share/pkgconfig
  read -r -a setup_words <<<"$(field setup_args)"
  if test "$class" = meson; then
    meson setup "$build/source/build-flow" "$build/source" --prefix=/usr --buildtype=release --wrap-mode=nodownload "${setup_words[@]}" 2>&1|tee "$evidence/setup.log"
    ninja -C "$build/source/build-flow" 2>&1|tee "$evidence/build.log"
    if test "$(field tests)" = enabled; then meson test -C "$build/source/build-flow" --print-errorlogs 2>&1|tee "$evidence/test.log"; fi
    DESTDIR="$stage" ninja -C "$build/source/build-flow" install 2>&1|tee "$evidence/install.log"
  elif test "$class" = autotools; then
    cd "$build/source"
    ./configure --prefix=/usr "${setup_words[@]}" 2>&1|tee "$evidence/setup.log"
    make -j"$(nproc)" 2>&1|tee "$evidence/build.log"
    if test "$(field tests)" = enabled; then make check 2>&1|tee "$evidence/test.log"; fi
    make DESTDIR="$stage" install 2>&1|tee "$evidence/install.log"
  else
    install -d -m0755 "$stage/usr/share/fonts/truetype/$package"
    find "$build/source" -type f \( -name '*.ttf' -o -name '*.otf' \) -exec install -m0644 -t "$stage/usr/share/fonts/truetype/$package" {} +
    find "$stage/usr/share/fonts/truetype/$package" -type f -print -quit | grep -q . || die font-data
    printf 'canonical font data; no executable build\n' | tee "$evidence/build.log" "$evidence/install.log"
  fi
  install -D -m0644 "$spec" "$stage/usr/share/flowforge/specs/$package-$version.tsv"
  if test -n "$verify_pc"; then
    PKG_CONFIG_SYSROOT_DIR="$stage" PKG_CONFIG_PATH="$stage/usr/lib/pkgconfig:$stage/usr/share/pkgconfig" pkg-config --modversion "$verify_pc"|tee "$evidence/version.log"
    test "$(cat "$evidence/version.log")" = "$(field verify_version)"||die version
  elif test -n "$verify_path"; then
    test -x "$stage$verify_path"||die verify-path
    if test -n "$verify_output"; then "$stage$verify_path" --version 2>&1|head -1|tee "$evidence/version.log"; test "$(cat "$evidence/version.log")" = "$verify_output"||die version; fi
  else
    find "$stage/usr/share/fonts" -type f \( -name '*.ttf' -o -name '*.otf' \) -print -quit | grep -q . || die verify-font-data
  fi
  printf 'FLOWFORGE_SOURCE_V0_BUILD_PASS package=%s version=%s class=%s offline=yes\n' "$package" "$version" "$class"
}
admit(){
  root; chmod -R a+rX "$stage"
  digest=$(tar --sort=name --mtime=@0 --owner=0 --group=0 --numeric-owner -C "$stage" -cf - .|sha256sum|awk '{print $1}')
  object=$store/objects/sha256-$digest
  if test ! -e "$object"; then
    incoming=$store/objects/.incoming-$package-$digest-$$; mkdir "$incoming"; mv "$stage" "$incoming/root"; chown -R root:root "$incoming/root"
    (cd "$incoming/root"&&find . -mindepth 1 -printf '%y\t%m\t%u\t%g\t%p\t%l\n'|LC_ALL=C sort)>"$incoming/manifest.tsv"
    printf 'forge=flowforge.source-package.v0\nsource_sha256=%s\nbuild_class=%s\noffline=true\noutput_tree_sha256=%s\n' "$source_sha" "$class" "$digest">"$incoming/derivation.txt"
    chmod -R a-w "$incoming"; mv "$incoming" "$object"
  fi
  ln -sfn "$object" "$pointer"; printf 'FLOWFORGE_SOURCE_V0_ADMIT_PASS object=%s\n' "$object"
}
project(){ root; object=$(readlink -f "$pointer"); /usr/local/sbin/flowpkg-projector project "$object" / "$projection"; }
verify(){ root; object=$(readlink -f "$pointer"); test "$(cat "$projection/object")" = "$object"; if test -n "$verify_pc"; then test "$(pkg-config --modversion "$verify_pc")" = "$(field verify_version)"; elif test -n "$verify_path"; then test -x "$verify_path"; if test -n "$verify_output"; then test "$("$verify_path" --version 2>&1|head -1)" = "$verify_output"; fi; else find -L "/usr/share/fonts/truetype/$package" -type f \( -name '*.ttf' -o -name '*.otf' \) -print -quit|grep -q .; fi; printf 'FLOWFORGE_SOURCE_V0_VERIFY_PASS package=%s version=%s\n' "$package" "$version"; }
rollback(){ root; object=$(readlink -f "$pointer"); /usr/local/sbin/flowpkg-projector rollback "$object" / "$projection" "$projection.rolled-back.$(date +%s)"; }
case "$action" in prepare)prepare;; build)build_package;; admit)admit;; project)project;; verify)verify;; rollback)rollback;; *)die action;; esac
