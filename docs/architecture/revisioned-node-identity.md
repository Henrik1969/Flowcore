# Revisioned Node Identity

Status: architecture note  
Scope: Flowcore / FrankenCore model direction  
Implementation status: not implemented in Flowmini v0.24

## Purpose

Flowcore should prefer stable identity with revisioned state for important structural entities.

This applies especially to future representations such as:

```text
AST nodes
Graph IR nodes
semantic facts
diagnostics
editor state
runtime planning state
provenance recordThe goal is to make mutation visible, branchable, and inspectable instead of destructive and hidden.

Core idea

Classic mutation often means:

node changes in place
old state disappears

Classic copying often means:

entire structure is copied
new structure diverges from old structure
identity becomes harder to track

Flowcore should explore a third model:

stable identity
revisioned state
diff-layered mutation
branchable roots

In this model, mutation does not erase prior meaning.

A mutation creates a new revision.

Identity and revision

A useful human-readable form is:

NodeId 42 revision 7
NodeId 42 revision 8

Meaning:

NodeId 42
    stable entity identity

revision 7
    one particular state of that entity

revision 8
    a later or alternative state of that entity

The identity remains stable.

The state becomes revisioned.

Branching

When something mutates, the system can preserve both the old and new forms.

Conceptually:

root A
    NodeId 42 revision 7

root B
    NodeId 42 revision 8

This allows the old and new state to become separate inspectable branches.

The important point is not only that history exists. The important point is that history remains structurally meaningful.

Diff-layered mutation

Instead of eagerly copying an entire tree or graph, a new revision may be represented as a diff layer over an existing structure.

Conceptually:

base structure
    +
diff layer
    =
revisioned view

This means a mutated view can exist without destroying the original.

It also means several views may share most of their structure while still having distinct identities and revision numbers.

Why this matters

Revisioned identity can support:

clear logging
structural debugging
AST transformation history
Graph IR lowering history
semantic diagnostics tied to exact revisions
editor undo/redo models
branchable compilation experiments
provenance-aware transformations
parallel speculative lowering

The design goal is to make system change inspectable.

Architecture law
Flowcore entities should prefer stable identity with revisioned state.

Mutation should not erase prior meaning.

A mutation creates a new revision.

Revisions may branch.

Diff layers may represent change without copying the entire structure.
Relation to AST

For AST work, this suggests that future nodes may eventually carry or reference:

stable node identity
revision identity
source location
structural role
payload/state
parent/child relationships
provenance or transformation origin

Flowmini v0.24 does not implement this yet.

The current v0.24 AST work remains focused on:

explicit AST structure
observable AST dumps
golden regression tests
shallow expression graph population
future recursive expression population

Revisioned AST identity should not be forced into v0.24 prematurely.

Relation to Graph IR

Graph IR is a natural home for revisioned identity.

Future Graph IR nodes, ports, wires, and policy envelopes may benefit from stable identity plus revisioned state.

Examples:

NodeId 42 revision 7
PortId 42.output.value revision 3
WireId 88 revision 2
PolicyEnvelopeId 11 revision 5

This would allow Flowcore to log and inspect how a graph changes during semantic analysis, lowering, optimization, placement, or runtime planning.

Relation to diagnostics

Diagnostics should eventually be able to refer to precise structural revisions.

Example:

Diagnostic:
    node: NodeId 42 revision 8
    source: main.flow:12:5
    message: incompatible port contract

This avoids ambiguity when the same logical node has several transformation states.

Deferred implementation questions

Open questions:

How are NodeIds allocated?
Are revision numbers local to each node or global?
Are revisions immutable once created?
How are diff layers stored?
How are branches named?
How are source locations preserved across revisions?
How do diagnostics refer to obsolete revisions?
How does this interact with incremental compilation?
How does this interact with runtime state?

These questions are intentionally deferred.

The current purpose is to preserve the design law before implementation pressure forces premature decisions.

Current decision

Do not implement revisioned identity in Flowmini v0.24 yet.

Document the model.

Continue v0.24 AST work with simple structures.

Use this note to guide future AST, Graph IR, diagnostics, and provenance design.
