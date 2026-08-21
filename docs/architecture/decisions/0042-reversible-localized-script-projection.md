# ADR-0042: Reversible localized script projection

**Status:** accepted direction, provisional language projection  
**Date:** 2026-08-20  
**Scope:** localized scripts and capability-oriented tooling

## Decision

A script written with localized capability monikers may be translated through
the canonical capability model and rendered into another supported lingo,
without changing its intended operations.

```text
local script
    → parser and moniker resolver
    → canonical command/capability representation
    → policy and semantic checks
    → target-locale or substrate projection
```

For example, a script using `Spørg` can resolve to canonical `ask`, then be
projected as `Frage`, `Questionair`, or the native substrate spelling `ask`.
The operation identity, arguments, policies, schemas, and provenance remain
canonical while presentation vocabulary changes.

## Reversibility law

The translation is one-to-one only for a declared, versioned vocabulary whose
monikers resolve deterministically in the active scope. The translator must
preserve:

- canonical capability identity;
- argument and operand structure;
- policy and schema references;
- quoting and literal values;
- ordering and control flow;
- source locations and provenance;
- unsupported or unresolved constructs as explicit diagnostics.

Unknown, colliding, deprecated, or scope-incompatible monikers must not be
silently guessed. A translation may be rejected or retain an explicit
unresolved node until the caller supplies a policy decision.

## Canonical representation

The canonical intermediate representation is the semantic authority. Local
surface syntax, spelling, casing, help text, and display language are
projections. Translation must not operate by blind text substitution because
that would confuse literals, comments, identifiers, and capability names.

The representation should remain inspectable and exportable so an IDE, AI,
CLI, GUI, debugger, or remote service can consume the same resolved script.

## Scope

This pattern supports localized command and capability languages. It does not
claim that arbitrary natural-language prose is mechanically unambiguous. A
future natural-language adapter may propose a canonical representation, but it
must pass through the same semantic checks, diagnostics, policy, and explicit
resolution boundaries.

## Safety and provenance

Translation does not grant authority or change trust. The resulting artifact
records source locale, moniker catalogue and revision, canonical identities,
target projection, resolver provider, and unresolved/overridden decisions.

## Revisit triggers

Revisit when defining the canonical command IR, implementing the first
localized script parser, or specifying lossless round-trip behavior for
comments, formatting, and source maps.
