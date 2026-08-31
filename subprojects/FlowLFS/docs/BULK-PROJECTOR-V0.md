# Bulk projector v0

This is the first FlowLFS factory-floor realization engine. It replaces the
common shell-per-entry symlink projector with one native transaction process
while retaining immutable package objects, exact profile locks, collision
refusal, reversible projections, and explicit evidence.

The engine's normative boundary is in `BULK-PROJECTOR-V0-CONTRACT.md`.
Flowpkgprojector 0.1.0 was built from its captured source inside FlowLFS,
passed isolated adversarial tests and a 251-link differential comparison with
the shell reference, and was admitted as:

`sha256-96ac8e5d6a5c7199222cc1863cf43c90ce5cbdf52f4cbf896b627b65cf86a8c3`

Go 1.26.5, fzf 0.74.1, and Flowselection 0.1.1 retain their exact previous
object identities. Only their reversible realization mechanism changed.

## Exact Go measurement

The migration used the existing 15,027-link, 1,667-directory Go object on the
same KVM guest:

| Operation | Nanoseconds | Approximate time |
| --- | ---: | ---: |
| Shell rollback | 12,295,397,964 | 12.295 s |
| Native project | 279,074,296 | 0.279 s |
| Native rollback | 94,087,680 | 0.094 s |
| Native reprojection | 225,148,109 | 0.225 s |

Native rollback was about 131 times faster than the shell rollback. A complete
four-package profile deactivation took 107,731,829 ns and activation took
740,085,831 ns.

## Recovery and limits

The exact Go object was deliberately interrupted after creating 5,000 links.
The production recovery command consumed the prepared plan, removed only links
still pointing into that object, reversed its directories, and restored an
empty target. Changed-link rollback refusal was also proven before mutation.

The v0 engine intentionally handles only ordinary symlink-tree realization.
Copy/merge projections, accounts, `ldconfig`, and service hooks remain explicit
package adapters until each receives an equally precise transaction model.

Runnable image: `artifacts/FlowLFS-v0.1-bulk-projector-v0.qcow2`

SHA-256: `eda001b13fa9ae06fb5d1041c74278f3c1bc809ba9f9e4675622c02f6bcc5d8b`

Launch with `scripts/launch-bulk-projector-v0.sh`; SSH defaults to port 2238.
