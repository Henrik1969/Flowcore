---
title: Flowcore root CMake/Ninja superbuild
status: verified
date: 2026-08-21
---

# Result

The repository now has a canonical additive root `CMakeLists.txt`. It assembles
the existing Flowmini, Flowanalyst, Flowbind, Flowoptimize, Flowlower,
Flowkernel, Flowparallel, and Frankencore reference-tool projects without
changing their individual build contracts.

The root build also wires clean-tree cross-stage tests for:

- the larger three-program integration corpus;
- the accepted/blocked pipeline matrix;
- the 43-program pass corpus.

Verification from a fresh Ninja build directory:

```text
root CTest: 43/43 passed
integration corpus: 3/3 passed
pipeline matrix: 9 accepted, 1 blocked
pass corpus: 43 passed
architecture check: PASS
```

The first full CTest run exposed that registered test executables were not
part of the initial target build. Building the complete root target resolved
that honestly; the subsequent full run passed all 42 tests. This is now part
of the expected clean-tree procedure:

```sh
cmake -S . -B /tmp/flowcore-build -G Ninja
cmake --build /tmp/flowcore-build
ctest --test-dir /tmp/flowcore-build --output-on-failure
```
