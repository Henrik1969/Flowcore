---
title: Flowcore four-stage vertical slice checkpoint
status: verified-development-checkpoint
date: 2026-08-19
---

# Flowcore four-stage vertical slice

This checkpoint records the first complete executable walk through the current
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

The first trial program was:

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

The resulting executable is an x86-64 ELF binary and returns 0. The current
application proof is the packaged `flowcat` example, which emits a native ELF
binary and prints its command-line arguments through an authorized `puts`
capability.

The sibling-specific CTest gates also pass:

```text
  Flowanalyst:  1/1 PASS
Flowbind:     provider and fuzz gates PASS
Flowoptimize: 1/1 PASS
Flowlower:    pipeline matrix and executable profiles PASS
flowcat:      complete example runner PASS
```

## Honest boundary statement

This is a vertical model checkpoint, not a claim of a production compiler.

FlowMini has the structural AST/SymbolTable export. Flowanalyst has the initial
semantic graph, name/type/call/refined-type/record checks, diagnostics, and
target entrypoint checks. Flowbind verifies and authorizes selected external
capabilities. Flowoptimize currently performs no transformations. Flowlower
emits only explicitly accepted profiles; `flowcat_argv_main` is the first
application profile.

The checkpoint is valuable because every border is explicit, inspectable,
versioned, independently consumable, and recoverable before deeper maturity
work begins.

## Next expansion

1. Complete the final semantic integrity pass and green flag.
2. Add explicit target selection and separate artifacts for named targets.
3. Replace the optimizer identity boundary with provenance-preserving
   transformations.
4. Expand the standard library and file-content I/O beyond `flowcat` argv output.
5. Add policy-directed parallelism, CUDA providers, and bootstrap/self-hosting
   stages.
