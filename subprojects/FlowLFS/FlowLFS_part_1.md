# FlowLFS, Part 1: Building the control baseline

Status: revised after the successful FlowLFS v0.1 construction

Execution baseline: LFS `r13.0-201-systemd`

Control baseline: LFS 13.0-systemd

Verified artifact: `artifacts/FlowLFS-v0.1-x86_64.qcow2`

Certified Git tag: `flowlfs-v0.1-baseline`

## 1. Purpose

This is the practical handbook we wish we had before the first FlowLFS build.
It describes how to turn a pinned Linux From Scratch book and canonical source
set into a reproducible, inspectable, VM-runnable control system without
silently mutating the build host or confusing a chroot with a bootable machine.

It does not replace the canonical LFS book. The pinned book remains the
authority for package order, commands, tests, and configuration. This handbook
adds the engineering discipline needed around the book:

- isolation;
- source and decision provenance;
- resumable execution;
- reversible qcow2 checkpoints;
- explicit local policy;
- runtime diagnostics;
- artifact sealing and certification; and
- a clean boundary between LFS and later Flowcore adaptations.

Part 1 ends with a verified LFS control image and one declared post-book access
boundary: public-key OpenSSH for VM control. It does not claim that the system
is already Flowcore.

## 2. What we actually built

The certified baseline has these properties:

| Property | Value |
| --- | --- |
| Architecture | x86_64, pure 64-bit |
| Firmware | Legacy BIOS |
| Disk | 40 GiB qcow2, MBR, one ext4 partition |
| Filesystem label | `FLOWLFS_ROOT` |
| Root selector | `PARTUUID=d24fdc1a-01` |
| Kernel | Linux 7.1.8 |
| Init system | systemd 261.2 |
| Hostname | `flowlfs-control` |
| Locale | `en_US.UTF-8` |
| Timezone | `Europe/Copenhagen` |
| Network | Virtio NIC, systemd-networkd DHCP |
| Console | Serial console plus Danish console policy |
| Remote boundary | OpenSSH 10.5p1, public-key only |
| Artifact checksum | `14c2baf7801ae33051de6b836b6d71e1edaf5262543e781b92bf4e42b994ef68` |

The final qcow2 has no backing file. It boots independently of the builder and
has been verified through the exact launcher shipped with the project.

## 3. The rules that made the build sane

### 3.1 Preserve two authorities

We retain two distinct LFS records:

1. LFS 13.0-systemd is the immutable published control.
2. Official LFS development snapshot `r13.0-201-systemd` is the coherent
   execution book used for this image.

We did not place newer packages under older instructions. The execution book,
download list, checksum list, and source set were captured together. The
reason for advancing the execution baseline—post-release security advisories
and their coherent upstream treatment—is recorded in
`execution/MUTATION-LEDGER.md`.

### 3.2 Discovery is not authorization

A file being available locally does not make it a build input. Only archives
named by the captured execution checksum manifest are admitted. Every admitted
archive is checked before copying and again on the target filesystem.

The same rule applies after LFS. OpenSSH became authorized only after its BLFS
recipe, archive, retrieval time, authority note, and checksums were captured
under `execution/callback/`.

### 3.3 Never build the target into the host

The production host edits the repository and stores artifacts. It does not
receive the LFS user, target mounts, target packages, target bootloader, or
target system configuration.

All privileged construction happens inside a dedicated builder VM. The target
is a second virtual disk attached to that VM. Boot certification happens in a
separate VM without the builder disk.

```text
production host
  |
  +-- repository and canonical source cache (read-only to builder)
  |
  +-- builder VM
  |     +-- vda: builder operating system
  |     +-- vdb: FlowLFS target disk
  |
  +-- standalone verification VM
        +-- vda: sealed FlowLFS artifact only
```

### 3.4 A checkpoint is a boundary, not a backup afterthought

Every major transition is sealed only after validation, unmount, sync, and
clean poweroff. A fresh writable overlay is then based on the sealed state.

The successful chain is:

```text
ch4-ready
  -> ch5-ready
  -> ch6-ready
  -> ch7-ready
  -> ch8-ready
  -> boot-ready
  -> callback candidates
  -> baseline-ssh-fixed
  -> fresh mutation overlay
```

This made destructive experiments recoverable. It also allowed individual
libraries damaged during stripping to be compared with canonical earlier
state rather than guessed back into existence.

### 3.5 “It compiled” is not a completion condition

Each phase has an explicit exit gate. Markers are written only after install
and local validation succeed. A phase is sealed only when its broad gate is
proven. The final system is complete only after the exact promoted artifact
boots and accepts the declared control path.

## 4. Repository map

The important paths are:

```text
book/                    pinned control book
manifests/               control retrieval records
execution/               coherent execution snapshot and mutation ledger
execution/callback/      captured BLFS/OpenSSH authority and source
sources/                 large canonical cache, deliberately untracked
builder/                 builder contract, state, media, and evidence
construction/            phase contracts and durable evidence
scripts/                 bounded construction and verification mechanics
artifacts/               runnable images and serial logs, deliberately untracked
```

Large binary state is not committed merely to make Git look complete.
Identity is retained through manifests, hashes, evidence reports, and immutable
local checkpoints. The small canonical OpenSSH source used by the declared
callback is committed with its retrieval record.

## 5. Phase 0: pin authority before touching a disk

Read these first:

1. `docs/AUTHORITY-AND-METHOD.md`
2. `docs/DEVELOPMENT-ENVIRONMENT.md`
3. `docs/SECURITY-BASELINE.md`
4. `execution/MUTATION-LEDGER.md`
5. `builder/BUILDER-CONTRACT.md`

Then establish the inputs:

```bash
scripts/fetch-lfs-inputs.sh
scripts/fetch-lfs-sources.sh
scripts/fetch-execution-snapshot.sh
scripts/fetch-execution-sources.sh
scripts/compare-execution-inputs.sh
```

Run the host requirements check, but do not “repair” the production host into
an LFS build machine:

```bash
scripts/check-host-requirements.sh
```

Our host failed as an admissible builder because Texinfo was absent and
`/bin/sh` selected Dash. That was useful evidence, not permission to alter the
host. The isolated builder was provisioned instead.

Gate before continuing:

- the book revision is pinned;
- retrieval time and authority are recorded;
- all 94 execution inputs verify;
- control/execution differences are explained;
- no cache extra has silently entered the source contract; and
- the Git tree contains a sane baseline tag.

## 6. Phase 1: create and seal the builder

The builder is disposable infrastructure with persistent virtual disks. Fetch
and verify its installation medium, create its disks, install it, provision
required tools, then seal its base according to the builder contract.

Relevant mechanics:

```text
fetch-builder-media.sh
verify-builder-media.sh
create-builder-disks.sh
launch-builder-installer.sh
provision-builder.sh
seal-builder.sh
launch-builder.sh
```

The normal runtime topology is:

- builder OS as `/dev/vda`;
- FlowLFS target as `/dev/vdb`;
- repository attached read-only through QEMU 9p;
- SSH forwarded to the builder for controlled operation; and
- enough RAM to support the declared parallel build policy.

Do not attach the target during builder OS installation. Disk identity must be
unambiguous before any destructive command is possible.

Gate before target preparation:

- builder media checksum passes;
- builder disk and target disk identities are distinct;
- the builder passes the LFS version check;
- the repository mount is read-only;
- the sealed builder base is immutable beneath a disposable overlay; and
- both qcow2 images pass `qemu-img check` after clean shutdown.

## 7. Phase 2: Chapters 2–4, target and clean build identity

This is the first destructive boundary. The preparation helper must prove all
of the following about `/dev/vdb` before partitioning:

- type is `disk`;
- exact size is 42,949,672,960 bytes;
- there are no child partitions;
- there is no filesystem signature;
- it is not mounted; and
- the caller supplied the literal destructive confirmation for `/dev/vdb`.

The selected local policy is one MBR primary partition beginning at 1 MiB,
formatted ext4 and labelled `FLOWLFS_ROOT`. No swap partition is used.

The target is mounted at `/mnt/lfs`. Exactly 94 authorized sources are copied
to `$LFS/sources`, reverified there, and assigned the ownership required by the
book.

Chapter 4 then creates the limited filesystem layout, `lfs` user/group, clean
profiles, target triplet, config site, and bounded parallel-build policy.

Observed clean environment:

```text
LFS=/mnt/lfs
LFS_TGT=x86_64-lfs-linux-gnu
LC_ALL=POSIX
PATH=/mnt/lfs/tools/bin:/usr/bin
CONFIG_SITE=/mnt/lfs/usr/share/config.site
MAKEFLAGS=-j4
umask=0022
```

Use the tracked preparation mechanics and seal with
`seal-target-checkpoint.sh` only after the Chapters 2–4 evidence gate passes.

Lesson: a destructive helper should identify its target from several
independent facts. A device name alone is not a safety boundary.

## 8. Phase 3: Chapter 5, cross-toolchain

Build in book order as the unprivileged `lfs` user:

1. Binutils pass 1
2. GCC pass 1
3. Linux API headers
4. Glibc
5. target Libstdc++

`scripts/build-chapter05.sh` starts every incomplete package from a freshly
extracted verified archive. Durable markers under
`$LFS/var/log/flowlfs/ch05` allow completed predecessors to remain untouched
after a later failure.

The toolchain sanity checks are more important than version banners. They must
prove the target interpreter, start files, headers, linker paths, libc, and
loader all resolve through the intended sysroot. The requested interpreter
must be `/lib64/ld-linux-x86-64.so.2`, never a path containing `/mnt/lfs`.

Seal `ch5-ready` only after all five package markers and all toolchain checks
pass.

Lesson: resumption is safe only when completion markers represent validated
postconditions, not merely the start or exit of a shell block.

## 9. Phase 4: Chapter 6, temporary tools

Chapter 6 cross-builds the temporary tools in the exact book order using
`scripts/build-chapter06.sh`. Every archive is local and verified; no network
or substitution is admitted.

The exit gate requires:

- all 17 package markers;
- the expected temporary executable set;
- second-pass linker and compiler;
- `/bin/sh -> bash`;
- `/usr/bin/cc -> gcc`; and
- removal of book-forbidden archives.

Do not treat execution of a target dynamic binary from the builder namespace
as a valid native test. Its loader searches the builder filesystem. Native
target execution begins only after entering the Chapter 7 chroot.

Lesson: build-time executability depends on namespace and loader context, not
only on architecture and file format.

## 10. Phase 5: Chapter 7, chroot transition

`prepare-chapter07.sh` performs the ownership transition, mounts the virtual
kernel filesystems, copies the target-native runner, and enters the clean
chroot. `build-chapter07-chroot.sh` creates identity scaffolding and builds the
temporary native packages in book order.

Our first entry check incorrectly assumed `findmnt` existed inside the chroot.
Util-linux had not yet been built. The check was corrected to use
`/proc/self/mounts`, which is available at that stage.

This yields a general rule: validation code must depend only on capabilities
already established by the current phase. A diagnostic with a future-stage
dependency can block a correct build.

The Chapter 7 gate requires:

- filesystem and identity markers;
- all eight package markers;
- target-native version checks;
- validated removal of `/tools`;
- explicit unmount of virtual kernel filesystems; and
- clean shutdown before sealing `ch7-ready`.

## 11. Phase 6: Chapter 8, final userspace

Chapter 8 is large enough that hand-maintained shell translation becomes a
provenance risk. `compile-chapter08-runner.py` deterministically derives the
ordered 80-package runner from the pinned book. The generated runner must be
byte-identical to its tracked copy before execution.

Each package has its own marker under `/var/log/flowlfs/ch08`. Test failures
remain visible even where the book allows construction to continue. We
observed:

- one GCC `auto-init-padding-9.c` failure;
- one initial Coreutils `chroot-credentials` failure caused by an additional
  supplementary group; and
- three Util-linux `waitpid/pidfd-ino` failures.

The handbook rule is not “all upstream tests must always be green.” It is:

1. run the book-required tests;
2. preserve exact failures and skips;
3. distinguish book-allowed nonfatal observations from construction defects;
4. correct runner defects without hiding prior evidence; and
5. do not seal while target integrity is uncertain.

### 11.1 The stripping failure

The most dangerous failure occurred during final stripping. Replacing a
library used by the currently running `strip` or `install` process can truncate
that library while it is still demand-paged. In our build this damaged
`libzstd` and then `libsframe`.

The corrected finalizer identifies its own live executable and library
dependency set and exempts those objects from in-process replacement. Other
eligible inactive ELF files are still stripped. Any unexpected stripping error
is fatal.

Recovery was accepted only because provenance remained demonstrable:

- `libzstd` was rebuilt from the pinned canonical archive after a temporary
  host-compatible bootstrap restored the ability to run the target tool;
- the temporary bootstrap was then replaced completely;
- `libsframe` came from immutable `ch7-ready` state built from the same pinned
  Binutils input; and
- final dynamic linking and file identities were validated.

Lesson: “online optimization” is a mutation of the running substrate. Either
operate offline or calculate and protect the live dependency closure.

Seal `ch8-ready` only after all 80 package markers, finalization marker,
target-native compile/run smoke test, version checks, and dynamic-dependency
checks pass.

## 12. Phase 7: Chapters 9–10, configuration and boot

Interactive book placeholders must become declared policy, not accidental
copies of the builder host. Our policy is recorded in
`construction/CHAPTERS-9-10-CONTRACT.md`.

`prepare-chapters09-10.sh` establishes the kernel virtual filesystems and
enters the target. `build-chapters09-10-chroot.sh` installs configuration,
builds Linux 7.1.8, installs GRUB for legacy BIOS, and writes the boot menu.

### 12.1 Build required drivers into the kernel

The baseline has no initramfs. Therefore the kernel must contain—not merely
provide as modules—the capabilities needed before root mount:

- Virtio PCI;
- Virtio block;
- ext4;
- devtmpfs; and
- 8250 serial console.

Network support must also cover the QEMU Virtio NIC for the callback boundary.

### 12.2 Root identity must be resolvable by the bare kernel

Our first boot used `root=LABEL=FLOWLFS_ROOT`. The bare kernel did not resolve
that label without an initramfs and failed before mounting root.

The corrected command line uses the partition's stable MBR PARTUUID:

```text
root=PARTUUID=d24fdc1a-01
```

GRUB may locate its own files through the filesystem label, but the kernel root
argument must use an identifier the chosen boot architecture can resolve at
that moment.

Lesson: a stable identifier is useful only if the consumer at that boot stage
can resolve it.

## 13. Phase 8: declared post-book callback

The first LFS control image is useful only if we can inspect and control it
after standalone boot. The callback is therefore a separate, reversible
post-book mutation, not an invisible addition to LFS.

Inputs:

- official BLFS development recipe for OpenSSH 10.5p1;
- canonical OpenSSH 10.5p1 release archive;
- retrieval metadata and SHA-256 manifest; and
- an explicitly selected operator public key.

`build-callback-chroot.sh` runs the complete OpenSSH test suite, installs the
daemon, creates a strict system-wide authorized-key location, disables password
and keyboard-interactive authentication, and enables two units:

- `sshd.service` provides the control transport;
- `flowlfs-callback.service` announces readiness and the VM-local callback
  address on the serial console.

### 13.1 The authentication trap

The first callback image reached multi-user state and started OpenSSH, but the
correct key was rejected. Client diagnostics proved that the expected key was
offered. Repeated changes to `AuthorizedKeysFile` did not solve it because key
lookup was not the failing stage.

Persistent server-side DEBUG3 evidence finally proved:

```text
platform_locked_account: password matches locked prefix '!'
User root not allowed because account is locked
userauth_pubkey: disabled because of invalid user
```

The target had no `/etc/shadow`. OpenSSH therefore saw the locked marker in
`/etc/passwd` and rejected root before evaluating the authorized key.

The corrected procedure is:

1. create the canonical shadow database with `pwconv` if absent;
2. assign an unrecoverable random password hash;
3. date the password record with `chage`;
4. discard the random password immediately;
5. keep all SSH password mechanisms disabled; and
6. fail construction if the effective root hash is absent or begins with a
   lock prefix.

The result is not password access. It is an account OpenSSH recognizes as
valid while the only admitted remote authentication method remains the
declared public key.

Lesson: client evidence can prove transport, negotiation, and key offer, but
only server evidence can prove why authorization rejected the account.

## 14. Phase 9: seal and certify the artifact

Never flatten a target that is still mounted or attached to a running builder.
The certification sequence is:

1. validate the target from inside the builder;
2. `sync` the target;
3. unmount target and virtual kernel filesystems;
4. power the builder off cleanly;
5. wait for QEMU to exit;
6. run `qemu-img check` on the complete chain;
7. flatten the clean target into a separately named standalone candidate;
8. confirm `qemu-img info` reports no backing file;
9. hash the candidate;
10. boot that exact candidate without the builder disk;
11. prove root mounted read/write and multi-user state was reached;
12. prove the serial callback appeared;
13. authenticate in batch mode with the declared key;
14. query hostname, kernel, architecture, account state, and key fingerprint;
15. power the guest off;
16. promote the byte-identical candidate to the artifact path; and
17. record the hash, evidence, commit, and tag.

The final runtime proof was:

```text
FLOWLFS_CALLBACK READY phone=ssh://127.0.0.1:2222 host=flowlfs-control
FLOWLFS_SSH_PASS hostname=flowlfs-control kernel=7.1.8 machine=x86_64
root P 2026-08-30 -1 -1 -1 -1
```

Run the certified artifact with:

```bash
cd /home/henrik/Projekter/Udvikling/Flowcore
subprojects/FlowLFS/scripts/launch-standalone.sh
```

Then connect through the callback:

```bash
ssh -p 2222 root@127.0.0.1
```

## 15. Diagnostic method: stop guessing at the correct boundary

The callback failure consumed far more time than compilation because evidence
was collected at the wrong layer. Use this order for future boot/runtime
faults:

1. **Artifact structure** — `qemu-img info`, backing chain, and integrity.
2. **Firmware/bootloader** — confirm GRUB actually loads the intended kernel.
3. **Kernel** — capture the complete command line and root-mount result.
4. **Init/services** — prove the target and service reached active state.
5. **Transport** — prove port forwarding reaches the expected server version
   and host key.
6. **Client authentication** — prove the intended identity was offered.
7. **Server authorization** — capture server-side decisions persistently.
8. **On-disk state** — inspect the exact booted artifact, including journal
   replay where necessary.

Important operational lessons:

- Do not repeatedly flatten multi-gigabyte images after speculative edits.
- Make one disposable overlay, add persistent diagnostics, collect one failing
  transaction, and inspect that evidence.
- If a VM is killed abruptly, ext4 metadata may remain in the journal. A raw
  `debugfs` view can show pre-replay state. Replay/check a disposable copy
  before treating absence or ownership as fact.
- Debug logs may be unlinked but still recoverable by inode after a crash.
- A successful offline chroot test does not prove the independently booted
  image contains the same committed filesystem state.
- A service being active does not prove its application-level contract works.

## 16. What is now Flowcore-shaped

FlowLFS Part 1 does not yet replace LFS mechanics with Flowcore providers, but
it exposes the structure that a later Flowcore system must represent:

| Observed construction fact | Flowcore-shaped interpretation |
| --- | --- |
| Captured book and manifests | authority and immutable input identity |
| Verified release archives | authorized source capabilities |
| Builder VM | governed execution provider |
| Target qcow2 | durable state projection |
| Package marker | validated transformation postcondition |
| Chroot transition | namespace and capability boundary |
| Kernel configuration | boot capability closure |
| Checkpoint chain | reversible transformation history |
| Serial log | diagnostic evidence stream |
| Callback service | declared access capability |
| Standalone artifact | emitted runnable projection |
| Hash plus Git tag | identity joined to evidence and policy |

The next phase should describe this graph without pretending that shell order
alone is semantic authority. Build, install, boot, runtime, and representation
must remain distinct.

## 17. Definition of done for a future FlowLFS rebuild

A rebuild is not complete until all of the following are true:

- control and execution authorities are pinned and reconciled;
- every admitted source verifies;
- construction occurs only inside the builder boundary;
- every phase marker represents a validated postcondition;
- all phase evidence and allowed test exceptions are retained;
- each major boundary has a clean, immutable checkpoint;
- target virtual filesystems are unmounted before sealing;
- qcow2 integrity passes;
- the final image has no backing file;
- the exact promoted image boots without the builder;
- root mounts read/write;
- the intended multi-user target is reached;
- the serial callback appears;
- public-key SSH succeeds noninteractively;
- runtime identity matches declared policy;
- artifact checksum matches its durable manifest;
- scripts pass syntax and static checks;
- the Git worktree is clean; and
- a signed or annotated baseline tag points at the complete evidence record.

Anything less is progress, not certification.

## 18. Permanent lessons from Part 1

1. The canonical book is necessary, but a safe build also needs contracts,
   provenance, isolation, and exit gates.
2. Coherent input upgrades are safer than mixing new packages with old
   instructions.
3. Checkpoints turn experiments and recovery into auditable transformations.
4. Resumable runners must mark proven outcomes, never mere execution.
5. Diagnostics must depend only on capabilities already present at that stage.
6. Chroot success and standalone boot success are different claims.
7. A bare kernel needs its root identifier and root drivers available before
   userspace exists.
8. Mutating live libraries is unsafe unless the live dependency closure is
   protected.
9. Correct key material does not imply a valid SSH account.
10. Client logs explain what was offered; server logs explain why it was
    refused.
11. Abrupt VM shutdown can make offline filesystem inspection misleading until
    journal replay.
12. Flatten only cleanly shut-down state, and boot-test the exact promoted
    bytes.
13. Preserve failures. They are part of the provenance of the corrected
    method.
14. Flowcore begins here as explicit meaning and evidence around mechanics—not
    as a premature rewrite of mechanics that already work.

## 19. Evidence index

- `construction/evidence/CHAPTERS-2-4-2026-08-28.md`
- `construction/evidence/CHAPTER-5-2026-08-28.md`
- `construction/evidence/CHAPTER-6-2026-08-28.md`
- `construction/evidence/CHAPTER-7-2026-08-28.md`
- `construction/evidence/CHAPTER-8-2026-08-29.md`
- `construction/evidence/CHAPTERS-9-10-AND-CALLBACK-2026-08-30.md`
- `construction/evidence/baseline-v01.sha256`
- `execution/MUTATION-LEDGER.md`
- `builder/evidence/BUILDER-GATE-2026-08-28.md`

The certified implementation is commit `585fb39`, tagged
`flowlfs-v0.1-baseline`. This handbook records what that build taught us; the
underlying evidence remains the authority for claims about what actually ran.
