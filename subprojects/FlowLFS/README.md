# FlowLFS

Status: LFS r13.0-201-systemd baseline built, sealed, and verified as a
standalone VM-runnable qcow2 image

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

`artifacts/FlowLFS-v0.1-x86_64.qcow2` is the verified executable deliverable.
It is a standalone 40 GiB qcow2 disk image for legacy-BIOS x86_64 VMs. Run it
with `scripts/launch-standalone.sh`; the launcher provides the callback at
`ssh://127.0.0.1:2222` and retains serial evidence locally.

The image boots Linux 7.1.8 and systemd 261.2 to multi-user state. Public-key
SSH access and the serial callback marker were verified against the exact
promoted artifact. Documentation, source manifests, checksums, build logs, and
evidence are its supporting artifacts.

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

FlowLFS is a bootable LFS control baseline with one explicitly declared
post-book callback adaptation. It is not yet a Flowcore runtime, FrankenPOP
composition, general-purpose distribution, or conforming Flowcore system. No
public capability API is defined here.
