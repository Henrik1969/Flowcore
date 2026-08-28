# Chapter 7 contract: chroot and temporary target-native tools

Authority is LFS r13.0-201-systemd, Chapter 7. The tracked scripts translate
the book's interactive shell transitions into explicit, resumable execution
without changing package configuration choices.

## Entry conditions

- Chapter 6 has completed and its target image is checkpointed.
- `/dev/vdb1` is mounted at `/mnt/lfs` by the sealed builder VM.
- All canonical source archives are already present in `/mnt/lfs/sources`.
- No package is fetched or substituted during construction.

## Boundary

`prepare-chapter07.sh` performs the book's ownership transition and virtual
kernel filesystem mounts, copies the target-native runner into the target,
and enters the target using the book's clean chroot environment.

`build-chapter07-chroot.sh` creates the required filesystem and identity
scaffolding, then builds Gettext, Bison, Perl, Zlib, Mpdecimal, Python,
Texinfo, and Util-linux in book order. Each completed unit gets a durable
marker below `/var/log/flowlfs/ch07`.

## Exit evidence

- Every expected marker exists.
- The temporary target-native executables pass explicit presence checks.
- `/tools` is removed only after all checks pass.
- The builder unmounts the virtual kernel filesystems before shutdown.
- The powered-off target is sealed as the `ch7-ready` checkpoint.
