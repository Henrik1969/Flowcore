# Flowlower

Flowlower is the target-lowering sibling after Flowoptimize:

```text
source -> Flowmini -> Flowanalyst -> Flowoptimize -> Flowlower
```

The first target provider is `llvm`. The implementation supports narrow
source-derived profiles, including the executable `flowcat_argv_main` profile,
and emits valid LLVM modules for them. General source lowering follows after
the semantic-bundle and optimizer contracts are mature enough to lower
truthfully.
