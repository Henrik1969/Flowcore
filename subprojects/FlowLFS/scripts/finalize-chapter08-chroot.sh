#!/usr/bin/env bash
set -euo pipefail

log_dir=/var/log/flowlfs/ch08
marker="$log_dir/finalization.done"
mkdir -p "$log_dir"
exec > >(tee -a "$log_dir/finalization.log") 2>&1

if [[ -e "$marker" ]]; then
    echo "Chapter 8 finalization already complete"
    exit 0
fi

echo "===== START chapter08-finalization $(date -u +%FT%TZ) ====="

# LFS 13.0-systemd section 8.84: retain debug companions for libraries that
# must remain live while the rest of the final system is stripped.
save_usrlib="$(cd /usr/lib; ls ld-linux*[^g])
 libc.so.6
 libthread_db.so.1
 libquadmath.so.0.0.0
 libstdc++.so.6.0.36
 libitm.so.1.0.0
 libatomic.so.1.2.0"

cd /usr/lib
for LIB in $save_usrlib; do
    objcopy --only-keep-debug --compress-debug-sections=zstd "$LIB" "$LIB.dbg"
    cp "$LIB" "/tmp/$LIB"
    strip --strip-unneeded "/tmp/$LIB"
    objcopy --add-gnu-debuglink="$LIB.dbg" "/tmp/$LIB"
    install -vm755 "/tmp/$LIB" /usr/lib
    rm "/tmp/$LIB"
done

online_usrbin=""
online_usrlib=""

# The book's online replacement set is loaded by the running shell, strip, or
# install on this build. Preserve it in-process; the generic pass strips every
# other eligible binary. This avoids demand-paging a library after its target
# has been truncated.
live_tool_usrbin="bash find strip tee"
live_tool_usrlib="libbfd-2.47.20260726.so
 libsframe.so.3.0.0
 libhistory.so.8.3
 libncursesw.so.6.6
 libm.so.6
 libreadline.so.8.3
 libz.so.1.3.2
 libzstd.so.1.5.7
 $(cd /usr/lib; find libnss*.so* -type f)"

for BIN in $online_usrbin; do
    cp "/usr/bin/$BIN" "/tmp/$BIN"
    strip --strip-unneeded "/tmp/$BIN"
    install -vm755 "/tmp/$BIN" /usr/bin
    rm "/tmp/$BIN"
done

for LIB in $online_usrlib; do
    cp "/usr/lib/$LIB" "/tmp/$LIB"
    strip --strip-unneeded "/tmp/$LIB"
    install -vm755 "/tmp/$LIB" /usr/lib
    rm "/tmp/$LIB"
done

while IFS= read -r -d '' item; do
    item_name=$(basename "$item")
    preserve=false
    for preserved_name in $online_usrbin $online_usrlib $save_usrlib \
                          $live_tool_usrbin $live_tool_usrlib; do
        if [[ "$item_name" == "$preserved_name" ]]; then
            preserve=true
            break
        fi
    done
    if [[ "$preserve" == false ]] && readelf -h "$item" >/dev/null 2>&1; then
        strip --strip-unneeded "$item"
    fi
done < <(find /usr/lib -type f \( -name '*.so*' ! -name '*dbg' -o -name '*.a' \) -print0; \
         find /usr/bin /usr/sbin /usr/libexec -type f -print0)

unset BIN LIB save_usrlib online_usrbin online_usrlib live_tool_usrbin live_tool_usrlib

# LFS section 8.85 cleanup.  Restrict every deletion to the constructed root.
find /tmp -mindepth 1 -maxdepth 1 -exec rm -rf -- {} +
find /usr/lib /usr/libexec -name '*.la' -delete
find /usr -depth -name "$(uname -m)-lfs-linux-gnu*" -exec rm -rf -- {} +
userdel -r tester 2>/dev/null || true

date -u +%FT%TZ > "$marker"
echo "===== PASS chapter08-finalization $(date -u +%FT%TZ) ====="
