# Frankencore core

This directory contains neutral Frankencore capability contracts and reusable
C++ implementations. Core APIs are storage-neutral and must not depend on a
CLI, IDE, GUI, JSON consumer, or particular Linux provider.

## Current core

- `Core/Provenance` — canonical mutation-provenance record, validation, and
  inspectable JSON projection. It also owns the neutral ULID identity
  generator used for event, attempt, correlation, actor, provider, and policy
  references.
- `Core/Packages` — read-only package facts from the native dpkg status
  database and APT list metadata. It preserves substrate evidence and does
  not replace apt, dpkg, nala, or their verification machinery.
- `Core/Contracts` — minimal versioned C++20 value types and validation for
  normalized verification evidence, language maps, chain policies, and
  executable facade invocations.
- `Core/Policy` — optional ConfigResolve adapter over its public C ABI; the
  adapter is disabled unless a ConfigResolve root is supplied at configure
  time.
- `Core/Language` — validated language-map moniker resolution with explicit
  unresolved and collision diagnostics.
- `Core/Requirements` — conservative dotted-version range evaluation with
  rejection of unsupported range syntax.
- `Core/Runtime` — read-only runtime capability discovery for logical CPU
  capacity, memory, and optional CUDA driver/device availability. It reports
  facts only; provider selection remains a later policy-resolved stage.

The core is intentionally small. New APIs belong here only when their
identity, authority, policy, versioning, and failure semantics are clear.

User-local discovery storage is a separate materialized-view concern. The
initial catalog backend is SQLite under the XDG data location; it is not part
of the project-local Frankencore core or an authority over project artifacts.
