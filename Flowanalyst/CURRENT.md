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
matrix view. It is the current green-flag input for downstream stages when its
status is `ok`; the final integrity pass and broader semantic check family are
still expansion work.

Flowanalyst does not construct Graph IR, optimize, select named target
artifacts, or execute code. It establishes the semantic facts and consumer
contract those later stages use.
