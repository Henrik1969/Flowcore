# Zsh 5.9.2 source-forge evidence

Date: 2026-08-30  
Profile: `flowlfs.source-forge.v0`  
Target: writable Flowcore twin only

## Result

Zsh 5.9.2 was acquired from the canonical Zsh HTTPS origin, verified against
the upstream SHA-256 list, and verified with the upstream detached signature
and captured release keyring. It was built and tested as the dedicated
unprivileged `flowbuilder` user, installed into a staging root, admitted into
the immutable guest store, projected, rolled back, reprojected without a
network or rebuild, and verified after a cold boot.

First reproducible root-only object:

```text
/flow/store/objects/sha256-d646930bd47443cf02114513c355c5935bde892b25e73e08fa4a5e131f2f912e
```

The ordinary-user shell experiment later discovered that this object's
`root/` directory was sealed as mode 0550. Zsh was therefore executable by
root but not traversable by an ordinary user. The object was not mutated.
Zsh was rebuilt with a normalized mode-0755 staging root and admitted as:

```text
/flow/store/objects/sha256-35919dcaea12c0c67ea22ef0598feb20148c6be78d331bd84aa117a8bdcd9b73
```

This user-accessible object is the current active Zsh projection. Its evidence
is under `user-access/`.

VM twin SHA-256 at the first Zsh cold-boot checkpoint:

```text
e222e27133a13cc8208c4d78550f0fee59bfd8c3ef9b301d7b1346a298325e02
```

Canonical source SHA-256:

```text
36fa734374b44783582cec09bcd67822e2f992c779ec1624ab5596df078d2f81
```

Projected executable SHA-256:

```text
4958d87a00b98f5ad05d612325553ea19ce1ae3993f20643b91eca88250945e9
```

## Verification summary

- upstream checksum: pass;
- detached RSA signature: pass, fingerprint
  `7CA7ECAAF06216B90F894146ACF8146CAE8CBBC4`;
- unsafe archive path scan: pass;
- configuration detected PCRE2, GDBM, libcap, ncursesw, and dynamic modules;
- Zsh test suite: 64 scripts successful, zero failures, one privileged suite
  skipped by the unprivileged test boundary;
- staged installation: pass, no direct package install to `/`;
- collision preflight: pass after one correctly rejected candidate;
- active projection: 1,610 package paths plus separately managed
  `/etc/shells` state;
- Zsh runtime and `zsh/pcre` module: pass;
- root login shell remains `/bin/bash`: pass;
- `sshd.service` remains active: pass;
- rollback restored absence of `/usr/bin/zsh` and the prior absent
  `/etc/shells`: pass;
- offline reprojection: pass;
- cold-boot persistence: pass;
- two controlled clean builds produced the same final object identity: pass;
- certified baseline purity: pass;
- mutated twin `qemu-img check`: pass.

## Rejected and superseded objects

The store deliberately retains unsuccessful or superseded immutable outputs:

1. `f5f84b04...65e98` contained package-generated `/usr/share/info/dir`.
   Projection preflight rejected it because the base system already owns that
   shared registry.
2. `85563b59...cf99b` excluded the Info registry and was fully functional, but
   its Groff HTML embedded the wall-clock build date.
3. `a95e8ecb...887b4` was the second functional build. Comparison isolated the
   nondeterminism to the `CreationDate` comment in 21 `intro*.html` files.
4. `d646930b...f912e` controls `SOURCE_DATE_EPOCH=1783881773`; two clean builds
   converged on this identity. It was the first reproducible projection, but
   was superseded by the user-accessible `35919dca...cd9b73` object described
   above.

No prior object was rewritten to manufacture success.

## Lessons admitted into the design

- A capability requirement such as privilege dropping must not hard-code a
  provider: this guest has `su` but not `runuser`.
- Test results need coverage facts. Non-interactive SSH prevented some
  job-control coverage, terminal-dependent cases were skipped, and the
  privileged suite was intentionally unavailable.
- Build warnings are evidence, not an automatic binary pass/fail decision.
  Observed warnings include Zsh's `mktemp` link warning and Texinfo undefined
  flags during documentation generation.
- Generated registries such as `/usr/share/info/dir` and `/etc/shells` are
  shared resolved state, not ordinary package-owned files.
- Wall-clock time is a build input unless controlled explicitly.
- Collision detection belongs before mutation; the first rejected projection
  changed no live package path.
- Object admission and live projection are independent authority decisions.

## Evidence files

- `guest/configure.log` — detected build capabilities and configuration;
- `guest/build.log` — compiler and linker output;
- `guest/test.log` — complete test result;
- `guest/install.log` — staged installation output;
- `object-manifest.tsv` — final sealed object manifest;
- `object-derivation.txt` — final source, builder, toolchain, and output facts;
- `repro-admit-{1,2}.txt` — matching independent object admissions;
- `zsh-binary.sha256` — final executable content identity.
