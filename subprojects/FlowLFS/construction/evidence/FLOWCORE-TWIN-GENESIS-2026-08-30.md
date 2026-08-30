# Flowcore twin genesis — 2026-08-30

## Boundary

The certified FlowLFS v0.1 control is now permanently separated from the
writable image used for Flowcore mutations.

| Role | Local path | Policy |
| --- | --- | --- |
| Certified artifact | `artifacts/FlowLFS-v0.1-x86_64.qcow2` | read-only control |
| Sealed checkpoint | `builder/state/flowlfs-target-baseline-ssh-fixed.qcow2` | read-only control |
| Flowcore twin | `artifacts/FlowLFS-v0.1-flowcore-twin.qcow2` | writable mutation target |

Both controls and the twin had this SHA-256 at the instant of cloning:

```text
14c2baf7801ae33051de6b836b6d71e1edaf5262543e781b92bf4e42b994ef68
```

The twin was created as a full independent copy with `cp --reflink=never`.
It is not a hard link, qcow2 overlay, or image with a backing file. Its inode is
distinct from the control artifact, and mutations to the twin cannot be
written through to the baseline.

## Purity policy

- The certified artifact and sealed checkpoint have mode `0444` and the ext4
  immutable flag. Removing that flag requires an explicit privileged action.
- The twin has mode `0644` and is the only image admitted for mutation.
- `scripts/verify-baseline-purity.sh` verifies both control checksums,
  read-only modes, standalone qcow2 topology, image integrity, and distinct
  twin identity.
- The original launcher continues to use QEMU snapshot mode with the control.
- `scripts/launch-flowcore-twin.sh` refuses to start unless the purity gate
  passes, then launches the writable twin without snapshot mode.

The twin genesis checksum records ancestry, not an invariant for its future
contents. Once Flowcore mutations begin, the twin is expected to diverge while
the two control hashes must remain unchanged.

## Observed genesis proof

```text
control artifact mode: 0444
sealed checkpoint mode: 0444
control immutable flag: set on both control images
twin mode:              0644
control and twin inode: distinct
control backing file:   none
twin backing file:      none
qemu-img check:         PASS for both images
```
