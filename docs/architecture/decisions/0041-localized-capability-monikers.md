# ADR-0041: Localized capability monikers

**Status:** accepted direction, provisional naming contract  
**Date:** 2026-08-20  
**Scope:** command, capability, and facade naming

## Decision

Frankencore capabilities have one canonical internal identity and may expose
localized or user-chosen monikers as additive projections.

```text
canonical capability identity: ask
        ↓ locale/scoped moniker resolver
Spørg / Frage / Questionair / ask
        ↓
same capability and semantics
```

The canonical identity remains explicit and stable inside the system. A
moniker changes how a human or local tool names the capability; it does not
create a second implementation or alter the capability's meaning.

## Name layers

- **Canonical identity:** stable, language-neutral identifier used in APIs,
  provenance, policies, schemas, and machine-to-machine references.
- **Native substrate name:** the established name provided by the operating
  system or tool, such as `ask` or `ls`.
- **Localized moniker:** locale- and scope-aware user-facing alias, such as
  `Spørg` or `Frage`.
- **User/project alias:** an explicitly configured local nickname with a
  declared scope and source.

Aliases resolve toward the canonical identity. They do not replace canonical
identifiers in durable records or silently change the native substrate
contract.

## Resolution rules

The resolver considers, in order:

1. explicit canonical identity;
2. explicit scoped alias;
3. project or user moniker policy;
4. locale-specific catalogue;
5. native substrate name;
6. unresolved/ambiguous diagnostic.

An alias collision must never be guessed through locale familiarity. The
resolver reports the candidates and requires an explicit disambiguation or
policy decision. Resolution records the input moniker, locale, scope,
catalogue revision, canonical result, and resolver provider.

## CLI and API boundary

Localized monikers are appropriate for interactive command use and human
interfaces. Stable scripts, APIs, schemas, and provenance should use canonical
identities or explicitly versioned aliases. A localized alias may be accepted
in a CLI only when its resolution is deterministic in the active locale and
scope.

Help, about, error, and diagnostic text may be translated independently from
the canonical capability identity. This preserves both local usability and
machine interoperability.

## Safety and ownership

Adding a moniker is additive vocabulary. It must not grant authority, bypass
policy, change trust, or broaden a capability's scope. Project and user
monikers remain scoped to their owner unless deliberately promoted through the
capability-discovery and policy process.

## Revisit triggers

Revisit when implementing the first moniker catalogue, defining locale and
scope precedence in ConfigResolve, or integrating monikers with a transparent
executable facade.
