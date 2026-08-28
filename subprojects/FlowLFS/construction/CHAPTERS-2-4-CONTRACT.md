# Chapters 2–4 construction contract

Status: executable pre-toolchain gate

## Book authority

Commands and environment follow LFS `r13.0-201-systemd`, sections 2.5–2.7,
3.1, and 4.2–4.4. Partition-table policy is necessarily local because the
book deliberately leaves partitioning to the builder.

## Target policy

- Guest target: `/dev/vdb`, exactly 40 GiB (42,949,672,960 bytes).
- Partition table: MBR (`msdos`), suitable for the declared BIOS baseline.
- Layout: one primary partition from 1 MiB through the end of disk.
- Filesystem: ext4, label `FLOWLFS_ROOT`.
- Mount: `/mnt/lfs`, default options with `suid` and device nodes enabled.
- Swap: none; the builder has 8 GiB RAM and host-backed VM memory.

Before writing, the helper requires all of these facts simultaneously: block
device type `disk`, exact byte size, no child partitions, no filesystem
signature, and no mount. It additionally requires the literal argument
`--confirm-destroy=/dev/vdb`. Any mismatch stops construction.

## Source policy

The repository is attached to the guest read-only as QEMU 9p tag
`flowlfs_project`. Only filenames present in
`execution/manifests/md5sums` are copied into `$LFS/sources`. Cache extras are
not admitted. The copied 94 files are verified again on the target filesystem,
then owned by root as required by the book.

## User and environment

The exact limited directory layout, `lfs` user/group, ownership, clean Bash
profile, Bash rc, `LFS_TGT`, `CONFIG_SITE`, and bounded `MAKEFLAGS=-j$(nproc)`
follow Chapter 4. Ubuntu's `/etc/bash.bashrc` is moved aside in the disposable
builder overlay, as explicitly required by the book, and recorded for later
restoration.

## Gate

Compilation may begin only after evidence proves:

- `/dev/vda` remains the builder and `/dev/vdb1` is mounted at `/mnt/lfs`;
- all 94 inputs verify on `/mnt/lfs/sources`;
- `/mnt/lfs/usr/lib64` does not exist;
- the `lfs` login environment contains only the book-authorized values; and
- the sealed builder base remains unchanged beneath the disposable overlay.
