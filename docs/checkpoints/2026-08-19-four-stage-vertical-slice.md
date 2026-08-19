---
title: Flowcore four-stage vertical slice checkpoint
status: verified-development-checkpoint
date: 2026-08-19
---

# Flowcore four-stage vertical slice

This checkpoint records the first complete executable walk through the planned
Flowcore compiler model:

```text
Flowcore source
  -> FlowMini
  -> Flowanalyst
  -> Flowoptimize
  -> Flowlower
  -> LLVM IR
  -> ELF executable
  -> execution
```

## What is proven

The trial program is:

```flow
program empty_program_main

main {
}
```

FlowMini emits its versioned frontend bundle. Flowanalyst validates the bundle,
resolves the available semantic facts, and identifies the explicit
`empty_program_main` lowering profile. Flowoptimize accepts the semantic report
and crosses its versioned optimization boundary with no transforms. Flowlower
emits LLVM IR for that narrow, explicitly recognized profile. Clang compiles
the IR to an ELF executable, and the executable exits successfully with code
0.

## Verified commands

Build each sibling with CMake/Ninja, then run the complete trial pipeline:

```sh
./Flowmini/flowmini_v25_symboltable_projection/cmake-build-debug/flowmini \
  --dump-frontend-bundle \
  Flowlower/tests/empty_program_main.flow \
  | ./Flowanalyst/build/flowanalyst \
  | ./Flowoptimize/build/flowoptimize \
  | ./Flowlower/build/flowlower --emit-llvm /tmp/flowcore-trial.ll

clang /tmp/flowcore-trial.ll -o /tmp/flowcore-trial
/tmp/flowcore-trial
```

The resulting executable is an x86-64 ELF binary and returns 0.

The sibling-specific CTest gates also pass:

```text
Flowanalyst:  1/1 PASS
Flowoptimize: 1/1 PASS
Flowlower:    1/1 PASS
```

## Honest boundary statement

This is a vertical model checkpoint, not a claim of a production compiler.

FlowMini has the mature structural AST/SymbolTable export. Flowanalyst has the
initial semantic graph, name/type/call/refined-type checks, and diagnostics, but
the full semantic bundle and final green-flag integrity pass are still being
implemented. Flowoptimize currently performs no transformations. Flowlower
currently emits source-derived LLVM IR only for `empty_program_main`; other
programs report that IR has not been emitted.

The checkpoint is valuable because every border is explicit, inspectable,
versioned, independently consumable, and recoverable before deeper maturity
work begins.

## Next expansion

1. Complete Flowanalyst's semantic checks and accepted semantic-bundle export.
2. Add the final single-thread integrity pass and green flag.
3. Replace the optimizer boundary stub with one provenance-preserving
   transformation.
4. Expand Flowlower from the empty-main profile to a small real target subset.
5. Validate generated LLVM with the installed LLVM toolchain and extend the
   executable firetests.

