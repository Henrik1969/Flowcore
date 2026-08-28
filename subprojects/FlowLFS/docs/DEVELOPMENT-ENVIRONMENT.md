# Development environment

Status: prepared for review; no LFS build has started

## Host-independent construction boundary

The LFS root will be an image-backed filesystem, never a production host
partition. All privileged Chapter 2 onward operations must occur inside a
dedicated disposable builder or an equivalently reviewed isolation boundary.
The resulting root is emitted as a VM disk image; boot validation occurs in a
separate disposable VM.

The production host supplies repository editing and source storage only. It is
not the LFS target and must not receive LFS users, mounts, packages, chroots, or
bootloader changes.

## Pinned inputs

```text
LFS edition:       13.0-systemd
book publication:  2026-03-05
target:            x86_64-lfs-linux-gnu
source list:       manifests/wget-list
source checksums:  manifests/md5sums
```

The manifests are retrieved from the same stable-systemd release directory as
the book. The exact upstream release archives named by the book are canonical
construction inputs. Repository snapshots are not substitutes for release
tarballs.

The upstream `wget-list` also names the SysV-only LFS boot-scripts archive.
That archive is not part of the 13.0-systemd checksum manifest and is not a
required input. The edition-specific `md5sums` file defines source-set
completion.

## Local directories

`book/` and `manifests/` are permanent review inputs. `sources/`, `work/`, and
`artifacts/` are deliberately untracked because they can be large or generated.
Their identity is carried by manifests, checksums, logs, and final artifact
records rather than by committing binary payloads to the Flowcore repository.

## Environment gate

Run:

```sh
scripts/check-host-requirements.sh
```

The check is read-only and follows the book's section 2.2 version check.
Passing it does not authorize building on the production host.

Observed on 2026-08-28: the production host is not an admissible builder.
Texinfo is absent and `/bin/sh` resolves to Dash instead of Bash. These facts
will be corrected only in the isolated builder, never by rewriting the
production host for this project.

## Construction phases

1. Verify the book, download manifest, and checksum manifest.
2. Retrieve every named release source and patch.
3. Verify the complete source set with the book checksum manifest.
4. Create a fresh disposable builder with an image-backed target disk.
5. Run the LFS version check inside the builder.
6. Follow Chapters 2 through 11 exactly, capturing commands and results.
7. Shut down the builder and seal the baseline disk image.
8. Boot-test a copy or overlay, never the sealed baseline.
9. Only then create a separately named Flowcore-derived image.

## Not yet performed

- No host packages were installed.
- No `lfs` user or group was created.
- No filesystem was formatted or mounted.
- No source package was compiled.
- No chroot was entered.
- No VM was launched.
