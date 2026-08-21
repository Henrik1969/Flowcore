# ADR-0033: Substrate verification adapter contract

**Status:** accepted direction, provisional contract  
**Date:** 2026-08-20  
**Scope:** release, repository, and artifact verification

## Decision

Frankencore defines one semantic verification contract and permits each
substrate to provide its native verification mechanism through an adapter.
The core does not replace APT, OpenPGP, platform package signing, installer
verification, or future mechanisms with one universal wire format.

```text
native substrate evidence
          ↓
verification adapter
          ↓
Frankencore verification result
          ↓
ConfigResolve and admission policy
```

## Adapter obligations

A compatible adapter must:

- identify the substrate and adapter version;
- discover the relevant artifact, repository, or release metadata;
- verify signatures and digests using the substrate's native rules;
- report the signing identity, key state, and trust-store source;
- report artifact identity, digest, version, origin, and scope;
- expose expiry, supersession, revocation, and downgrade information;
- preserve the original evidence or a stable reference to it;
- return diagnostics explaining success, failure, uncertainty, or absence;
- distinguish `verified`, `unverified`, `invalid`, `expired`, `revoked`,
  `unsupported`, and `unavailable` results;
- declare compatibility and capability limits rather than guessing.

The adapter reports facts and verification results. It does not independently
decide whether an operation is permitted; that remains a ConfigResolve and
admission-policy decision.

## Substrate examples

- Debian-family package providers adapt APT's signed repository metadata and
  configured keyrings.
- Release-media providers adapt signed checksum manifests and image digests.
- A future platform provider may adapt its own package, installer, or signing
  system if it can produce the same semantic result set.

## Failure law

An invalid, revoked, expired, or contradictory result must never be converted
into acceptance by the adapter. Missing or unsupported verification produces a
diagnostic and is resolved as `unknown`, `quarantined`, or `rejected` by
policy. A substrate may be permissive only through an explicit, scoped,
recorded policy override.

## Conservative baseline

The initial contract is intentionally small and safety-awake:

- verified evidence is accepted only within its declared scope;
- missing, ambiguous, stale, or contradictory evidence is not promoted to
  trust;
- relaxed behavior requires an explicit, scoped, recorded policy decision;
- adapters may add substrate-specific facts and capabilities without changing
  the core meaning of existing results;
- future cryptography, transports, package systems, and hardware-backed
  providers are additive extensions rather than prerequisites for the basic
  system.

This keeps ordinary systems usable without making the default permissive, and
leaves room for stronger assurance when a substrate or operator can provide
it.

## Explicit local trust overrides

Native configuration can contain deliberate operator overrides such as APT's
`trusted=yes`. The adapter must preserve the distinction between:

1. substrate-authenticated evidence, such as a valid `Release` signature
   checked against a scoped keyring; and
2. an operator policy assertion that permits use despite missing or bypassed
   substrate authentication.

The second case is an honest local assertion that the operator has chosen to
trust the source despite the absence of substrate proof. It remains available
for the operator's own local development and recovery workflows, but it must
be reported as an explicit policy relaxation with provenance and a diagnostic.
It must never be represented as `verified`.

Frankencore must never treat `trusted=yes` as a supplier credential or sign
off on another party's work on that party's behalf. A third-party repository
must provide its own authenticated metadata and scoped verification key
reference, equivalent to APT's `Signed-By`, before it can receive a verified
status. Otherwise it remains unverified and is admitted only if an explicitly
resolved local policy permits that risk.

## Evidence-first evolution

The stable result schema and API/ABI must be derived from examination of the
actual substrates and surrounding ecosystems that FrankenPOP intends to use.
The design process therefore begins with read-only inventory and concrete
verification traces: operating-system identity, package-manager behavior,
repository metadata, keyring layout, signing formats, update transitions,
failure diagnostics, and available platform facilities.

Observed facts become versioned adapter evidence. They do not automatically
become universal Frankencore law. The core is expanded only when multiple
substrates or a necessary contract obligation demonstrate that an additive
semantic field is needed.

## Revisit triggers

Revisit when defining the stable C++ API/ABI, the result schema, the first APT
and signed-manifest providers, or a second platform provider.
