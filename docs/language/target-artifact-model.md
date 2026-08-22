---
title: FlowMini Target and Artifact Model
status: binding-direction-staged-implementation
---

# FlowMini Target and Artifact Model

FlowMini describes a shared source universe. Targets project selected parts of
that universe into products. “Program” and “library” therefore describe target
artifacts, not exclusive identities of the source.

```text
source universe
    -> AST
    -> SymbolTable
    -> semantic contracts and dependency graph
    -> selected target closure
    -> artifact
    -> native executable, library, binding, plugin, or service
```

## Structural frontend responsibility

The AST/SymbolTable boundary may record:

- target names and source locations;
- target-local declarations and ownership;
- optional target entrypoints;
- declared prerequisites and written API/ABI facts;
- provenance from projected symbols and scopes back to source structure.

It must not record backend realization decisions such as instruction selection,
register allocation, platform ABI lowering, or memory layout.

## Artifact responsibility

Later stages select a target and compute its dependency closure. The resulting
artifact may contain:

- a manifest and dispatcher;
- versioned capability lookup tables;
- API and ABI contracts;
- dependency and provenance metadata;
- lazily materialized implementation sections.

Multiple versions of a capability may coexist when their compatibility
contracts permit it. A consumer selects by capability identity, API version,
ABI, and policy. Removal is safe only after no selected target references the
deprecated implementation.

## Binding responsibility

Bindings are projections of exported contracts and dependency metadata. They
must not infer public interfaces by inspecting private implementation code.
The same target closure may produce C, C++, Lua, Python, FlowMini, editor, or
remote/service bindings as separate ecosystem projections.

## Boundary rule

FlowMini v0.25 accepts one anonymous root `main` or multiple named targets.
Flowanalyst validates named-target entrypoint shape. Flowlower requires an
explicit `--target` when a report contains multiple targets and attributes each
emitted LLVM artifact to that selection. Runtime lazy loading, dispatcher
implementation, and dependency resolution remain downstream work.
