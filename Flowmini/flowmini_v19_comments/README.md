# flowmini v19 — comments

v19 adds source comments before the next Text/String step.

## Added

- `//` line comments.
- `/* ... */` block comments.
- Nested block comments.
- Comments are recognized only outside string literals.
- Import scanning now masks comments before looking for `import`, `module`, or `main`, so commented-out `main` blocks do not confuse import validation.
- Unterminated block comments are rejected with a diagnostic.

## Comment rules

Comments are whitespace to the parser/lowerer.

```flow
// line comment

/*
    block comment

    /* nested block comment */
*/
```

Inside strings, comment markers are literal text, not comments:

```flow
strlen("/* not a comment */ // not a comment either") -> len
```

Recommended style is to put comments outside string payloads:

```flow
x : int(40) // explain x

/* temporarily disabled
x + y + y -> x
*/
```

When Text arrives in v20, multiline/annotated text should be built from expression chunks rather than comments inside strings:

```flow
message : Text(
    "first part " +     // comment between expressions
    "second part"
)
```

## New examples

- `examples/comment_demo.flow`
- `examples/comment_markers_inside_string.flow`
- `examples/bad_unterminated_block_comment.flow`

## Smoke tests

```bash
cmake -S . -B build
cmake --build build -j4

./build/flowmini examples/comment_demo.flow
./build/flowmini examples/comment_markers_inside_string.flow
./build/flowmini examples/capable.flow
./build/flowmini examples/abi_struct_demo.flow
./build/flowmini examples/internal_record_demo.flow
./build/flowmini examples/bad_unterminated_block_comment.flow
```

Expected highlights:

```text
comment_demo.flow                  -> 42
comment_markers_inside_string.flow -> 41
bad_unterminated_block_comment     -> fatal diagnostic
```

## Note

While testing comments, v19 also fixes an old lowerer lifetime bug where function-call lowering could invalidate references to scoped symbol paths. That surfaced when running broader regression tests and is now patched by copying paths/types before lowering sub-expressions that may temporarily change scopes.
