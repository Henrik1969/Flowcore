# Frankencore implementation roadmap

**Status:** active working roadmap  
**Date:** 2026-08-20

This roadmap turns the architecture decisions through ADR-0046 into
buildable work. It preserves the additive model: observe the substrate,
normalize facts, resolve policy, and add projections or facades incrementally.

## Current baseline

Already present:

- Flowmini → Flowanalyst → Flowparallel → Flowoptimize → Flowlower language-chain slices;
- C++20/CMake/Ninja project structure;
- Frankencore provenance and ULID implementation;
- architecture checks and conformance tests;
- project-local architecture, policy, trust, and provenance decisions;
- read-only dpkg status projection;
- read-only APT list and source-configuration projection;
- inspectable JSON projections and focused CTest coverage.

Not yet implemented:

- stable JSON exchange schemas and projections for package, verification,
  language, and chain-policy facts;
- ConfigResolve integration;
- native verification result delegation;
- language-map loading and localized parsing;
- chain-policy/version-range evaluation;
- compatibility facade runtime and backend resolution;
- differential facade tests;
- package mutation providers;
- signed map/policy/release distribution;
- multi-substrate and multi-target execution.

Gate 0 progress: provisional schemas and the first C++20 contract types and
validators now exist. They are not yet declared stable until ConfigResolve
integration and fixture compatibility tests are complete.

Gate 0 implementation progress: the C++ contract validators and schema
fixtures now build and pass focused tests. The optional ConfigResolve adapter
also builds against the local 1.1.0 C ABI and resolves a policy outcome in its
reference probe. Language-map moniker resolution and conservative version
range evaluation also have focused C++ probes.

Gate 2 implementation progress: package inventory ordering is deterministic,
the JSON envelope is versioned, and the provider delegates read-only index
target discovery to APT's native `apt-get indextargets` interface. The
delegation is shell-quoted, read-only, and reports native failure explicitly.

Gate 6 implementation progress: `Flowtools/facades/ls/franken_ls` now
delegates ordinary calls to the fixed `/usr/bin/ls` backend and has native
compatibility tests. Its policy/schema extension flags fail explicitly as
unresolved until the extended operation contract is implemented.

## Gate 0 — freeze the semantic contracts

1. Review ADR-0035 through ADR-0046 for overlapping terminology.
2. Define the first versioned JSON schemas for:
   - normalized verification evidence;
   - package facts and APT source facts;
   - language maps;
   - dialect/profile declarations;
   - chain-build policies;
   - facade invocation and result records.
3. Define the matching C++20 value types and validation functions.
4. Define stable status vocabularies and error codes.
5. Add schema fixtures and invalid-fixture tests.

**Gate:** every provider can report facts without inventing missing fields,
and every consumer can distinguish observed facts, derived analysis, policy
outcome, and diagnostics.

## Gate 1 — make ConfigResolve the policy boundary

1. Add a narrow Frankencore policy-resolution adapter around ConfigResolve.
2. Resolve source/project/user/system scope and precedence explicitly.
3. Add policy decisions for verification, overrides, target selection, and
   facade delegation.
4. Export explanation, policy revisions, provenance, and unresolved state.
5. Test contradiction, missing policy, explicit relaxation, and recovery.

**Gate:** providers report evidence; ConfigResolve alone decides admission or
operation permission.

## Gate 2 — finish the read-only substrate inventory

1. Add a provider identity/version record for Pop!_OS, Ubuntu/Debian, APT,
   dpkg, and available frontends such as Nala.
2. Improve dpkg parsing for multiline fields, package source identity,
   installed state variants, and malformed-record locations.
3. Improve APT Deb822/legacy source parsing and deterministic ordering.
4. Correlate APT `InRelease`/`Release` metadata with configured source files.
5. Record `Signed-By`, keyring paths, `trusted=yes`, disabled sources, and
   missing metadata as separate evidence.
6. Use APT's own verification result where a safe native query exists; never
   infer cryptographic success from a filename alone.

**Gate:** a read-only inventory report can explain what is installed, what
repositories are configured, what evidence exists, and what remains unknown.

## Gate 3 — native verification delegation

1. Identify the documented APT verification/invocation boundary.
2. Implement a provider that delegates to APT without duplicating signature
   verification or dependency solving.
3. Capture native identity, configuration, exit status, diagnostics, and
   evidence references.
4. Normalize the result into ADR-0035 vocabulary.
5. Test valid metadata, missing signatures, expired/revoked keys, explicit
   `trusted=yes`, changed repositories, and unavailable keyrings.

**Gate:** Frankencore never reports a stronger verification result than APT
provided, and local overrides remain visibly distinct from verification.

## Gate 4 — language maps and canonical script representation

1. Implement language-map manifest loading and validation.
2. Define the canonical command/capability IR for executable scripts.
3. Implement moniker resolution with locale, scope, revision, collision, and
   deprecation diagnostics.
4. Implement `lang=<map>` source declaration.
5. Add optional `dialect=` and `profile=` resolution.
6. Preserve source maps, literals, comments, policies, and provenance.
7. Build one small Danish map and round-trip the clock-check example.

**Gate:** Danish and canonical source forms produce the same canonical IR;
ambiguous or missing monikers fail deterministically.

## Gate 5 — chain-policy evaluation

1. Load and validate chain-policy manifests.
2. Implement version/range facts and comparisons.
3. Resolve prerequisites against observed providers and capabilities.
4. Select compatible optimizer and lowering providers.
5. Emit per-target reports and aggregate status.
6. Support partial compilation with isolated target failures.
7. Perform a final aggregate pass before declaring a multi-target release green.

**Gate:** a policy can explain why a target is compatible, unresolved, or
rejected without silently substituting tools or lowering paths.

## Gate 6 — first transparent facade

1. Create a user-local facade runtime, never replacing `/usr/bin` initially.
2. Define canonical backend discovery that cannot recurse through `PATH`.
3. Implement an `ls`-compatible delegating facade using the recorded GNU 9.4
   profile.
4. Preserve arguments, environment, streams, exit status, signals, terminal
   behavior, and machine-readable modes.
5. Add explicit policy/schema options for extended behavior.
6. Run native-vs-facade differential tests over the recorded matrix.
7. Add rollback and canary configuration through XDG policy.

**Gate:** ordinary callers see native-compatible behavior; opt-in callers get
   Frankencore records and policy behavior; failures remain recoverable.

## Gate 7 — package facade and mutation boundary

1. Keep dpkg/APT/Nala as backends and projections.
2. Add package query projections over the canonical package model.
3. Add a dry-run operation provider that asks ConfigResolve for permission.
4. Record intent, native backend invocation, result, changed facts, and
   rollback/recovery state.
5. Only then add scoped acquisition/install/remove operations.

**Gate:** no direct state-file edits; every mutation is policy-authorized,
delegated, observable, diagnosable, and recoverable.

## Gate 8 — signed distribution of maps and policies

1. Define signed release/map/policy manifests and digest rules.
2. Use project-controlled keys and explicit trust-store scope.
3. Implement key rotation, expiry, revocation, and recovery diagnostics.
4. Preserve supplier authentication separately from owner attestation.
5. Add Debian-family package metadata integration and standalone artifact
   verification.

**Gate:** users can inspect and verify the exact map, policy, provider, and
   artifact used to produce a result.

## Gate 9 — broaden the forest

Only after the previous gates are green:

- add Windows, macOS, Android, and other substrate adapters;
- add more localized language maps;
- add additional executable facades;
- add alternative optimizers, lowerers, and parallel providers;
- add GUI, IDE, AI, and remote projections;
- add stronger isolation and hardware-backed assurance where available.

Every addition remains a provider or projection until it proves compatibility
and earns promotion through the existing policy and provenance process.

The `ls` facade is the first smoke test for this gate. A future substrate-wide
pass inventories executable capabilities and applies the same facade workflow
per command family; it is never treated as one atomic replacement operation.

## Permanent test requirements

Every gate must include:

- unit and fixture tests;
- malformed and ambiguous input tests;
- ASan/UBSan builds;
- Valgrind where practical;
- deterministic JSON/schema validation;
- failure and recovery tests;
- provenance and policy assertions;
- differential tests against the native substrate for facades;
- no network or privileged mutation in default tests.
