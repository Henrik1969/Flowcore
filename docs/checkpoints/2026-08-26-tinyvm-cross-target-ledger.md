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

### Sectioned artifact authority — `tinyvm-artifact-sections-v2`

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

### Flow-capable ISA v1 — `tinyvm-flow-isa-v1`

- Defined a separate ISA version rather than changing recovered ISA 0.
- Added typed virtual slots for `i1`, `i32`, `i64` and opaque handles.
- Added constant definition, move, conversion, modular arithmetic, signed
  division, six comparisons, absolute jump, conditional branch, return, trap,
  halt, string/storage handle and authorized-import call opcodes.
- Opaque handles encode stable tagged identities and never host addresses.
- `ADD`, `SUB` and `MUL` use modulo-width semantics to match the current LLVM
  lowerer's plain operations. Division faults are explicit rather than relying
  on C undefined behavior. Checked arithmetic remains a future distinct
  semantic operation.
- Added step-limit, uninitialized-slot, type, division and unresolved-import
  traps.
- Added portable switch and GNU computed-goto engines. Conformance compares
  complete slot state, carriers, bits, PC, step count, result and traps.
- Added independent file emission, validation and execution proving typed
  `i64(42)` with the producer absent.
- Format 2 now validates ISA 1 constant/string/storage/import references,
  virtual-slot ranges, import argument spans, control targets and terminal
  behavior.
- GCC 13 and Clang 18 passed 9/9 tests. GCC ASan/UBSan passed 9/9 with the
  documented LeakSanitizer exclusion.
- Runtime provider resolution for `CALL_IMPORT` remains Gate 5; both engines
  explicitly trap and the command-line runner refuses unresolved imports.

### Backend-neutral lowering artifact — `backend-neutral-lowering-v1`

- Added independently invocable `flowprepare`, which consumes captured
  optimization and optional binding files and emits canonical
  `flowcore.backend_lowering_artifact` version 1 JSON.
- The artifact preserves source, complete target catalog and selected target,
  ABI contracts, external operations, operation/block/symbol identities, exact
  authorization capabilities and optimization-transform provenance.
- Public validation rejects target drift, duplicate authorization identities
  and any difference between authorized tuples and external-call tuples.
- LLVM `flowlower` and TinyVM `flowtinylower` consume the same captured file;
  replay tests invoke the producer neither in-process nor as a binary.
- TinyVM deterministically lowers the first provider-free empty program to ISA
  v1 returning typed `i32(0)`. It reports structured `unsupported` for valid
  non-empty plans pending Gate 4 rather than falsely admitting them.
- Direct optimization-report input to LLVM remains a documented temporary
  corpus compatibility path, not the public backend boundary.
- The complete GCC superbuild and all 68 canonical tests passed. Focused Clang
  18 ASan/UBSan builds and both backend-boundary tests passed with leak
  detection disabled for the managed traced-host limitation.

### Provider-free scalar parity — `tinyvm-provider-free-scalar-parity`

- TinyVM lowering now admits typed integer and Boolean literals, identifiers,
  i32/i64 conversions, unary negation, modular arithmetic, signed division,
  comparisons, local definitions, assignment, structured branches, loops and
  typed returns.
- Symbol storage and expression temporaries lower to distinct virtual slots;
  control references resolve to artifact-local instruction indexes.
- Every emitted instruction retains source, lowering-plan, operation, block and
  symbol derivation. Compiler-generated control/return operations use explicit
  reserved provenance identities rather than pretending to be source nodes.
- Differential tests run the same public backend-neutral files through LLVM
  and TinyVM for empty, literal-return, expression, local-symbol, comparison
  branch and integer-loop programs and compare observable return values.
- Unsupported operation and expression kinds still produce structured
  `unsupported` results without creating a partial executable artifact.
- The complete GCC superbuild and all 69 canonical tests passed. Focused Clang
  18 ASan/UBSan builds and the boundary/parity tests passed with the documented
  LeakSanitizer exclusion.

### Provider-free execution inputs and opaque values — `tinyvm-provider-free-parity`

- Added typed ISA 2 as a versioned execution-input extension; ISA 1 bytecode
  meaning remains frozen.
- ISA 2 slot 0 receives process-style argument count and subsequent reserved
  slots receive tagged opaque argument identities, never host pointers or
  serialized argument bytes.
- Lowering selects ISA 2 only for argument intrinsics, calculates the required
  static index span and emits a checked count guard returning `i32(64)` before
  access. Dynamic indexing remains explicitly unsupported.
- No-argument and one-argument executions of the current LLVM argument-count
  fixture are differentially equal through TinyVM.
- Switch and computed-goto engines compare complete ISA 2 execution state,
  including input metadata and opaque argument slots.
- String constants and writable-storage compatibility declarations now lower
  into validated artifact sections and tagged opaque handles. Host resolution
  remains governed provider work in Gate 5.
- The complete GCC superbuild and all 69 canonical tests passed. Focused Clang
  18 ASan/UBSan conformance, boundary and parity tests passed with the
  documented LeakSanitizer exclusion.

### Governed pure calls — `tinyvm-governed-pure-calls`

- TinyVM lowering admits only two exact initial import tuples:
  `libc.so.6:abs(c_int)->c_int` and
  `libc.so.6:strlen(c_string)->c_size_t`, both under contract `libc`, C calling
  convention and `pure` effect.
- The executable import section preserves contract, library, symbol,
  convention, effect, parameter/result carriers and stable authorization
  evidence identity.
- `flowtinyrun` refuses import-bearing artifacts without an explicit active
  policy and rechecks the complete policy tuple before library loading.
- Runtime dispatch uses named typed thunks, not a generic FFI. Host symbol
  addresses exist only after policy admission and are never serialized.
- Artifact string and process-argument handles resolve inside the typed string
  thunk. Temporary NUL termination is provider-local and bounded.
- LLVM/TinyVM differential tests prove `abs(-42) == 42` and
  `strlen("Flowcore") == 8`; missing and effect-mismatched policies fail closed.
- Switch and computed-goto engines differentially execute the same resolved
  import callback and complete slot/result state.
- The complete GCC superbuild and all 70 canonical tests passed. Focused Clang
  18 ASan/UBSan conformance and both parity suites passed with the documented
  LeakSanitizer exclusion.

### Governed readonly identity calls — `tinyvm-governed-readonly-calls`

- Added exact typed zero-argument thunks for `getpid`, `getuid`, `getgid`,
  `geteuid`, `getegid`, `getppid` and `getpgrp` under their declared
  `kernel`/`linux`, `libc.so.6`, C, `readonly`, `()->c_int` authorities.
- Empty parameter lists serialize canonically as `none`; the ISA validator and
  runtime agree that they consume zero argument slots.
- Existing four-field Flowbind policy grants remain accepted only for
  zero-argument imports. Typed result and contract admission are still checked
  by the artifact and named thunk, so this compatibility does not become a
  generic symbol grant.
- Differential executions compare normalized process exit results. Parent-ID
  execution is invoked without a command-substitution intermediary so LLVM and
  TinyVM observe the same parent process.
- The complete GCC superbuild and all 70 canonical tests passed. Focused Clang
  18 ASan/UBSan provider, scalar and ISA conformance suites passed with the
  documented LeakSanitizer exclusion.

### Governed output call — `tinyvm-governed-output-call`

- Added the exact typed `libc.so.6:puts(c_string)->c_int` thunk under contract
  `libc`, C calling convention and `io` effect.
- The same artifact-string handle used by `strlen` resolves through the
  provider-local bounded NUL-terminated view; no pointer enters the artifact.
- Parity now runs a plan containing `strlen`, `abs` and `puts` together and
  compares program stdout byte-for-byte separately from the structured TinyVM
  execution record.
- Missing and mismatched active policy checks remain applied to the I/O fixture
  as well as pure and readonly fixtures.
- The complete GCC superbuild and all 70 canonical tests passed. Focused Clang
  18 ASan/UBSan governed-provider and ISA conformance tests passed with the
  documented LeakSanitizer exclusion.

### Current governed-provider inventory — `tinyvm-governed-provider-inventory`

- Added exact differential parity for `labs`, `getpgid`, `getsid`,
  `getpriority` and checked indexed argument access through `puts`.
- Runtime policy is preflighted for every import before the first instruction,
  so an unreachable call cannot bypass authorization.
- Published `tinyvm-current-llvm-parity-inventory.md`, separating every
  executed tuple from readonly-output, filesystem, IPC, loopback, namespace,
  memory, ncurses and aggregate tuples that still lack safe runtime laws.
- A validated authorized `fork` plan proves structured unsupported exit 2 and
  proves that no partial executable is created.
- Legacy ISA 0 payloads inside the v2 artifact envelope remain executable;
  governed import preflight applies to ISA 1/2 artifacts with imports and does
  not silently change the frozen recovered format's behavior.
- The complete GCC superbuild and all 70 canonical tests passed. Focused Clang
  18 ASan/UBSan governed-provider, scalar, artifact-boundary and ISA
  conformance tests passed with the documented LeakSanitizer exclusion.
