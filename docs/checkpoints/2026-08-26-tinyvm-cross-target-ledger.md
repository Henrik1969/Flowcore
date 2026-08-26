# TinyVM, cross-target and bootstrap maturation ledger

## Mission baseline — 2026-08-26

- Branch: `v25-symboltable-projection`
- Prior mission: v0.28 typed artifact contracts complete and pushed.
- New authority: `docs/tasks/tinyvm-cross-target-bootstrap.md`.
- Run state returned from `DONE` to `CONTINUE` for the new mission.
- Historical TinyVM source was absent; reconstruction provenance is documented
  under `subprojects/TinyVM`.

## Completed precursor checkpoints

### Recovered prototype — `d3bd592`

- Restored a buildable recovered-ISA C17 runner using GNU computed goto.
- Kept it isolated from canonical Graph IR and the installed Flowcore graph.
- Tag: `tinyvm-recovered-poc-2026-08-26`.

### Dispatch baseline — `9be3635`

- Added computed-goto, switch and function-pointer engines and comparative
  workloads.
- GCC 13 Release+LTO local observation favored computed goto across the three
  captured workloads; results remain non-universal exploratory evidence.
- Tag: `tinyvm-dispatch-baseline-2026-08-26`.

### Recovered ISA conformance — `614f045`

- Added 24 semantic/fault cases across all three dispatch engines.
- Covered all 27 recovered opcode identities; `CTX_*` consistently returns an
  explicit unsupported fault rather than invented historical semantics.
- GCC 13, Clang 18 and GCC ASan/UBSan suites passed. Leak detection was disabled
  because the managed traced host cannot run LeakSanitizer.
- Tag: `tinyvm-isa-conformance`.

### Artifact envelope slice — `73ba557`

- Added deterministic `flowcore.tinyvm_artifact` envelope format 1 carrying
  recovered ISA version 0.
- Added SHA-256 integrity and strict header, identity, size, resource, opcode,
  register, jump, reserved-field and terminal-HALT validation.
- Added separate `flowtinyvalidate` and `flowtinyrun` processes plus captured
  file, corruption, truncation and trailing-byte tests.
- Normal and GCC ASan/UBSan suites passed.
- This is an intermediate envelope, not the complete Flow-capable artifact.
- Tag: `tinyvm-artifact-envelope-v1`.

## Current work

Gate 1 is active: extend the executable artifact authority for typed constants,
strings, storage, imports and instruction/source provenance without weakening
the already captured recovered-ISA boundary.

### Sectioned artifact authority — pending checkpoint

- Specified format 2 without reinterpreting format 1 reserved bytes.
- Added ordered code, typed-constant, string, storage, authorized-import and
  per-instruction provenance sections.
- Added canonical alignment/padding, exact coverage, unique/sorted identities,
  carrier/storage/import/provenance validation and deterministic byte round-trip.
- `flowtinyvalidate` now independently identifies and validates formats 1 and 2.
- `flowtinyrun` revalidates format 2 and refuses declared imports until an
  authorized runtime resolver exists.
- Hostile tests recompute a valid SHA-256 after mutating identity padding,
  section uniqueness, boolean constants, import reserved bytes, provenance and
  alignment padding; every mutation is rejected structurally.
- GCC 13 and Clang 18 Debug suites passed 7/7 tests.
- GCC 13 ASan/UBSan passed 7/7 with leak detection disabled for the managed
  traced-host limitation.
- Format 2 currently admits recovered ISA 0. ISA 1 is explicitly rejected until
  its typed execution semantics and cross-section references are implemented.
