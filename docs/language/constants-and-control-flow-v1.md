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

The dedicated guard form is:

```flow
guard outcome.valid else {
    failure -> return
}
```

It continues on the true path and evaluates the `else` block only when the
condition is false. The UTF-8 source-reader probe uses guards for every
validation invariant, keeping the maximum validation nesting at one
conditional level while retaining the same success/failure result and C++
comparison surface.

The value-oriented follow-up is specified in
[`when-match-v1.md`](when-match-v1.md). It remains a separate construct so
that `guard` continues to communicate invariant enforcement directly.

The `when`/`match` design is deferred until constants and outcome records have
stable semantics. It must lower to the same branch meaning as the existing
structured form and remain available to freestanding targets.
