# Project Hygiene

This document describes how this repository separates active project material, local development state, generated output, and historical archive material.

## Core rule

```text
Git is the project history.
The working tree is the current project cockpit.
Build/test artifacts are exhaust.
Old experiments are archaeology, not garbage.
Tracked project material

The following belongs in Git:

source code
public headers
tests
test goldens
examples
standard library files
maintained tools
documentation
project metadata needed for normal development
selected IDE cockpit files when intentionally maintained
Local/generated material

The following should normally not be committed:

build/
cmake-build-*/
test-report/
temporary output
scratch reports
generated dumps
local machine reports
cache files
accidental nested directories
IDE project files

This repository may track selected IDE project files when they describe the intended development cockpit.

For Flowmini, CLion is an important part of the workflow because the project exposes buildable CMake targets such as:

flowmini
flowmini_suite
flowmini_ast_golden_tests

Therefore .idea/ is not automatically considered garbage in this repository.

Generated build directories, reports, caches, and temporary files remain project exhaust and should not be committed.

Archive policy

Old work should not clutter the active cockpit.

Historical material can be archived in one of two ways:

_archive/
    repository archive for material intentionally preserved in Git

_local_archive/
    local-only archive for private/untracked archaeology, generated reports, old build trees, and accidental debris

Use _archive/ only when the archived material should remain part of the repository.

Use _local_archive/ when the material is useful locally but should not enter project history.

Flowmini active stage policy

The active Flowmini implementation should be easy to find.

Current active stage:

Flowmini/flowmini_v24_explicit_ast

Older Flowmini implementation stages should not sit beside the active stage unless they are intentionally part of the current project surface.

Cleanup law
Do not delete valuable project history.
Do not commit generated exhaust.
Do not hide active work.
Archive old stages.
Keep the entrance obvious.
Keep the cockpit clean.

