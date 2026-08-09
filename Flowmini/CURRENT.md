# Current Flowmini Version

Current active implementation:

```text
flowmini_v24_explicit_ast
```

Current milestone:

```text
Flowmini v0.24 explicit AST stabilization
Road C5 statement deepening in progress
C5.1 return expression ownership complete
```

## Status

```text
build: PASS
AST golden tests: PASS (12)
suite: PASS (76 / 76)
bad: 0
```

Flowmini is still experimental and unfinished.

The current v0.24 line provides an observable, regression-guarded AST with
canonical expression payloads, canonical type-reference payloads, and the
first canonical statement-expression role: return value ownership.

## Run the current build

```bash
cd Flowmini/flowmini_v24_explicit_ast

cmake -S . -B cmake-build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build cmake-build-debug -j"$(nproc)"
```

## Run the current tests

```bash
cd Flowmini/flowmini_v24_explicit_ast

cmake --build cmake-build-debug --target flowmini_ast_golden_tests
cmake --build cmake-build-debug --target flowmini_suite
```

Expected result:

```text
AST golden tests: PASS (12)

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
remaining statement-role ownership beyond return
else blocks
semantic validation of type references
generic value arguments
pointer/reference source semantics if adopted
semantic name resolution
symbol table integration
type checking
Graph IR lowering
runtime execution semantics
provider/capability resolution
```
