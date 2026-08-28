---
title: Revisioned Node Identity
status: binding-direction-provisional-mechanisms
kind: architecture-note
scope: Flowcore and FrankenCore model direction
implementation: not implemented in active Flowmini v0.25
---

# Revisioned Node Identity

## Authority

Stable identity, observable published revisions, non-destructive history, and
provenance are binding architectural direction. Exact ID spelling, allocation,
revision numbering, diff storage, branch naming, and persistence mechanisms are
provisional.

## Purpose

Important structural entities should preserve identity while their state
evolves. This applies especially to future AST nodes, Graph IR entities,
semantic facts, diagnostics, editor state, runtime planning state, and
provenance records.

The goal is to make change visible, branchable, and inspectable instead of
destructive and hidden.

## Core model

Classic in-place mutation erases the previous state. Eager copying retains
state but can obscure identity and duplicate whole structures. Flowcore should
support a third model:

```text
stable identity
revisioned published state
non-destructive change
branchable roots
```

A mutation produces a new revision rather than rewriting an already published
state.

An illustrative human-facing form is:

```text
Node 42:r7
Node 42:r8
```

The notation is provisional. The distinction is binding: entity identity says
what the entity is; revision says which observable state is being referenced.

## Branching and structural sharing

Several descendants may remain inspectable:

```text
root A -> Node 42:r7
root B -> Node 42:r8
```

A provider may represent a new view through a delta over shared structure
instead of copying the complete tree or graph. Diff layers, tombstones,
copy-on-write, persistent maps, and arena snapshots are possible mechanisms,
not architectural requirements.

## Why this matters

Revisioned identity supports:

- structural debugging and logging;
- AST and Graph IR transformation history;
- diagnostics tied to exact states;
- editor undo and redo;
- branchable compilation experiments;
- provenance-aware transformations;
- speculative lowering and optimization.

## Relationship to Flowmini v0.25

Flowmini v0.25 inherits v0.24's arena-owned declaration, statement, block, and
expression IDs to avoid raw-pointer ownership and provide stable references
within one exported AST revision. Its typed structural origins make derivation
provenance independently inspectable. Those pool IDs and origins are not yet
the full cross-edit revision model.

Do not force revision machinery into the active v0.25 projection milestone.
Current choices must nevertheless avoid making future stable identity or
mutation provenance impossible.

## Relationship to Graph IR

Canonical Graph IR is the natural first major consumer of revisioned identity.
Nodes, ports, wires, contracts, and policy envelopes may each need stable
identity, published revisions, and derivation lineage during analysis,
transformation, placement, and lowering.

## Relationship to diagnostics

Diagnostics should eventually identify both source provenance and the precise
structural revision they describe. This prevents ambiguity when several legal
or historical states of one conceptual entity remain observable.

## Deferred implementation questions

- How are identities allocated?
- Are revision counters entity-local or representation-global?
- How are deltas, snapshots, and branches stored?
- How is provenance retained across derivation, fusion, and splitting?
- How do diagnostics refer to superseded revisions?
- How does the model interact with incremental compilation and runtime state?

## Current decision

Preserve the revisioned-identity law and defer its concrete machinery. Continue
v0.25 with stable-within-one-export arena IDs and explicit structural origins
while maturing the SymbolTable projection boundary. Define the runtime mutation
provenance contract separately before selecting storage machinery.
