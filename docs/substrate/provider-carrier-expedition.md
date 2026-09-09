# Provider carrier expedition ledger

This ledger records the bounded provider experiments defined by the autonomous
task [`docs/tasks/provider-carrier-expedition-autonomous.md`](../tasks/provider-carrier-expedition-autonomous.md).
It publishes capability evidence rather than host fingerprints. Status values
are `OBSERVED`, `IMPLEMENTED`, `TESTED`, `DEFERRED`, `NOT SUPPORTED — MISSING`,
`NOT SUPPORTED — INTENTIONALLY EXCLUDED`, and `UNKNOWN`.

## PC-00 — scouting baseline

**Status:** OBSERVED / IMPLEMENTED (pre-existing)

**Finding:** The redacted substrate scout and machine-readable inventory show
that the project already has versioned Flowcontracts, Flowmini, Flowanalyst,
Flowbind, Flowparallel, Flowlower, and a Clang-backed `flowbind-gen` prototype.
The highest-leverage warehouse families remain buffer-plus-length values,
opaque create/use/release resources, and verified aggregate layouts.

**Current support observed in source/contracts:**

- `stdin.bytes()` and `file.bytes(path)` materialize the existing safe
  `list<int>` carrier. Native file work is provider-side open/read/materialize/
  close; Flowmini does not receive a pointer.
- Flowanalyst records `flowcore.filesystem`, `file.bytes`, `list<int>`, and
  `filesystem.read` effect/provenance facts for the path operation.
- Flowbind validates scalar/string/pointer/opaque-handle ABI declarations and
  linked/dynamic resolution policy. `flowbind-gen` emits deterministic partial
  C-binding artifacts for representative libm, zlib, SQLite, and curl headers.
- Explicit resource metadata and cleanup validation exist for provider-bound
  operations. Unknown aliasing/concurrency remains conservative in
  Flowparallel.
- Flowbind can verify a provider-owned aggregate ABI manifest. Aggregate call
  lowering remains explicitly blocked until a separate implementation gate.

**Architecture decision:** Reuse existing contracts and `list<int>` first.
No new carrier, ownership system, universal FFI, or parser path is justified by
the baseline alone.

**Next gate:** PC-01 scalar control, then a bounded zlib buffer experiment if
the existing evidence does not already answer it.

## PC-01 — scalar control

**Status:** OBSERVED; control already satisfied by existing Flowbind generator
and real-library gates. A new scalar implementation is not warranted before
the carrier experiments.

## Open experiment entries

The following entries are intentionally reserved for evidence gathered by the
next gates:

```text
PC-02 safe buffer input/output and byte materialization
PC-03 opaque resource identity and create/use/release
PC-04 resource conflict/effect evidence
PC-05 aggregate ABI and target-layout evidence
PC-06 Flowbind coverage
PC-07 std semantic implications
PC-08 internal brick/provider map
PC-09 carrier priority and next action
```

## PC-02 — safe buffer input/output and byte materialization

**Status:** NOT SUPPORTED — MISSING (general provider execution)

**Experiment:** The Clang-backed generator was run against zlib's public header
in a temporary artifact. `compress`, `compress2`, and `uncompress` are
classified `unsupported carrier` because their ABI is native buffer pointers
plus lengths (`Bytef *`, `uLongf *`, `uLong`). The generated artifact is
deterministic and the library is discoverable, but no Flowbind/Flowmini
execution contract currently turns a safe `list<int>` into a provider buffer
and materializes the result back.

**Decision:** Keep the filesystem `list<int>` carrier unchanged and reject a
zlib call at the binding boundary until a general safe buffer-plus-length
contract is designed and tested. Do not expose pointers or add a
`ByteBuffer`-style type from this evidence alone.

**Gate evidence:** The temporary zlib artifact is `partial`; the three bounded
compression operations are explicit unsupported entries; repeated
serialization is byte-for-byte deterministic.

## PC-03 — opaque resource identity and create/use/release

**Status:** IMPLEMENTED / TESTED (contract metadata); DEFERRED (generic runtime
execution)

**Experiment:** The generator produced a deterministic SQLite artifact with an
explicit `sqlite3_open -> sqlite3_handle -> sqlite3_close` cleanup contract.
`sqlite3_close` and `sqlite3_errmsg` are admitted with `sqlite3_handle` input
and `c_int`/`c_string` results; broader operations remain explicitly
unsupported. Existing Flowbind boundary tests preserve cleanup metadata and
reject unauthorized or malformed declarations.

**Decision:** The current contract describes provider-owned opaque identity and
required cleanup, but the generic Flowmini runtime does not yet execute a
SQLite lifecycle through that contract. Keep lifecycle metadata authoritative
and defer a runtime slice until a safe result carrier and provider-call path
are selected. Unknown handle concurrency remains serial in Flowparallel.

## PC-04 — resource/effect evidence and aggregate layout

**Status:** TESTED / DEFERRED

**Experiment:** Existing resource/effect gates preserve provider identity,
access, ownership, lifetime, and unknown concurrency; same-resource conflicts
remain serial. The test provider emits a `flowcore.abi_manifest` for a small
`Point` aggregate. Flowanalyst records its declaration and Flowbind verifies
size, alignment, field order, types, offsets, and provider identity. Mismatched
size, offsets, ordering, field types, provider, and duplicate fields are
rejected. A verified manifest still returns an explicit blocked result for
aggregate call lowering.

**Decision:** Aggregate layout evidence preserves ABI facts but does not claim
aggregate execution support. Target layout remains provider-owned and is not
canonical Flowmini semantics.

## PC-05 — cross-family assessment

| Interface family | Reused support | New mechanism required | Current result | Recommendation |
| --- | --- | --- | --- | --- |
| scalar | Flowbind/Clang generator and exact grants | none | control satisfied | use existing path |
| buffer + length | filesystem `list<int>` materialization | safe bidirectional buffer contract | zlib explicitly rejected at binding boundary | next implementation target |
| opaque resource | handle carriers and cleanup metadata | generic provider execution/lifecycle call path | SQLite contract tested, execution deferred | investigate after buffer contract |
| aggregate struct | provider-owned ABI manifest and validation | aggregate call lowering/carrier | layout verified, call blocked | defer until concrete need |

The evidence favors one general safe buffer contract first: it recurs across
compression, archives, networking, codecs, and database blobs. Opaque resource
metadata is the next reusable contract, while callbacks, variadics, templates,
and arbitrary aggregates remain outside this expedition.
