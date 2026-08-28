# FlowLFS builder VM contract

Status: Phase 2 implementation contract

## Purpose

The builder VM is disposable machinery for executing the pinned LFS book. It
is neither the FlowLFS result nor part of the trusted target runtime. It fixes
the two known host-gate failures (`texi2any` absent and `/bin/sh` not Bash)
without changing the production workstation.

## Pinned builder base

- Distribution: Ubuntu Server 24.04.4 LTS, amd64.
- Medium: `ubuntu-24.04.4-live-server-amd64.iso`.
- Authority: Canonical's signed `SHA256SUMS` manifest, retained in
  `builder/manifests/`.
- Firmware: legacy BIOS for the first baseline, matching the simplest LFS GRUB
  path. UEFI is a separately declared later variant.
- Emulator: QEMU. Use KVM only when `/dev/kvm` is actually usable; otherwise
  use TCG software emulation.

The builder distribution is a provider choice, not an LFS source substitution.
The LFS target is constructed only from the verified execution source contract.

## Virtual hardware

| Resource | Contract |
|---|---|
| Builder OS disk | 32 GiB sparse QCOW2, disposable |
| LFS target disk | 40 GiB sparse QCOW2, independently sealable |
| Memory | 8 GiB |
| CPUs | 4 virtual CPUs |
| Network | user-mode NAT during builder installation/provisioning only |
| Source attachment | repository attached read-only through QEMU 9p |
| Console | serial from installation onward |

Only `builder-os.qcow2` is attached during OS installation, eliminating the
possibility that the Ubuntu installer selects the LFS target. Afterward,
`builder-os.qcow2` is the first disk and `flowlfs-target.qcow2` the second.
Before Chapter 2, their guest device identities must be recorded and checked.

## Guest prerequisite remedy

The builder installs only the tools required by the LFS host contract and
image workflow. The package set is declared by `scripts/provision-builder.sh`.
Inside this disposable VM only, `/bin/sh` is linked to Bash. The exact LFS host
check is then run and its output retained as evidence.

The automated installer creates the temporary local account `flowbuilder` with
the documented bootstrap password `ubuntu`. SSH is forwarded only to host
loopback port 2222. This credential is installation scaffolding, not a secret
or a production credential, and must not survive into any target artifact.

## Isolation and lifecycle

1. Verify the signed installation-media manifest and ISO digest.
2. Create fresh sparse builder and target disks.
3. Install Ubuntu Server on the builder disk only.
4. Attach the project read-only and provision the builder packages.
5. Pass the execution book's host check inside the guest.
6. Snapshot or seal the prepared builder before Chapter 2.
7. Disable networking for the LFS construction run after inputs are copied.
8. Install only to the dedicated target disk.
9. Seal the target image before boot validation; boot-test an overlay/copy.

The host repository, host users, host partitions, and host bootloader remain
out of scope. A target image must never be mounted as the workstation's live
root or installed directly onto a physical host disk.

## Phase gate

Phase 2 is complete when the signed ISO verifies, both disk images exist, the
builder boots, and the unmodified LFS version-check logic passes inside it.
Creation of LFS partitions and package compilation belongs to the following
construction phase.

## Commands

```sh
scripts/fetch-builder-media.sh
scripts/create-builder-disks.sh
scripts/launch-builder-installer.sh
scripts/launch-builder.sh
```

The disk creator refuses to overwrite existing state. Resetting a builder or
target disk therefore requires an explicit, separately reviewed removal rather
than an accidental rerun.
