# Flowmini examples

The examples directory is organized by test intent.

```text
examples/
├── pass/       runnable root programs expected to succeed
├── fail/       negative tests expected to fail with diagnostics
├── support/    support/library/non-root flow files
├── apps/       reproducible end-to-end application examples
├── integration/ larger multi-feature programs for the complete sibling chain
└── docs/       documentation related to examples
```

## Rules

- `pass/*.flow` should execute successfully.
- `fail/*.flow` should fail with a meaningful diagnostic.
- `support/*` is not run directly as a root program unless a test explicitly asks for it.
- `apps/*` owns its own pipeline runner, capability policy, and expected output.
- `integration/*` is run by `tools/run-flowcore-integration-corpus.sh` through
  Flowmini, Flowanalyst, Flowparallel, and Flowoptimize.
- `docs/*` is documentation and is ignored by the regression runner.

This split makes it possible to build a real test harness instead of treating every
file in `examples/` as a root executable.
