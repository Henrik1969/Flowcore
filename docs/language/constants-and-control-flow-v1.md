# Constants and readable control flow v1

## Constants

Flow constants use the existing declaration shape with an explicit modifier:

```flow
const continuation_width : int(64)
```

The declaration creates an immutable semantic symbol. It has no independent
runtime activation or ordering effect. A use site materializes the declared
value where the lowering graph needs it. Assignments, field writes, and indexed
writes rooted at a constant are rejected.

This keeps names such as `continuation_width` authoritative without introducing
hidden runtime nodes or changing provider sequencing. Constant identity and
value remain available to frontend and lowering artifacts for diagnostics and
provenance.

## Control flow

Nested conditionals remain valid, but they should not be the only way to state
multiple semantic cases. The current migration order is:

1. use named constants and guard exits to remove repeated literal predicates;
2. use `else if` chains for mutually exclusive cases;
3. introduce a value-oriented `when`/`match` form for decoder states and
   outcome cases;
4. preserve explicit branch provenance and boundedness in the lowering file.

The UTF-8 source-reader probe now applies the first migration step: each
validation invariant is a top-level guard with an explicit failure return.
This keeps the maximum validation nesting at one conditional level while
retaining the same success/failure result and the same C++ comparison surface.
The temporary `validation_stage` carrier makes the true path non-empty until
the language grows a dedicated guard statement.

The `when`/`match` design is deferred until constants and outcome records have
stable semantics. It must lower to the same branch meaning as the existing
structured form and remain available to freestanding targets.
