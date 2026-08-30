# Captured BLFS books

Retrieved from the official Linux From Scratch project on
2026-08-30T16:19:52Z.

## Authority model

FlowLFS retains two BLFS identities for the same reason it retains two LFS
identities:

1. `control/` is the published BLFS 13.0-systemd release, published
   2026-03-05 and explicitly targeting LFS 13.0-systemd.
2. `execution/` is the official systemd development snapshot available at
   retrieval time. Its embedded identity is `r13.0-1379`, published
   2026-08-29. It is the appropriate starting point for analysis against the
   FlowLFS execution substrate, LFS `r13.0-201-systemd`.

The development book is not silently promoted over the stable control. A BLFS
package becomes an authorized FlowLFS mutation only when a later contract names
the exact recipe, sources, checksums, dependencies, policy, and rollback path.
Capturing the book does not authorize installing its entire package universe.

## Official origins

- Stable downloads:
  `https://www.linuxfromscratch.org/blfs/downloads/stable-systemd/`
- Development snapshots:
  `https://www.linuxfromscratch.org/blfs/downloads/systemd/`
- Stable online book:
  `https://www.linuxfromscratch.org/blfs/view/stable-systemd/`
- Development online book:
  `https://www.linuxfromscratch.org/blfs/view/systemd/`
- Official download index:
  `https://www.linuxfromscratch.org/blfs/download.html`

## Contents

`control/` contains both the official compressed single-page book and the
official multi-page HTML archive, plus the stable package download list.

`execution/` contains the official complete multi-page development archive and
a separately captured rendered index proving the online revision visible at
retrieval time.

Each directory includes authority, revision, retrieval-time, and SHA-256
records created locally over the exact retrieved bytes. The upstream XZ CRC64
checks also pass.

## Reading locally

Single-page stable book:

```bash
xzcat control/BLFS-BOOK-13.0-systemd-nochunks.html.xz > /tmp/blfs-13.0.html
```

Multi-page stable or development book:

```bash
mkdir -p /tmp/blfs-book
tar -xf execution/blfs-book-systemd-2026-08-30.tar.xz -C /tmp/blfs-book
```

Do not edit the captured archives. Derived notes, execution selections, and
Flowcore mappings belong outside this directory and must refer back to these
identities.
