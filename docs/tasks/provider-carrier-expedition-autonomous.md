# Autonomous Provider Carrier Expedition

## Authority

This is the active autonomous Flowcore mission. Execute it on the checked-out
development branch until the definition of done is reached, a genuine external
blocker is recorded, or the next experiment would require disproportionate
architectural widening. Normal coherent commits and pushes are authorized by
the repository autonomous-chain policy. Preserve unrelated user work, in
particular the existing unstaged `Flowselection` changes.

Recover after a fresh turn by reading this task, the TinyVM and product-scale
architecture plans, the current maturation ledger, `.codex-run-state`, recent
history, and the worktree. Keep `.codex-run-state` as `CONTINUE` while any gate
is unfinished; use `BLOCKED` only for a genuine external or architectural
decision that prevents all safe progress; use `DONE` only after every gate,
final verification, final commit, push, and clean-worktree requirement passes.

## Objective

Use the redacted substrate-warehouse reconnaissance to experimentally find the
smallest provider bindings and safe value/resource carriers that unlock the
largest useful portion of the existing software substrate. Prove three
representative interface families through narrow vertical slices:

1. buffer plus length / byte collections;
2. opaque resource handles with create/use/release;
3. one bounded aggregate/struct ABI layout.

The governing chain remains:

```text
Flowmini application -> std semantic API -> capability contract -> policy
  -> provider binding -> existing library/substrate
```

Keep library identity, capability identity, provider identity, policy, effect,
resource, failure, provenance, and evidence distinct. Discovery metadata such
as `pkg-config`, CMake exports, Cargo/Python metadata, JAR metadata, and ELF
inspection is evidence only; it is not semantic authorization.

## Privacy and repository scope

Published evidence must remain machine-neutral. Do not commit personal paths,
usernames, hostnames, machine fingerprints, unrelated exact tool/package
versions, local cache details, or personal addresses. Review every report and
commit for those items. Temporary local evidence may contain host details only
when execution requires them and must not be published. Do not install or
modify host packages, toolchains, linker configuration, or system files.

## Execution protocol

Work through the gates in order. At every coherent checkpoint:

1. record observed evidence in `docs/substrate/provider-carrier-expedition.md`;
2. run focused positive, negative, malformed-input, provenance/effect/resource,
   determinism, regression, and sanitizer checks appropriate to the slice;
3. run `git diff --check` and inspect the diff for unrelated/generated files;
4. update the maturation ledger with status, limitations, and the next gate;
5. commit only the green mission-related files and push the current branch;
6. leave the state as `CONTINUE` and proceed to the next justified gate.

Do not create broad bindings or a universal FFI. Stop and report `BLOCKED` if
safe progress requires universal pointer semantics, a general ownership system,
a major parser rewrite, callback/template/variadic machinery outside scope, a
target-specific representation becoming canonical semantics, a specimen-only
compiler hack, or verification that cannot be made green. A blocked report must
preserve the passing checkpoint and name the smallest decision or authority
needed.

## Gate PC-00 — Reconnaissance baseline

Read the redacted warehouse report and inventory. Inspect current Flowbind,
`flowbind-gen`, provider contracts, resource metadata, safe byte carrier,
Flowanalyst effect/resource facts, and Flowparallel resource proof. Create and
maintain the expedition ledger at this path. Use the statuses `OBSERVED`,
`IMPLEMENTED`, `TESTED`, `DEFERRED`, `NOT SUPPORTED — MISSING`, `NOT SUPPORTED —
INTENTIONALLY EXCLUDED`, and `UNKNOWN`.

## Gate PC-01 — Scalar control

Use existing libm evidence as a control only. If the scalar binding path is
already proven, record `CONTROL ALREADY SATISFIED` and continue without widening
scope.

## Gate PC-02 — Safe buffer carrier

Use zlib when available and already discoverable. Select one bounded
compress/decompress operation with a pointer-plus-length native ABI. First test
whether the existing safe `list<int>` byte carrier is sufficient. Native code
may allocate, call zlib, materialize bytes, and release native memory; Flowmini
must never receive pointers or perform pointer arithmetic. Capture provider,
capability, operation, input/output carriers, effect, library identity/target
requirements, failure, materialization, and provenance.

Test empty, ASCII, UTF-8, binary/NUL, larger input, deterministic output where
promised, round-trip, malformed input, and provider failure. Test Flowanalyst
and Flowparallel treatment conservatively; unknown provider concurrency remains
serial. Add a new carrier only if evidence proves `list<int>` inadequate.

## Gate PC-03 — Opaque resource lifecycle

Use SQLite when available. Prove one deterministic in-memory or equivalent
`open -> use -> result -> close` path with an explicit provider-owned resource
identity, effect, failure, and provenance. Test open failure, invalid operation,
and use-after-release/double-release where the current runtime can represent
them. Do not add ownership checking. For same or distinct database handles,
retain `unknown -> serial` unless the provider contract explicitly proves
concurrency compatibility.

## Gate PC-04 — Bounded aggregate ABI

Only after PC-02 and PC-03 are green, choose one modest stable C aggregate with
fixed documented layout, no callbacks, unions, or variadics, and an easy oracle.
Preserve type identity, size, alignment, field order/types/offsets, source
provenance, and target constraints. Keep semantic field meaning separate from
target ABI layout. Reject incomplete or mismatched layout evidence explicitly;
do not claim arbitrary aggregate support.

## Gate PC-05 — Cross-family assessment

Produce a comparison matrix for scalar, buffer, opaque-resource, and aggregate
families covering reused support, new mechanism, complexity, ecosystem leverage,
and recommendation. Update measured Flowbind coverage for scalar, enum, string,
buffer-plus-length, opaque handle, lifecycle, aggregate, callback, variadic,
template/class, and managed-runtime shapes. Mark unsupported shapes honestly.

## Gate PC-06 — Semantic and internal-provider implications

Derive only evidence-backed `std.*` semantic candidates; do not build a broad
stdlib. Map internal bricks (including configuration, environment, arguments,
text, AI, task graph, and other discovered components) to possible capabilities
or providers without rewriting them. Document how the same capability could use
current userland, future Flow-native, bare-metal, remote, or test providers
without changing application semantics.

## Required evidence identifiers

Use or extend these ledger entries:

```text
PC-00 scouting baseline
PC-01 scalar control
PC-02 safe buffer input/output and byte materialization
PC-03 opaque resource identity and create/use/release
PC-04 resource conflict/effect evidence
PC-05 aggregate ABI and target-layout evidence
PC-06 Flowbind coverage
PC-07 std semantic implications
PC-08 internal brick/provider map
PC-09 carrier priority and next action
```

Each entry records the finding, experiment, observed evidence, existing
support, required change, architecture cost, implementation, verification,
remaining limits, and decision.

## Non-goals

Do not implement universal FFI, arbitrary C++ class/template binding,
callbacks, variadics, managed-runtime bridges, GC, a borrow checker or general
ownership system, unsafe Flowmini source, inline foreign code, concurrency
syntax, a new scheduler, a large standard library, a full database/compression
framework, or a bare-metal runtime. Do not infer permission from discovery,
symbol names, signatures, or undocumented thread-safety. Do not turn specimen
details into canonical semantics.

## Verification and completion

At every implementation gate run the relevant focused Flowmini, Flowbind,
Flowanalyst, Flowparallel, provider, malformed-binding, provenance/effect/
resource, determinism, regression, and sanitizer checks. At completion run the
root CTest, focused suites, provider experiments, ASan/UBSan, categorized
Flowmini suite, and `git diff --check`. Do not cosmetically change known-gap
accounting.

The final report must include commit hashes, experiments and all three results,
native/Flowmini boundaries, carriers, effects, resources, provenance,
Flowparallel outcomes, Flowbind and provider-contract changes, Flowmini/native
files, std implications, internal brick map, remaining blockers, exact test and
sanitizer counts, privacy audit, and Git status. It must answer which smallest
carrier/binding mechanisms unlock the largest warehouse share and rank at most
five next actions from evidence rather than roadmap fashion.

## Definition of done

The expedition is complete when at least one safe buffer-plus-length API, one
opaque create/use/release resource, and one bounded aggregate layout have each
been proven or explicitly rejected with evidence; provider identity/effects/
resources/provenance remain explicit; Flowbind coverage and `std.*` implications
are updated; no unsafe Flowmini semantics or speculative broad framework was
added; privacy review passes; all accepted checkpoints are regression- and
sanitizer-green; the final checkpoint is committed and pushed. Otherwise leave
the state `CONTINUE` or, only under the stop conditions above, `BLOCKED`.
