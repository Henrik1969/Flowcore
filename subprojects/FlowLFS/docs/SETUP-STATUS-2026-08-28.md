# FlowLFS setup status — 2026-08-28

## Facts

- The project exists as the independent `subprojects/FlowLFS` island.
- LFS 13.0-systemd PDF, single-page HTML, and book-source archive are stored in
  `book/`.
- The release download list and edition checksum manifest are stored in
  `manifests/`.
- Every archive and patch required by the edition checksum manifest is present
  in `sources/` and passes `md5sum --check`.
- The source directory contains 96 download-list payloads (about 605 MB), plus
  its checksum file. The SysV-only boot-scripts archive is not required by the
  systemd checksum manifest.
- No LFS package has been unpacked or compiled.
- The production host fails the exact LFS host gate because Texinfo is absent,
  and `sh` is not Bash.
- Official 13.0 errata and advisories are saved and hashed. The March control
  source set has critical post-release advisories and is not deployable as-is.

## Decision

The production host remains unchanged. The build will use a disposable,
image-backed builder satisfying section 2.2. The target filesystem will live
on a virtual disk intended to become a VM-runnable image.

## Next action

Pin the separate advisory-corrected execution source manifest, then define and
review the isolated builder-image contract: firmware, disk layout, host
distribution and pinned installation medium, required host tools, source
attachment, network policy, snapshot boundary, output formats, and launch
isolation. Do not begin LFS Chapter 2 until both gates have passed review.
