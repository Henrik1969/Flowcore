# flowmini v6 scopes

This version keeps the v5 split:

```text
.flow    = human/sweet source
.flowir  = explicit lowered graph IR
```

The primitive `.flowir` graph language remains valid and executable. The new work is in the frontend: `main { ... }`, structured blocks, lexical scopes, initialized declarations, expression placement, `print`, and `while` lowering.

## Build

```bash
cmake -S . -B build
cmake --build build -j20
```

## New structured source example

`examples/countdown_structured.flow`:

```flow
module countdown_structured

main {
    n    : int(stdin.int())
    zero : int(0)
    one  : int(1)

    while n > zero {
        print n
        n - one -> n
    }
}
```

Run:

```bash
echo 5 | ./build/flowmini examples/countdown_structured.flow
```

Output:

```text
5
4
3
2
1
```

Inspect lowered graph IR:

```bash
./build/flowmini --emit-flowir - examples/countdown_structured.flow
```

## Hard semantic rules now enforced

### 1. Declaration must initialize

Valid:

```flow
one : int(1)
```

Invalid:

```flow
one : int
```

Diagnostic example:

```text
fatal in parser: declaration of 'one' requires initializer
```

### 2. Assignment never declares

Valid:

```flow
one : int(1)
2 -> one
```

Invalid:

```flow
1 -> one
```

Diagnostic example:

```text
fatal in lowerer: cannot assign to undeclared target 'one'
```

### 3. Implicit shadowing is forbidden

Invalid:

```flow
main {
    person : int(30)

    while person > 0 {
        person : int(12)
    }
}
```

Diagnostic example:

```text
fatal in parser: declaration of 'person' conflicts with visible symbol 'bad_shadowing::main::person'; implicit shadowing is forbidden
```

## Scope model

Current v6 rules:

```text
module creates the root scope
main creates an executable child scope
while creates a nested child scope
inner scopes may read/write visible outer symbols
outer scopes cannot see inner symbols
reusing a visible name in an inner scope is a hard error
```

Names in structured source lower to mangled internal record paths, for example:

```flow
n : int(stdin.int())
```

may lower to a path like:

```text
__countdown_structured__main_n
```

This keeps source names readable while making lowered `.flowir` unambiguous.

## New frontend sugar in v6

Supported inside `main { ... }` and nested `while { ... }` blocks:

```flow
name : int(<expr>)
name : list<int>([1,2,3])
<expr> -> existing_name
print name
while <int comparison> {
    ...
}
```

Supported expressions are intentionally small for now:

```text
integer literal
identifier
stdin.int()
a + b
a - b
a * b
a / b
a % b
a > b
a < b
a == b
```

`>=`, `<=`, and `!=` are lexed/reserved but not lowered yet in this prototype.

## Backward compatibility

The v5 graph-sugar examples still work, including:

```bash
echo 5 | ./build/flowmini examples/countdown_sugar.flow
echo 0 | ./build/flowmini examples/sum_list_sugar.flow
```

The older explicit `.flowir`-style graph language is still accepted.

## What v6 proves

v6 proves that FlowMini can now lower a more human structured source form into the explicit primitive graph form:

```text
.flow human structured source
    -> lexer
    -> parser + scoped symbol table
    -> semantic lowering
    -> .flowir graph IR
    -> validator
    -> runtime graph traversal
```

The runtime remains mostly unchanged. The new value is frontend semantics.

## Known limitations

This is still a prototype frontend.

- No explicit `scope::name` syntax yet.
- No named user scopes yet beyond `main` and generated `while` scopes.
- No `if` block yet.
- No `for`/range sugar yet.
- No field/member assignment syntax yet.
- No physical runtime cleanup for scoped locals yet; scope lifetime is currently enforced semantically by the frontend.
- Expression precedence is still minimal: one binary operator per expression in this version.

