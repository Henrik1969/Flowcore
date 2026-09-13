# Integration programs

These programs are intentionally larger than the focused language probes.
Each combines several already-supported Flowmini features and is exercised
through the complete inspectable chain:

```text
Flowmini -> Flowanalyst -> Flowparallel -> Flowoptimize
```

They are integration evidence, not golden AST fixtures. The focused examples,
goldens, negative corpus, and firetests remain the authoritative checks for
individual language rules.

The current set covers:

- imported functions, refined values, lists, loops, and branching;
- nested loops, indexed arrays, matrix accumulation, and checksums;
- ABI declarations, records, shared functions, and multiple targets.
