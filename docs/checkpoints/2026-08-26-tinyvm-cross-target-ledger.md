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
