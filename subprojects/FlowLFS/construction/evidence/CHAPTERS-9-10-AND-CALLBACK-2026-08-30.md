# Chapters 9–10 and callback evidence — 2026-08-30

## Construction result

- LFS r13.0-201-systemd Chapters 9 and 10 configured the declared locale,
  timezone, console, hostname, DHCP network, Linux 7.1.8 kernel, and GRUB
  legacy-BIOS boot path.
- The kernel uses the stable root partition identifier
  `PARTUUID=d24fdc1a-01`; the filesystem label remains `FLOWLFS_ROOT`.
- The post-book callback uses OpenSSH 10.5p1 retrieved under the official BLFS
  development recipe. Its complete `make -j1 tests` run passed before install.
- The accepted ED25519 key fingerprint is
  `SHA256:DfRqGUeMsLqXBkofwbhwfdmydh3ZQDuM+g3MF2bKEfU`.

## Authentication fault and correction

The first flattened image booted correctly and started OpenSSH, but rejected
the correct key. A persistent OpenSSH DEBUG3 capture proved the decisive
runtime messages:

```text
platform_locked_account: password matches locked prefix '!'
User root not allowed because account is locked
userauth_pubkey: disabled because of invalid user
```

The target had no `/etc/shadow`; OpenSSH therefore observed the locked marker
in `/etc/passwd` before authorized-key evaluation. Callback construction now
runs `pwconv` when necessary, assigns an unrecoverable random hash, dates the
record with `chage`, and asserts that the effective shadow hash has no lock
prefix. The corrected target was synced, unmounted, and powered off before
sealing.

## Exact artifact verification

The promoted artifact and verified checkpoint are byte-identical at SHA-256:

```text
14c2baf7801ae33051de6b836b6d71e1edaf5262543e781b92bf4e42b994ef68
```

`qemu-img check` reported no errors and `qemu-img info` reported no backing
file. The documented standalone launcher then booted the promoted artifact and
the serial log proved the writable root mount, callback, and multi-user target:

```text
EXT4-fs (vda1): re-mounted ... r/w.
FLOWLFS_CALLBACK READY phone=ssh://127.0.0.1:2222 host=flowlfs-control
Reached target Multi-User System.
```

A batch-mode public-key SSH command against that same boot returned exit 0:

```text
FLOWLFS_SSH_PASS hostname=flowlfs-control kernel=7.1.8 machine=x86_64
root P 2026-08-30 -1 -1 -1 -1
256 SHA256:DfRqGUeMsLqXBkofwbhwfdmydh3ZQDuM+g3MF2bKEfU ... (ED25519)
```

The guest was powered off after verification. Earlier failed images and logs
remain local and ignored as reversible diagnostic evidence; they are not the
promoted deliverable.
