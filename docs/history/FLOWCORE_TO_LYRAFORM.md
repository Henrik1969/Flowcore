# Flowcore to Lyraform

Effective date: 2026-09-13

Flowcore was the original project name. The project was renamed to Lyraform
before broad external adoption to reduce name confusion and to give the
language and semantic architecture a more distinctive identity.

Lyraform is the authoritative current identity for the language, compiler-
visible semantic model, and project architecture. `Igor` is the human-facing
compiler/toolchain driver introduced with that identity.

Historical documents retain `Flowcore`, `Flowmini`, and version-era names when
they describe actual earlier states, such as v23, v24, v25, v28, or v29
checkpoints. Flowmini remains part of the historical bootstrap/prototype
lineage. Existing `flowmini` executable names and `flowcore.*` / `flowmini.*`
serialized namespaces are compatibility surfaces, not current branding.

The current active implementation moved from:

```text
Flowmini/flowmini_v29_reusable_native_chain
```

to:

```text
Lyraform/compiler
```

The earlier v0.24 implementation remains available at
`Lyraform/flowmini_v24_explicit_ast` as historical material. The independent
FlowLFS experiment and the historical `master` branch are not part of this
migration and are not merged, rewritten, or deleted.

This document is part of the project history. The migration checkpoint and
repository rename evidence are recorded in
`LYRAFORM_MIGRATION_2026-09-13.md`.
