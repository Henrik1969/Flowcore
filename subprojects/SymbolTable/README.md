# SymbolTable

SymbolTable is a reusable policy-free symbol fact store.

Core idea:

```text
SymbolTable      stores facts
ISymbolPolicy    interprets facts for a specific view
SymbolTreeView   exposes a filtered projection
Semantic layer   decides language-law
```

The raw table is not an enforcer. It stores facts. Policy/view/semantic layers decide what those facts mean in a language context.
