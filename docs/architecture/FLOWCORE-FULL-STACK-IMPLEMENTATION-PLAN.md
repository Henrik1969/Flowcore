---
title: Flowcore and Frankencore full-stack implementation plan
status: active-consolidated-plan
date: 2026-08-21
---

# Purpose

This document reconciles the original Flowcore language-chain promise with the
current Frankencore constitution, hardening plan, implementation roadmap,
runtime decisions, and algebraic-provider work.

The governing pattern is:

```text
canonical data and meaning
  -> functional capability bricks
  -> policy and provenance
  -> independent projections/providers
```

No later phase may silently collapse these layers.

# Non-negotiable laws

1. Semantic identity and canonical graph meaning precede representation.
2. Graph IR is canonical; matrix, JSON, LLVM, CLI, GUI, CUDA, and ELF are
   projections or providers.
3. Compile time proves legality; runtime discovers local capability; policy
   decides permission and cost; providers execute; reports explain decisions.
4. Every stage is independently consumable through a versioned contract.
5. Every provider has explicit unavailable, failed, fallback, and recovery
   semantics.
6. CPU fallback remains valid whenever an accelerator is absent, unsuitable, or
   rejected by policy.
7. Provenance follows every derived artifact and decision.
8. Policies are additive and configurable; ambiguity becomes an explicit
   unresolved decision, not an implicit hardcoded guess.
9. Frankencore remains substrate-oriented and optional; Flowcore is a powerful
   consumer, not a constitutional dependency.
10. No optimization is accepted without an independent correctness baseline.

# Current proven vertical slice

```text
Flowmini
  -> Flowanalyst
  -> Flowparallel
  -> Flowoptimize
  -> Flowlower
  -> LLVM
  -> native ELF
```

Proven narrow capabilities include AST/SymbolTable export, semantic reports,
ABI binding verification, graph and Boolean COO matrix projection, pure/disjoint
parallel candidates, CPU execution, CUDA/cuBLAS execution, runtime capability
facts, hostile CUDA testing, and narrow LLVM lowering.

# Maturity gates

## Gate A — language truth

Complete the language matrix without widening semantics accidentally:

- syntax and lossless AST coverage;
- SymbolTable identity, scope, origin, and source mapping;
- declared/refined types and contracts;
- name, call, target, effect, and ABI resolution;
- partial analysis with isolated diagnostics;
- final aggregate semantic pass;
- multiplex targets and per-target status;
- independent consumer contract and versioned reports.

Exit condition: every accepted construct has a semantic fact or an explicit
unsupported/unresolved result; no accepted construct reaches lowering through a
generic unknown path.

## Gate B — canonical graph and algebra

- make graph IR an explicit typed contract;
- preserve graph provenance through all projections;
- define graph operations independently of storage;
- implement one CPU reference operation;
- implement one CUDA provider for the same operation;
- compare providers against the reference on hostile graph shapes;
- add sparse/dense and transfer-aware calibration.

First operation: dependency reachability or dependency layering over the
existing Boolean `region_dependency` graph. Do not begin with a broad algebra
library.

## Gate C — runtime decision layer

- consume execution plan, capability snapshot, active policy, and calibration;
- emit a versioned explainable provider decision;
- select CUDA only when available, contract-compatible, and measurably better;
- select CPU on missing capability, insufficient benefit, policy denial, or
  provider failure;
- preserve decision inputs and fallback in the report;
- never infer hardware from the build machine.

## Gate D — optimization

- implement identity-preserving graph transforms one at a time;
- prove transform preconditions in Flowanalyst;
- emit transformed graph plus derivation/provenance, never overwrite input;
- run CPU reference differential tests after every transform;
- permit runtime specialization only as an additive projection.

## Gate E — lowering and language production

- expand lowering profiles from examples into explicit capability contracts;
- implement multiplex target selection and independent artifact emission;
- support libraries and programs as the same target graph with selected roots;
- lower common/runtime capabilities through bindings;
- preserve ABI, version, dependency, and deprecation expectations;
- add portable runtime/VM or C++ emitter only after graph and provider gates.

## Gate F — self-hosting and build stack

- establish a canonical CMake/Ninja superbuild;
- remove sibling build-path coupling;
- make Flowtools/CLion consume imported stage targets;
- build the chain from clean trees;
- progressively replace host-language stage implementations with Flowcore
  implementations only after differential parity;
- retain bootstrap escape hatches until self-hosting is independently verified.

## Gate G — Frankencore substrate and ecosystem

In the roadmap order:

1. freeze shared vocabulary and status/error contracts;
2. complete ConfigResolve policy integration;
3. finish read-only package/substrate inventory;
4. delegate native verification without inventing trust;
5. implement canonical language maps and round trips;
6. evaluate chain policies and version ranges;
7. add transparent `ls` facade with differential tests and rollback;
8. add package mutation only after dry-run, authorization, provenance, and
   recovery are proven;
9. sign and distribute maps/policies/artifacts;
10. broaden substrates, facades, displays, IDE, AI, and remote providers.

# Immediate execution order

The next concrete slices are:

Completed in the current maturation pass:

1. Runtime provider decision planner, schema fixtures, and negative tests.
2. Graph reachability CPU reference and CUDA provider with differential gate.
3. Sparse/dense/transfer calibration and threshold policy.
4. First explainable identity-preserving optimizer transform.
5. Multiplex-target selection contract and tests.
6. Root CMake/Ninja superbuild and clean-tree chain test.

Next:

7. Independent per-target artifact emission.
8. Declared standard-library boundary and capability artifact (completed for
   the current narrow surface; libc/file I/O/memory/kernel binding-ready,
   type-only pointers inventoried, aggregate ABI explicitly deferred).

Next maturation slice:

9. Versioned aggregate ABI/layout evidence for the internal struct provider;
   keep unsupported aggregate calls blocked until provider and lowerer checks
   are implemented.

# Permanent evidence requirements

Every slice requires normal tests, malformed-input tests, deterministic JSON,
provenance assertions, CPU fallback, ASan/UBSan where applicable, Valgrind or
documented environment compensation, Firetest attacks, and a written result.
Hardware benchmarks are local evidence and must never become universal
compile-time assumptions.
