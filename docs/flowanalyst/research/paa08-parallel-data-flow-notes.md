---
title: Flowanalyst research notes — parallel data-flow analysis
status: research-input
source: /home/henrik/Hentet/paa08.pdf
---

# Parallel data-flow analysis: applicability to Flowanalyst

Source inspected: `paa08.pdf`, *A Parallel Approach for Solving Data Flow
Analysis Problems*, Marcus Edvinsson and Welf Löwe, 2010.

This is research input, not yet a binding implementation requirement. The
paper studies parallel data-flow analysis over SSA-based program graphs and
introduces Process-Order-Graphs (POGs).

## Useful ideas for Flowanalyst

### 1. Make semantic dependencies explicit

Analysis work should be represented as nodes with explicit dependency edges.
Independent nodes or regions can then be analysed concurrently, while the
dependency graph prevents conclusions from being consumed too early.

For Flowanalyst, candidate nodes may eventually represent:

```text
type resolution
name resolution
function contract checking
target dependency checking
ABI validation
refined-type checking
effect/resource analysis
```

The semantic report should retain these relationships so consumers can explain
why a region is sane, rejected, or blocked by another region.

### 2. Treat cycles as stabilization regions

The paper uses strongly connected components (SCCs) to identify cycles and
stabilize inner cycles before propagating results to their successors.

This maps naturally to Flowcore dependency analysis:

```text
dependency graph
  -> strongly connected components
  -> acyclic component graph
  -> local fixpoint analysis
  -> propagate stable facts
```

Recursive functions, mutually recursive targets, cyclic type relationships,
and dependency cycles should therefore be represented as explicit analysis
regions rather than causing uncontrolled diagnostic cascades.

### 3. Parallelize independent layers, not arbitrary work

POGs extend loop trees with dependency edges and containment edges. Nodes in the
same dependency layer may be analysed in parallel; cyclic containers are
re-entered until their facts stabilize.

This supports our intended model:

```text
parallel regional analysis
  -> merge facts
  -> stabilize cyclic regions
  -> final single-thread integrity pass
```

The final pass remains valuable because it validates the merged result and
detects conflicts or inconsistent assumptions between analyzers.

### 4. Work and parallel depth are separate concerns

The paper distinguishes total analysis work from parallel time and measures
queue width, dependency depth, speed-up, and processor efficiency. Flowanalyst
should eventually record similar internal metrics, but must not expose them as
semantic truth in the stable report unless explicitly versioned.

Useful future telemetry includes:

```text
analysis region count
dependency edge count
SCC count and maximum depth
fixpoint iteration count
parallel layer width
blocked-region count
```

## Limits and precautions

The paper analyses context-insensitive data-flow problems over SSA graphs. That
does not directly define Flowmini semantics or prove that every Flowanalyst
check is safely parallelizable.

We must therefore preserve these rules:

- semantic facts are immutable snapshots or transactional results;
- analyzers publish facts only after their declared prerequisites are stable;
- cyclic regions use explicit convergence/fixpoint rules;
- diagnostic identity and provenance are independent of execution order;
- parallel and serial runs must produce equivalent canonical reports;
- the final integrity pass remains authoritative for lowering eligibility.

## Architectural conclusion

The paper strengthens, rather than changes, our existing design:

```text
FlowMini structural bundle
  -> Flowanalyst dependency/region graph
  -> parallel semantic checks
  -> merged semantic facts and diagnostics
  -> stabilization and integrity pass
  -> consumer projections or lowering eligibility
```

The next implementation step should be an internal analysis-region and
dependency model, before adding broad parallel execution.

