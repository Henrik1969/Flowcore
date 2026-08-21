# Frankencore Repository Audit — 2026-08-20

**Status:** completed repository-grounded audit  
**Reference:** `docs/architecture/FRANKENCORE-HARDENING-PLAN.md`  
**Scope:** Flowcore repository and its checked-in architecture/build/test surfaces

## Executive result

Flowcore already has a strong, versioned frontend and transformation pipeline:

```text
FlowMini -> Flowanalyst -> Flowbind -> Flowoptimize -> Flowlower -> LLVM/ELF
```

The frontend boundary is the most conformant part of the repository. It has
explicit authority boundaries, versioned JSON contracts, provenance, negative
tests, and independent consumers.

Frankencore as a constitutional system is not yet implemented in the
repository. Its vocabulary, invariants, provider model, lifecycle semantics,
authority model, and introspection model are primarily architectural
documentation and local conventions. This is consistent with the proposal's
status, but the distinction must remain explicit.

## 1. Current repository structure

### Active implementation areas

- `Flowmini/flowmini_v25_symboltable_projection` — active C++20 frontend,
  AST, SymbolTable projection, and frontend bundle.
- `Flowanalyst` — independent semantic-report consumer.
- `Flowbind` — policy-authorized `dlopen`/`dlsym` provider boundary.
- `Flowoptimize` — versioned identity optimization boundary.
- `Flowlower` — narrow LLVM lowering boundary.
- `Flowkernel` — isolated Linux-kernel probe brick.
- `Flowtools` — emerging CLion/CMake/Ninja projection.
- `subprojects/` — reusable TokenTree and SymbolTable libraries.
- `Pattern_explored/` and `_archive/` — experiments and historical stages.

There is no repository-root `CMakeLists.txt`; stage builds are separate CMake
projects. The new `Flowtools` CMake project provides a usable IDE-facing
pipeline, but it is not yet the canonical superbuild.

## 2. Existing architectural contracts

The strongest implemented contracts are:

| Boundary | Contract | Evidence |
| --- | --- | --- |
| FlowMini -> consumer | `flowmini.frontend_bundle` v2 | bundle parser, golden bundles, negative attacks |
| Flowanalyst output | `flowanalyst.semantic_report` v1 | consumer contract and CTest |
| Flowbind output | `flowbind.binding_report` v1 | exact policy grants, `dlopen`/`dlsym`, fuzz gate |
| Flowoptimize output | `flowoptimize.optimization_report` v1 | identity transform boundary and CTest |
| Flowlower output | `flowlower.lowering_report` v1 plus LLVM IR | profile gates and executable tests |
| Flowtools build projection | CMake custom targets | CLion/Ninja smoke build |

The contracts are mostly serialized report contracts. They are not yet a
shared Frankencore meta-model or common result/error library.

## 3. Existing vocabulary

Flowcore already uses several proposal-aligned terms: capability, provider,
policy, projection, provenance, revision, target, contract, backend, and
adapter. Their meanings are not yet centralized in one canonical vocabulary
document or machine-readable registry.

The most important current ambiguity is that `provider`, `backend`, and
`consumer` are used consistently in individual projects but are not enforced
across the repository. `Flowbind` is called a provider boundary, while LLVM is
called a target provider; the distinction is understandable but not formally
typed.

## 4. Dependency graph

The intended downward pipeline is real and testable:

```text
FlowMini
   |
   v
Flowanalyst -> Flowbind
   |
   v
Flowoptimize
   |
   v
Flowlower -> clang/LLVM
```

FlowMini directly links TokenTree and SymbolTable static libraries. Flowbind
directly uses the host dynamic-loader API. Flowkernel directly uses Linux and
POSIX kernel-facing headers. Flowlower emits Linux-oriented LLVM declarations
for the current executable profiles.

The main dependency weakness is build discovery, not semantic layering:

- several CMake test definitions refer to sibling `build/` and
  `cmake-build-debug/` paths directly;
- stage executables are passed through environment variables rather than
  imported CMake targets;
- the active Flowmini CMake file still contains the legacy
  `build/libflowmini_testabi.so` bridge;
- there is no single root build graph governing all stages.

These are architecture-governance risks because a component can appear to
work only inside one source-tree layout.

## 5. Substrate leaks

Observed substrate-specific code is concentrated in explicit boundaries:

- `Flowbind` owns `dlopen`/`dlsym` discovery.
- `Flowkernel` owns Linux/POSIX probes, `/tmp`, sockets, namespaces, and
  syscall-adjacent headers.
- `Flowlower` owns the current LLVM/Linux ABI emission profiles.
- `flowcat_file_main` declares and authorizes libc `open`, `read`, `write`,
  and `close` through an explicit ABI contract.

This is good boundary placement for the current prototypes. However, there is
no reusable Frankencore adapter interface or substrate-neutral capability
contract beneath these implementations. The substrate is isolated by project
convention and report boundaries, not by a common enforceable API.

## 6. Flowcore coupling

The repository currently respects the most important direction: Flowanalyst
does not link FlowMini internals and consumes the frontend bundle instead.
Flowbind, Flowoptimize, and Flowlower also consume reports rather than parser
objects.

The unresolved architectural boundary is the future relationship between
Frankencore semantic objects and Flowcore Graph IR. The documents explicitly
state that Frankencore must not require Flowcore, but no Frankencore library or
language-independent capability API exists yet to test that rule.

## 7. Capability, provider, backend, and policy audit

### Present

- Flowbind has an exact allow-list policy format.
- Flowanalyst emits external binding requirements.
- Flowbind verifies library and symbol availability without executing foreign
  functions.
- Flowlower requires a ready binding report before external-capability lowering.
- Flowtools exposes the pipeline as CMake/Ninja targets.

### Missing or provisional

- no common `CapabilityId`/version/requirements/limits model;
- no general `discover`, `query`, `negotiate`, `invoke`, or `observe` contract;
- no provider selection explanation or `why` operation;
- capability and authority are only partially separated through policy grants;
- no provider replacement or degradation protocol;
- no machine-readable provider/backend distinction;
- no cross-stage contract registry.

The current Flowbind policy proves authorization for a narrow lowering case; it
does not yet constitute a general Frankencore authority model.

## 8. Lifecycle and error semantics

### Present

- pipeline stages fail closed on malformed or unsupported report versions;
- Flowbind produces `ready` or `blocked` reports;
- Flowanalyst provides partial analysis regions and diagnostics;
- Flowkernel reports probe-level success, error, and skip states;
- the flowcat executable returns failure for selected I/O errors.

### Missing

- no shared semantic `Result<T, Error>` contract;
- no standard error fields for domain, code, cause, recoverability, and context;
- no provider disappearance/replacement lifecycle contract;
- no general object availability or detachment semantics;
- no mutation actor, policy, before/after revision, atomicity, or rollback
  record;
- no durable cross-edit identity model for compiler entities.

This confirms the existing conformance declaration's recommendation: define
the runtime mutation-provenance contract before changing runtime storage.

## 9. Identity, revision, and provenance

Structural frontend provenance is strong within one exported revision:

- source locations and source maps;
- typed AST paths;
- structural roles;
- matching AST/symbol IDs;
- independent origin validation.

The IDs are explicitly only revision-local. The proposed revisioned identity
model for runtime and Graph IR is not implemented. The current optimizer and
lowerer preserve report/profile boundaries, but do not yet expose entity-level
derivation lineage.

## 10. Introspection and explainability

Current introspection exists as report inspection:

- AST and SymbolTable dumps;
- frontend bundles;
- semantic reports;
- binding reports;
- optimization reports;
- lowering reports;
- Flowkernel JSON probe reports.

There is no unified `fc inspect` model and no `why` decision provenance. The
reports explain stage facts, but not a general provider-selection decision.

## 11. Existing tests relevant to the invariants

The current gates include:

- FlowMini TokenTree and SymbolTable CTest tests;
- AST, SymbolTable, frontend-bundle, and negative source suites;
- Flowanalyst semantic pipeline tests;
- Flowbind provider and malformed-input fuzz tests;
- Flowoptimize boundary tests;
- Flowlower profile, matrix, corpus, and executable tests;
- Flowkernel probe tests;
- Flowtools CMake/Ninja flowcat lower/run smoke path;
- Valgrind-capable FlowMini test infrastructure;
- historical Firetest reports.

At audit time, all active CTest gates passed:

```text
FlowMini:       2/2
Flowanalyst:    1/1
Flowbind:       2/2
Flowoptimize:   1/1
Flowlower:     3/3
```

The main missing test class is architectural conformance testing: forbidden
dependency checks, capability truthfulness, provider failure lifecycle,
projection ownership, and mutation provenance.

## 12. Conflicts with the hardening proposal

### Confirmed alignment

- semantic reports and projections are separated from frontend authority;
- Flowcore stages communicate through inspectable versioned boundaries;
- substrate-facing code is concentrated in named probes/providers/backends;
- compatibility paths are documented rather than silently treated as future
  canonical architecture;
- the project already uses negative tests and fail-closed report handling;
- no universal Frankencore base-object hierarchy exists.

### Partial alignment

- vocabulary is distributed rather than canonical;
- policy exists locally but not as a generalized capability/authority model;
- provider/backend/adapter distinctions are documented but not typed or tested;
- provenance is strong for the frontend but absent for general runtime mutation;
- lifecycle and error behavior is stage-specific;
- dependency direction is intended but not mechanically enforced;
- introspection exists per stage but not as a unified semantic surface.

### Direct mismatches or governance debt

- the historical conformance declaration predates the current v0.26 language
  chain and must not be treated as a current status report;
- individual CMake projects still depend on hard-coded sibling build paths;
- the active repository contains both the real hardening plan and an older
  misleading meta-description with a similar name;
- the current working tree contains uncommitted Flowcat file-I/O and Flowtools
  changes; these must be separated from any constitutional baseline commit.

## 13. Recommended smallest first patch set

No large implementation should begin yet. The smallest useful hardening set is:

1. Create `FRANKENCORE-CONSTITUTION.md` containing the first 10–20 invariants,
   canonical boundaries, vocabulary, and dependency law.
2. Mark the old conformance declaration explicitly historical and create a
   current conformance-status document tied to the present revision.
3. Add a small machine-readable contract inventory recording each active
   report format, version, authority, producer, consumer, and failure status.
4. Add a repository check that rejects forbidden dependency directions and
   flags hard-coded sibling build-tree paths in active CMake definitions.
5. Define, but do not yet implement storage for, a mutation-provenance record:
   entity identity, old revision, new revision, actor/provider, authorizing
   policy, before/after evidence, atomicity, recoverability, and rollback.
6. Add one conformance fixture for the simplest semantic reference object—Clock
   should follow only after the contracts above are written.

The first implementation target should be contract/governance tooling, not a
large Frankencore runtime library.

## Follow-up remediation

After the initial audit, the stage CMake test definitions were corrected so
they no longer inject hard-coded sibling build paths. The test runners retain
relocatable defaults and environment-variable overrides. The architecture
checker now reports:

```text
constitution: PASS
contract inventory: PASS
build-path debt: none
presentation leakage: none
```

The original findings above remain useful as audit history; this remediation
records their current status.

The first Clock reference object was then added under
`Flowtools/reference/clock`. It exercises the documented Linux adapter,
semantic object, capability, provider/backend distinction, policy decision,
structured error, and CLI projection. Its CTest gates cover monotonic,
realtime, and unknown-clock behavior.

The reusable `flowcore_conformance` target now validates the constitutional
law identifiers, contract inventory, strict architecture check, Clock report
contract, and fail-closed Flowbind policy behavior.

## Audit conclusion

Flowcore is already a credible experimental implementation of the
projection-and-boundary philosophy. It is not yet a Frankencore constitutional
implementation. The next safe move is to make the existing architectural
claims executable and current, then validate them with a small reference
object before expanding into display, Linux substrate, authority, or
distributed-provider work.
