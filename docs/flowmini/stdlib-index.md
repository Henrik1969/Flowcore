# Flowmini standard-library status

This index lists only facilities present in the checkout and covered by a
test or verified probe.

| Unit | Status | Current surface | Evidence |
| --- | --- | --- | --- |
| `std/math.flow` | IMPLEMENTED / TESTED | `identity`, `add`, `abs`, `square`, `cube`, `factorial`, `min`, `max`, `clamp` over `int` | `flowmini_stdlib_math` |
| `std/numbers.flow` | EXPERIMENTAL | refined integer aliases and invariants | AST/runtime fixtures |
| `std/abi/libc.flow` | EXPERIMENTAL | declared C ABI/provider examples | provider contract tests |
| `std/file_io.flow`, `std/memory.flow`, `std/kernel.flow` | EXPERIMENTAL | provider-facing declarations | provider/kernel probes |
| `std/text`, `std/collections`, `std/io`, `std/fs`, `std/time`, `std/net` | NOT SUPPORTED — DEFERRED | no stable general API in this release | mature-probe and generic-pressure evidence |

Pure Flowmini semantics and capability-backed implementations remain separate.
Existing system libraries are preferred providers where they already solve the
problem; Flowmini source does not embed their implementation.
