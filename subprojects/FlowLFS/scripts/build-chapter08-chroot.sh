#!/usr/bin/env bash

set -euo pipefail
test "$(id -u)" -eq 0
test -r /proc/self/mounts

log_dir=/var/log/flowlfs/ch08
mkdir -p "$log_dir"
exec > >(tee -a "$log_dir/chapter08.log") 2>&1

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

unpack_source()
{
  local archive=$1 top
  cd /sources
  top=$(tar -tf "$archive" | sed -n '1{s#/.*##;p}')
  test -n "$top"
  rm -rf "$top"
  tar -xf "$archive"
  cd "$top"
  FLOWLFS_SOURCE_TOP=$top
}

finish_source()
{
  cd /sources
  rm -rf "$FLOWLFS_SOURCE_TOP"
}


package_man_pages()
{
  unpack_source man-pages-6.18.tar.xz
rm -v man3/crypt*
make -R GIT=false prefix=/usr install
  finish_source
}
run_step man-pages package_man_pages

package_iana_etc()
{
  unpack_source iana-etc-20260805.tar.gz
cp -v services protocols /etc
  finish_source
}
run_step iana-etc package_iana_etc

package_glibc()
{
  unpack_source glibc-2.44.tar.xz
patch -Np1 -i ../glibc-fhs-1.patch
patch -Np1 -i ../glibc-2.44-upstream_fixes-1.patch
mkdir -v build
cd       build
../configure --prefix=/usr                   \
             --disable-werror                \
             --disable-nscd                  \
             libc_cv_slibdir=/usr/lib        \
             --enable-stack-protector=strong \
             --enable-kernel=5.10
make
  set +e
make check
  test_status=$?
  set -e
  printf 'TEST-STATUS glibc block-6=%s\n' "$test_status"
grep "Timed out" $(find -name \*.out) || true
touch /etc/ld.so.conf
sed '/test-installation/s@$(PERL)@echo not running@' -i ../Makefile
make DESTDIR=$PWD/dest install
install -vm755 dest/usr/lib/*.so.* /usr/lib
DIR=$(dirname $(gcc -print-libgcc-file-name))
[ -e $DIR/include/limits.h ]    || mv $DIR/include{-fixed,}/limits.h
[ -e $DIR/include/syslimits.h ] || mv $DIR/include{-fixed,}/syslimits.h
rm -rfv $DIR/include-fixed/*
unset DIR
make install
sed '/RTLDLIST=/s@/usr@@g' -i /usr/bin/ldd
localedef -i C -f UTF-8 C.UTF-8
localedef -i cs_CZ -f UTF-8 cs_CZ.UTF-8
localedef -i de_DE -f ISO-8859-1 de_DE
localedef -i de_DE@euro -f ISO-8859-15 de_DE@euro
localedef -i de_DE -f UTF-8 de_DE.UTF-8
localedef -i el_GR -f ISO-8859-7 el_GR
localedef -i en_GB -f ISO-8859-1 en_GB
localedef -i en_GB -f UTF-8 en_GB.UTF-8
localedef -i en_HK -f ISO-8859-1 en_HK
localedef -i en_PH -f ISO-8859-1 en_PH
localedef -i en_US -f ISO-8859-1 en_US
localedef -i en_US -f UTF-8 en_US.UTF-8
localedef -i es_ES -f ISO-8859-15 es_ES@euro
localedef -i es_MX -f ISO-8859-1 es_MX
localedef -i fa_IR -f UTF-8 fa_IR
localedef -i fr_FR -f ISO-8859-1 fr_FR
localedef -i fr_FR@euro -f ISO-8859-15 fr_FR@euro
localedef -i fr_FR -f UTF-8 fr_FR.UTF-8
localedef -i is_IS -f ISO-8859-1 is_IS
localedef -i is_IS -f UTF-8 is_IS.UTF-8
localedef -i it_IT -f ISO-8859-1 it_IT
localedef -i it_IT -f ISO-8859-15 it_IT@euro
localedef -i it_IT -f UTF-8 it_IT.UTF-8
localedef -i ja_JP -f EUC-JP ja_JP
localedef -i ja_JP -f UTF-8 ja_JP.UTF-8
localedef -i nl_NL@euro -f ISO-8859-15 nl_NL@euro
localedef -i ru_RU -f KOI8-R ru_RU.KOI8-R
localedef -i ru_RU -f UTF-8 ru_RU.UTF-8
localedef -i se_NO -f UTF-8 se_NO.UTF-8
localedef -i ta_IN -f UTF-8 ta_IN.UTF-8
localedef -i tr_TR -f UTF-8 tr_TR.UTF-8
localedef -i zh_CN -f GB18030 zh_CN.GB18030
localedef -i zh_HK -f BIG5-HKSCS zh_HK.BIG5-HKSCS
localedef -i zh_TW -f UTF-8 zh_TW.UTF-8
make localedata/install-locales
  finish_source
}
run_step glibc package_glibc

package_zlib()
{
  unpack_source zlib-1.3.2.tar.gz
./configure --prefix=/usr
make
  set +e
make check
  test_status=$?
  set -e
  printf 'TEST-STATUS zlib block-3=%s\n' "$test_status"
make install
rm -fv /usr/lib/libz.a
  finish_source
}
run_step zlib package_zlib

package_bzip2()
{
  unpack_source bzip2-1.0.8.tar.gz
patch -Np1 -i ../bzip2-1.0.8-install_docs-1.patch
sed -i 's@\(ln -s -f \)$(PREFIX)/bin/@\1@' Makefile
sed -i "s@(PREFIX)/man@(PREFIX)/share/man@g" Makefile
make -f Makefile-libbz2_so
make clean
make
make PREFIX=/usr install
cp -av libbz2.so.* /usr/lib
ln -sfv libbz2.so.1.0.8 /usr/lib/libbz2.so
ln -sfv libbz2.so.1.0.8 /usr/lib/libbz2.so.1
cp -v bzip2-shared /usr/bin/bzip2
for i in /usr/bin/{bzcat,bunzip2}; do
  ln -sfv bzip2 $i
done
rm -fv /usr/lib/libbz2.a
  finish_source
}
run_step bzip2 package_bzip2

package_xz()
{
  unpack_source xz-5.8.3.tar.xz
./configure --prefix=/usr    \
            --disable-static \
            --docdir=/usr/share/doc/xz-5.8.3
make
  set +e
make check
  test_status=$?
  set -e
  printf 'TEST-STATUS xz block-3=%s\n' "$test_status"
make install
  finish_source
}
run_step xz package_xz

package_lz4()
{
  unpack_source lz4-1.10.0.tar.gz
make BUILD_STATIC=no PREFIX=/usr
  set +e
make -j1 check
  test_status=$?
  set -e
  printf 'TEST-STATUS lz4 block-2=%s\n' "$test_status"
make BUILD_STATIC=no PREFIX=/usr install
  finish_source
}
run_step lz4 package_lz4

package_zstd()
{
  unpack_source zstd-1.5.7.tar.gz
make prefix=/usr
  set +e
make check
  test_status=$?
  set -e
  printf 'TEST-STATUS zstd block-2=%s\n' "$test_status"
make prefix=/usr install
rm -v /usr/lib/libzstd.a
  finish_source
}
run_step zstd package_zstd

package_file()
{
  unpack_source file-5.48.tar.gz
./configure --prefix=/usr
make
  set +e
make check
  test_status=$?
  set -e
  printf 'TEST-STATUS file block-3=%s\n' "$test_status"
make install
  finish_source
}
run_step file package_file

package_readline()
{
  unpack_source readline-8.3.tar.gz
sed -i '/MV.*old/d' Makefile.in
sed -i '/{OLDSUFF}/c:' support/shlib-install
sed -i 's/-Wl,-rpath,[^ ]*//' support/shobj-conf
sed -e '270a\
     else\
       chars_avail = 1;'      \
    -e '288i\   result = -1;' \
    -i.orig input.c
./configure --prefix=/usr    \
            --disable-static \
            --with-curses    \
            --docdir=/usr/share/doc/readline-8.3
make SHLIB_LIBS="-lncursesw"
make install
install -v -m644 doc/*.{ps,pdf,html,dvi} /usr/share/doc/readline-8.3
  finish_source
}
run_step readline package_readline

package_pcre2()
{
  unpack_source pcre2-10.47.tar.bz2
./configure --prefix=/usr                       \
            --docdir=/usr/share/doc/pcre2-10.47 \
            --enable-unicode                    \
            --enable-jit                        \
            --enable-pcre2-16                   \
            --enable-pcre2-32                   \
            --enable-pcre2grep-libz             \
            --enable-pcre2grep-libbz2           \
            --enable-pcre2test-libreadline      \
            --disable-static
make
  set +e
make check
  test_status=$?
  set -e
  printf 'TEST-STATUS pcre2 block-3=%s\n' "$test_status"
make install
  finish_source
}
run_step pcre2 package_pcre2

package_m4()
{
  unpack_source m4-1.4.21.tar.xz
./configure --prefix=/usr
make
  set +e
make check
  test_status=$?
  set -e
  printf 'TEST-STATUS m4 block-3=%s\n' "$test_status"
make install
  finish_source
}
run_step m4 package_m4

package_bc()
{
  unpack_source bc-7.0.3.tar.xz
CC='gcc -std=c99' ./configure --prefix=/usr -G -O3 -r
make
  set +e
make test
  test_status=$?
  set -e
  printf 'TEST-STATUS bc block-3=%s\n' "$test_status"
make install
  finish_source
}
run_step bc package_bc

package_flex()
{
  unpack_source flex-2.6.4.tar.gz
./configure --prefix=/usr    \
            --disable-static \
            --docdir=/usr/share/doc/flex-2.6.4
make
  set +e
make check
  test_status=$?
  set -e
  printf 'TEST-STATUS flex block-3=%s\n' "$test_status"
make install
ln -sv flex   /usr/bin/lex
ln -sv flex.1 /usr/share/man/man1/lex.1
  finish_source
}
run_step flex package_flex

package_tcl()
{
  unpack_source tcl8.6.18-src.tar.gz
SRCDIR=$(pwd)
cd unix
./configure --prefix=/usr           \
            --mandir=/usr/share/man \
            --disable-rpath
make

sed -e "s|$SRCDIR/unix|/usr/lib|" \
    -e "s|$SRCDIR|/usr/include|"  \
    -i tclConfig.sh

sed -e "s|$SRCDIR/unix/pkgs/tdbc1.1.13|/usr/lib/tdbc1.1.13|" \
    -e "s|$SRCDIR/pkgs/tdbc1.1.13/generic|/usr/include|"     \
    -e "s|$SRCDIR/pkgs/tdbc1.1.13/library|/usr/lib/tcl8.6|"  \
    -e "s|$SRCDIR/pkgs/tdbc1.1.13|/usr/include|"             \
    -i pkgs/tdbc1.1.13/tdbcConfig.sh

sed -e "s|$SRCDIR/unix/pkgs/itcl4.3.7|/usr/lib/itcl4.3.7|" \
    -e "s|$SRCDIR/pkgs/itcl4.3.7/generic|/usr/include|"    \
    -e "s|$SRCDIR/pkgs/itcl4.3.7|/usr/include|"            \
    -i pkgs/itcl4.3.7/itclConfig.sh

unset SRCDIR
  set +e
LC_ALL=C.UTF-8 make test
  test_status=$?
  set -e
  printf 'TEST-STATUS tcl block-3=%s\n' "$test_status"
make install 
chmod 644 /usr/lib/libtclstub8.6.a
chmod -v u+w /usr/lib/libtcl8.6.so
make install-private-headers
ln -sfv tclsh8.6 /usr/bin/tclsh
mv -v /usr/share/man/man3/{Thread,Tcl_Thread}.3
cd ..
tar -xf ../tcl8.6.18-html.tar.gz --strip-components=1
mkdir -v -p /usr/share/doc/tcl-8.6.18
cp -v -r  ./html/* /usr/share/doc/tcl-8.6.18
  finish_source
}
run_step tcl package_tcl

package_expect()
{
  unpack_source expect5.45.4.tar.gz
python3 -c 'from pty import spawn; spawn(["echo", "ok"])'
patch -Np1 -i ../expect-5.45.4-gcc15-1.patch
./configure --prefix=/usr           \
            --with-tcl=/usr/lib     \
            --enable-shared         \
            --disable-rpath         \
            --mandir=/usr/share/man \
            --with-tclinclude=/usr/include
make
  set +e
make test
  test_status=$?
  set -e
  printf 'TEST-STATUS expect block-5=%s\n' "$test_status"
make install
ln -svf expect5.45.4/libexpect5.45.4.so /usr/lib
  finish_source
}
run_step expect package_expect

package_dejagnu()
{
  unpack_source dejagnu-1.6.3.tar.gz
mkdir -v build
cd       build
../configure --prefix=/usr
makeinfo --html --no-split -o doc/dejagnu.html ../doc/dejagnu.texi
makeinfo --plaintext       -o doc/dejagnu.txt  ../doc/dejagnu.texi
  set +e
make check
  test_status=$?
  set -e
  printf 'TEST-STATUS dejagnu block-3=%s\n' "$test_status"
make install
install -v -dm755  /usr/share/doc/dejagnu-1.6.3
install -v -m644   doc/dejagnu.{html,txt} /usr/share/doc/dejagnu-1.6.3
  finish_source
}
run_step dejagnu package_dejagnu

package_ninja()
{
  unpack_source ninja-1.13.2.tar.gz
sed -i '/int Guess/a \
  int   j = 0;\
  char* jobs = getenv( "NINJAJOBS" );\
  if ( jobs != NULL ) j = atoi( jobs );\
  if ( j > 0 ) return j;\
' src/ninja.cc
python3 configure.py --bootstrap --verbose
install -vm755 ninja /usr/bin/
install -vDm644 misc/bash-completion /usr/share/bash-completion/completions/ninja
install -vDm644 misc/zsh-completion  /usr/share/zsh/site-functions/_ninja
  finish_source
}
run_step ninja package_ninja

package_pkgconf()
{
  unpack_source pkgconf-3.0.5.tar.xz
tar -xf ../meson-1.12.0.tar.gz
mkdir build
cd    build

python3 ../meson-1.12.0/meson.py setup --prefix=/usr --buildtype=release ..
ninja
  set +e
ninja test
  test_status=$?
  set -e
  printf 'TEST-STATUS pkgconf block-4=%s\n' "$test_status"
ninja install
mv /usr/share/doc/pkgconf{,-3.0.5}
ln -sv pkgconf   /usr/bin/pkg-config
ln -sv pkgconf.1 /usr/share/man/man1/pkg-config.1
  finish_source
}
run_step pkgconf package_pkgconf

package_binutils()
{
  unpack_source binutils-2.47.tar.xz
mkdir -v build
cd       build
../configure --prefix=/usr       \
             --sysconfdir=/etc   \
             --enable-ld=default \
             --enable-plugins    \
             --enable-shared     \
             --disable-werror    \
             --enable-64-bit-bfd \
             --enable-new-dtags  \
             --with-system-zlib  \
             --with-lib-path=/usr/lib \
             --enable-default-hash-style=gnu
make tooldir=/usr
  set +e
make -k check
  test_status=$?
  set -e
  printf 'TEST-STATUS binutils block-4=%s\n' "$test_status"
grep '^FAIL:' $(find -name '*.log')
make tooldir=/usr install
rm -rfv /usr/lib/lib{bfd,ctf,ctf-nobfd,gprofng,opcodes,sframe}.a \
        /usr/share/doc/gprofng/
  finish_source
}
run_step binutils package_binutils

package_gmp()
{
  unpack_source gmp-6.3.0.tar.xz
sed -i '/long long t1;/,+1s/()/(...)/' configure
./configure --prefix=/usr    \
            --enable-cxx     \
            --disable-static \
            --docdir=/usr/share/doc/gmp-6.3.0
make
make html
  set +e
make check
  test_status=$?
  set -e
  printf 'TEST-STATUS gmp block-4=%s\n' "$test_status"
cat $(find -name '*.log') | grep -c ^PASS
make install
make install-html
  finish_source
}
run_step gmp package_gmp

package_mpfr()
{
  unpack_source mpfr-4.2.2.tar.xz
./configure --prefix=/usr        \
            --disable-static     \
            --enable-thread-safe \
            --docdir=/usr/share/doc/mpfr-4.2.2
make
make html
  set +e
make check
  test_status=$?
  set -e
  printf 'TEST-STATUS mpfr block-3=%s\n' "$test_status"
make install
make install-html
  finish_source
}
run_step mpfr package_mpfr

package_mpc()
{
  unpack_source mpc-1.4.1.tar.xz
./configure --prefix=/usr    \
            --disable-static \
            --docdir=/usr/share/doc/mpc-1.4.1
make
make html
  set +e
make check
  test_status=$?
  set -e
  printf 'TEST-STATUS mpc block-3=%s\n' "$test_status"
make install
make install-html
  finish_source
}
run_step mpc package_mpc

package_attr()
{
  unpack_source attr-2.6.0.tar.gz
./configure --prefix=/usr     \
            --disable-static  \
            --sysconfdir=/etc \
            --docdir=/usr/share/doc/attr-2.6.0
make
  set +e
make check
  test_status=$?
  set -e
  printf 'TEST-STATUS attr block-3=%s\n' "$test_status"
make install
  finish_source
}
run_step attr package_attr

package_acl()
{
  unpack_source acl-2.4.0.tar.xz
./configure --prefix=/usr    \
            --disable-static \
            --docdir=/usr/share/doc/acl-2.4.0
make
  set +e
make check
  test_status=$?
  set -e
  printf 'TEST-STATUS acl block-3=%s\n' "$test_status"
make install
  finish_source
}
run_step acl package_acl

package_libcap()
{
  unpack_source libcap-2.78.tar.xz
sed -i '/install -m.*STA/d' libcap/Makefile
make prefix=/usr lib=lib
  set +e
make test
  test_status=$?
  set -e
  printf 'TEST-STATUS libcap block-3=%s\n' "$test_status"
make prefix=/usr lib=lib install
  finish_source
}
run_step libcap package_libcap

package_libxcrypt()
{
  unpack_source libxcrypt-4.5.2.tar.xz
sed -i '/strchr/s/const//' lib/crypt-{sm3,gost}-yescrypt.c
./configure --prefix=/usr                \
            --enable-hashes=strong,glibc \
            --enable-obsolete-api=no     \
            --disable-static             \
            --disable-failure-tokens
make
  set +e
make check
  test_status=$?
  set -e
  printf 'TEST-STATUS libxcrypt block-4=%s\n' "$test_status"
make install
make distclean
./configure --prefix=/usr                \
            --enable-hashes=strong,glibc \
            --enable-obsolete-api=glibc  \
            --disable-static             \
            --disable-failure-tokens
make
cp -av --remove-destination .libs/libcrypt.so.1* /usr/lib
  finish_source
}
run_step libxcrypt package_libxcrypt

package_shadow()
{
  unpack_source shadow-4.20.2.tar.xz
find man -name Makefile.in -exec sed -i 's/getspnam\.3 / /' {} \;
find man -name Makefile.in -exec sed -i 's/passwd\.5 / /'   {} \;
sed -e 's:#ENCRYPT_METHOD SHA512:ENCRYPT_METHOD YESCRYPT:' \
    -e 's:/var/spool/mail:/var/mail:'                      \
    -e '/PATH=/{s@/sbin:@@;s@/bin:@@}'                     \
    -i etc/login.defs
touch /usr/bin/passwd
./configure --sysconfdir=/etc   \
            --disable-static    \
            --with-{b,yes}crypt \
            --without-libbsd    \
            --disable-logind    \
            --with-group-name-max-length=32
make
make exec_prefix=/usr install
make -C man install-man
  finish_source
}
run_step shadow package_shadow

package_gawk()
{
  unpack_source gawk-5.4.1.tar.xz
sed -i 's/extras//' Makefile.in
./configure --prefix=/usr
make
  set +e
chown -R tester .
su tester -c "PATH=$PATH make check"
  test_status=$?
  set -e
  printf 'TEST-STATUS gawk block-4=%s\n' "$test_status"
rm -f /usr/bin/gawk-5.4.1
make install
ln -sv gawk.1 /usr/share/man/man1/awk.1
install -vDm644 doc/{awkforai.txt,*.{eps,pdf,jpg}} -t /usr/share/doc/gawk-5.4.1
  finish_source
}
run_step gawk package_gawk

package_gcc()
{
  unpack_source gcc-16.2.0.tar.xz
case $(uname -m) in
  x86_64)
    sed -e '/m64=/s/lib64/lib/' \
        -i.orig gcc/config/i386/t-linux64
  ;;
esac
mkdir -v build
cd       build
../configure --prefix=/usr            \
             LD=ld                    \
             --enable-languages=c,c++ \
             --enable-default-pie     \
             --enable-default-ssp     \
             --enable-host-pie        \
             --enable-targets=all     \
             --disable-multilib       \
             --disable-bootstrap      \
             --disable-fixincludes    \
             --with-system-zlib
make
ulimit -s -H unlimited
  set +e
chown -R tester .
su tester -c "PATH=$PATH make -k check"
  test_status=$?
  set -e
  printf 'TEST-STATUS gcc block-6=%s\n' "$test_status"
  set +e
../contrib/test_summary -t
  test_status=$?
  set -e
  printf 'TEST-STATUS gcc block-7=%s\n' "$test_status"
make install
chown -v -R root:root $(gcc -print-file-name=include){,-fixed}
ln -svr /usr/bin/cpp /usr/lib
ln -sv gcc.1 /usr/share/man/man1/cc.1
ln -sfvr $(gcc -print-prog-name=liblto_plugin.so) /usr/lib/bfd-plugins/
echo 'int main(){}' | cc -x c - -v -Wl,--verbose &> dummy.log
readelf -l a.out | grep ': /lib'
grep -E -o '/usr/lib.*/S?crt[1in].*succeeded' dummy.log
grep -B4 '^ /usr/include' dummy.log
grep 'SEARCH.*/usr/lib' dummy.log |sed 's|; |\n|g'
grep "/lib.*/libc.so.6 " dummy.log
grep found dummy.log
rm -v a.out dummy.log
mkdir -pv /usr/share/gdb/auto-load/usr/lib
mv -v /usr/lib/*gdb.py /usr/share/gdb/auto-load/usr/lib
  finish_source
}
run_step gcc package_gcc

package_ncurses()
{
  unpack_source ncurses-6.6.tar.gz
./configure --prefix=/usr           \
            --mandir=/usr/share/man \
            --with-shared           \
            --without-debug         \
            --without-normal        \
            --with-cxx-shared       \
            --enable-pc-files       \
            --with-pkg-config-libdir=/usr/lib/pkgconfig
make
make DESTDIR=$PWD/dest install
sed -e 's/^#if.*XOPEN.*$/#if 1/' \
    -i dest/usr/include/curses.h
cp --remove-destination -av dest/* /
for lib in ncurses form panel menu ; do
    ln -sfv lib${lib}w.so /usr/lib/lib${lib}.so
    ln -sfv ${lib}w.pc    /usr/lib/pkgconfig/${lib}.pc
done
ln -sfv libncursesw.so /usr/lib/libcurses.so
cp -v -R doc -T /usr/share/doc/ncurses-6.6
make distclean
./configure --prefix=/usr    \
            --with-shared    \
            --without-normal \
            --without-debug  \
            --without-cxx-binding \
            --with-abi-version=5
make sources libs
cp -av lib/lib*.so.5* /usr/lib
  finish_source
}
run_step ncurses package_ncurses

package_sed()
{
  unpack_source sed-4.10.tar.xz
./configure --prefix=/usr
make
make html
  set +e
chown -R tester .
su tester -c "PATH=$PATH make check"
  test_status=$?
  set -e
  printf 'TEST-STATUS sed block-3=%s\n' "$test_status"
make install
install -vDm644 doc/sed.html -t /usr/share/doc/sed-4.10
  finish_source
}
run_step sed package_sed

package_psmisc()
{
  unpack_source psmisc-23.7.tar.xz
./configure --prefix=/usr
make
  set +e
make check
  test_status=$?
  set -e
  printf 'TEST-STATUS psmisc block-3=%s\n' "$test_status"
make install
  finish_source
}
run_step psmisc package_psmisc

package_gettext()
{
  unpack_source gettext-1.0.tar.xz
./configure --prefix=/usr    \
            --disable-static \
            --docdir=/usr/share/doc/gettext-1.0
make
  set +e
make check
  test_status=$?
  set -e
  printf 'TEST-STATUS gettext block-3=%s\n' "$test_status"
make install
chmod -v 0755 /usr/lib/preloadable_libintl.so
  finish_source
}
run_step gettext package_gettext

package_bison()
{
  unpack_source bison-3.8.2.tar.xz
./configure --prefix=/usr --docdir=/usr/share/doc/bison-3.8.2
make
  set +e
make check
  test_status=$?
  set -e
  printf 'TEST-STATUS bison block-3=%s\n' "$test_status"
make install
  finish_source
}
run_step bison package_bison

package_grep()
{
  unpack_source grep-3.12.tar.xz
sed -i "s/echo/#echo/" src/egrep.sh
./configure --prefix=/usr
make
  set +e
make check
  test_status=$?
  set -e
  printf 'TEST-STATUS grep block-4=%s\n' "$test_status"
make install
  finish_source
}
run_step grep package_grep

package_bash()
{
  unpack_source bash-5.3.tar.gz
./configure --prefix=/usr             \
            --without-bash-malloc     \
            --with-installed-readline \
            --docdir=/usr/share/doc/bash-5.3
make
chown -R tester .
  set +e
LC_ALL=C.UTF-8 su -s /usr/bin/expect tester << "EOF"
set timeout -1
spawn make tests
expect eof
lassign [wait] _ _ _ value
exit $value
EOF
  test_status=$?
  set -e
  printf 'TEST-STATUS bash block-4=%s\n' "$test_status"
make install
  finish_source
}
run_step bash package_bash

package_libtool()
{
  unpack_source libtool-2.6.2.tar.xz
./configure --prefix=/usr
make
  set +e
make check
  test_status=$?
  set -e
  printf 'TEST-STATUS libtool block-3=%s\n' "$test_status"
make install
rm -fv /usr/lib/libltdl.a
  finish_source
}
run_step libtool package_libtool

package_gdbm()
{
  unpack_source gdbm-1.26.tar.gz
./configure --prefix=/usr    \
            --disable-static \
            --enable-libgdbm-compat
make
  set +e
make check
  test_status=$?
  set -e
  printf 'TEST-STATUS gdbm block-3=%s\n' "$test_status"
make install
  finish_source
}
run_step gdbm package_gdbm

package_gperf()
{
  unpack_source gperf-3.3.tar.gz
./configure --prefix=/usr --docdir=/usr/share/doc/gperf-3.3
make
  set +e
make check
  test_status=$?
  set -e
  printf 'TEST-STATUS gperf block-3=%s\n' "$test_status"
make install
  finish_source
}
run_step gperf package_gperf

package_expat()
{
  unpack_source expat-2.8.3.tar.xz
./configure --prefix=/usr    \
            --disable-static \
            --docdir=/usr/share/doc/expat-2.8.3
make
  set +e
make check
  test_status=$?
  set -e
  printf 'TEST-STATUS expat block-3=%s\n' "$test_status"
make install
install -v -m644 doc/*.{html,css} /usr/share/doc/expat-2.8.3
  finish_source
}
run_step expat package_expat

package_inetutils()
{
  unpack_source inetutils-2.8.tar.gz
sed -i 's/def HAVE_TERMCAP_TGETENT/ 1/' telnet/telnet.c
./configure --prefix=/usr        \
            --bindir=/usr/bin    \
            --localstatedir=/var \
            --disable-logger     \
            --disable-whois      \
            --disable-rcp        \
            --disable-rexec      \
            --disable-rlogin     \
            --disable-rsh        \
            --disable-servers
make
  set +e
make check
  test_status=$?
  set -e
  printf 'TEST-STATUS inetutils block-4=%s\n' "$test_status"
make install
mv -v /usr/{,s}bin/ifconfig
  finish_source
}
run_step inetutils package_inetutils

package_less()
{
  unpack_source less-704.tar.gz
./configure --prefix=/usr --sysconfdir=/etc
make
  set +e
make check
  test_status=$?
  set -e
  printf 'TEST-STATUS less block-3=%s\n' "$test_status"
make install
  finish_source
}
run_step less package_less

package_perl()
{
  unpack_source perl-5.44.0.tar.xz
export BUILD_ZLIB=False
export BUILD_BZIP2=0
sh Configure -des                                          \
             -D prefix=/usr                                \
             -D vendorprefix=/usr                          \
             -D privlib=/usr/lib/perl5/5.44/core_perl      \
             -D archlib=/usr/lib/perl5/5.44/core_perl      \
             -D sitelib=/usr/lib/perl5/5.44/site_perl      \
             -D sitearch=/usr/lib/perl5/5.44/site_perl     \
             -D vendorlib=/usr/lib/perl5/5.44/vendor_perl  \
             -D vendorarch=/usr/lib/perl5/5.44/vendor_perl \
             -D man1dir=/usr/share/man/man1                \
             -D man3dir=/usr/share/man/man3                \
             -D pager="/usr/bin/less -isR"                 \
             -D useshrplib                                 \
             -D usethreads
make
  set +e
TEST_JOBS=$(nproc) make test_harness
  test_status=$?
  set -e
  printf 'TEST-STATUS perl block-4=%s\n' "$test_status"
make install
unset BUILD_ZLIB BUILD_BZIP2
  finish_source
}
run_step perl package_perl

package_autoconf()
{
  unpack_source autoconf-2.73.tar.xz
./configure --prefix=/usr
make
  set +e
make check
  test_status=$?
  set -e
  printf 'TEST-STATUS autoconf block-3=%s\n' "$test_status"
make install
  finish_source
}
run_step autoconf package_autoconf

package_automake()
{
  unpack_source automake-1.18.1.tar.xz
./configure --prefix=/usr --docdir=/usr/share/doc/automake-1.18.1
make
  set +e
make -j$(($(nproc)>4?$(nproc):4)) check
  test_status=$?
  set -e
  printf 'TEST-STATUS automake block-3=%s\n' "$test_status"
make install
  finish_source
}
run_step automake package_automake

package_openssl()
{
  unpack_source openssl-4.0.1.tar.gz
./config --prefix=/usr         \
         --openssldir=/etc/ssl \
         --libdir=lib          \
         shared                \
         zlib-dynamic
make
  set +e
make test
  test_status=$?
  set -e
  printf 'TEST-STATUS openssl block-3=%s\n' "$test_status"
make INSTALL_LIBS= MANSUFFIX=ssl install
mv -v /usr/share/doc/openssl /usr/share/doc/openssl-4.0.1
cp -vfr doc/* /usr/share/doc/openssl-4.0.1
  finish_source
}
run_step openssl package_openssl

package_libelf()
{
  unpack_source elfutils-0.195.tar.bz2
./configure --prefix=/usr        \
            --disable-debuginfod \
            --enable-libdebuginfod=dummy
make -C lib
make -C libelf
  set +e
make -k check
  test_status=$?
  set -e
  printf 'TEST-STATUS libelf block-3=%s\n' "$test_status"
make -C libelf install
install -vm644 config/libelf.pc /usr/lib/pkgconfig
rm /usr/lib/libelf.a
  finish_source
}
run_step libelf package_libelf

package_libffi()
{
  unpack_source libffi-3.8.0.tar.gz
./configure --prefix=/usr    \
            --disable-static \
            --with-gcc-arch=native
make
  set +e
make check
  test_status=$?
  set -e
  printf 'TEST-STATUS libffi block-3=%s\n' "$test_status"
make install
  finish_source
}
run_step libffi package_libffi

package_sqlite()
{
  unpack_source sqlite-autoconf-3530400.tar.gz
python3 -m zipfile -e ../sqlite-doc-3530400.zip .
./configure --prefix=/usr     \
            --disable-static  \
            --enable-fts{4,5} \
            CPPFLAGS="-D SQLITE_ENABLE_COLUMN_METADATA=1 \
                      -D SQLITE_ENABLE_UNLOCK_NOTIFY=1   \
                      -D SQLITE_ENABLE_DBSTAT_VTAB=1     \
                      -D SQLITE_SECURE_DELETE=1"
make LDFLAGS.rpath=""
make install
cp -v -R sqlite-doc-3530400 -T /usr/share/doc/sqlite-3.53.4
  finish_source
}
run_step sqlite package_sqlite

package_mpdecimal()
{
  unpack_source mpdecimal-4.0.1.tar.gz
./configure --prefix=/usr    \
            --disable-static \
            --docdir=/usr/share/doc/mpdecimal-4.0.1
make
  set +e
make check_local
  test_status=$?
  set -e
  printf 'TEST-STATUS mpdecimal block-3=%s\n' "$test_status"
make install
  finish_source
}
run_step mpdecimal package_mpdecimal

package_python()
{
  unpack_source Python-3.14.7.tar.xz
patch -Np1 -i ../Python-3.14.7-openssl_4-1.patch
./configure --prefix=/usr          \
            --enable-shared        \
            --with-system-expat    \
            --enable-optimizations \
            --without-static-libpython
make
  set +e
make test TESTOPTS="--timeout 120"
  test_status=$?
  set -e
  printf 'TEST-STATUS Python block-4=%s\n' "$test_status"
make install
cat > /etc/pip.conf << EOF
[global]
root-user-action = ignore
disable-pip-version-check = true
EOF
install -v -dm755 /usr/share/doc/python-3.14.7/html

tar --strip-components=1  \
    --no-same-owner       \
    --no-same-permissions \
    -C /usr/share/doc/python-3.14.7/html \
    -xvf ../python-3.14.7-docs-html.tar.bz2
  finish_source
}
run_step Python package_python

package_flit_core()
{
  unpack_source flit_core-4.0.2.tar.gz
pip3 wheel -w dist --no-cache-dir --no-build-isolation --no-deps $PWD
pip3 install --no-index --find-links dist flit_core
  finish_source
}
run_step flit-core package_flit_core

package_packaging()
{
  unpack_source packaging-26.3.tar.gz
pip3 wheel -w dist --no-cache-dir --no-build-isolation --no-deps $PWD
pip3 install --no-index --find-links dist packaging
  finish_source
}
run_step packaging package_packaging

package_wheel()
{
  unpack_source wheel-0.48.0.tar.gz
pip3 wheel -w dist --no-cache-dir --no-build-isolation --no-deps $PWD
pip3 install --no-index --find-links dist wheel
  finish_source
}
run_step wheel package_wheel

package_setuptools()
{
  unpack_source setuptools-84.0.0.tar.gz
pip3 wheel -w dist --no-cache-dir --no-build-isolation --no-deps $PWD
pip3 install --no-index --find-links dist setuptools
  finish_source
}
run_step setuptools package_setuptools

package_meson()
{
  unpack_source meson-1.12.0.tar.gz
pip3 wheel -w dist --no-cache-dir --no-build-isolation --no-deps $PWD
pip3 install --no-index --find-links dist meson
install -vDm644 data/shell-completions/bash/meson /usr/share/bash-completion/completions/meson
install -vDm644 data/shell-completions/zsh/_meson /usr/share/zsh/site-functions/_meson
  finish_source
}
run_step meson package_meson

package_kmod()
{
  unpack_source kmod-34.2.tar.xz
mkdir -p build
cd       build

meson setup --prefix=/usr ..    \
            --buildtype=release \
            -D manpages=false
ninja
ninja install
  finish_source
}
run_step kmod package_kmod

package_coreutils()
{
  unpack_source coreutils-9.11.tar.xz
patch -Np1 -i ../coreutils-9.11-i18n-1.patch
autoreconf -fv
automake -af
FORCE_UNSAFE_CONFIGURE=1 ./configure \
            --prefix=/usr
make
  set +e
make NON_ROOT_USERNAME=tester check-root
  test_status=$?
  set -e
  printf 'TEST-STATUS coreutils block-4=%s\n' "$test_status"
groupadd -g 102 dummy -U tester
chown -R tester .
  set +e
su tester -c "PATH=$PATH make -k RUN_EXPENSIVE_TESTS=yes check" \
   < /dev/null
  test_status=$?
  set -e
  printf 'TEST-STATUS coreutils block-7=%s\n' "$test_status"
groupdel dummy
make install
mv -v /usr/bin/chroot /usr/sbin
mv -v /usr/share/man/man1/chroot.1 /usr/share/man/man8/chroot.8
sed -i 's/"1"/"8"/' /usr/share/man/man8/chroot.8
  finish_source
}
run_step coreutils package_coreutils

package_diffutils()
{
  unpack_source diffutils-3.12.tar.xz
./configure --prefix=/usr
make
  set +e
make check
  test_status=$?
  set -e
  printf 'TEST-STATUS diffutils block-3=%s\n' "$test_status"
make install
  finish_source
}
run_step diffutils package_diffutils

package_findutils()
{
  unpack_source findutils-4.11.0.tar.xz
./configure --prefix=/usr --localstatedir=/var/lib/locate
make
  set +e
chown -R tester .
su tester -c "PATH=$PATH make check -k"
  test_status=$?
  set -e
  printf 'TEST-STATUS findutils block-3=%s\n' "$test_status"
make install
  finish_source
}
run_step findutils package_findutils

package_groff()
{
  unpack_source groff-1.24.1.tar.gz
PAGE=A4 ./configure --prefix=/usr
make -j1
  set +e
make check
  test_status=$?
  set -e
  printf 'TEST-STATUS groff block-3=%s\n' "$test_status"
make install
  finish_source
}
run_step groff package_groff

package_grub()
{
  unpack_source grub-2.14.tar.xz
sed 's/--image-base/--nonexist-linker-option/' -i configure
./configure --prefix=/usr     \
            --sysconfdir=/etc \
            --disable-efiemu  \
            --disable-werror
make
make install
make clean
./configure --prefix=/usr       \
            --sysconfdir=/etc   \
            --target=x86_64     \
            --with-platform=efi \
            --disable-efiemu    \
            --disable-werror
make
make install
make clean
./configure --prefix=/usr       \
            --sysconfdir=/etc   \
            --target=i386       \
            --with-platform=efi \
            --disable-efiemu    \
            --disable-werror
make
make install
  finish_source
}
run_step grub package_grub

package_gzip()
{
  unpack_source gzip-1.14.tar.xz
./configure --prefix=/usr
make
  set +e
make check
  test_status=$?
  set -e
  printf 'TEST-STATUS gzip block-3=%s\n' "$test_status"
make install
  finish_source
}
run_step gzip package_gzip

package_iproute2()
{
  unpack_source iproute2-7.1.0.tar.xz
sed -i /ARPD/d Makefile
rm -fv man/man8/arpd.8
make NETNS_RUN_DIR=/run/netns
make SBINDIR=/usr/sbin install
install -vDm644 COPYING README* -t /usr/share/doc/iproute2-7.1.0
  finish_source
}
run_step iproute2 package_iproute2

package_kbd()
{
  unpack_source kbd-2.10.0.tar.xz
patch -Np1 -i ../kbd-2.10.0-backspace-1.patch
sed -i '/RESIZECONS_PROGS=/s/yes/no/' configure
sed -i 's/resizecons.8 //' docs/man/man8/Makefile.in
./configure --prefix=/usr --disable-vlock
make
  set +e
make check
  test_status=$?
  set -e
  printf 'TEST-STATUS kbd block-5=%s\n' "$test_status"
make install
cp -R -v docs/doc -T /usr/share/doc/kbd-2.10.0
  finish_source
}
run_step kbd package_kbd

package_libpipeline()
{
  unpack_source libpipeline-1.5.8.tar.gz
./configure --prefix=/usr
make
make install
  finish_source
}
run_step libpipeline package_libpipeline

package_make()
{
  unpack_source make-4.4.1.tar.gz
./configure --prefix=/usr
make
  set +e
chown -R tester .
su tester -c "PATH=$PATH make check"
  test_status=$?
  set -e
  printf 'TEST-STATUS make block-3=%s\n' "$test_status"
make install
  finish_source
}
run_step make package_make

package_patch()
{
  unpack_source patch-2.8.tar.xz
./configure --prefix=/usr
make
  set +e
make check
  test_status=$?
  set -e
  printf 'TEST-STATUS patch block-3=%s\n' "$test_status"
make install
  finish_source
}
run_step patch package_patch

package_tar()
{
  unpack_source tar-1.35.tar.xz
patch -Np1 -i ../tar-1.35-acl_fix-1.patch
FORCE_UNSAFE_CONFIGURE=1  \
./configure --prefix=/usr
make
  set +e
make check
  test_status=$?
  set -e
  printf 'TEST-STATUS tar block-4=%s\n' "$test_status"
make install
make -C doc install-html docdir=/usr/share/doc/tar-1.35
  finish_source
}
run_step tar package_tar

package_texinfo()
{
  unpack_source texinfo-7.3.tar.xz
./configure --prefix=/usr
make
  set +e
make check
  test_status=$?
  set -e
  printf 'TEST-STATUS texinfo block-3=%s\n' "$test_status"
make install
make TEXMF=/usr/share/texmf install-tex
pushd /usr/share/info
  rm -v dir
  for f in *
    do install-info $f dir 2>/dev/null
  done
popd
  finish_source
}
run_step texinfo package_texinfo

package_vim()
{
  unpack_source vim-9.2.0954.tar.gz
echo '#define SYS_VIMRC_FILE "/etc/vimrc"' >> src/feature.h
./configure --prefix=/usr
make
chown -R tester .
sed '/test_plugin_glvs/d' -i src/testdir/Make_all.mak
  set +e
su tester -c "TERM=xterm-256color LANG=en_US.UTF-8 make -j1 test" \
   &> vim-test.log
  test_status=$?
  set -e
  printf 'TEST-STATUS vim block-5=%s\n' "$test_status"
make install
ln -sv vim /usr/bin/vi
for L in  /usr/share/man/{,*/}man1/vim.1; do
    ln -sv vim.1 $(dirname $L)/vi.1
done
ln -sv ../vim/vim92/doc /usr/share/doc/vim-9.2.0954
  finish_source
}
run_step vim package_vim

package_markupsafe()
{
  unpack_source markupsafe-3.0.3.tar.gz
pip3 wheel -w dist --no-cache-dir --no-build-isolation --no-deps $PWD
pip3 install --no-index --find-links dist Markupsafe
  finish_source
}
run_step markupsafe package_markupsafe

package_jinja2()
{
  unpack_source jinja2-3.1.6.tar.gz
pip3 wheel -w dist --no-cache-dir --no-build-isolation --no-deps $PWD
pip3 install --no-index --find-links dist Jinja2
  finish_source
}
run_step jinja2 package_jinja2

package_systemd()
{
  unpack_source systemd-261.2.tar.gz
sed -e 's/GROUP="render"/GROUP="video"/' \
    -e 's/GROUP="sgx", //'               \
    -i rules.d/50-udev-default.rules.in
mkdir -p build
cd       build

meson setup ..                \
      --prefix=/usr           \
      --buildtype=release     \
      -D default-dnssec=no    \
      -D firstboot=false      \
      -D install-tests=false  \
      -D ldconfig=false       \
      -D sysusers=false       \
      -D rpmmacrosdir=no      \
      -D homed=disabled       \
      -D man=disabled         \
      -D mode=release         \
      -D pamconfdir=no        \
      -D dev-kvm-mode=0660    \
      -D nobody-group=nogroup \
      -D sysupdate=disabled   \
      -D ukify=disabled       \
      -D docdir=/usr/share/doc/systemd-261.2
ninja
  set +e
echo 'NAME="Linux From Scratch"' > /etc/os-release
unshare -m ninja test
  test_status=$?
  set -e
  printf 'TEST-STATUS systemd block-4=%s\n' "$test_status"
ninja install
tar -xf ../../systemd-man-pages-261.2.tar.xz \
    --no-same-owner --strip-components=1     \
    -C /usr/share/man
systemd-machine-id-setup
systemctl preset-all
  finish_source
}
run_step systemd package_systemd

package_dbus()
{
  unpack_source dbus-1.16.2.tar.xz
mkdir build
cd    build

meson setup --prefix=/usr --buildtype=release --wrap-mode=nofallback ..
ninja
  set +e
ninja test
  test_status=$?
  set -e
  printf 'TEST-STATUS dbus block-3=%s\n' "$test_status"
ninja install
ln -sfv /etc/machine-id /var/lib/dbus
  finish_source
}
run_step dbus package_dbus

package_man_db()
{
  unpack_source man-db-2.13.1.tar.xz
./configure --prefix=/usr                         \
            --docdir=/usr/share/doc/man-db-2.13.1 \
            --sysconfdir=/etc                     \
            --disable-setuid                      \
            --enable-cache-owner=bin              \
            --with-browser=/usr/bin/lynx          \
            --with-vgrind=/usr/bin/vgrind         \
            --with-grap=/usr/bin/grap
make
  set +e
make check
  test_status=$?
  set -e
  printf 'TEST-STATUS man-db block-3=%s\n' "$test_status"
make install
  finish_source
}
run_step man-db package_man_db

package_procps_ng()
{
  unpack_source procps-ng-4.0.7.tar.xz
./configure --prefix=/usr                           \
            --docdir=/usr/share/doc/procps-ng-4.0.7 \
            --disable-static                        \
            --disable-kill                          \
            --enable-watch8bit                      \
            --with-systemd
make
  set +e
chown -R tester .
su tester -c "PATH=$PATH make check"
  test_status=$?
  set -e
  printf 'TEST-STATUS procps-ng block-3=%s\n' "$test_status"
make install
  finish_source
}
run_step procps-ng package_procps_ng

package_util_linux()
{
  unpack_source util-linux-2.42.2.tar.xz
./configure --bindir=/usr/bin     \
            --libdir=/usr/lib     \
            --runstatedir=/run    \
            --sbindir=/usr/sbin   \
            --disable-chfn-chsh   \
            --disable-login       \
            --disable-nologin     \
            --disable-su          \
            --disable-setpriv     \
            --disable-runuser     \
            --disable-pylibmount  \
            --disable-liblastlog2 \
            --disable-static      \
            --without-python      \
            ADJTIME_PATH=/var/lib/hwclock/adjtime \
            --docdir=/usr/share/doc/util-linux-2.42.2
make
  set +e
bash tests/run.sh --srcdir=$PWD --builddir=$PWD
  test_status=$?
  set -e
  printf 'TEST-STATUS util-linux block-3=%s\n' "$test_status"
  set +e
touch /etc/fstab
chown -R tester .
su tester -c "make -k check"
  test_status=$?
  set -e
  printf 'TEST-STATUS util-linux block-4=%s\n' "$test_status"
make install
  finish_source
}
run_step util-linux package_util_linux

package_e2fsprogs()
{
  unpack_source e2fsprogs-1.47.4.tar.gz
mkdir -v build
cd       build
../configure --prefix=/usr       \
             --sysconfdir=/etc   \
             --enable-elf-shlibs \
             --disable-libblkid  \
             --disable-libuuid   \
             --disable-uuidd     \
             --disable-fsck
make
  set +e
make check
  test_status=$?
  set -e
  printf 'TEST-STATUS e2fsprogs block-4=%s\n' "$test_status"
make install
rm -fv /usr/lib/{libcom_err,libe2p,libext2fs,libss}.a
gunzip -v /usr/share/info/libext2fs.info.gz
install-info --dir-file=/usr/share/info/dir /usr/share/info/libext2fs.info
makeinfo -o      doc/com_err.info ../lib/et/com_err.texinfo
install -v -m644 doc/com_err.info /usr/share/info
install-info --dir-file=/usr/share/info/dir /usr/share/info/com_err.info
  finish_source
}
run_step e2fsprogs package_e2fsprogs

printf '\nCHAPTER 8 PACKAGES COMPLETE %s\n' "$(date --utc +'%Y-%m-%dT%H:%M:%SZ')"
