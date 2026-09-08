# flowinspect

`flowinspect` is a small read-only consumer for versioned Flowcore artifacts.
It projects structured artifact facts into a deterministic terminal summary;
the artifact and its producer remain authoritative.

## Usage

```text
flowinspect artifact.json
cat artifact.json | flowinspect
flowinspect --help
```

The v0 inspector accepts one file (or standard input) and currently validates
and summarizes:

| Artifact | Version | Summary |
| --- | --- | --- |
| `flowanalyst.semantic_report` | 1 | source/status, symbols, regions, external operations, candidates, rejections, diagnostics |
| `flowparallel.execution_plan` | 1 | source/status, operations, candidates, rejections, dependency entries, fallback |
| `flowoptimize.optimization_report` | 1 | source/status, operations, transforms, projections, targets |
| `flowcore.lowering_plan` | 1, 2 | source/status, operations, functions |
| `flowmini.frontend_bundle` | 2 | source, declarations, expressions, symbols, scopes |

Unknown formats, unsupported versions, malformed JSON, and invalid required
fields fail explicitly. Exit codes are `0` for a successfully inspected
artifact, `1` for usage or I/O errors, `2` for malformed/invalid input, and
`3` for an unsupported format or version.

The implementation uses the shared `flowcontracts` parser and validators. It
does not infer facts from human-readable diagnostics and does not write back to
artifacts.
