# TokenTree

TokenTree is a reusable structural token tree library.

It is not the parser, semantic checker, or final AST.

Purpose:

```text
external tokens
  -> structural grouping
  -> traversal/mutation/inspection
  -> parser input boundary
```

Flowmini currently uses TokenTree as a structural bridge/inspection layer. A later Flowmini version should make the parser consume TokenTree more directly.
