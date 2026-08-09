# Current Flowmini Version

Current active implementation:

```text
flowmini_v24_explicit_ast
```

Current milestone:

```text
Flowmini v0.24 explicit AST
shallow expression AST checkpoint
```

## Status

```text
build: expected OK
AST golden tests: PASS (8)
suite: PASS (76 / 76)
bad: 0
```

Flowmini is still experimental and unfinished.

The current v0.24 line provides an observable, regression-guarded AST with a first shallow expression graph pass.

## Run the current build

```bash
cd Flowmini/flowmini_v24_explicit_ast

cmake -S . -B cmake-build-debug
cmake --build cmake-build-debug -j20
```

Adjust `-j20` to match your machine.

## Run the current tests

```bash
cd Flowmini/flowmini_v24_explicit_ast

cmake --build cmake-build-debug --target flowmini_ast_golden_tests
cmake --build cmake-build-debug --target flowmini_suite
```

Expected result:

```text
AST golden tests: PASS (8)

total: 76
pass:  76
bad:   0
```

## Current important architecture law

```text
TokenTree remembers what the source looked like.
AST states what the source means.
```

## Current important language rule

For v0.24:

```text
A root program supports exactly one root main block.
```

Future Flowmini/Flowcore may support named targets, where each target owns its own main block.

That future direction is documented in:

```text
docs/language/named-targets.md
```

## Not yet complete

The following are still future or incomplete work:

```text
recursive expression population
operator precedence and associativity
else blocks
full type-reference parsing
semantic name resolution
symbol table integration
type checking
Graph IR lowering
runtime execution semantics
provider/capability resolution
```
