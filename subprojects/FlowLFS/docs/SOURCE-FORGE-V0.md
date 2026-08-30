# Source Forge profile v0

Status: first executable experiment

## Purpose

`flowlfs.source-forge.v0` is the smallest FlowLFS profile that can turn a
reviewed canonical source archive into an immutable package object and expose
that object through a reversible filesystem projection.

Profiles are composable requirement sets. They are not mutually exclusive
system editions.

## Required capabilities

- bootable system and local recovery console;
- remote maintenance without changing the root login shell;
- canonical source acquisition outside the trust-deficient guest bootstrap;
- source integrity and signature evidence;
- unprivileged compilation under a dedicated identity;
- staged installation that cannot write directly into the live root;
- digest-addressed immutable object storage;
- collision-checked filesystem projection;
- ownership/provenance query;
- complete projection rollback;
- durable build, test, and projection evidence.

The first specimen is Zsh 5.9.2. Bash remains `/bin/sh`, root's login shell,
and the recovery shell.

## v0 record boundaries

| Record | Canonical question |
| --- | --- |
| source envelope | What bytes were acquired, from where, and with what verification? |
| recipe | What transformation and dependencies were selected? |
| adaptation | How and why does FlowLFS differ from upstream or BLFS? |
| derivation | Which exact resolved inputs produced an output? |
| store object | Which immutable files and metadata resulted? |
| projection transaction | Which live names were exposed, changed, or restored? |

JSON and shell are bootstrap representations of these records, not their
permanent semantic identity.

## Source mutation law

Canonical upstream source is an immutable lineage root, not a prohibition on
change. FlowLFS may patch, replace, extend, or deeply restructure any admitted
source—including shells, Binutils, Coreutils, systemd, libraries, and the
kernel—when the derived source receives its own explicit identity.

```text
canonical upstream source
  + Flowcore adaptation patch set
  + Flowcore-native additive source
  -> named Flowcore source revision
  -> derivation
  -> immutable object
```

Every mutation must retain:

- the exact canonical ancestor and content digest;
- ordered patches, replacements, and additive files;
- the law, requirement, or owner policy authorizing each change;
- intended semantic and compatibility effects;
- affected capability claims and consumers;
- build, regression, compatibility, and migration evidence;
- a distinct version/lineage identity and rollback route.

A Flowcore-derived source tree must never be presented as unmodified upstream.
Conversely, upstream compatibility is not sacred or presumed: it is a
versioned capability that may be preserved, constrained, or deliberately
rejected with visible diagnostics. Additive native capabilities are encouraged
when they remain discoverable and do not acquire hidden authority merely by
being compiled into a foundational tool.

## Store and projection rules

- `/flow/store/objects/sha256-<digest>` contains sealed output trees.
- An object is never updated in place.
- Builds run as `flowbuilder`; only object admission and projection run as
  root.
- Package installation targets a staging root and never `/`.
- Existing live paths cause projection failure unless an explicit compatible
  shared-state rule exists.
- Projected package files are symlinks into the store in this experiment.
- Shared `/etc/shells` state is resolved separately and backed up before
  mutation.
- Generated `/usr/share/info/dir` is shared registry state and is excluded
  from package ownership; the v0 projection leaves the existing registry
  unchanged.
- The recipe fixes `SOURCE_DATE_EPOCH` to the canonical source release time so
  generated documentation cannot acquire the build wall clock as hidden input.
- Rollback removes only links created by the recorded transaction and restores
  the exact previous `/etc/shells` state.
- A failed build, test, admission, or projection remains evidence and is never
  promoted silently.

## Trust boundary

The bootstrap guest has no downloader or populated CA trust bundle. Source is
therefore acquired on the host over HTTPS, checked against the upstream
SHA-256 list, verified with the detached upstream signature and captured
release keyring, and transferred as inert input. Guest-native trusted HTTPS is
deferred to the certificate-store tranche.

Discovery, integrity, signature verification, recipe selection, build
permission, store admission, and live projection remain separate decisions.

## Zsh acceptance contract

The experiment succeeds only if:

1. the source digest is `36fa734374b44783582cec09bcd67822e2f992c779ec1624ab5596df078d2f81`;
2. the archive is path-safe and extracts into one expected source directory;
3. configuration and compilation run as `flowbuilder`;
4. `make check` passes;
5. installation occurs only under a staging root;
6. the admitted object is digest-addressed and read-only;
7. every projected package file resolves into that object;
8. Zsh runs explicitly and reports version 5.9.2 with PCRE available;
9. `/etc/shells` contains exactly one `/bin/zsh` contribution;
10. root continues to use `/bin/bash` and `sshd.service` remains active;
11. rollback restores the pre-projection state;
12. reprojection succeeds without rebuilding or network access;
13. the certified baseline purity gate still passes;
14. the resulting twin remains a VM-runnable QCOW2 artifact.

## Explicit deferrals

Dependency solving, remote binary repositories, garbage collection, package
upgrades, PAM, default-shell selection, user dotfiles, Zsh plugin frameworks,
and Flowcore-language self-hosting are outside v0.
