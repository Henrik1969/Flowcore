# flowwc application-pressure ledger

This ledger records evidence from building `Flowwc/src/flowwc.flow`. Entries
are written before any architectural remedy is attempted.

## FW-P001 — path-based file input is not expressed by the current Flowmini carrier

* **Severity / category:** BLOCKER, P0, FILESYSTEM/TEXT.
* **Requirement:** `flowwc file.txt` must obtain file bytes while keeping line
  and word counting in Flowmini.
* **Attempt:** the Flowmini program uses `stdin.bytes()` and performs the full
  counting state machine in source. The existing `std/abi/file_io.flow` was
  then inspected for a path-based alternative.
* **Observed limitation:** the current hosted Flowmini input surface exposes
  `stdin.bytes()` as a `list<int>`, but file providers expose `read(fd,
  c_pointer, count)` only. The language has no admitted operation that turns a
  file path/read buffer into a Flowmini byte collection. The opaque pointer
  carrier cannot be indexed by Flowmini counting logic.
* **Workaround:** shell redirection (`flowwc < file.txt`) exercises the real
  Flowmini counting algorithm. A native helper that pre-counts the file would
  violate the application boundary; using `sendfile` would only copy bytes and
  still leaves counting outside Flowmini.
* **Workaround cost:** the required `flowwc file.txt` interface is unavailable;
  callers must coordinate redirection and lose per-file path/error policy.
* **Remedy options:** add a general provider-backed byte/text read operation;
  add a safe buffer-to-list carrier; or redesign the application around stdin.
* **Decision before remedy:** record first and investigate the smallest reusable
  provider contract. Do not add a flowwc-specific compiler dispatch or native
  counter.
* **Historical decision:** the stdin workaround was retained while the partial
  `file.bytes(path)` experiment was reverted; that experiment exposed the need
  for a coordinated dynamic argument/list contract rather than a second
  authority path. The blocker is resolved below by the shared contract.
* **Verification required:** missing-file and readable-file tests, byte
  preservation, provider/resource provenance, and Flowmini-side counting
  output must remain visible.

## FW-P002 — counting state machine is expressible through Flowmini bytes

* **Severity / category:** MAJOR FRICTION, P1, TEXT/LANGUAGE.
* **Requirement:** count lines, words, and bytes with documented semantics.
* **Evidence:** `flowwc.flow` currently uses `stdin.bytes()` followed by indexed
  `list<int>` traversal, a bounded loop, branches, integer state, and `length`.
  The path-based provider remains unresolved; `file.bytes(path)` was attempted
  and reverted under FW-P001 after exposing the parser/runtime argument gap.
  The word definition is a maximal non-whitespace byte sequence; `bytes` is the
  number of provider bytes consumed.
* **Decision:** no language remedy is justified for the state machine itself.
  The same logic should be reused once path input exists.
* **Verification:** ASCII, empty, whitespace, no-final-newline, and UTF-8 byte
  fixtures; compare expected byte-oriented counts.

## FW-P003 — `<=` is not admitted by the runtime lowerer

* **Severity / category:** MINOR FRICTION, P2, LANGUAGE/LOWERING.
* **Requirement:** classify whitespace with an inclusive upper bound.
* **Attempt:** `if current <= 32` is natural for the word state machine.
* **Observed limitation:** the current runtime lowerer rejects this condition
  with `if condition must be Bool`, while the equivalent strict comparison
  `current < 33` is admitted. This is a language/backend inconsistency exposed
  by the application, not a provider issue.
* **Workaround:** use the mathematically equivalent strict comparison.
* **Workaround cost:** small source awkwardness; no loss of counting semantics.
* **Decision:** record the discrepancy and keep the general workaround for this
  checkpoint. A compiler fix is justified only after a minimal regression
  fixture confirms whether `<=` is intended language syntax or compatibility
  parser debt.
* **Verification:** retain a negative/diagnostic fixture and a positive
  `current < 33` application test.

## FW-P004 — records and typed Result remain untested at the application boundary

* **Severity / category:** MINOR FRICTION, P2, RECORD/RESULT.
* **Requirement:** eventually return per-file `FileStats` or a typed failure.
* **Observed limitation:** the first blocker occurs earlier at file-to-bytes
  input, so adding a structured result now would hide the carrier boundary.
* **Decision:** defer until a real path-read provider value can cross a function
  boundary. Do not special-case `flowwc` or preemptively widen variant storage.

## FW-P005 — multi-file and automatic parallelism are not yet reached

* **Severity / category:** ERGONOMIC REQUEST, P3, COLLECTION/FLOWPARALLEL.
* **Requirement:** process several paths, aggregate totals, and allow existing
  proof-based Flowparallel to decide independence.
* **Observed limitation:** no path API means there is no honest multi-file
  application to analyze yet.
* **Decision:** defer collection aggregation and scheduler work until FW-P001
  is resolved or the application is explicitly redesigned around stdin.

## Current decision

The counting algorithm is genuinely Flowmini. The path-input boundary is a real
P0 grievance and has been recorded before remediation. No C/C++ application
logic has been added. The next action is a proportional provider/carrier
experiment, not a flowwc-specific native implementation.

## FW-P001A — existing byte-carrier path inventory

* **Finding:** `stdin.bytes()` already materializes provider bytes as the
  ordinary runtime `list<int>` value. The prior file ABI exposed only a native
  pointer-oriented read contract.
* **Decision:** reuse `list<int>`; add a provider operation that materializes
  bytes before they enter Flowmini.

## FW-P001B — canonical provider-result carrier

* **Implementation:** `file.bytes(path)` is a governed Flowmini intrinsic.
  Its graph atom accepts a path-bearing record and writes a safe `list<int>`;
  the native implementation reads binary bytes into ordinary runtime values.
* **Contract:** provider `flowcore.filesystem`, symbol `file.bytes`, effect
  `filesystem.read`, result `list<int>`.

## FW-P001C — structural/runtime consistency

* **Evidence:** the structural bundle/Flowanalyst report emits one
  `external_call` with the same provider, effect, argument type, and result
  type consumed by the runtime graph atom. No application-specific native
  counting path was added.

## FW-P001D — path argument propagation

* **Implementation:** positional arguments after the source are exposed by the
  governed `process.args` producer; `args[0]` is materialized as a string and
  passed to `file.bytes`.
* **Gate:** `flowmini Flowwc/src/flowwc.flow <temporary-file>` passes.

## FW-P001E — provider effect/resource provenance

* **Evidence:** Flowanalyst reports `provider_contract=flowcore.filesystem`,
  `provider_symbol=file.bytes`, and `effect_class=filesystem.read`; the value
  carrier does not become pure. A concrete resource identity remains deferred
  for path-based files because the provider currently owns the opened handle.

## FW-P001F — file failure behavior

* **Implementation:** missing/open/read failures raise explicit `file.bytes`
  diagnostics; failures are not converted to an empty list.
* **Gate:** missing-file test returns non-zero and names the path.

## FW-P001G — flowwc path acceptance

* **Evidence:** path, empty, UTF-8, whitespace, no-final-newline, and embedded
  NUL files are counted by the existing Flowmini state machine. The stdin
  implementation remains in `flowwc_stdin.flow` as a regression path.

## FW-P003A — inclusive comparison investigation

* **Finding:** lexer support for `<=` existed, but the runtime parser omitted
  the predicate and atom mapping.
* **Implementation:** parser admission, `int.lte`/related comparison atoms,
  and runtime evaluation now agree. The application uses `current <= 32`.

## FW-NEXT — first remaining application blocker

* **Finding:** single-path input is now usable. The next pressure is typed
  per-file result/error composition and multi-file argument collection.
* **Decision:** do not widen records/Result or parallel scheduling in this
  carrier mission; record them for the next application-led slice.
