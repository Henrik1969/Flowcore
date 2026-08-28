#!/usr/bin/env bash

set -euo pipefail
test "$(id -u)" -eq 0
test -r /proc/self/mounts
test -x /usr/bin/gcc

log_dir=/var/log/flowlfs/ch07
mkdir -p "$log_dir"
exec > >(tee -a "$log_dir/chapter07.log") 2>&1

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

filesystem_setup()
{
  mkdir -pv /{boot,home,mnt,opt,srv}
  mkdir -pv /etc/{opt,sysconfig}
  mkdir -pv /lib/firmware
  mkdir -pv /media/{floppy,cdrom}
  mkdir -pv /usr/{,local/}{include,src}
  mkdir -pv /usr/lib/locale
  mkdir -pv /usr/local/{bin,lib,sbin}
  mkdir -pv /usr/{,local/}share/{color,dict,doc,info,locale,man}
  mkdir -pv /usr/{,local/}share/{misc,terminfo,zoneinfo}
  mkdir -pv /usr/{,local/}share/man/man{1..8}
  mkdir -pv /var/{cache,local,log,mail,opt,spool}
  mkdir -pv /var/lib/{color,misc,locate}
  ln -sfv /run /var/run
  ln -sfv /run/lock /var/lock
  install -dv -m 0750 /root
  install -dv -m 1777 /tmp /var/tmp
  ln -sfv /proc/self/mounts /etc/mtab
  cat > /etc/hosts <<EOF
127.0.0.1  localhost $(hostname)
::1        localhost
EOF
  cat > /etc/passwd <<'EOF'
root:x:0:0:root:/root:/bin/bash
bin:x:1:1:bin:/dev/null:/usr/bin/false
daemon:x:6:6:Daemon User:/dev/null:/usr/bin/false
messagebus:x:18:18:D-Bus Message Daemon User:/run/dbus:/usr/bin/false
systemd-journal-gateway:x:73:73:systemd Journal Gateway:/:/usr/bin/false
systemd-journal-remote:x:74:74:systemd Journal Remote:/:/usr/bin/false
systemd-journal-upload:x:75:75:systemd Journal Upload:/:/usr/bin/false
systemd-network:x:76:76:systemd Network Management:/:/usr/bin/false
systemd-resolve:x:77:77:systemd Resolver:/:/usr/bin/false
systemd-timesync:x:78:78:systemd Time Synchronization:/:/usr/bin/false
systemd-coredump:x:79:79:systemd Core Dumper:/:/usr/bin/false
uuidd:x:80:80:UUID Generation Daemon User:/dev/null:/usr/bin/false
systemd-oom:x:81:81:systemd Out Of Memory Daemon:/:/usr/bin/false
nobody:x:65534:65534:Unprivileged User:/dev/null:/usr/bin/false
tester:x:101:101::/home/tester:/bin/bash
EOF
  cat > /etc/group <<'EOF'
root:x:0:
bin:x:1:daemon
sys:x:2:
kmem:x:3:
tape:x:4:
tty:x:5:
daemon:x:6:
floppy:x:7:
disk:x:8:
lp:x:9:
dialout:x:10:
audio:x:11:
video:x:12:
utmp:x:13:
clock:x:14:
cdrom:x:15:
adm:x:16:
messagebus:x:18:
systemd-journal:x:23:
input:x:24:
mail:x:34:
kvm:x:61:
systemd-journal-gateway:x:73:
systemd-journal-remote:x:74:
systemd-journal-upload:x:75:
systemd-network:x:76:
systemd-resolve:x:77:
systemd-timesync:x:78:
systemd-coredump:x:79:
uuidd:x:80:
systemd-oom:x:81:
wheel:x:97:
users:x:999:
nogroup:x:65534:
tester:x:101:
EOF
  install -o tester -d /home/tester
  touch /var/log/{btmp,lastlog,faillog,wtmp}
  chgrp -v utmp /var/log/lastlog
  chmod -v 664 /var/log/lastlog
  chmod -v 600 /var/log/btmp
}

unpack()
{
  local archive=$1 directory=$2
  cd /sources
  rm -rf "$directory"
  tar -xf "$archive"
  cd "$directory"
}

finish_source()
{
  cd /sources
  rm -rf "$1"
}

gettext_build()
{
  unpack gettext-1.0.tar.xz gettext-1.0
  ./configure --disable-shared
  make
  cp -v gettext-tools/src/{msgfmt,msgmerge,xgettext} /usr/bin
  finish_source gettext-1.0
}

bison_build()
{
  unpack bison-3.8.2.tar.xz bison-3.8.2
  ./configure --prefix=/usr --docdir=/usr/share/doc/bison-3.8.2
  make
  make install
  finish_source bison-3.8.2
}

perl_build()
{
  unpack perl-5.44.0.tar.xz perl-5.44.0
  sh Configure -des -D prefix=/usr -D vendorprefix=/usr -D useshrplib \
    -D privlib=/usr/lib/perl5/5.44/core_perl \
    -D archlib=/usr/lib/perl5/5.44/core_perl \
    -D sitelib=/usr/lib/perl5/5.44/site_perl \
    -D sitearch=/usr/lib/perl5/5.44/site_perl \
    -D vendorlib=/usr/lib/perl5/5.44/vendor_perl \
    -D vendorarch=/usr/lib/perl5/5.44/vendor_perl
  make
  make install
  finish_source perl-5.44.0
}

zlib_build()
{
  unpack zlib-1.3.2.tar.gz zlib-1.3.2
  ./configure --prefix=/usr
  make
  make install
  rm -fv /usr/lib/libz.a
  finish_source zlib-1.3.2
}

mpdecimal_build()
{
  unpack mpdecimal-4.0.1.tar.gz mpdecimal-4.0.1
  ./configure --prefix=/usr --disable-static \
    --docdir=/usr/share/doc/mpdecimal-4.0.1
  make
  make install
  finish_source mpdecimal-4.0.1
}

python_build()
{
  unpack Python-3.14.7.tar.xz Python-3.14.7
  ./configure --prefix=/usr --enable-shared --without-ensurepip \
    --without-static-libpython
  make
  make install
  finish_source Python-3.14.7
}

texinfo_build()
{
  unpack texinfo-7.3.tar.xz texinfo-7.3
  ./configure --prefix=/usr
  make
  make install
  finish_source texinfo-7.3
}

util_linux_build()
{
  unpack util-linux-2.42.2.tar.xz util-linux-2.42.2
  mkdir -pv /var/lib/hwclock
  ./configure --libdir=/usr/lib --runstatedir=/run \
    --disable-chfn-chsh --disable-login --disable-nologin --disable-su \
    --disable-setpriv --disable-runuser --disable-pylibmount \
    --disable-static --disable-liblastlog2 --without-python \
    ADJTIME_PATH=/var/lib/hwclock/adjtime \
    --docdir=/usr/share/doc/util-linux-2.42.2
  make
  make install
  finish_source util-linux-2.42.2
}

cleanup()
{
  rm -rf /usr/share/{info,man,doc}/*
  find /usr/{lib,libexec} -name '*.la' -delete
  rm -rf /tools
}

run_step filesystem-setup filesystem_setup
run_step gettext gettext_build
run_step bison bison_build
run_step perl perl_build
run_step zlib zlib_build
run_step mpdecimal mpdecimal_build
run_step python python_build
run_step texinfo texinfo_build
run_step util-linux util_linux_build

for binary in bison msgfmt msgmerge perl python3 texi2any xgettext; do
  command -v "$binary" >/dev/null || { printf 'Missing executable: %s\n' "$binary" >&2; exit 1; }
done
test -x /usr/bin/mount
run_step cleanup cleanup
test ! -e /tools
printf '\nCHAPTER 7 COMPLETE %s\n' "$(date --utc +'%Y-%m-%dT%H:%M:%SZ')"
