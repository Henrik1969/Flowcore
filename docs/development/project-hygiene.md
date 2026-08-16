# Project hygiene

This document describes how this repository separates active project material,
local development state, generated output, and historical archive material.

## Core rule

```text
Git is the project history.
The working tree is the current project cockpit.
Build and test artifacts are generated output.
Old experiments are archaeology, not garbage.
```

## Tracked project material

The following belongs in Git:

```text
source code
public headers
tests and test goldens
examples and standard-library files
maintained tools
governed evidence
documentation
project metadata needed for normal development
selected IDE cockpit files when intentionally maintained
```

## Local and generated material

The following should normally not be committed:

```text
build/
cmake-build-*/
test-report/
temporary and scratch output
generated dumps
local-machine reports
cache files
accidental nested directories
```

The repository may track selected IDE files when they intentionally describe
the development cockpit. For Flowmini, CLion exposes maintained CMake targets
such as `flowmini`, `flowmini_suite`, `flowmini_ast_golden_tests`,
`flowmini_symbol_projection_tests`, and `flowmini_frontend_bundle_tests`.
Therefore `.idea/` is not automatically generated exhaust in this repository.

Generated build directories, reports, caches, and temporary files remain local
unless a deliberate change promotes specific output into governed test,
release, or preservation evidence.

## Archive policy

Historical material may be preserved in one of two repository-local areas:

```text
_archive/
    material intentionally preserved in Git

_local_archive/
    local-only archaeology, reports, old build trees, and accidental debris
```

Use `_archive/` only when the material should remain part of repository
history. Use `_local_archive/` when it is useful locally but should not enter
project history. Age or generated appearance alone does not make material
disposable.

## Flowmini active-stage policy

The active implementation must be easy to find:

```text
Flowmini/flowmini_v25_symboltable_projection
```

`Flowmini/flowmini_v24_explicit_ast/` remains beside it intentionally as the
closed raw frontend/export checkpoint inherited by v0.25. Earlier stages live
under `_archive/flowmini/previous-stages/`. A preserved stage remains evidence,
not active implementation authority.

## Cleanup law

```text
Do not delete valuable project history.
Do not commit unclassified generated output.
Do not hide active work.
Archive old stages deliberately.
Keep the entrance obvious.
Keep the cockpit clean.
```
