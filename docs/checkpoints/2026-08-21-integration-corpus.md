---
title: Flowcore larger integration corpus
status: verified
date: 2026-08-21
---

# Purpose

The focused probes remain necessary for precise language and boundary
regression. This corpus adds larger multi-feature programs to expose failures
that only appear when several correct pieces interact.

| Program | Combined coverage | Result |
|---|---|---|
| `numeric_workbench.flow` | import, refined record-like values, lists, functions, loops, branching | complete chain passed |
| `matrix_workload.flow` | 4×4 arrays, nested loops, indexed reads/writes, accumulation, helper function | complete chain passed |
| `multi_target_service.flow` | libc ABI, records, shared functions, CLI and daemon targets | complete chain passed |

Each program passed:

```text
Flowmini -> Flowanalyst -> Flowparallel -> Flowoptimize
```

The optimizer reported the canonical graph as unchanged and applied or
considered the Boolean COO deduplication transform without changing program
semantics.

Run the corpus with:

```sh
tools/run-flowcore-integration-corpus.sh
```

This corpus is intentionally separate from the focused pass corpus and golden
fixtures. New programs should be added here when they represent realistic
cross-feature interactions rather than a single isolated rule.
