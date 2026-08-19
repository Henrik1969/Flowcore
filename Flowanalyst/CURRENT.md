---
title: Flowanalyst current status
status: active-development
---

# Flowanalyst current status

The first implementation establishes the sibling boundary and the
`flowanalyst.semantic_report` version 1 output contract.

Implemented checks:

- preserve and report frontend diagnostics from Flowmini;
- duplicate symbol names within one exported scope;
- declared type spelling resolution against built-ins and exported type-like
  symbols;
- identifier and call-name resolution through exported scope ancestry;
- resolved-name facts and semantic dependency edges;
- callable arity checking for resolved calls;
- refined-type base resolution and invariant binding checks;
- each named target must expose exactly one `main` procedure.

The report also exposes the first semantic analysis graph and Boolean sparse
matrix view. The accepted semantic bundle and green-flag gate remain planned
until the complete semantic check family and final integrity pass exist.

This is semantic analysis, not Graph IR construction, optimization, target
lowering, or execution.
