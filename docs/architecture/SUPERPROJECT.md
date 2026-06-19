# Flowcore Superproject Architecture

Flowcore is the upper controlling entity for the compiler chain. It is responsible
for wiring components together, not for collapsing every component into one
implementation.

## Constitutional Model

```text
Wires are the medium.
Signals are what move.
Nodes act on signals through ports.
Contracts and ABI decide whether plugs fit.
Effects mark contact with reality.
The graph is truth; syntax is sugar over the graph.
```

## Initial Chain

```text
flowlex -> flowparse -> flowoptimize -> flowlink -> flowemit
```

These names are stage targets and architectural ports. Their internal contracts
will become stricter as the language definition matures.

## Stage Boundaries

`flowlex`
: Reads source text and emits token signals.

`flowparse`
: Consumes token signals and produces syntax or graph artifacts.

`flowoptimize`
: Consumes graph artifacts and emits equivalent optimized graph artifacts.

`flowlink`
: Combines artifacts and resolves cross-artifact contracts.

`flowemit`
: Emits a target representation.

## AstLiib Placement

AstLiib is a subproject under Flowcore because it provides a generic token-tree
artifact useful to parser experiments.

It remains separate by design:

- AstLiib owns `AstLiibContext`, `AstLiibTree`, and node storage.
- The host owns token buffers, token text backing memory, and tokenizer ABI.
- AstLiib receives token signals through `AstLiibTokenAbi`.
- AstLiib builds a tolerant, lossless structural token tree.
- AstLiib does not validate Flowcore grammar.
- AstLiib does not perform semantic analysis.

In Flowcore terms:

```text
foreign token memory -> AstLiibTokenAbi -> BuildAst implementation -> AstLiibTree
```

## Inspectability

The superproject should make each stage visible and independently replaceable.
Early stages may be prototypes, but their target names and directories should state
which part of the chain they represent.

The current placeholder stages exist so the chain can be built and inspected before
their real contracts are finalized.
