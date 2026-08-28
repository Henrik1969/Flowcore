# Execution mutation ledger

Status: Phase 1 execution-input contract

## Decision

The immutable control is LFS 13.0-systemd, published 2026-03-05. It remains in
the top-level `book/` and `manifests/` trees and is never rewritten.

The first build candidate uses the exact official LFS development snapshot
`r13.0-201-systemd`, retrieved 2026-08-28T14:43:00Z. Its book, source list, and
checksum list were captured together. This is a coherent replacement contract,
not a claim that the development book is a stable release.

## Why the execution contract differs

The official 13.0 advisory record makes the published control unsuitable for a
new deployable image. Critical post-release corrections affect glibc, Linux,
Perl, Python, XML-Parser, and xz; additional advisories affect Expat,
inetutils, libcap, OpenSSL, sed, systemd, util-linux, and Vim.

Rather than put new packages under old instructions, the execution baseline
advances the complete book and complete source set as one unit. Consequently,
its changes include both advisory-required corrections and ordinary upstream
development drift. Every difference is therefore classified as follows:

- `security-required`: replacement of an input named by the 13.0 advisory
  record, or its treatment by the newer book.
- `book-structural`: addition, removal, or patch required by the newer book
  (for example mpdecimal is added while XML-Parser and intltool leave the core
  source set).
- `snapshot-coherence`: remaining version movement required to execute the
  newer official book without creating a hand-built hybrid.

This ledger authorizes none of those changes independently. Authority comes
from the captured official execution book and its matching manifests.

## Notable security replacements

| Component | 13.0 control | Execution input |
|---|---|---|
| glibc | 2.43 | 2.44 plus `glibc-2.44-upstream_fixes-1.patch` |
| Linux | 6.18.10 | 7.1.8 |
| Expat | 2.7.4 | 2.8.3 |
| inetutils | 2.7 | 2.8 |
| libcap | 2.77 | 2.78 |
| OpenSSL | 3.6.1 | 4.0.1 |
| Perl | 5.42.0 | 5.44.0 |
| Python | 3.14.3 | 3.14.7 plus `Python-3.14.7-openssl_4-1.patch` |
| sed | 4.9 | 4.10 |
| systemd | 259.1 | 261.2 |
| util-linux | 2.41.3 | 2.42.2 |
| Vim | 9.2.0078 | 9.2.0954 |
| xz | 5.8.2 | 5.8.3 |

XML-Parser 2.47 is not replaced one-for-one: the newer book removes it from
the core source set. That structural removal is the execution treatment.

## Machine-verifiable evidence

- `manifests/book-revision` identifies the exact book revision.
- `manifests/retrieved-at` records the UTC capture time.
- `manifests/retrieval.sha256` seals the book and retrieval metadata.
- `manifests/md5sums` is the matching official LFS input checksum list.
- `scripts/compare-execution-inputs.sh` reproduces the filename-level delta.

The official download list can contain auxiliary files outside the checksum
contract. The retrieval helper reports these explicitly. They are cache extras,
not authorized execution inputs, unless a later ledger entry supplies integrity
evidence and admits them.

The control contains 92 checksum entries. The execution snapshot contains 94:
39 filenames are unchanged, 53 occur only in the control, and 55 occur only in
the execution contract.

## Construction gate

No construction may begin until all 94 execution inputs verify against the
captured checksum list. Construction must occur in an isolated image-backed
boundary and must produce a VM-runnable disk image and/or ISO, never mutate the
host into the target system.
