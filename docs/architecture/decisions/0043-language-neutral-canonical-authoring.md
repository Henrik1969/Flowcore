# ADR-0043: Language-neutral canonical authoring

**Status:** accepted direction, binding architectural pattern  
**Date:** 2026-08-20  
**Scope:** localized and user-defined programming surfaces

## Decision

Any human or machine lingo may author a Flowcore capability script when its
syntax and vocabulary provide a complete, versioned, and unambiguous mapping
to the canonical representation.

```text
Klingon / Danish / Tweelik / project lingo
                    ↓
          deterministic language adapter
                    ↓
           canonical semantic representation
                    ↓
       analysis, policy, execution, projection
```

The lingo does not need to resemble English, Bash, C++, or any other existing
language. It may use localized words, invented words, alternate grammar,
different notation, or a domain-specific surface. The canonical model is the
semantic meeting point.

## Minimal declaration

The only mandatory source-level declaration is the language map identity:

```text
lang=Danish
```

or:

```text
lang=Klingon
```

The declared map resolves the language's grammar and vocabulary catalogue.
Map revision, provider identity, compatibility, and optional source metadata
are carried by the map and surrounding project/configuration policy rather
than repeated as compulsory ceremony in every source file. If the named map
cannot be resolved uniquely, compilation stops with an explicit diagnostic.

## One-to-one law

A source surface is executable only when each accepted construct resolves to
one canonical meaning in its active language and catalogue revision. The
adapter must reject or preserve as unresolved:

- aliases with multiple canonical targets;
- missing vocabulary entries;
- grammar with multiple parses;
- scope- or policy-dependent meanings that were not explicitly selected;
- deprecated names without a declared compatibility mapping;
- constructs that cannot preserve source locations, arguments, effects, or
  control flow.

The translator must not use guesswork, approximate translation, or cultural
intuition to resolve executable meaning.

## Canonical authority

The canonical representation—not any surface lingo—is authoritative for:

- capability identity and composition;
- arguments, data, and control flow;
- effects and failure behavior;
- policy and schema references;
- provenance, source maps, and diagnostics;
- compatibility and target lowering.

Multiple surface languages may therefore compile to the same canonical form,
and one canonical form may be rendered into multiple surface languages.

## Language package contract

A language adapter must declare:

- language identity and version;
- grammar and vocabulary catalogue revision;
- canonical constructs it supports;
- translation direction(s);
- source-map and round-trip guarantees;
- unsupported and extension behavior;
- diagnostics and provenance requirements.

Adding a new lingo is additive. It must not modify the canonical semantics or
weaken existing language adapters.

## Boundary

This law applies to structured executable languages and capability scripts. It
does not claim that unrestricted natural-language conversation is inherently
unambiguous. A natural-language adapter may propose a structured program, but
that proposal must still pass deterministic parsing, semantic analysis,
policy resolution, and explicit confirmation where required.

## Revisit triggers

Revisit when implementing the first non-English language adapter, defining
the canonical semantic IR for scripts, or specifying cross-language
round-trip/source-map conformance tests.
