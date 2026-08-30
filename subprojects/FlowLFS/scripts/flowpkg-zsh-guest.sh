#!/usr/bin/env bash

set -euo pipefail

version=5.9.2
source_sha=36fa734374b44783582cec09bcd67822e2f992c779ec1624ab5596df078d2f81
source_date_epoch=1783881773
input_dir=/var/tmp/flowpkg-input/zsh-$version
archive=$input_dir/zsh-$version.tar.xz
build_root=/var/lib/flowbuild/zsh-$version
source_root=$build_root/source
stage_root=$build_root/stage
evidence_root=/flow/evidence/zsh-$version
store_root=/flow/store
object_pointer=$store_root/packages/zsh/$version/object
projection=$store_root/projections/zsh-$version

die() {
  printf 'FLOWPKG_ZSH_ERROR %s\n' "$*" >&2
  exit 1
}

require_root() {
  test "$(id -u)" -eq 0 || die "$1 requires root"
}

require_builder() {
  test "$(id -un)" = flowbuilder || die "$1 must run as flowbuilder"
}

prepare() {
  require_root prepare
  if ! getent group flowbuilder >/dev/null; then
    groupadd --system flowbuilder
  fi
  if ! getent passwd flowbuilder >/dev/null; then
    useradd --system --gid flowbuilder --home-dir /var/lib/flowbuild \
      --create-home --shell /bin/bash flowbuilder
  fi
  install -d -m 0755 /flow "$store_root" "$store_root/objects" \
    "$store_root/packages/zsh/$version" "$store_root/projections" /flow/evidence
  install -d -o flowbuilder -g flowbuilder -m 0750 \
    "$input_dir" "$build_root" "$evidence_root"
  printf 'FLOWPKG_ZSH_PREPARED user=flowbuilder\n'
}

build() {
  require_builder build
  test -f "$archive" || die "missing source archive $archive"
  printf '%s  %s\n' "$source_sha" "$archive" | sha256sum -c -

  if tar -tf "$archive" | awk '
      /^\// { bad=1 }
      { n=split($0,a,"/"); for (i=1;i<=n;i++) if (a[i]=="..") bad=1 }
      END { exit bad ? 1 : 0 }
    '; then
    :
  else
    die 'archive contains an unsafe path'
  fi

  rm -rf "$source_root" "$stage_root"
  install -d -m 0750 "$source_root" "$stage_root"
  tar -xf "$archive" --strip-components=1 -C "$source_root"
  cd "$source_root"
  export SOURCE_DATE_EPOCH="$source_date_epoch"

  sed -e 's|/etc/z|/etc/zsh/z|g' -i Doc/*.*

  {
    printf 'package=zsh\nversion=%s\nsource_sha256=%s\nsource_date_epoch=%s\n' \
      "$version" "$source_sha" "$source_date_epoch"
    printf 'builder=%s\n' "$(id)"
    printf 'kernel=%s\n' "$(uname -srvmo)"
    gcc --version | sed -n '1p'
    make --version | sed -n '1p'
  } > "$evidence_root/environment.txt"

  ./configure --prefix=/usr \
    --sysconfdir=/etc/zsh \
    --enable-etcdir=/etc/zsh \
    --enable-cap \
    --enable-gdbm \
    --enable-pcre 2>&1 | tee "$evidence_root/configure.log"

  make -j"$(nproc)" 2>&1 | tee "$evidence_root/build.log"
  makeinfo Doc/zsh.texi --html -o Doc/html
  makeinfo Doc/zsh.texi --plaintext -o zsh.txt
  makeinfo Doc/zsh.texi --html --no-split --no-headers -o zsh.html
  make check 2>&1 | tee "$evidence_root/test.log"

  make DESTDIR="$stage_root" install 2>&1 | tee "$evidence_root/install.log"
  make infodir="$stage_root/usr/share/info" install.info >> "$evidence_root/install.log" 2>&1
  make htmldir="$stage_root/usr/share/doc/zsh-$version/html" install.html >> "$evidence_root/install.log" 2>&1
  install -d -m 0755 "$stage_root/usr/share/doc/zsh-$version"
  install -m 0644 zsh.html zsh.txt Etc/FAQ "$stage_root/usr/share/doc/zsh-$version"

  # This is a generated shared registry, not package-owned output. A future
  # Info registry resolver will derive it from projected *.info contributions.
  rm -f "$stage_root/usr/share/info/dir"

  test -x "$stage_root/usr/bin/zsh-$version" || die 'staged zsh executable missing'
  printf 'FLOWPKG_ZSH_BUILD_PASS stage=%s\n' "$stage_root"
}

admit() {
  require_root admit
  test -d "$stage_root/usr" || die 'completed staging root is absent'
  test "$(stat -c %U "$stage_root")" = flowbuilder || die 'stage was not produced by flowbuilder'
  chmod 0755 "$stage_root"

  digest=$(tar --sort=name --mtime=@0 --owner=0 --group=0 --numeric-owner \
    -C "$stage_root" -cf - . | sha256sum | awk '{print $1}')
  object=$store_root/objects/sha256-$digest

  if test -e "$object"; then
    test -d "$object/root" -a -f "$object/manifest.tsv" -a -f "$object/derivation.txt" \
      || die "invalid existing object $object"
  else
    incoming=$store_root/objects/.incoming-zsh-$digest-$$
    test ! -e "$incoming" || die "stale incoming object $incoming"
    install -d -m 0755 "$incoming"
    mv "$stage_root" "$incoming/root"
    chown -R root:root "$incoming/root"
    (
      cd "$incoming/root"
      find . -mindepth 1 -printf '%y\t%m\t%u\t%g\t%p\t%l\n' | LC_ALL=C sort
    ) > "$incoming/manifest.tsv"
    cp "$evidence_root/environment.txt" "$incoming/derivation.txt"
    printf 'source_sha256=%s\noutput_tree_sha256=%s\n' "$source_sha" "$digest" >> "$incoming/derivation.txt"
    chmod -R a-w "$incoming"
    mv "$incoming" "$object"
  fi

  ln -sfn "$object" "$object_pointer"
  printf '%s\n' "$object" > "$evidence_root/admitted-object.txt"
  printf 'FLOWPKG_ZSH_ADMIT_PASS object=%s\n' "$object"
}

project() {
  require_root project
  test -L "$object_pointer" || die 'no admitted Zsh object'
  object=$(readlink -f "$object_pointer")
  root=$object/root
  test -x "$root/usr/bin/zsh-$version" || die 'admitted object is incomplete'
  test ! -e "$projection" || die "projection already exists: $projection"

  while IFS= read -r -d '' entry; do
    relative=${entry#"$root"}
    destination=$relative
    if test -e "$destination" || test -L "$destination"; then
      die "projection collision at $destination"
    fi
  done < <(find "$root" -mindepth 1 \! -type d -print0 | sort -z)

  install -d -m 0755 "$projection"
  : > "$projection/links.list"
  : > "$projection/dirs.list"

  while IFS= read -r -d '' directory; do
    relative=${directory#"$root"}
    destination=$relative
    if test ! -d "$destination"; then
      mkdir "$destination"
      printf '%s\n' "$destination" >> "$projection/dirs.list"
    fi
  done < <(find "$root" -mindepth 1 -type d -print0 | sort -z)

  while IFS= read -r -d '' entry; do
    relative=${entry#"$root"}
    destination=$relative
    ln -s "$entry" "$destination"
    printf '%s\t%s\n' "$destination" "$entry" >> "$projection/links.list"
  done < <(find "$root" -mindepth 1 \! -type d -print0 | sort -z)

  if test -e /etc/shells; then
    printf 'present\n' > "$projection/shells.state"
    cp -a /etc/shells "$projection/shells.before"
  else
    printf 'absent\n' > "$projection/shells.state"
  fi
  install -d -m 0755 /etc
  touch /etc/shells
  if ! grep -Fxq /bin/zsh /etc/shells; then
    printf '/bin/zsh\n' >> /etc/shells
  fi

  printf '%s\n' "$object" > "$projection/object"
  printf 'FLOWPKG_ZSH_PROJECT_PASS object=%s\n' "$object"
}

verify() {
  require_root verify
  test -d "$projection" || die 'projection is not active'
  test "$(getent passwd root | cut -d: -f7)" = /bin/bash || die 'root recovery shell changed'
  systemctl is-active --quiet sshd.service || die 'sshd recovery service is not active'
  su -s /bin/bash flowbuilder -c '/usr/bin/zsh --version >/dev/null' \
    || die 'projected Zsh is not executable by an unprivileged user'
  test "$(grep -Fxc /bin/zsh /etc/shells)" -eq 1 || die '/etc/shells contribution is not unique'
  /usr/bin/zsh -fc '[[ $ZSH_VERSION == 5.9.2 ]] && zmodload zsh/pcre && print -r -- "$ZSH_VERSION pcre-ok"'

  while IFS=$'\t' read -r destination target; do
    test -L "$destination" || die "projected path is not a symlink: $destination"
    test "$(readlink "$destination")" = "$target" || die "projection target changed: $destination"
  done < "$projection/links.list"
  printf 'FLOWPKG_ZSH_VERIFY_PASS\n'
}

rollback() {
  require_root rollback
  test -d "$projection" || die 'projection is not active'

  while IFS=$'\t' read -r destination target; do
    test -L "$destination" || die "refusing to remove changed path: $destination"
    test "$(readlink "$destination")" = "$target" || die "refusing to remove retargeted path: $destination"
    rm "$destination"
  done < "$projection/links.list"

  case "$(cat "$projection/shells.state")" in
    present) cp -a "$projection/shells.before" /etc/shells ;;
    absent) rm -f /etc/shells ;;
    *) die 'unknown /etc/shells rollback state' ;;
  esac

  tac "$projection/dirs.list" | while IFS= read -r directory; do
    rmdir "$directory" 2>/dev/null || :
  done
  mv "$projection" "$projection.rolled-back.$(date -u +%Y%m%dT%H%M%SZ)"
  printf 'FLOWPKG_ZSH_ROLLBACK_PASS\n'
}

query() {
  require_root query
  path=${2:-/usr/bin/zsh}
  test -L "$path" || die "$path is not a projected symlink"
  target=$(readlink "$path")
  case "$target" in
    /flow/store/objects/sha256-*/root/*) ;;
    *) die "$path does not resolve to a FlowLFS store object" ;;
  esac
  object=${target%%/root/*}
  printf 'path=%s\nprojection_target=%s\nobject=%s\n' "$path" "$target" "$object"
  sed -n '1,80p' "$object/derivation.txt"
}

case "${1:-}" in
  prepare) prepare ;;
  build) build ;;
  admit) admit ;;
  project) project ;;
  verify) verify ;;
  rollback) rollback ;;
  query) query "$@" ;;
  *) die 'usage: flowpkg-zsh-guest.sh prepare|build|admit|project|verify|rollback|query [path]' ;;
esac
