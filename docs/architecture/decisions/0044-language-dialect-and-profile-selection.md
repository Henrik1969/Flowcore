# ADR-0044: Language dialect and profile selection

**Status:** accepted direction, provisional source declaration  
**Date:** 2026-08-20  
**Scope:** language maps, dialects, and local language versions

## Decision

Flowcore separates language identity from dialect and profile selection.

```text
lang=C
dialect=c23
profile=gnu
```

or:

```text
lang=Flowmini
dialect=Flowmini.v25
profile=project-local
```

`lang=` selects the grammar and vocabulary map. `dialect=` selects a declared
language standard, semantic revision, or compatible local version.
`profile=` may select additive implementation extensions, target assumptions,
or project policy where such a profile is declared.

The minimum source declaration remains `lang=<map>`. Dialect and profile are
optional when the language map supplies an unambiguous default, but they may be
required by project policy or by a source using features outside that default.

## Established compiler pattern

This follows the proven separation used by GCC: its `-std=` option selects
language standards and GNU dialects, while the compiler still exposes target
and implementation-specific behavior. See the [GCC C dialect
options](https://gcc.gnu.org/onlinedocs/gcc/C-Dialect-Options.html).

Flowcore adopts the semantic pattern without requiring GCC's option spelling
or internal implementation.

## Local versions

A project may declare an explicit local dialect when it owns or controls the
map, for example:

```text
lang=Flowmini
dialect=Flowmini.v25.symboltable
```

Such a dialect is a named, versioned map/profile—not an unrecorded personal
interpretation. It must declare its canonical constructs, extensions,
compatibility behavior, and translation relation to its parent language where
one exists.

## Resolution and safety

The resolver combines language, dialect, profile, project policy, and target
compatibility. An unknown, conflicting, or unavailable dialect fails with an
explicit diagnostic. A dialect may add constructs, but cannot silently change
the meaning of existing canonical constructs or weaken policy and trust laws.

The selected language map, dialect, profile, revisions, and resolver provider
are recorded in provenance and exported artifacts.

## Revisit triggers

Revisit when implementing language-map loading, declaring the first stable
Flowmini dialect, or defining compatibility rules between local profiles and
parent languages.
