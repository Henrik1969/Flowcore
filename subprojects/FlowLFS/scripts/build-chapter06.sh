#!/usr/bin/env bash

set -euo pipefail

if test "$(id -un)" != lfs; then
  printf '%s\n' 'Run as lfs in the clean Chapter 4 environment.' >&2
  exit 1
fi
# Installed verbatim from construction/lfs.bashrc.
# shellcheck disable=SC1091
source "$HOME/.bashrc"
test "$LFS" = /mnt/lfs
test "$LFS_TGT" = x86_64-lfs-linux-gnu
test "$(umask)" = 0022
test "$(findmnt -nro SOURCE "$LFS")" = /dev/vdb1
test -x "$LFS/tools/bin/$LFS_TGT-gcc"
test ! -e "$LFS/usr/lib64"

log_dir="$LFS/var/log/flowlfs/ch06"
mkdir -p "$log_dir"
exec > >(tee -a "$log_dir/chapter06.log") 2>&1

run_step()
{
  local name=$1 marker="$log_dir/$1.done"
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

m4_build()
{
  cd "$LFS/sources"; rm -rf m4-1.4.21; tar -xf m4-1.4.21.tar.xz; cd m4-1.4.21
  mkdir -p "$LFS/usr/share"
  printf '%s\n' \
    'ac_cv_func_posix_spawn_file_actions_addchdir=yes' \
    'ac_cv_func_posix_spawn_file_actions_addfchdir=yes' \
    > "$LFS/usr/share/config.site"
  ./configure --prefix=/usr --host="$LFS_TGT" --build="$(build-aux/config.guess)"
  make; make DESTDIR="$LFS" install
  test -x "$LFS/usr/bin/m4"
  cd "$LFS/sources"; rm -rf m4-1.4.21
}

ncurses_build()
{
  cd "$LFS/sources"; rm -rf ncurses-6.6; tar -xf ncurses-6.6.tar.gz; cd ncurses-6.6
  mkdir build; pushd build
  ../configure --prefix="$LFS/tools" AWK=gawk
  make -C include; make -C progs tic
  install progs/tic "$LFS/tools/bin"
  popd
  ./configure --prefix=/usr --host="$LFS_TGT" --build="$(./config.guess)" \
    --mandir=/usr/share/man --with-manpage-format=normal --with-shared \
    --without-normal --with-cxx-shared --without-debug --without-ada \
    --disable-stripping AWK=gawk
  make; make DESTDIR="$LFS" install
  ln -sv libncursesw.so "$LFS/usr/lib/libncurses.so"
  sed -e 's/^#if.*XOPEN.*$/#if 1/' -i "$LFS/usr/include/curses.h"
  test -x "$LFS/tools/bin/tic"; test -L "$LFS/usr/lib/libncurses.so"
  cd "$LFS/sources"; rm -rf ncurses-6.6
}

bash_build()
{
  cd "$LFS/sources"; rm -rf bash-5.3; tar -xf bash-5.3.tar.gz; cd bash-5.3
  ./configure --prefix=/usr --build="$(sh support/config.guess)" \
    --host="$LFS_TGT" --without-bash-malloc --docdir=/usr/share/doc/bash-5.3
  make; make DESTDIR="$LFS" install
  ln -sv bash "$LFS/bin/sh"
  test -x "$LFS/usr/bin/bash"; test "$(readlink "$LFS/bin/sh")" = bash
  cd "$LFS/sources"; rm -rf bash-5.3
}

coreutils_build()
{
  cd "$LFS/sources"; rm -rf coreutils-9.11; tar -xf coreutils-9.11.tar.xz; cd coreutils-9.11
  ./configure --prefix=/usr --host="$LFS_TGT" \
    --build="$(build-aux/config.guess)" --enable-install-program=hostname
  make; make DESTDIR="$LFS" install
  mv -v "$LFS/usr/bin/chroot" "$LFS/usr/sbin"
  mkdir -pv "$LFS/usr/share/man/man8"
  mv -v "$LFS/usr/share/man/man1/chroot.1" "$LFS/usr/share/man/man8/chroot.8"
  sed -i 's/"1"/"8"/' "$LFS/usr/share/man/man8/chroot.8"
  test -x "$LFS/usr/bin/ls"; test -x "$LFS/usr/sbin/chroot"
  cd "$LFS/sources"; rm -rf coreutils-9.11
}

simple_autoconf_build()
{
  local archive=$1 directory=$2 binary=$3 build_guess=$4 build
  shift 4
  cd "$LFS/sources"; rm -rf "$directory"; tar -xf "$archive"; cd "$directory"
  build=$("$build_guess")
  ./configure --prefix=/usr --host="$LFS_TGT" --build="$build" "$@"
  make; make DESTDIR="$LFS" install
  test -x "$LFS/usr/bin/$binary"
  cd "$LFS/sources"; rm -rf "$directory"
}

diffutils_build()
{
  cd "$LFS/sources"; rm -rf diffutils-3.12; tar -xf diffutils-3.12.tar.xz; cd diffutils-3.12
  ./configure --prefix=/usr --host="$LFS_TGT" \
    gl_cv_func_strcasecmp_works=yes --build="$(./build-aux/config.guess)"
  make; make DESTDIR="$LFS" install
  test -x "$LFS/usr/bin/diff"
  cd "$LFS/sources"; rm -rf diffutils-3.12
}

file_build()
{
  cd "$LFS/sources"; rm -rf file-5.48; tar -xf file-5.48.tar.gz; cd file-5.48
  mkdir build; pushd build
  ../configure --disable-bzlib --disable-libseccomp --disable-xzlib --disable-zlib
  make
  popd
  ./configure --prefix=/usr --host="$LFS_TGT" --build="$(./config.guess)"
  make FILE_COMPILE="$(pwd)/build/src/file"
  make DESTDIR="$LFS" install
  rm -v "$LFS/usr/lib/libmagic.la"
  test -x "$LFS/usr/bin/file"; test ! -e "$LFS/usr/lib/libmagic.la"
  cd "$LFS/sources"; rm -rf file-5.48
}

findutils_build()
{
  cd "$LFS/sources"; rm -rf findutils-4.11.0; tar -xf findutils-4.11.0.tar.xz; cd findutils-4.11.0
  ./configure --prefix=/usr --localstatedir=/var/lib/locate \
    --host="$LFS_TGT" --build="$(build-aux/config.guess)"
  make; make DESTDIR="$LFS" install
  test -x "$LFS/usr/bin/find"
  cd "$LFS/sources"; rm -rf findutils-4.11.0
}

gawk_build()
{
  cd "$LFS/sources"; rm -rf gawk-5.4.1; tar -xf gawk-5.4.1.tar.xz; cd gawk-5.4.1
  sed -i 's/extras//' Makefile.in
  ./configure --prefix=/usr --host="$LFS_TGT" --build="$(build-aux/config.guess)"
  make; make DESTDIR="$LFS" install
  test -x "$LFS/usr/bin/gawk"
  cd "$LFS/sources"; rm -rf gawk-5.4.1
}

gzip_build()
{
  cd "$LFS/sources"; rm -rf gzip-1.14; tar -xf gzip-1.14.tar.xz; cd gzip-1.14
  ./configure --prefix=/usr --host="$LFS_TGT"
  make; make DESTDIR="$LFS" install
  test -x "$LFS/usr/bin/gzip"
  cd "$LFS/sources"; rm -rf gzip-1.14
}

xz_build()
{
  cd "$LFS/sources"; rm -rf xz-5.8.3; tar -xf xz-5.8.3.tar.xz; cd xz-5.8.3
  ./configure --prefix=/usr --host="$LFS_TGT" --build="$(build-aux/config.guess)" \
    --disable-static --docdir=/usr/share/doc/xz-5.8.3
  make; make DESTDIR="$LFS" install
  rm -v "$LFS/usr/lib/liblzma.la"
  test -x "$LFS/usr/bin/xz"; test ! -e "$LFS/usr/lib/liblzma.la"
  cd "$LFS/sources"; rm -rf xz-5.8.3
}

binutils_pass2()
{
  cd "$LFS/sources"; rm -rf binutils-2.47; tar -xf binutils-2.47.tar.xz; cd binutils-2.47
  # $add_dir is intentionally literal in the book's sed expression.
  # shellcheck disable=SC2016
  sed '6031s/$add_dir//' -i ltmain.sh
  mkdir -v build; cd build
  ../configure --prefix=/usr --build="$(../config.guess)" --host="$LFS_TGT" \
    --disable-nls --enable-shared --enable-gprofng=no --disable-werror \
    --enable-64-bit-bfd --enable-new-dtags --enable-default-hash-style=gnu
  make; make DESTDIR="$LFS" install
  rm -v "$LFS"/usr/lib/lib{bfd,ctf,ctf-nobfd,opcodes,sframe}.{a,la}
  test -x "$LFS/usr/bin/ld"; test -x "$LFS/usr/bin/as"
  cd "$LFS/sources"; rm -rf binutils-2.47
}

gcc_pass2()
{
  cd "$LFS/sources"; rm -rf gcc-16.2.0; tar -xf gcc-16.2.0.tar.xz; cd gcc-16.2.0
  tar -xf ../mpfr-4.2.2.tar.xz; mv -v mpfr-4.2.2 mpfr
  tar -xf ../gmp-6.3.0.tar.xz; mv -v gmp-6.3.0 gmp
  tar -xf ../mpc-1.4.1.tar.xz; mv -v mpc-1.4.1 mpc
  case $(uname -m) in
    x86_64) sed -e '/m64=/s/lib64/lib/' -i.orig gcc/config/i386/t-linux64 ;;
  esac
  mkdir -v build; cd build
  ../configure --build="$(../config.guess)" --host="$LFS_TGT" \
    --target="$LFS_TGT" --prefix=/usr --with-build-sysroot="$LFS" \
    --enable-default-pie --enable-default-ssp --disable-fixincludes \
    --disable-nls --disable-multilib --disable-libatomic --disable-libgomp \
    --disable-libquadmath --disable-libsanitizer --disable-libssp \
    --disable-libvtv --enable-languages=c,c++ \
    CXX_FOR_TARGET="$LFS_TGT-gcc -nostdinc++" \
    LDFLAGS_FOR_TARGET="-L$PWD/$LFS_TGT/libgcc" \
    target_configargs=gcc_cv_target_thread_file=posix
  make; make DESTDIR="$LFS" install
  ln -sv gcc "$LFS/usr/bin/cc"
  test -x "$LFS/usr/bin/gcc"; test -x "$LFS/usr/bin/g++"
  test "$(readlink "$LFS/usr/bin/cc")" = gcc
  cd "$LFS/sources"; rm -rf gcc-16.2.0
}

run_step m4 m4_build
run_step ncurses ncurses_build
run_step bash bash_build
run_step coreutils coreutils_build
run_step diffutils diffutils_build
run_step file file_build
run_step findutils findutils_build
run_step gawk gawk_build
run_step grep simple_autoconf_build \
  grep-3.12.tar.xz grep-3.12 grep ./build-aux/config.guess
run_step gzip gzip_build
run_step make simple_autoconf_build \
  make-4.4.1.tar.gz make-4.4.1 make build-aux/config.guess
run_step patch simple_autoconf_build \
  patch-2.8.tar.xz patch-2.8 patch build-aux/config.guess
run_step sed simple_autoconf_build \
  sed-4.10.tar.xz sed-4.10 sed ./build-aux/config.guess
run_step tar simple_autoconf_build \
  tar-1.35.tar.xz tar-1.35 tar build-aux/config.guess
run_step xz xz_build
run_step binutils-pass2 binutils_pass2
run_step gcc-pass2 gcc_pass2

for binary in bash cat chroot diff file find gawk gcc grep gzip ld m4 make patch sed tar xz; do
  find "$LFS/usr/bin" "$LFS/usr/sbin" -maxdepth 1 -type f -name "$binary" -perm /111 |
    grep -q . || { printf 'Missing temporary executable: %s\n' "$binary" >&2; exit 1; }
done
test "$(readlink "$LFS/bin/sh")" = bash
test "$(readlink "$LFS/usr/bin/cc")" = gcc
test ! -e "$LFS/usr/lib64"
printf '\nCHAPTER 6 COMPLETE %s\n' "$(date --utc +'%Y-%m-%dT%H:%M:%SZ')"
