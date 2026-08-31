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

The certified artifact and sealed checkpoint are read-only, filesystem-
immutable controls. All post-baseline work belongs in
`artifacts/FlowLFS-v0.1-flowcore-twin.qcow2`. Run
`scripts/verify-baseline-purity.sh` before mutation and use
`scripts/launch-flowcore-twin.sh` when intentionally booting the writable twin.

The writable twin now carries the first experimental source-forge projection:
Zsh 5.9.2 built from verified upstream source as an unprivileged user, admitted
as a reproducible digest-addressed object, and projected reversibly without
changing the recovery shell. See `docs/SOURCE-FORGE-V0.md` and
`flowpkg/evidence/zsh-5.9.2/REPORT.md`.

It also carries the composable `flowlfs.shell-environment.v0` profile. Its
owner-authored package provides dependency-free global Bash/Zsh defaults and
materialized new-user templates without enabling anonymous login or copying
personal shell state. See `docs/SHELL-ENVIRONMENT-V0.md`.

The optional acquisition and inspection stack is realized under
`docs/MODERN-CLI-V0.md`. It is explicitly owner-selected and is not part of
the recovery base. The independently flattened, boot-tested deliverable is
`artifacts/FlowLFS-v0.1-modern-cli-v0.qcow2`; launch it with
`scripts/launch-modern-cli-v0.sh` (default callback port 2226).

The additive system basis is realized under `docs/BASIS-V1.md`. Its sealed
runnable image is `artifacts/FlowLFS-v0.1-basis-v1.qcow2`; launch it with
`scripts/launch-basis-v1.sh` (default callback port 2228). It preserves
canonical `/etc/skel` and account behavior while adding Zsh, system-local PATH,
per-user history policy, editable monikers, and the native Flowcore `sel` tool.

The terminal-insulated revision is documented in
`docs/BASIS-V2-TERMINAL.md`. Its sealed image is
`artifacts/FlowLFS-v0.1-basis-v2-terminal.qcow2`; launch it with
`scripts/launch-basis-v2-terminal.sh` (default callback port 2230). It replaces
`sel`'s direct ncurses dependency with a provider-neutral terminal ABI and an
independent terminal-restoration guardian.

Interactive navigation is completed by the immutable basis v2.1 correction,
`artifacts/FlowLFS-v0.1-basis-v2.1-terminal-navigation.qcow2`, launched with
`scripts/launch-basis-v2.1-navigation.sh` on default callback port 2232. The
original sealed v2 image remains unchanged as failure and lineage evidence.

The first interchangeable selection capability is described in
`docs/SELECTION-PROVIDERS-V0.md`. Its runnable image adds source-built Go,
canonical source-built fzf, and explicit native/fzf provider policy without
changing the native default.

The immutable v1 correction is described in
`docs/SELECTION-PROVIDERS-V1.md`. It gives the native provider a complete,
portable candidate-list projection while retaining the same selection
contract, explicit provider policy, and unchanged fzf object.
The durable architectural and package-store findings from this tranche are in
`docs/SELECTION-CAPABILITY-LESSONS-V1.md`.

The first native realization engine is documented in
`docs/BULK-PROJECTOR-V0.md`, with its exact transaction boundary in
`docs/BULK-PROJECTOR-V0-CONTRACT.md`. It retains the existing Go, fzf, and
Flowselection objects while replacing their shell-per-entry projection loops.

## Layout

```text
book/                 canonical LFS book and book-source archive
blfs/                 captured stable and execution BLFS systemd books
flowpkg/              experimental profiles, recipes, sources, and evidence
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

1. Read [FlowLFS, Part 1](FlowLFS_part_1.md), the revised build handbook.
2. Read the [captured BLFS authority](blfs/README.md).
3. Read [authority and method](docs/AUTHORITY-AND-METHOD.md).
4. Read [development environment](docs/DEVELOPMENT-ENVIRONMENT.md).
5. Read the [security baseline](docs/SECURITY-BASELINE.md).
6. Read the [execution mutation ledger](execution/MUTATION-LEDGER.md).
7. Run `scripts/check-host-requirements.sh`.
8. Review the retrieved books and manifests.
9. Do not begin Chapter 2 disk operations until an isolated image-backed build
   boundary has been reviewed.

## Non-claims

FlowLFS has a bootable immutable LFS control and a separate experimental twin
with callback and source-forge adaptations. It is not yet a Flowcore runtime,
FrankenPOP composition, general-purpose distribution, or conforming Flowcore
system. The source-forge schemas are experimental and no public capability API
is frozen here.
