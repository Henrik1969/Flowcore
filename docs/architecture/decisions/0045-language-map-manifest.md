# ADR-0045: Inspectable language-map manifest

**Status:** accepted direction, provisional artifact format  
**Date:** 2026-08-20  
**Scope:** language maps and moniker resolution

## Decision

A language map is an inspectable, versioned artifact. JSON is the initial
interchange and inspection projection; native providers may use another
internal representation as long as they preserve the same contract.

Minimal shape:

```json
{
  "format": "frankencore.language-map",
  "version": "1",
  "id": "Danish",
  "revision": "Danish.v1",
  "parser": "frankencore.shell.surface.v1",
  "parent": "canonical",
  "monikers": {
    "ask": ["Spørg"],
    "echo": ["skriv"],
    "exit": ["afslut"]
  }
}
```

The source declaration remains minimal:

```text
lang=Danish
```

The resolver discovers the declared map, validates its identity and revision,
then uses its parser and moniker catalogue to produce canonical constructs.

## Required map facts

A map must identify:

- interchange format and map-schema version;
- language-map identity;
- map revision;
- parser/surface provider;
- parent or canonical semantic base;
- canonical-to-local moniker mappings;
- collision and deprecation behavior;
- supported dialect/profile relations where applicable.

The map may additionally contain localized help, diagnostics, aliases,
formatting rules, source-map rules, and target projections. These are additive
and must not redefine canonical semantics.

## Mapping direction

Canonical identities are the authoritative keys. Moniker lookup may resolve
local input to a canonical identity, while rendering uses the reverse mapping
selected by locale, scope, and policy. A canonical identity may have several
local monikers, but a local moniker must resolve to one canonical identity in
its active map and scope.

## Trust and lifecycle

Maps are project/distribution artifacts and therefore carry ordinary
provenance, source, revision, and trust facts. A new or modified map may add
vocabulary but must not silently change the meaning of an existing moniker.
Breaking changes require a new revision or explicit compatibility declaration.

An unavailable, unsigned where policy requires signing, malformed, or
ambiguous map produces a diagnostic and prevents executable translation.

## Revisit triggers

Revisit when implementing map loading, defining the canonical map schema,
adding the first real Danish adapter, or specifying signed map distribution.
