---
title: Flowparallel and algebraic provider audit
status: current-experimental
date: 2026-08-21
---

# Current audit

## Proven

| Area | Evidence | Maturity |
|---|---|---|
| FlowMini frontend bundle | AST/SymbolTable/provenance gates | strong narrow boundary |
| Flowanalyst semantics | reports, diagnostics, regions, graph and Boolean COO matrix | strong narrow boundary |
| Flowparallel planning | versioned plan, pure/disjoint candidates, serial fallback | experimental but tested |
| Runtime provider planner | plan + capability + policy + calibration → explainable decision | host-verified |
| CPU graph reference | Boolean `region_dependency` reachability with malformed-input gate | verified |
| CUDA graph provider | cuBLAS Boolean-threshold reachability, CPU differential | host-verified |
| Graph representation planner | explicit sparse/dense policy, runtime capability and calibration gate | tested |
| Graph transfer calibration | CPU/CUDA timings and observed speedup emitted by CUDA graph provider | host-verified; small graph correctly rejected by policy |
| CUDA graph shape firetest | empty, chain, cycle, and dense graph differential | host-verified 4/4 |
| Flowoptimize decision consumption | verified provider decision accepted as identity-preserving policy input; unknown provider blocked | tested |
| Flowoptimize COO transform | duplicate Boolean COO entries removed with proof and before/after counts | tested |
| Larger language integration corpus | three multi-feature programs through Flowmini → Flowanalyst → Flowparallel → Flowoptimize | verified 3/3 |
| Root CMake/Ninja superbuild | clean sibling assembly with cross-stage corpus and full CTest registration | verified 43/43 |
| Multiplex target selection | named targets preserved through the chain; Flowlower requires explicit selection | verified |
| Per-target artifact emission | separate target-specific LLVM artifacts for accepted narrow profile | verified |
| Sanitizer hardening | clean root ASan/UBSan chain regression; Valgrind smoke | ASan/UBSan 13/13; Valgrind clean |
| Standard-library capability boundary | all six declared `std/abi` modules inventoried; libc, file I/O, memory, and the 23-symbol Flowkernel matrix authorized; `getpid` and `clock_gettime` have narrow native profiles; aggregate calls remain an explicit lowering gate | verified boundary; 6/6 modules, 4 binding-ready |
| Representative graph calibration | sparse/dense sizes 4, 8, and 16 | host-verified 6/6; CPU retained |
| CPU execution | approved independent-task API and failure propagation | experimental |
| Runtime discovery | Frankencore CPU/memory/CUDA capability schema and probe | experimental |
| CUDA availability | real RTX 5060 Ti, driver 580.173.02, device count 1 | host-verified |
| CUDA execution | dynamic CUDA Runtime/cuBLAS, numerical verification | host-verified |
| CUDA hostile testing | 27/27 executions, nine sizes, three repetitions | passed with one serializer correction |
| Performance evidence | CPU/CUDA end-to-end size sweep | local evidence |
| Flowoptimize | matrix projection and runtime provider policy | identity stage plus policy metadata |
| Flowlower | narrow LLVM-to-ELF profiles | working narrow boundary |
| Frankencore architecture | architecture, conformance, and mutation-provenance checks | passed prototype laws |

## Important limits

The optimizer does not yet transform or execute arbitrary Flowcore graph
matrices. The current CUDA workload is a controlled cuBLAS matrix benchmark,
not general graph algebra. The CPU reachability reference and CUDA graph
provider now exist, with differential comparison. The runtime planner selects
providers explainably, but provider invocation from arbitrary graph plans
remains a separate gate.

The current matrix is Boolean COO, while cuBLAS operates on dense floating
point matrices. The graph planner now has an explicit sparse/dense selection
policy and Boolean-threshold semantics, with CPU differential verification.
Transfer-aware cost calibration remains necessary before this policy can be
called production-grade. The current four-node host probe verified the graph
result but measured CUDA at roughly `0.000005x` end-to-end speedup, so the
conservative policy correctly retains the CPU provider.

The root superbuild is now available and the full root suite is green. Sibling
tests still retain source-tree defaults for focused development. Shared
report/result/error types and a unified provider-selection explanation contract
are not yet implemented. The standard-library boundary is intentionally not a
claim of all glibc: aggregate ABI layout and broader host-library coverage
remain separate gates.

## Verified commands

```text
pipeline matrix:       9 accepted, 1 blocked
pass corpus:            43 programs
Flowparallel CTest:     8/8
root CTest:              43/43
Flowoptimize CTest:     1/1
architecture check:     PASS
conformance laws:       PASS
mutation provenance:    PASS
CUDA firetest:          27/27
CUDA graph firetest:    4/4 shapes
Graph calibration:      6/6 cases; CPU retained
```

## Recommended next order

1. Complete the aggregate-ABI gate with versioned layout/signature evidence;
   keep unsupported structs blocked until that evidence exists.
2. Extend calibration over representative sparse/dense sizes and record
   transfer/launch break-even evidence; keep thresholds runtime-derived.
3. Only then widen the algebra catalog and connect transformed results to
   Flowoptimize/Flowlower.
4. Separately remove remaining build-path coupling and introduce shared,
   versioned report/result/error types.

The conservative conclusion is that the architecture has crossed from
capability demonstration into evidence-backed experimentation, but not yet
into a general automatic graph optimizer.
