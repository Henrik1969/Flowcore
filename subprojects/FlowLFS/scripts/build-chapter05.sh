#!/usr/bin/env bash

set -euo pipefail

if test "$(id -un)" != lfs; then
  printf '%s\n' 'Run this script as the lfs user in the clean Chapter 4 environment.' >&2
  exit 1
fi

# The file is installed verbatim from construction/lfs.bashrc.
# shellcheck disable=SC1091
source "$HOME/.bashrc"
test "$LFS" = /mnt/lfs
test "$LFS_TGT" = x86_64-lfs-linux-gnu
test "$(umask)" = 0022
test "$(findmnt -nro SOURCE "$LFS")" = /dev/vdb1
test -d "$LFS/sources"
test ! -e "$LFS/usr/lib64"

log_dir="$LFS/var/log/flowlfs/ch05"
mkdir -p "$log_dir"
master_log="$log_dir/chapter05.log"
exec > >(tee -a "$master_log") 2>&1

run_step()
{
  local name=$1
  local marker="$log_dir/$name.done"
  shift
  if test -e "$marker"; then
    printf 'SKIP completed step: %s\n' "$name"
    return
  fi
  printf '\n===== START %s %s =====\n' "$name" "$(date --utc +'%Y-%m-%dT%H:%M:%SZ')"
  "$@"
  printf 'completed=%s\n' "$(date --utc +'%Y-%m-%dT%H:%M:%SZ')" > "$marker"
  printf '===== PASS %s =====\n' "$name"
}

binutils_pass1()
{
  cd "$LFS/sources"
  rm -rf binutils-2.47
  tar -xf binutils-2.47.tar.xz
  cd binutils-2.47
  mkdir -v build
  cd build
  ../configure --prefix="$LFS/tools" \
               --with-sysroot="$LFS" \
               --target="$LFS_TGT" \
               --disable-nls \
               --enable-gprofng=no \
               --disable-werror \
               --enable-new-dtags \
               --enable-default-hash-style=gnu
  make
  make install
  "$LFS_TGT-ld" --version
  "$LFS_TGT-as" --version
  test -x "$LFS/tools/bin/$LFS_TGT-ld"
  cd "$LFS/sources"
  rm -rf binutils-2.47
}

gcc_pass1()
{
  cd "$LFS/sources"
  rm -rf gcc-16.2.0
  tar -xf gcc-16.2.0.tar.xz
  cd gcc-16.2.0
  tar -xf ../mpfr-4.2.2.tar.xz
  mv -v mpfr-4.2.2 mpfr
  tar -xf ../gmp-6.3.0.tar.xz
  mv -v gmp-6.3.0 gmp
  tar -xf ../mpc-1.4.1.tar.xz
  mv -v mpc-1.4.1 mpc
  case $(uname -m) in
    x86_64)
      sed -e '/m64=/s/lib64/lib/' -i.orig gcc/config/i386/t-linux64
      ;;
  esac
  mkdir -v build
  cd build
  ../configure \
    --target="$LFS_TGT" \
    --prefix="$LFS/tools" \
    --with-glibc-version=2.44 \
    --with-sysroot="$LFS" \
    --with-newlib \
    --without-headers \
    --enable-default-pie \
    --enable-default-ssp \
    --disable-fixincludes \
    --disable-nls \
    --disable-shared \
    --disable-multilib \
    --disable-threads \
    --disable-libatomic \
    --disable-libgomp \
    --disable-libquadmath \
    --disable-libssp \
    --disable-libvtv \
    --disable-libstdcxx \
    --enable-languages=c,c++
  make
  make install
  cat ../gcc/{limitx,glimits,limity}.h > \
    "$("$LFS_TGT-gcc" -print-file-name=include)/limits.h"
  "$LFS_TGT-gcc" -v
  test "$("$LFS_TGT-gcc" -print-sysroot)" = "$LFS"
  test -s "$("$LFS_TGT-gcc" -print-file-name=include)/limits.h"
  cd "$LFS/sources"
  rm -rf gcc-16.2.0
}

linux_headers()
{
  cd "$LFS/sources"
  rm -rf linux-7.1.8
  tar -xf linux-7.1.8.tar.xz
  cd linux-7.1.8
  make mrproper
  make headers
  find usr/include -type f ! -name '*.h' -delete
  cp -rv usr/include "$LFS/usr"
  test -s "$LFS/usr/include/linux/version.h"
  test -s "$LFS/usr/include/asm/unistd.h"
  cd "$LFS/sources"
  rm -rf linux-7.1.8
}

glibc_build()
{
  cd "$LFS/sources"
  rm -rf glibc-2.44
  tar -xf glibc-2.44.tar.xz
  cd glibc-2.44
  case $(uname -m) in
    i?86)
      ln -sfv ld-linux.so.2 "$LFS/lib/ld-lsb.so.3"
      ;;
    x86_64)
      ln -sfv ../lib/ld-linux-x86-64.so.2 "$LFS/lib64"
      ln -sfv ../lib/ld-linux-x86-64.so.2 \
        "$LFS/lib64/ld-lsb-x86-64.so.3"
      ;;
  esac
  patch -Np1 -i ../glibc-fhs-1.patch
  patch -Np1 -i ../glibc-2.44-upstream_fixes-1.patch
  mkdir -v build
  cd build
  printf '%s\n' 'rootsbindir=/usr/sbin' > configparms
  ../configure \
    --prefix=/usr \
    --host="$LFS_TGT" \
    --build="$(../scripts/config.guess)" \
    --disable-nscd \
    libc_cv_slibdir=/usr/lib \
    --enable-kernel=5.10
  make
  make DESTDIR="$LFS" install
  sed '/RTLDLIST=/s@/usr@@g' -i "$LFS/usr/bin/ldd"

  printf '%s\n' 'int main(){}' | "$LFS_TGT-gcc" -x c - -v \
    -Wl,--verbose &> dummy.log
  "$LFS_TGT-readelf" -l a.out | grep ': /lib'
  if "$LFS_TGT-readelf" -l a.out | grep -q "/mnt/lfs"; then
    printf '%s\n' 'The target interpreter incorrectly contains /mnt/lfs.' >&2
    return 1
  fi
  grep -E -o "$LFS/lib.*/S?crt[1in].*succeeded" dummy.log
  grep -B3 "^ $LFS/usr/include" dummy.log
  grep 'SEARCH.*/usr/lib' dummy.log | sed 's|; |\n|g'
  grep "/lib.*/libc.so.6 " dummy.log
  grep found dummy.log
  "$LFS_TGT-readelf" -l a.out | grep -q \
    'Requesting program interpreter: /lib64/ld-linux-x86-64.so.2'
  grep -q "attempt to open $LFS/usr/lib/libc.so.6 succeeded" dummy.log
  grep -q "found ld-linux-x86-64.so.2 at $LFS/usr/lib/ld-linux-x86-64.so.2" \
    dummy.log
  rm -v a.out dummy.log
  cd "$LFS/sources"
  rm -rf glibc-2.44
}

libstdcpp_build()
{
  cd "$LFS/sources"
  rm -rf gcc-16.2.0
  tar -xf gcc-16.2.0.tar.xz
  cd gcc-16.2.0
  mkdir -v build
  cd build
  ../libstdc++-v3/configure \
    --host="$LFS_TGT" \
    --build="$(../config.guess)" \
    CXX="$LFS_TGT-gcc" \
    --prefix=/usr \
    --disable-multilib \
    --disable-nls \
    --disable-libstdcxx-pch \
    --with-gxx-include-dir="/tools/$LFS_TGT/include/c++/16.2.0"
  make
  make DESTDIR="$LFS" install
  rm -v "$LFS"/usr/lib/lib{stdc++{,exp,fs},supc++}.la
  test -s "$LFS/usr/lib/libstdc++.so.6"
  test -d "$LFS/tools/$LFS_TGT/include/c++/16.2.0"
  if find "$LFS/usr/lib" -maxdepth 1 \
      \( -name 'libstdc*.la' -o -name 'libsupc++.la' \) | grep -q .; then
    printf '%s\n' 'Harmful Libstdc++ libtool archives remain installed.' >&2
    return 1
  fi
  cd "$LFS/sources"
  rm -rf gcc-16.2.0
}

run_step binutils-pass1 binutils_pass1
run_step gcc-pass1 gcc_pass1
run_step linux-headers linux_headers
run_step glibc glibc_build
run_step libstdcpp libstdcpp_build

printf '\nCHAPTER 5 COMPLETE %s\n' "$(date --utc +'%Y-%m-%dT%H:%M:%SZ')"
