# LangLab parser contract 0.1-A

```text
input schema:
    langlab.tokenstream.jsonl v0.1

output schema:
    langlab.ast.json v0.1-A

stdin:
    JSONL token records and optional ghost metadata records

stdout:
    one JSON AST document

stderr:
    human-readable parser diagnostics
```

Ghost records are consumed as provenance metadata and do not become grammar tokens.

The AST root preserves source declarations and ghost annotations separately from syntactic nodes.
