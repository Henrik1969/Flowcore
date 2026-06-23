# Session Note: Flowmini v22 Unit Kinds

## Summary

Flowmini v22 introduced explicit source roles:

```text
program = executable/root source unit
unit    = defining/importable source unit
```

## Why

The old flat `examples/` layout mixed root programs, negative tests, support files, and documentation. Categorizing examples exposed hidden import/path assumptions.

## Decisions

- `program` files may contain `main` and may be root-executed.
- `unit` files may be imported and must not contain `main`.
- Ordinary imports accept units and reject programs.
- Program-as-callable-entrypoint remains speculative future work.

## Testing

The examples were categorized:

```text
examples/pass/
examples/fail/
examples/support/
examples/docs/
```

The suite runner is environment-aware and checks expected stdout and diagnostic substrings.

Baseline:

```text
suite: 75 / 75
```

## Next

- clean compiler warnings
- add programmer's manual draft
- v23 TokenTree parser bridge
