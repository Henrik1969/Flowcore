# Builder gate evidence — 2026-08-28

## Result

PASS. The isolated builder satisfies the LFS host contract. Construction has
not started and the LFS target disk remains blank.

## Identity

- OS: Ubuntu 24.04.4 LTS
- Kernel: Linux 6.8.0-138-generic
- Architecture: x86-64
- Virtualization: KVM/QEMU q35, legacy BIOS
- CPUs: 4
- Memory: 8 GiB
- Callback: `FLOWLFS_CALLBACK READY phone=ssh://127.0.0.1:2222`
- Callback service: enabled and exited successfully after SSH became ready

## Remedied requirements

- Texinfo 7.1 is installed and exceeds the required 5.0.
- `/bin/sh` resolves to Bash inside the builder.
- The production workstation was not changed to satisfy either requirement.

## Complete host gate

Coreutils 9.4, Bash 5.2.21, Binutils 2.42, Bison 3.8.2, Diffutils
3.10, Findutils 4.9.0, Gawk 5.2.1, GCC/G++ 13.3.0, Grep 3.11, Gzip
1.12, M4 1.4.19, Make 4.3, Patch 2.7.6, Perl 5.38.2, Python 3.12.3,
Sed 4.9, Tar 1.35, Texinfo 7.1, Xz 5.4.5, and Linux 6.8.0 all passed
the book minima. GNU awk, Bison yacc, Bash sh, UNIX 98 PTY, the C++ compiler,
and `nproc` checks also passed.

## Disk boundary

- `/dev/vda`: 32 GiB builder disk; ext4 root on `/dev/vda2`.
- `/dev/vdb`: 40 GiB LFS target; no partition, filesystem, or mount.
- The Ubuntu installer ran with only the builder disk attached and explicitly
  reported partitioning `disk-vda`.
- Both QCOW2 images passed `qemu-img check` after clean shutdown.

## Sealed identities

- Prepared read-only builder base SHA-256:
  `d2ce03a42f62ff2dfc62ff7f3375e2cc94475c26c925a877f56a7200cfa278d6`
- Pristine LFS target SHA-256:
  `04082b5e0bf83aa59cde7b5bb9328d9707bb1e6389bbf5bc824ed23111cb225e`
- Builder base virtual size: 32 GiB; allocated size at seal: 5.13 GiB.
- Target virtual size: 40 GiB; allocated size at gate: 196 KiB.

The builder working disk is a disposable QCOW2 overlay backed by the read-only
sealed base. These large state files remain intentionally untracked; their
identities and reproduction inputs are permanent evidence.
