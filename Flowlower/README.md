# Flowlower

Flowlower is the target-lowering sibling after Flowoptimize:

```text
source -> Flowmini -> Flowanalyst -> Flowoptimize -> Flowlower
```

The first target provider is `llvm`. The initial implementation supports one
deliberately narrow source-derived trial profile, `empty_program_main`, and
emits a valid LLVM module for it. General source lowering follows after the
semantic-bundle and optimizer contracts are mature enough to lower truthfully.
