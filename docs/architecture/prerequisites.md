---
title: Flowcore and Flowmini Prerequisites
status: provisional-language-design
kind: architecture-note
authority:
  binding:
    - prerequisite-versus-import distinction
    - explicit-environment-contract rule
    - declare-resolve-verify lifecycle boundary
  provisional:
    - source syntax
    - prerequisite kinds
    - version-constraint syntax
    - provider and preparation mechanisms
---

# Flowcore and Flowmini Prerequisites

## Authority

The architectural rule in this note is binding: hidden environmental
assumptions should become explicit prerequisite contracts. The concrete syntax,
complete kind taxonomy, provider mechanism, and version syntax remain
provisional until separately approved and implemented.

## Purpose

A `prerequisite` declares an external condition that must be satisfied before a
program, unit, ABI binding, test, runtime stage, or build profile can be used.
A prerequisite is not an import.

```text
import
    brings source declarations into scope

prerequisite
    declares environment, build, or runtime requirements
```

The purpose is to replace hidden local assumptions with explicit contracts and
structured diagnostics.

## Conceptual lifecycle

```text
declare
    Source declares what external capability is needed.

resolve
    Build or runtime policy searches for matching providers.

verify
    A candidate is checked against kind, family, version, implementation,
    symbols, ABI, platform, permissions, and other declared constraints.

prepare
    Tooling may build, copy, link, mount, configure, or otherwise prepare it.

execute
    Execution begins only after required prerequisites are valid.

fail
    Failure is structured and explanatory.
```

The lifecycle boundaries are architectural. The machinery implementing them is
provider-specific.

## Provisional syntax sketches

These examples communicate intent; they are not approved language syntax.

```flow
prerequisite shared_library "flowmini_testabi"
prerequisite command "ffmpeg" version >= "6.0"
prerequisite package "sqlite3" version >= "3.40"
```

```flow
prerequisite shared_library testabi {
    family: "flowmini_testabi"
    version >= "1.0.0"
    version < "2.0.0"
    implementation: "flowmini-testabi-c"
    symbols: [
        "point_sum",
        "point_weighted_sum"
    ]
}
```

Candidate comparison operators are `==`, `!=`, `>`, `>=`, `<`, and `<=`.
Range notation may later be accepted as sugar but should lower to explicit
constraints.

## Candidate prerequisite kinds

The taxonomy remains provisional:

```text
shared_library
static_library
command
file
directory
package
service
capability
abi
runtime
toolchain
compiler
platform
kernel_feature
device
network_endpoint
permission
```

## Build-time and runtime meaning

The same declaration may participate at both boundaries. Build and test tooling
may prepare a provider; runtime may verify that the selected provider still
satisfies its contract.

## Current Flowmini example

The current ABI examples assume that
`std/abi/testabi.flow` can use `./build/libflowmini_testabi.so`. A future
prerequisite contract should name the required library family and symbols while
allowing policy to resolve a concrete build-tree or installed provider.

## Governing rule

> Hidden environmental assumptions should become explicit prerequisites.
