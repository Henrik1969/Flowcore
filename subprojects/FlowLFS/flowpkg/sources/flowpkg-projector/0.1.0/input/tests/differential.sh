#!/usr/bin/env bash
set -euo pipefail
tool=${1:?tool required}; work=$(mktemp -d); trap 'chmod -R u+w "$work" 2>/dev/null || :; rm -rf "$work"' EXIT
object=$work/object; native=$work/native; reference=$work/reference
mkdir -p "$object/root/usr/bin" "$object/root/usr/lib/demo/deep" "$native" "$reference"
for index in $(seq 1 250); do printf '%s\n' "$index" >"$object/root/usr/lib/demo/deep/item-$index"; done
ln -s ../lib/demo/deep/item-1 "$object/root/usr/bin/demo-link"
chmod -R a-w "$object"

reference_projection=$work/reference-projection; mkdir "$reference_projection"; : >"$reference_projection/links.list"; : >"$reference_projection/dirs.list"
while IFS= read -r -d '' directory; do destination=$reference/${directory#"$object/root/"}; if test ! -d "$destination"; then mkdir "$destination"; printf '%s\n' "$destination" >>"$reference_projection/dirs.list"; fi; done < <(find "$object/root" -mindepth 1 -type d -print0 | sort -z)
while IFS= read -r -d '' entry; do destination=$reference/${entry#"$object/root/"}; ln -s "$entry" "$destination"; printf '%s\t%s\n' "$destination" "$entry" >>"$reference_projection/links.list"; done < <(find "$object/root" -mindepth 1 ! -type d -print0 | sort -z)

"$tool" project "$object" "$native" "$work/native-projection" >/dev/null
(cd "$reference" && find . -type l -printf '%P\t%l\n' | sort) >"$work/reference.tsv"
(cd "$native" && find . -type l -printf '%P\t%l\n' | sort) >"$work/native.tsv"
cmp "$work/reference.tsv" "$work/native.tsv"

while IFS=$'\t' read -r destination target; do test -L "$destination" && test "$(readlink "$destination")" = "$target"; rm "$destination"; done <"$reference_projection/links.list"
tac "$reference_projection/dirs.list" | while IFS= read -r directory; do rmdir "$directory" 2>/dev/null || :; done
"$tool" rollback "$object" "$native" "$work/native-projection" "$work/native-archive" >/dev/null
test -z "$(find "$reference" -mindepth 1 -print -quit)"; test -z "$(find "$native" -mindepth 1 -print -quit)"
echo FLOWPKG_PROJECTOR_DIFFERENTIAL_PASS links=251

