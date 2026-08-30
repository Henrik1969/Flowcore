#!/usr/bin/env bash

set -euo pipefail

package=flow-shell-defaults
version=0.1.0
source_sha=76383d4759c348337afa1d87f1393231b6b5b6f2177d9c0ab2a7b7b48ce2c497
archive=/var/tmp/flowpkg-input/$package-$version/$package-$version.tar.xz
build_root=/var/lib/flowbuild/$package-$version
stage_root=$build_root/stage
evidence_root=/flow/evidence/$package-$version
store_root=/flow/store
object_pointer=$store_root/packages/$package/$version/object
projection=$store_root/projections/$package-$version

die() { printf 'FLOWPKG_SHELL_ERROR %s\n' "$*" >&2; exit 1; }
require_root() { test "$(id -u)" -eq 0 || die "$1 requires root"; }
require_builder() { test "$(id -un)" = flowbuilder || die "$1 must run as flowbuilder"; }

prepare() {
  require_root prepare
  getent passwd flowbuilder >/dev/null || die 'flowbuilder identity is required'
  install -d -m 0755 "$store_root/objects" "$store_root/packages/$package/$version" \
    "$store_root/projections" /flow/evidence
  install -d -o flowbuilder -g flowbuilder -m 0750 \
    "${archive%/*}" "$build_root" "$evidence_root"
  printf 'FLOWPKG_SHELL_PREPARED\n'
}

build() {
  require_builder build
  printf '%s  %s\n' "$source_sha" "$archive" | sha256sum -c -
  tar -tf "$archive" | awk '
    /^\// { bad=1 }
    { n=split($0,a,"/"); for (i=1;i<=n;i++) if (a[i]=="..") bad=1 }
    END { exit bad ? 1 : 0 }
  ' || die 'archive contains an unsafe path'

  rm -rf "$stage_root"
  install -d -m 0750 "$stage_root"
  tar -xf "$archive" -C "$stage_root"

  bash -n "$stage_root/etc/flow-shell/environment.sh" \
    "$stage_root/etc/flow-shell/bashrc" \
    "$stage_root/etc/flow-shell/profile.sh" \
    "$stage_root/etc/profile.d/flow-shell.sh" \
    "$stage_root/etc/skel/.bashrc" \
    "$stage_root/etc/skel/.bash_profile"
  zsh -fn "$stage_root/etc/zsh/zshenv"
  zsh -fn "$stage_root/etc/zsh/zshrc"

  {
    printf 'package=%s\nversion=%s\nsource_sha256=%s\n' "$package" "$version" "$source_sha"
    printf 'builder=%s\n' "$(id)"
    bash --version | sed -n '1p'
    zsh --version
  } > "$evidence_root/environment.txt"
  printf 'FLOWPKG_SHELL_BUILD_PASS stage=%s\n' "$stage_root"
}

admit() {
  require_root admit
  test -d "$stage_root/etc" || die 'completed staging root is absent'
  test "$(stat -c %U "$stage_root")" = flowbuilder || die 'stage was not produced by flowbuilder'
  test -f "$evidence_root/environment.txt" || die 'completed build evidence is absent'
  chmod 0755 "$stage_root"
  digest=$(tar --sort=name --mtime=@0 --owner=0 --group=0 --numeric-owner \
    -C "$stage_root" -cf - . | sha256sum | awk '{print $1}')
  object=$store_root/objects/sha256-$digest
  if test -e "$object"; then
    test -d "$object/root" -a -f "$object/manifest.tsv" -a -f "$object/derivation.txt" \
      || die "invalid existing object $object"
  else
    incoming=$store_root/objects/.incoming-shell-$digest-$$
    test ! -e "$incoming" || die "stale incoming object $incoming"
    install -d -m 0755 "$incoming"
    mv "$stage_root" "$incoming/root"
    chown -R root:root "$incoming/root"
    (cd "$incoming/root" && find . -mindepth 1 -printf '%y\t%m\t%u\t%g\t%p\t%l\n' | LC_ALL=C sort) \
      > "$incoming/manifest.tsv"
    cp "$evidence_root/environment.txt" "$incoming/derivation.txt"
    printf 'output_tree_sha256=%s\n' "$digest" >> "$incoming/derivation.txt"
    chmod -R a-w "$incoming"
    mv "$incoming" "$object"
  fi
  ln -sfn "$object" "$object_pointer"
  printf '%s\n' "$object" > "$evidence_root/admitted-object.txt"
  printf 'FLOWPKG_SHELL_ADMIT_PASS object=%s\n' "$object"
}

project() {
  require_root project
  test -L "$object_pointer" || die 'no admitted shell-defaults object'
  object=$(readlink -f "$object_pointer")
  root=$object/root
  test ! -e "$projection" || die "projection already exists: $projection"

  while IFS= read -r -d '' entry; do
    destination=${entry#"$root"}
    if test -e "$destination" || test -L "$destination"; then
      die "projection collision at $destination"
    fi
  done < <(find "$root" -mindepth 1 \! -type d -print0 | sort -z)

  install -d -m 0755 "$projection"
  : > "$projection/links.list"
  : > "$projection/copies.list"
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
    case "$destination" in
      /etc/skel/*)
        cp -a "$entry" "$destination"
        chmod 0644 "$destination"
        digest=$(sha256sum "$destination" | awk '{print $1}')
        printf '%s\t%s\n' "$destination" "$digest" >> "$projection/copies.list"
        ;;
      *)
        ln -s "$entry" "$destination"
        printf '%s\t%s\n' "$destination" "$entry" >> "$projection/links.list"
        ;;
    esac
  done < <(find "$root" -mindepth 1 \! -type d -print0 | sort -z)

  cp -a /etc/profile "$projection/profile.before"
  if ! grep -Fq '# Begin FlowLFS profile.d projection' /etc/profile; then
    cat >> /etc/profile <<'EOF'

# Begin FlowLFS profile.d projection
if [ -d /etc/profile.d ]; then
  for _flow_profile in /etc/profile.d/*.sh; do
    [ -r "$_flow_profile" ] && . "$_flow_profile"
  done
  unset _flow_profile
fi
# End FlowLFS profile.d projection
EOF
  fi
  printf '%s\n' "$object" > "$projection/object"
  printf 'FLOWPKG_SHELL_PROJECT_PASS object=%s\n' "$object"
}

verify() {
  require_root verify
  test -d "$projection" || die 'projection is not active'
  bash --noprofile --norc -ic \
    'source /etc/flow-shell/environment.sh; source /etc/flow-shell/bashrc; test "$EDITOR" = vim; alias ll >/dev/null'
  zsh -dfc 'source /etc/zsh/zshenv; source /etc/zsh/zshrc; [[ $EDITOR == vim ]]'
  test "$(getent passwd root | cut -d: -f7)" = /bin/bash || die 'root recovery shell changed'
  systemctl is-active --quiet sshd.service || die 'sshd is inactive'
  printf 'FLOWPKG_SHELL_VERIFY_PASS\n'
}

rollback() {
  require_root rollback
  test -d "$projection" || die 'projection is not active'
  while IFS=$'\t' read -r destination target; do
    test -L "$destination" && test "$(readlink "$destination")" = "$target" \
      || die "refusing to remove changed path: $destination"
    rm "$destination"
  done < "$projection/links.list"
  if test -f "$projection/copies.list"; then
    while IFS=$'\t' read -r destination digest; do
      test -f "$destination" && test "$(sha256sum "$destination" | awk '{print $1}')" = "$digest" \
        || die "refusing to remove changed materialized path: $destination"
      rm "$destination"
    done < "$projection/copies.list"
  fi
  cp -a "$projection/profile.before" /etc/profile
  tac "$projection/dirs.list" | while IFS= read -r directory; do rmdir "$directory" 2>/dev/null || :; done
  mv "$projection" "$projection.rolled-back.$(date -u +%Y%m%dT%H%M%SZ)"
  printf 'FLOWPKG_SHELL_ROLLBACK_PASS\n'
}

case "${1:-}" in
  prepare) prepare ;;
  build) build ;;
  admit) admit ;;
  project) project ;;
  verify) verify ;;
  rollback) rollback ;;
  *) die 'usage: prepare|build|admit|project|verify|rollback' ;;
esac
