#!/usr/bin/env bash
set -euo pipefail

package=flow-basis; version=0.1.0
source_sha=56daaf8df7783edbe91f524eb943c9d847a662ae608b45f8f7c97395adbd0b47
archive=/var/tmp/flowpkg-input/$package-$version/$package-$version.tar.xz
build_root=/var/lib/flowbuild/$package-$version; stage_root=$build_root/stage
evidence_root=/flow/evidence/$package-$version; store_root=/flow/store
object_pointer=$store_root/packages/$package/$version/object
projection=$store_root/projections/$package-$version

die(){ printf 'FLOWPKG_BASIS_ERROR %s\n' "$*" >&2; exit 1; }
require_root(){ test "$(id -u)" -eq 0 || die "$1 requires root"; }
require_builder(){ test "$(id -un)" = flowbuilder || die "$1 must run as flowbuilder"; }

prepare(){
  require_root prepare; getent passwd flowbuilder >/dev/null || die 'flowbuilder identity absent'
  install -d -m0755 "$store_root/objects" "$store_root/packages/$package/$version" "$store_root/projections"
  install -d -o flowbuilder -g flowbuilder -m0750 "${archive%/*}" "$build_root" "$evidence_root"
  printf 'FLOWPKG_BASIS_PREPARED\n'
}

build(){
  require_builder build; cd /
  printf '%s  %s\n' "$source_sha" "$archive" | sha256sum -c -
  tar -tf "$archive" | awk '/^\//{bad=1}{n=split($0,a,"/");for(i=1;i<=n;i++)if(a[i]=="..")bad=1}END{exit bad}' || die 'unsafe archive path'
  test ! -d "$stage_root" || chmod -R u+w "$stage_root"
  rm -rf "$stage_root"; install -d -m0750 "$stage_root"; tar -xf "$archive" -C "$stage_root"
  bash -n "$stage_root/etc/flow-basis/environment.sh" "$stage_root/etc/flow-basis/bashrc" \
    "$stage_root/etc/flow-basis/profile.sh" "$stage_root/etc/profile.d/flow-basis.sh" \
    "$stage_root/etc/skel/.bash_profile" "$stage_root/etc/skel/.bashrc"
  zsh -fn "$stage_root/etc/zsh/zshenv" "$stage_root/etc/zsh/zshrc" \
    "$stage_root/usr/share/flowcore/zsh/monikers.zsh"
  test -d "$stage_root/etc/skel/bin" -a -d "$stage_root/etc/skel/.local/bin" \
    -a -d "$stage_root/etc/skel/Projects" || die 'canonical home directory templates absent'
  { printf 'package=%s\nversion=%s\nsource_sha256=%s\nbuilder=%s\n' "$package" "$version" "$source_sha" "$(id)"; bash --version|sed -n 1p; zsh --version; } >"$evidence_root/environment.txt"
  printf 'FLOWPKG_BASIS_BUILD_PASS stage=%s\n' "$stage_root"
}

admit(){
  require_root admit; test -d "$stage_root/etc/skel" -a -f "$evidence_root/environment.txt" || die 'completed build absent'
  test "$(stat -c %U "$stage_root")" = flowbuilder || die 'stage owner mismatch'; chmod 0755 "$stage_root"
  digest=$(tar --sort=name --mtime=@0 --owner=0 --group=0 --numeric-owner -C "$stage_root" -cf - .|sha256sum|awk '{print $1}')
  object=$store_root/objects/sha256-$digest
  if test -e "$object"; then test -d "$object/root" -a -f "$object/manifest.tsv" -a -f "$object/derivation.txt" || die 'existing object invalid'; else
    incoming=$store_root/objects/.incoming-basis-$digest-$$; install -d -m0755 "$incoming"; mv "$stage_root" "$incoming/root"; chown -R root:root "$incoming/root"
    (cd "$incoming/root"&&find . -mindepth 1 -printf '%y\t%m\t%u\t%g\t%p\t%l\n'|LC_ALL=C sort)>"$incoming/manifest.tsv"
    cp "$evidence_root/environment.txt" "$incoming/derivation.txt"; printf 'output_tree_sha256=%s\n' "$digest">>"$incoming/derivation.txt"
    chmod -R a-w "$incoming"; mv "$incoming" "$object"
  fi
  ln -sfn "$object" "$object_pointer"; printf '%s\n' "$object">"$evidence_root/admitted-object.txt"
  printf 'FLOWPKG_BASIS_ADMIT_PASS object=%s\n' "$object"
}

project(){
  require_root project; object=$(readlink -f "$object_pointer"); root=$object/root
  test -d "$root/etc/skel" || die 'object absent'; test ! -e "$projection" || die 'projection active'
  while IFS= read -r -d '' entry; do destination=${entry#"$root"}; test ! -e "$destination" -a ! -L "$destination" || die "collision at $destination"; done < <(find "$root" -mindepth 1 \! -type d \! -path "$root/etc/skel/*" -print0|sort -z)
  while IFS= read -r -d '' entry; do destination=${entry#"$root"}; case "$destination" in /etc/skel/*) test ! -e "$destination" -a ! -L "$destination" || die "skeleton collision at $destination";; esac; done < <(find "$root/etc/skel" -mindepth 1 \! -type d -print0|sort -z)
  install -d -m0755 "$projection"; :>"$projection/links.list"; :>"$projection/copies.list"; :>"$projection/dirs.list"
  while IFS= read -r -d '' directory; do destination=${directory#"$root"}; if test ! -d "$destination"; then mkdir "$destination"; chmod --reference="$directory" "$destination"; printf '%s\n' "$destination">>"$projection/dirs.list"; fi; done < <(find "$root" -mindepth 1 -type d -print0|sort -z)
  while IFS= read -r -d '' entry; do destination=${entry#"$root"}; case "$destination" in
    /etc/skel/*) cp -a "$entry" "$destination"; digest=$(sha256sum "$destination"|awk '{print $1}'); printf '%s\t%s\n' "$destination" "$digest">>"$projection/copies.list";;
    *) ln -s "$entry" "$destination"; printf '%s\t%s\n' "$destination" "$entry">>"$projection/links.list";; esac
  done < <(find "$root" -mindepth 1 \! -type d -print0|sort -z)
  cp -a /etc/profile "$projection/profile.before"
  if ! grep -Fq '# Begin FlowLFS profile.d projection' /etc/profile; then cat >>/etc/profile <<'EOF'

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
  printf '%s\n' "$object">"$projection/object"; printf 'FLOWPKG_BASIS_PROJECT_PASS object=%s\n' "$object"
}

verify(){
  require_root verify; test -d "$projection" || die 'projection inactive'; probe=flowbasisprobe
  getent passwd "$probe" >/dev/null && die 'probe identity already exists'
  cleanup(){ userdel -r "$probe" >/dev/null 2>&1 || true; }; trap cleanup EXIT
  useradd -m -k /etc/skel -s /bin/bash "$probe"
  home=$(getent passwd "$probe"|cut -d: -f6); test "$(stat -c %U "$home")" = "$probe" || die 'canonical home ownership failed'
  for path in bin .local/bin .local/state/zsh .local/share/flowcore .cache/zsh Projects Downloads Documents .config/flowcore/monikers.tsv; do test -e "$home/$path" || die "new-user template absent: $path"; done
  su -s /bin/bash - "$probe" -c 'bash -lc '\''case ":$PATH:" in *:/usr/local/sbin:*) :;; *) exit 11;; esac; test "$EDITOR" = vim'\'''
  su -s /usr/bin/zsh - "$probe" -c 'zsh -lic '\''[[ $HISTFILE == $HOME/.local/state/zsh/history ]]; go2 projects; [[ $PWD == $HOME/Projects ]]; flow-moniker-show home >/dev/null'\'''
  test "$(getent passwd root|cut -d: -f7)" = /bin/bash || die 'root recovery shell changed'; systemctl is-active --quiet sshd.service || die 'sshd inactive'
  cleanup; trap - EXIT; printf 'FLOWPKG_BASIS_VERIFY_PASS canonical_useradd=pass additive_monikers=pass\n'
}

rollback(){
  require_root rollback; test -d "$projection" || die 'projection inactive'
  while IFS=$'\t' read -r destination target; do test -L "$destination" -a "$(readlink "$destination")" = "$target" || die "changed path $destination"; rm "$destination"; done <"$projection/links.list"
  while IFS=$'\t' read -r destination digest; do test -f "$destination" -a "$(sha256sum "$destination"|awk '{print $1}')" = "$digest" || die "changed skeleton $destination"; rm "$destination"; done <"$projection/copies.list"
  cp -a "$projection/profile.before" /etc/profile
  tac "$projection/dirs.list"|while IFS= read -r directory; do rmdir "$directory" 2>/dev/null || :; done
  mv "$projection" "$projection.rolled-back.$(date -u +%Y%m%dT%H%M%SZ)"; printf 'FLOWPKG_BASIS_ROLLBACK_PASS\n'
}

case "${1:-}" in prepare)prepare;;build)build;;admit)admit;;project)project;;verify)verify;;rollback)rollback;;*)die 'usage: prepare|build|admit|project|verify|rollback';;esac
