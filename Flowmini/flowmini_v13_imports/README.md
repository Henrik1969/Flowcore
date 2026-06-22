# flowmini v13 — source imports for reusable libraries

This version adds a first simple `import "file.flow"` layer so reusable function libraries can live outside the main program file.

The implementation is intentionally close to textual include/import expansion:

- imports are written before `module`
- import paths are resolved relative to the importing file
- `std/` is therefore normally a subdirectory next to the source tree, for example `import "../std/math.flow"` from `examples/`
- imported files may themselves import other files
- repeated imports of the same canonical file are skipped
- circular imports are rejected
- imported files must not define `main`
- the root file must define exactly one `main`
- imported declarations are merged into the same function namespace
- duplicate function names are rejected

For now modules are metadata only, not namespaces.

## Build

```bash
cmake -S . -B build
cmake --build build -j20
```

## Import examples

```flow
import "../std/math.flow"
import "../std/collatz.flow"

module import_demo

main {
    five   : int(5)
    result : int(0)

    factorial(five) -> result
    print result

    collatz_next(five) -> result
    print result
}
```

Run:

```bash
./build/flowmini examples/import_demo.flow
```

Expected:

```text
120
49
16
```

## Included library files

```text
std/math.flow
std/collatz.flow
```

`std/math.flow` currently provides:

```text
identity
add
square
cube
factorial
choose_larger
```

`std/collatz.flow` provides:

```text
collatz_next
```

## Useful tests

```bash
./build/flowmini examples/import_demo.flow
./build/flowmini examples/capable_imported.flow
./build/flowmini examples/import_repeat.flow
./build/flowmini examples/bad_import_main.flow
./build/flowmini examples/bad_import_cycle.flow
./build/flowmini examples/bad_import_duplicate_fn.flow
```

The `bad_*` examples should fail with diagnostics.

## Current import rules

```text
import "relative/path.flow"
```

Relative paths are resolved from the directory of the importing file, not from the shell current working directory.

A root program file may contain:

```text
imports
module
fn declarations
main block
```

A library file may contain:

```text
imports
module
fn declarations
```

A library file may not contain `main`.

## Existing v12/v11 capabilities remain

The language still supports:

- `fn name(arg : int, ...): int { ... }`
- value-bound input ports
- `return` as the default output port
- function call placement and call expressions
- recursion rejection
- `int`
- `list<int>`
- fixed-shape `array<int>[shape...]`
- rank-N `arr[i, j, k]` access
- loops, `if/else`, `break`, `continue`
- compound expressions
- old explicit `.flowir` files
