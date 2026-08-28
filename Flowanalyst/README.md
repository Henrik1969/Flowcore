# Flowanalyst

Flowanalyst is the semantic-analysis sibling of Flowmini.

```text
source -> Flowmini -> flowmini.frontend_bundle -> Flowanalyst
```

It consumes the versioned Flowmini frontend bundle. It does not reparse source,
link Flowmini internals, or rewrite the structural AST/SymbolTable projection.
Its output is a separate, versioned semantic report containing diagnostics and
facts established by analysis.

Every semantic report also carries `source.path`, copied from the frontend
bundle. This downstream provenance field is preserved by Flowoptimize and
Flowlower, so consumers can identify the source artifact without reopening
Flowmini internals.

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

Flowanalyst currently reports frontend diagnostics, duplicate declarations,
declared type resolution, identifier and call resolution, call arity, refined
invariants, record fields, external ABI requirements, and named-target
entrypoint shape. It emits analysis regions and a Boolean dependency matrix.
More checks will be added as explicit semantic contracts, without moving
semantic meaning backward into Flowmini.

It also emits `effect_facts`. The first proven effect is `pure` for function
bodies consisting only of return expressions over literals, parameters, and
pure unary/binary operators. Calls, mutation, control-state constructs,
external effects, and unsupported forms remain `unknown`.

The independent consumer boundary is specified in
[`docs/flowanalyst/v0.1-consumer-contract.md`](../docs/flowanalyst/v0.1-consumer-contract.md).
It defines version negotiation, provenance navigation, diagnostic identity,
partial-result handling, and exit semantics for IDEs, debuggers, AI tools, and
other consumers.
