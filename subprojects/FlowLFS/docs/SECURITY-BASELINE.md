# Security baseline

Status: construction gate

The exact LFS 13.0-systemd source set is retained as the reproducible control
baseline. It is not approved as a deployable system source set on 2026-08-28.

The book explicitly requires builders to consult its errata and security
advisories before construction. The saved 13.0 advisories identify
post-release fixes, including critical issues affecting glibc, Linux, Perl,
Python, XML-Parser, and xz. Other packages have high or medium advisories.

## Rule

- Never rewrite `manifests/wget-list` or `manifests/md5sums`; they identify the
  exact published control baseline.
- Preserve the saved errata and advisory pages with retrieval hashes.
- Before construction, create a separate hardened execution-source manifest
  using only official LFS directions and canonical upstream releases.
- Record every replacement as `baseline input -> advisory -> execution input`,
  including upstream verification evidence.
- Do not claim that the hardened image is byte-identical to the March baseline.
- Do not boot or distribute an image built from the uncorrected baseline.

This separation preserves both experimental control and responsible system
construction.
