# Flowlower

Flowlower is the target-lowering sibling after Flowoptimize:

```text
source -> Flowmini -> Flowanalyst -> Flowoptimize -> Flowlower
```

The first target provider is `llvm`. The implementation supports narrow
source-derived profiles, including the executable `flowcat_argv_main` profile,
and emits valid LLVM modules for them. General source lowering remains a later
expansion; every emitted profile must be explicit, policy-authorized where it
uses external capabilities, and executable-tested.
