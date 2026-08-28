# FlowLFS

Status: control baseline preserved; corrected execution inputs being prepared;
construction not started

FlowLFS is the first attempt to construct a small, real Flowcore system from
auditable source. Its baseline is Linux From Scratch 13.0-systemd, followed to
the letter before any Flowcore adaptation is admitted.

## Boundary

```text
canonical upstream release sources
  -> exact LFS baseline build
  -> captured build and runtime evidence
  -> separately declared Flowcore observations
  -> reversible Flowcore adaptations
```

The LFS baseline is evidence and a control system. A Flowcore proposal must not
silently alter an LFS command, package, patch, test, or configuration choice.
Any necessary deviation is recorded before execution with its cause, effect,
provenance, and rollback path.

## Current deliverable

This checkpoint prepares the project and development environment only. It does
not partition a host disk, create privileged users, mount filesystems, enter a
chroot, compile packages, or claim a bootable system.

The intended final executable artifacts are a VM-runnable ISO and/or disk
image. Documentation, source manifests, checksums, build logs, and evidence are
supporting artifacts.

## Layout

```text
book/                 canonical LFS book and book-source archive
manifests/            canonical download list and checksums
sources/              upstream release tarballs and LFS patches (untracked)
execution/            separately pinned, post-advisory execution snapshot
docs/                 authority, environment, and Flowcore analysis records
scripts/              bounded retrieval and validation helpers
work/                 untracked construction state
artifacts/            untracked ISO/disk images and evidence bundles
```

## Baseline

- Book: Linux From Scratch 13.0-systemd, published 2026-03-05.
- Architecture: x86_64, pure 64-bit baseline.
- Init/service baseline: systemd, because it matches the current LFS stable
  systemd edition and provides a concrete substrate to observe rather than a
  prematurely invented replacement.
- Source rule: use the exact release tarballs and patches named by the book.
- Integrity rule: verify the book's `md5sums` before construction; retain
  stronger upstream signatures or digests where the upstream publishes them.

## Start here

1. Read [authority and method](docs/AUTHORITY-AND-METHOD.md).
2. Read [development environment](docs/DEVELOPMENT-ENVIRONMENT.md).
3. Read the [security baseline](docs/SECURITY-BASELINE.md).
4. Read the [execution mutation ledger](execution/MUTATION-LEDGER.md).
5. Run `scripts/check-host-requirements.sh`.
6. Review the retrieved books and manifests.
7. Do not begin Chapter 2 disk operations until an isolated image-backed build
   boundary has been reviewed.

## Non-claims

FlowLFS is not yet a Flowcore runtime, FrankenPOP composition, distribution,
bootable image, or conforming system. No public capability API is defined here.
