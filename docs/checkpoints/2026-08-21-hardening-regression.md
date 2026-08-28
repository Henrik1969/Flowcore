---
title: Flowcore hardening regression
status: verified-with-environment-note
date: 2026-08-21
---

# Evidence

The clean root superbuild was rebuilt with AddressSanitizer and
UndefinedBehaviorSanitizer:

```sh
cmake -S . -B /tmp/flowcore-asan -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_C_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer'
cmake --build /tmp/flowcore-asan
ASAN_OPTIONS=detect_leaks=0 LSAN_OPTIONS=detect_leaks=0 \
  ctest --test-dir /tmp/flowcore-asan --output-on-failure
```

The selected chain, integration, provider, and optimizer gates passed 13/13
under ASan/UBSan.

LeakSanitizer cannot run in the current Codex host because ptrace restrictions
are active. Its first attempt failed before meaningful program execution with
`LeakSanitizer has encountered a fatal error`; rerunning with leak detection
disabled retained ASan/UBSan coverage and passed. This is an environment
limitation, not evidence of a memory error.

Valgrind Memcheck was also available and the Flowoptimize version-path smoke
test passed with zero errors and zero bytes still allocated at exit.
