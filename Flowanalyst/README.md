# Flowanalyst

Flowanalyst is the semantic-analysis sibling of Flowmini.

```text
source -> Flowmini -> flowmini.frontend_bundle -> Flowanalyst
```

It consumes the versioned Flowmini frontend bundle. It does not reparse source,
link Flowmini internals, or rewrite the structural AST/SymbolTable projection.
Its output is a separate, versioned semantic report containing diagnostics and
facts established by analysis.

## Try it

From the Flowcore repository:

```sh
./Flowmini/flowmini_v25_symboltable_projection/cmake-build-debug/flowmini \
  --dump-frontend-bundle \
  Flowmini/flowmini_v25_symboltable_projection/examples/ast/target_projection_probe.flow \
  | Flowanalyst/build/flowanalyst
```

The command also accepts a bundle file path instead of stdin:

```sh
Flowanalyst/build/flowanalyst bundle.json
```

Flowanalyst currently reports duplicate declarations in one scope, unresolved
declared type spellings, and target entrypoint shape. More checks will be added
as explicit semantic contracts, without moving semantic meaning backward into
Flowmini.

The independent consumer boundary is specified in
[`docs/flowanalyst/v0.1-consumer-contract.md`](../docs/flowanalyst/v0.1-consumer-contract.md).
It defines version negotiation, provenance navigation, diagnostic identity,
partial-result handling, and exit semantics for IDEs, debuggers, AI tools, and
other consumers.
