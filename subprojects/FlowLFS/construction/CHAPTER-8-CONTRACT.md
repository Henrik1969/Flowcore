# Chapter 8 contract: final system packages

Authority is LFS r13.0-201-systemd, Chapter 8. The generated runner compiles
the pinned book's installation command blocks into an ordered, resumable
target-chroot build without fetching or substituting source code.

## Entry conditions

- The immutable `ch7-ready` target checkpoint exists.
- All 94 execution inputs verify against the captured official checksum list.
- The builder mounts `/dev/vdb1` at `/mnt/lfs` and supplies only kernel virtual
  filesystems and the read-only project control plane.

## Boundary

`compile-chapter08-runner.py` deterministically generates the 80-package
`build-chapter08-chroot.sh`. Each package receives an individual durable marker
below `/var/log/flowlfs/ch08`; retries skip completed packages.

`finalize-chapter08-chroot.sh` implements the book's Sections 8.84 and 8.85.
It retains the book's selected debug companions, strips eligible inactive ELF
objects, and removes test debris, libtool archives, the temporary triplet
tools, and the `tester` account.

Libraries and executables loaded by the finalizer itself are exempt from
in-process replacement. The reason and recovery evidence are recorded in the
mutation ledger and Chapter 8 evidence report.

## Exit evidence

- All 80 package markers and the finalization marker exist.
- A target-native GCC compile-and-run smoke test succeeds.
- GCC, glibc, Bash, Python, systemd, Binutils, Findutils, and E2fsprogs report
  their expected target-native versions.
- Critical executables have no unresolved dynamic dependencies.
- Virtual kernel filesystems are unmounted and the builder powers off cleanly.
- `qemu-img check` passes before sealing `ch8-ready`.
