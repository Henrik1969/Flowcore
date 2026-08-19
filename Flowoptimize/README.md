# Flowoptimize

Flowoptimize is the sibling boundary after Flowanalyst:

```text
source -> Flowmini -> Flowanalyst -> Flowoptimize
```

It currently establishes the optimization input/output boundary. It accepts an
accepted `flowanalyst.semantic_report` v1 and emits an inspectable
`flowoptimize.optimization_report` v1 with an empty transform list. No
optimization is claimed yet.

Try the complete pipeline:

```sh
Flowmini ... | Flowanalyst/build/flowanalyst | Flowoptimize/build/flowoptimize
```

The future optimizer will consume the accepted semantic bundle, preserve
provenance, and emit a distinct versioned transformed state rather than
overwriting its input.

