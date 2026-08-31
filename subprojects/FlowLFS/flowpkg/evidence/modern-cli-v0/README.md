# Modern CLI v0 evidence

`modern-cli-v0-evidence.tar.xz` is the permanent evidence capsule copied from
the running FlowLFS twin after the first complete profile realization.

SHA-256:
`13757e54fb61f3d40ac30c1b008cc1ea3ba202d6cd3113e8b1d71aa71ac9f665`

The capsule contains the construction and upstream-test logs for all ten
resolved packages, each admitted object's manifest and derivation record, the
active realization lock, and the preceding inactive lock created by the full
rollback exercise. It deliberately excludes the object payloads; those remain
in the VM's immutable `/flow/store/objects` and will be carried by the VM image.

Lifecycle acceptance performed on 2026-08-31:

1. exact-object preflight passed for all ten packages;
2. profile activation and composed verification passed;
3. reverse-order deactivation removed every profile projection;
4. curl, Git, trust tools, make-ca, and certificate policy were absent again;
5. the independently admitted zsh/default-shell layer and SSH recovery path
   survived;
6. all ten packages were reprojected from the store with network verification
   disabled during activation;
7. the separate online HTTPS and Git capability verification passed.

Notable upstream results include p11-kit 67/67, libtasn1 31/31, libidn2 12/12,
libpsl 8/8, curl 1599/1599 applicable tests, and Git 31695 successful with zero
failures (391 upstream TODO/broken expectations). Git has the explicit
`NO_RUST=1` constraint until a Rust toolchain is independently admitted.

## Runnable realization

The profile was sealed as the standalone qcow2 image
`artifacts/FlowLFS-v0.1-modern-cli-v0.qcow2` after a clean shutdown.

- Virtual size: 40 GiB
- Stored size at sealing: 5.07 GiB
- SHA-256: `495e21c2e8246fb2fb2507425478087a97e4735adf707b606ce386b790b8f336`
- Backing file: none
- Host mode after sealing: read-only; the launcher uses a disposable snapshot
- `qemu-img check`: no errors

The separate image was booted on callback port 2226. In that image the profile
was fully deactivated, absence of its commands and trust policy was verified,
and the ten-object realization was reconstructed from the carried immutable
store with offline activation. The subsequent online HTTPS verification and
ordinary-user Git/curl runtime checks passed. It was then shut down cleanly and
hashed as recorded above.
