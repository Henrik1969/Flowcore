# ADR-0036: Canonical package data over native package substrates

**Status:** accepted direction, provisional model  
**Date:** 2026-08-20  
**Scope:** Debian-family package management and future package providers

## Decision

Frankencore leaves native package tools in place and builds a canonical,
inspectable data model above them.

```text
dpkg database / package files / APT metadata / repository keys
                         ↓
                 substrate adapters
                         ↓
              Frankencore package facts
                         ↓
           ConfigResolve and policy decisions
                         ↓
        apt / dpkg / nala / GUI / API / future tools
```

APT, dpkg, and Nala remain available as substrate capabilities:

- `dpkg` is the local package database and package-operation substrate;
- APT resolves repositories, versions, dependencies, metadata, and package
  acquisition;
- Nala is a user-facing package-management projection using the underlying
  Debian-family package mechanisms.

Frankencore does not duplicate their databases or silently replace their
operation semantics. It observes and adapts their authoritative facts, then
offers a stable cross-tool projection for inspection, provenance, policy, and
future consumers.

## Initial canonical package facts

The first model should cover only facts demonstrably available from the
substrate:

- package name, version, architecture, and source identity;
- installed, available, candidate, held, removed, or unknown state;
- repository origin, suite, component, and acquisition reference;
- dependency and relationship declarations as reported by package metadata;
- package and index digests where available;
- repository authentication result and key reference;
- local package database evidence and modification time;
- package-manager/provider identity and version;
- provenance references, diagnostics, and observation timestamp.

Facts must retain their source and confidence. A package reported as installed
by dpkg is not automatically current, secure, approved, or owner-attested.

## Operation boundary

The model is initially read-oriented. Package mutations continue through the
native substrate tools or a narrowly defined provider capability. A future
Frankencore operation provider may invoke APT or dpkg, but must first resolve
policy and then record intent, command/provider identity, result, and changed
facts.

No package operation is implemented by directly editing the dpkg database or
APT state files.

## Projection rule

Consumers may project the canonical facts into JSON, CLI tables, GUI views,
IDE panels, reports, or language bindings. They must preserve the distinction
between observed substrate facts, derived analysis, and policy decisions.

Provider-specific fields may be carried in namespaced extensions. A provider
must declare unsupported fields rather than fabricate portable meanings.

## Compatibility and evolution

The model is versioned independently of APT, dpkg, and Nala. Adapters declare
the substrate versions and capabilities they understand. A substrate change
produces an explicit compatibility result and diagnostics; it does not cause
the canonical model to silently reinterpret old facts.

## Revisit triggers

Revisit when implementing the first read-only package inventory provider,
adding package mutation operations, adapting a non-Debian package system, or
discovering a fact that the initial model cannot represent without ambiguity.
