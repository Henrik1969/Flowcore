---
title: Flowcore Transformation and Revision Architecture
status: binding-boundaries-with-provisional-mechanisms
kind: architecture-note
tags:
  - flowcore
  - compiler
  - ast
  - graph-ir
  - transformation
  - optimization
  - policy
  - provenance
  - revision
---

# Flowcore Transformation and Revision Architecture

## Authority

The compiler-stage boundaries, domain responsibilities, target boundary,
separation of transformation/policy/provider concerns, stable published-state
rule, and provenance requirements are binding architectural decisions.

Exact identity spelling, revision numbering, snapshot/delta storage,
structural-sharing strategy, command-line interfaces, and provider APIs are
provisional mechanisms. They may change without weakening the binding laws.

## Purpose

This note records the intended architecture between semantic analysis and target
emission.

It does not specify a finished optimizer implementation.

It defines the boundaries and laws that current Flowmini work must preserve so
that future optimization, policy, Graph IR, target lowering, and plugin
implementations can evolve without rewriting the compiler around one early
implementation.

The governing development order is:

```text
analysis
  -> architecture
  -> contracts and invariants
  -> default implementation
  -> alternative implementations
```

## Core premise

Flowcore expresses programmer intent and source-visible architecture first.

The compiler is then free to derive a better equivalent computational form
before choosing a concrete target realization.

The programmer's spelling is not automatically the required execution
mechanism.

The compiler may transform implementation shape when it can prove that required
observable semantics are preserved.

Examples include:

```text
constant expression
    -> constant fold

provably dead flow
    -> remove it

linear recursion
    -> iterative candidate

independent work
    -> expose parallel topology

equivalent algebraic form
    -> choose according to policy and cost
```

A useful rule is:

> Preserve observable meaning. Do not preserve accidental mechanics merely
> because the programmer used them to express the intent.

## Default compiler stages

The intended default pipeline is:

```text
SOURCE / SEMANTIC DOMAIN
────────────────────────────────────────────

source
  -> lexer / TokenTree
  -> parser / AST builder
  -> complete raw AST
  -> structural SymbolTable projection
  -> semantic analysis / facts / contracts / diagnostics
  -> canonical semantic frontend state

FLOW DOMAIN
────────────────────────────────────────────

canonical Graph IR
  -> graph analysis
  -> pruning
  -> semantic/topology transformation
  -> cost reasoning
  -> policy-directed selection
  -> optimized Graph IR

TARGET BOUNDARY
════════════════════════════════════════════

optimized Graph IR
  -> target lowering
  -> target IR
  -> target-specific optimization
  -> emitter
  -> artifact
```

The default implementation may initially collapse some adjacent stages into one
executable path, but the boundaries are architectural contracts and must remain
recoverable.

The current `ModuleSpec`/FlowIR representation is not canonical Graph IR. It
remains a compatibility execution target below Graph IR until a future runtime
or backend consumes Graph IR or a later lowered representation directly.

## Domain responsibilities

### TokenTree

TokenTree records source structure and spelling.

```text
TokenTree remembers what the source looked like.
```

Later compiler stages may retain source provenance, but must not need to
reinterpret raw source syntax to recover semantic meaning.

### Semantic AST

The AST states what the source means.

It owns semantic structure, not preferred machine realization.

Examples:

```text
multiply remains multiply
call remains call
flow remains flow
condition remains condition
type reference remains a semantic type reference
```

The AST must preserve enough information for later analysis and Graph IR
construction without consulting the original source representation.

The AST must not gradually become a storage location for target decisions such
as instruction choice, register selection, CPU feature assumptions, or
scheduler decisions.

### Graph IR

Graph IR owns abstract computational topology.

It is where Flowcore can represent and reason about:

```text
nodes
ports
wires
signals
dependencies
ordering
fan-in
fan-out
joins
cycles
effects
contracts
resource relationships
possible parallelism
possible fusion or splitting
placement opportunities
```

Graph IR is not merely a conventional low-level control-flow graph. It must
retain the Flowcore concepts that make source-level architecture
compiler-visible.

### Target lowering

Target knowledge belongs beyond an explicit target boundary.

Above the boundary, reasoning may use target-independent facts such as:

```text
operation count
dependency depth
duplicate work
known constants
abstract storage requirements
parallel potential
effect constraints
```

Below the boundary, reasoning may use implementation facts such as:

```text
ISA
registers
SIMD width
calling convention
ABI
instruction availability
cache and memory characteristics
accelerators
target runtime requirements
```

A target-specific optimization must not silently become the definition of
Flowcore semantics.

## Transformation rules, policy, and providers

These are separate concepts.

### Transformation rule

A transformation rule establishes that one form may legally become another
under explicit conditions.

Conceptually:

```text
A <=> B
when conditions X hold
```

The transformation engine owns the question:

```text
What changes are semantically legal?
```

### Policy

Policy chooses between legal alternatives.

Examples:

```text
prefer throughput
prefer low memory
prefer exposed parallelism
prefer source-shape preservation for diagnostics
prefer conservative transformation
```

Policy owns the question:

```text
Which legal result do we prefer?
```

Policy must not redefine language semantics.

### Provider / strategy

A provider or strategy chooses machinery used to implement a stage.

Examples may eventually include:

```text
mutable arena graph
persistent graph
equality-saturation optimizer
conventional rewrite optimizer
research graph builder
debug-oriented builder
```

This owns the question:

```text
Which implementation performs the stage?
```

Policy and provider selection are orthogonal.

## Versioned intermediate representations

Authority: stable published identities and observable lineage are binding. The
concrete identity notation and storage mechanisms below are provisional.

Flowcore intermediate representations should support versioned transformation
history.

The preferred human-facing identity form is:

```text
Node 42:r7
```

meaning:

```text
entity identity: Node 42
revision:        7
```

The identity answers:

```text
What conceptual entity is this?
```

The revision answers:

```text
Which published state of that entity is this?
```

### Published revisions are stable

Once a revision is published, it does not change.

A later modification creates another revision:

```text
Node 42:r7
    -> transformation
Node 42:r8
```

Internal implementation may use mutation while constructing `r8`, but `r7`
remains observable history.

### Revision versus derivation

A revision represents the same conceptual entity in a changed state:

```text
Node 42:r7
    -> simplify
Node 42:r8
```

A derivation creates new conceptual entities:

```text
Node 42:r7
    -> split
Node 81:r1
Node 82:r1
```

with provenance such as:

```text
Node 81:r1 derived-from Node 42:r7
Node 82:r1 derived-from Node 42:r7
```

Fusion likewise creates a new entity derived from multiple ancestors.

The architecture should distinguish at least:

```text
revision-of
derived-from
replaces
```

More specific reasons such as split or fusion can be transformation metadata.

## Layered deltas and structural sharing

A new representation state does not require a full copy.

Conceptually:

```text
Snapshot S1 = Root + Delta 1
Snapshot S2 = S1   + Delta 2
```

Unchanged structure can be shared.

A delta may contain changes such as:

```text
replace entity revision
add entity
tombstone entity
change relationship
attach derived analysis/provenance
```

Deletion in a newer snapshot need not destroy historical state. It may be
represented as a tombstone or equivalent delta operation.

Implementations may periodically materialize or compact deep delta chains
without changing semantic identity.

## Branching transformation lineage

Transformation history is not required to be linear.

A snapshot may have multiple legal descendants:

```text
                    S12
                   /   \
                  /     \
        iteration       parallel
            |              |
           S13            S14
```

Policy or cost analysis may compare candidates and select one.

Retention of unselected branches is itself a policy decision.

This permits exploratory optimization without destructively erasing the common
ancestor.

## Snapshots

A snapshot is a coherent observable view of IR entity revisions.

It need not physically contain a complete copied graph.

Conceptually:

```text
GraphSnapshot 27
  root: GraphRoot 1
  visible revisions:
    Node 1:r4
    Node 2:r9
    Node 3:r2
    Wire 7:r5
```

Another snapshot may share almost everything and differ only through a small
delta.

Snapshots give passes and analyses stable inputs while later transformations
produce new descendants.

## Pass protocol

A transformation pass should conceptually follow:

```text
take published snapshot S

derive candidate delta D

construct candidate snapshot S'

validate required invariants

if valid:
    publish S'
else:
    reject D
    retain S
```

This provides natural rollback and isolates experimental or plugin passes from
corrupting a valid published input.

A pass contract should state:

```text
required input representation
required invariants
semantic changes it is allowed to make
invariants it preserves
invariants it establishes
diagnostics/provenance it emits
```

It should not prescribe storage details such as vectors, maps, arenas, heaps,
copy-on-write, or persistent maps unless a specific provider requires them.

## Provenance and explainability

Transformation history is first-class compiler data.

The compiler should be able to retain a lineage such as:

```text
source location
  -> AST entity/revision
  -> Graph entity/revision
  -> optimized Graph entity/revision
  -> Target IR entity/revision
  -> emitted artifact location
```

A transformation log can therefore explain:

```text
entity
old revision
new revision or derived entities
pass
reason
policy
relevant cost decision
diagnostics
```

The long-term goal is abstraction without opacity.

Optimization should be inspectable and forensically traceable rather than
becoming an unexplained black box.

## Replaceable stages

The default path is only the implementation shipped by the project.

It is not the definition of the architecture.

Future selection may include forms such as:

```text
--use-policy <policy>
--emit-for <target>
```

and may later expose alternative stage providers.

The architectural law is:

> The stage contract is stable enough that a different valid provider can
> replace the default without redefining adjacent stages.

## Sane default direction

Authority: provisional engineering default, constrained by the binding laws
above.

Until experiments prove otherwise, a reasonable default direction is:

```text
AST
  explicit semantic structures
  stable identity where transformation/provenance benefits from it
  source provenance retained

Graph IR
  stable opaque entity IDs
  revisioned published states
  shared ancestry through layered deltas
  controlled pass-local construction/mutation
  invariant validation before publication

Pipeline
  externally transformational
  internally free to use efficient mutation and structural sharing

Policy
  selects desirable legal transformations

Provider
  selects implementation machinery
```

This is a default engineering choice, not a permanent language law.

## Meta-method

The architecture itself was reached through iterative problem-solving:

```text
state current model
  -> challenge assumptions
  -> construct counterexample
  -> revise model
  -> compare alternatives
  -> retain stronger model
```

This is a useful project method in its own right.

Design discussions are not merely commentary around implementation. They are
iterative reasoning tools used to expose assumptions before those assumptions
become expensive code.

## Governing laws

1. Syntax provenance survives, but syntax does not govern later stages.
2. Semantic AST expresses meaning, not preferred realization.
3. Graph IR owns abstract computational topology.
4. Transformations establish legal equivalence; policy chooses preferences.
5. Target-specific facts enter only beyond an explicit target boundary.
6. Stage contracts are stronger than stage implementations.
7. Published IR revisions are stable.
8. Transformations may branch from shared ancestry.
9. Provenance and revision lineage are first-class compiler data.
10. The default implementation is a provider, not the architecture itself.
