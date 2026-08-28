# Chapter 5 cross-toolchain contract

Status: executable construction gate

## Sequence

The package order is binding and follows LFS `r13.0-201-systemd` exactly:

1. Binutils 2.47, pass 1
2. GCC 16.2.0, pass 1
3. Linux 7.1.8 API headers
4. Glibc 2.44 with both book-required patches
5. Target Libstdc++ from GCC 16.2.0

The runner executes as the unprivileged `lfs` user in the Chapter 4 clean
environment. Every source tree is freshly extracted from the verified target
source directory. No network is used and no package is substituted.

## Evidence and resumption

Logs and completion markers live under `$LFS/var/log/flowlfs/ch05`. A marker is
written only after a package installs and its local validation succeeds. A
failed package has no marker and is rebuilt from a fresh extraction on the next
run; completed predecessors are not rebuilt.

## Mandatory checks

- Cross Binutils executables identify target `x86_64-lfs-linux-gnu`.
- GCC reports sysroot `/mnt/lfs` and creates its complete internal `limits.h`.
- Sanitized Linux API headers appear under `$LFS/usr/include`.
- Glibc's test program requests `/lib64/ld-linux-x86-64.so.2`, never a
  `/mnt/lfs` interpreter.
- GCC finds the target start files, target headers, sysrooted linker search
  paths, target libc, and target dynamic loader exactly as described by the
  book.
- Target Libstdc++ headers and shared library exist, and harmful libtool archive
  files are absent.

The target is sealed at the end of Chapter 5 before Chapter 6 begins.
