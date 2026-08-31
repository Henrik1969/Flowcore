# Flowpkgprojector

Flowpkgprojector is the native realization engine for immutable package objects
whose payload is projected as a symlink tree. It replaces the repeated
shell-per-entry mechanism without changing object identity or package policy.

```text
flowpkg-projector project  OBJECT TARGET_ROOT PROJECTION
flowpkg-projector rollback OBJECT TARGET_ROOT PROJECTION ARCHIVE
flowpkg-projector recover  OBJECT TARGET_ROOT PROJECTION
```

`OBJECT/root` is the immutable payload. `TARGET_ROOT` is `/` in FlowLFS and may
be an isolated directory in tests. `PROJECTION` is the transaction/evidence
directory. Project performs complete collision preflight before mutation,
writes a prepared plan, creates the tree, and atomically promotes the prepared
transaction. Rollback validates every owned link before removing anything.
Recovery reverses an interrupted `.incoming` transaction and refuses paths
whose state is no longer owned by that transaction.

The v0 engine intentionally covers only ordinary directory-plus-symlink-tree
projections. Packages requiring copies, merges, account changes, `ldconfig`, or
service-manager hooks retain explicit adapters around this engine.

