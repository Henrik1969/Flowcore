# Chapter 6 temporary-tools contract

Status: executable construction gate

The runner follows LFS `r13.0-201-systemd` sections 6.2–6.18 in order: M4,
Ncurses, Bash, Coreutils, Diffutils, File, Findutils, Gawk, Grep, Gzip, Make,
Patch, Sed, Tar, Xz, Binutils pass 2, and GCC pass 2.

All builds run as `lfs` in the clean cross environment, start from verified
archives, use no network, and install only below `$LFS`. Native helper programs
for Ncurses and File are built only where the book requires them. Logs and
completion markers live in `$LFS/var/log/flowlfs/ch06`; incomplete packages are
rebuilt from a fresh extraction while completed predecessors are skipped.

The gate requires all 17 markers, the expected temporary executable set,
`/bin/sh -> bash`, `/usr/bin/cc -> gcc`, the second-pass linker/compiler, and
the absence of the libtool/static archives explicitly removed by the book.
The target is sealed before Chapter 7 and chroot preparation.
