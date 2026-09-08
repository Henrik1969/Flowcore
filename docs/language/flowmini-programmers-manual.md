# Flowmini Programmer’s Guide

Flowmini is Flowcore’s small experimental language for making computation,
state movement, providers, and evidence visible to the toolchain. It is a
language laboratory, not a replacement for C, C++, Rust, Python, or the other
languages in this repository. **Flowmini is one tool among many. Choose
according to the problem.**

This guide describes the checkout’s v0.30 implementation. Status words have a
precise meaning: **IMPLEMENTED** means the source path accepts it;
**TESTED** means a repository test or verified probe covers it; **DESIGNED**
and **PLANNED** are documents, not working features; **EXPERIMENTAL** means a
working but compatibility-oriented path; **NOT SUPPORTED — MISSING**,
**NOT SUPPORTED — DEFERRED**, **NOT SUPPORTED — INTENTIONALLY EXCLUDED**, and
**UNKNOWN** are limits.

## Start here

```sh
cmake -S . -B /tmp/flowcore-build -G Ninja
cmake --build /tmp/flowcore-build
cmake -S Flowmini/flowmini_v25_symboltable_projection -B /tmp/flowmini-build -G Ninja
cmake --build /tmp/flowmini-build
```

A minimal runtime program is:

```flow
program hello
main {
    answer : int(40)
    answer + 2 -> answer
    print answer
}
```

Run it with the built Flowmini executable and a source file. The `pass/`,
`support/`, and `std/` trees contain runnable examples. The root CTest suite is
the canonical contract; the Flowmini directory also has focused AST, symbol,
bundle, UTF-8, and TokenTree tests.

## Two current entry paths

Flowmini currently has two executable paths and they must not be confused:

* **Structural/artifact path (IMPLEMENTED, TESTED):** `--dump-ast` and
  `--dump-frontend-bundle` use the TokenTree/AST builder and publish symbols,
  facts, and provenance for Flowanalyst, Flowbind, Flowoptimize, and Flowlower.
* **Runtime graph path (EXPERIMENTAL, TESTED for admitted probes):** normal
  execution uses the versioned compatibility parser `flowmini::parseModule`
  and provider runtime. Pass `--runtime-compat` to make that choice explicit;
  the legacy no-flag invocation remains an alias for compatibility. It accepts
  useful constructs that the structural path does not yet preserve. This is a
  compatibility path, not evidence that the two parsers have identical
  semantics.

The compiler stages communicate through files. An AST, symbol projection,
Graph IR, lowering plan, and runtime value are different representations with
different owners; none is a universal authority.

## Source units, layout, and names

`program name` declares an executable root; `unit name` declares an importable
source unit. `import "path/to/unit.flow"` names a unit. A plain root `main { … }`
is the supported entry form. Named multi-target projections are **DESIGNED**
but are not the active v0.29 executable contract.

Line comments use `//` and `#`; block comments use `/* … */` and may nest.
Braces delimit blocks. Newlines normally separate statements. Identifiers are
case-sensitive. Type names are source-level names, so `int`, `Bool`, `Point`,
and `c_int` are distinct.

## Values, declarations, and expressions

The common declaration form is `name : Type(initializer)`:

```flow
count : int(0)
ready : Bool(false)
message : string("hello")
```

The admitted expression forms are identifiers; integer and floating literals;
strings (`\\n`, `\\t`, `\\r` escapes); booleans; unary `not` and prefix `-`;
`+ - * / %`; comparisons `< <= > >= == !=`; calls; indexing; field access;
list literals; and record literals. Parentheses group expressions. Postfix
call, index, and field operators bind most tightly; multiplication binds more
tightly than addition; comparisons bind below arithmetic. Prefer parentheses
when readability or a provider boundary depends on grouping.

Placement is the central state operation:

```flow
value -> variable
value -> record.field
value -> collection[index]
```

`=` assignment is retained as a structural compatibility spelling. The runtime
path primarily uses placement and `expression -> return` for function results.
Keyword `return expression` is accepted by some structural examples, but is not
a portable runtime contract.

## Functions and data

```flow
fn add(a : int, b : int): int {
    a + b -> return
}

main {
    result : int(0)
    add(20, 22) -> result
    print result
}
```

Records use `type` and `field`:

```flow
type Point {
    field x : int
    field y : int
}
p : Point({x:10, y:32})
```

`list<T>([values])` and shaped `array<T>[extent, ...]([values])` are present
in runtime examples. Indexing is zero-based in those examples. Generic maps,
iterators, and classes are **NOT SUPPORTED** as general language guarantees.
The pure `std/math.flow` unit is the first
small standard-library slice: `identity`, `add`, `abs`, `square`, `cube`,
`factorial`, `min`, `max`, and `clamp` over `int` are IMPLEMENTED and TESTED by
`flowmini_stdlib_math`; this is a narrow library surface, not a complete
standard library. The verified availability index is
[`stdlib-index.md`](../flowmini/stdlib-index.md).

Refined types are **EXPERIMENTAL**:

```flow
type Percent refines int {
    invariant value >= 0
    invariant value <= 100
}
```

Do not assume every conversion or runtime path enforces every invariant.

## User-defined generics (v0.30)

User-defined type parameters are **EXPERIMENTAL/TESTED** on the structural
artifact path. A generic function declares its parameters once:

```flow
fn identity<T>(value : T) : T {
    value -> return
}

main {
    answer : int(identity<int>(42))
}
```

Generic records preserve their parameter names and field uses in semantic
artifacts:

```flow
record Pair<A, B> {
    first : A
    second : B
}
```

Explicit type arguments are supported first (`identity<int>(42)`). A simple
single-answer inference form (`identity(42)`) is also tested. The compiler
records a substitution map and deterministic instance identity such as
`identity<int>` before lowering; LLVM receives only the concrete carrier type.
The initial executable generic function subset is deliberately small: the
tested implementation forwards one argument to the return path. Generic
record declarations and concrete applications such as `Pair<int, Bool>` are
validated and preserved in artifacts; general record construction/layout is
still limited by the existing record backend.

Generic constraints, value-level generics, specialization syntax, and generic
collection carriers are **NOT SUPPORTED — DEFERRED**. Compile-time constants
remain a separate value-level facility. Generic variants are now an
**IMPLEMENTED / TESTED** structural slice; see the next section.
The runtime compatibility parser is an explicitly named legacy path and does
not yet execute generic syntax; use `--dump-frontend-bundle` followed by the
semantic/lowering stages for the tested generic path.

### Generic variants and `std/result`

Generic variants reuse the same owned type-parameter and substitution model as
generic functions and records:

```flow
variant Result<T, E> {
    ok(value : T)
    error(reason : E)
}

enum ParseError { invalid }

main {
    result : Result<int,ParseError>(Result.ok(42))
}
```

The declaration owns `T` and `E` in order. The concrete annotation supplies
`T -> int` and `E -> ParseError`; Flowanalyst retains both the generic origin
and the resolved member payload types. Construction and matching use the
existing variant model, including deterministic zero-based member
discriminants, and resolve to concrete payload carriers before LLVM lowering.
The tested member spelling is `Result.ok` with the concrete instance supplied
by the declaration; `Result<int,ParseError>.ok` is not admitted yet.

`Result<T,E>` is library data, not a compiler primitive. The ordinary unit
[`std/result.flow`](../../Flowmini/flowmini_v25_symboltable_projection/std/result.flow) declares it alongside the
bounded `Option<T>` shape. A neutral `Either<A,B>` construction is part of the
generic-variant gate so this capability is not special-cased for Result.

Result values do not erase effects or resources. A provider call that returns a
Result remains provider-bound and effectful in semantic facts, while `guard`
continues to represent a separate required-condition flow operation. This
slice adds no exceptions, implicit propagation, ownership checking, or generic
constraints. Integer and enum payloads execute through LLVM; wider record,
nested, resource-handle, payloadless, and TinyVM cases remain explicitly
limited by the current carrier/backend contract.

## Control flow, constants, and variants

`if`/`else`, `while`, `break`, and `continue` are IMPLEMENTED and TESTED in
runtime probes. `guard condition else { … }` is IMPLEMENTED and TESTED; the
structural AST preserves a dedicated `guard` node and the failure block’s
provenance. Flowanalyst carries that operation into the lowering plan and the
LLVM backend emits its condition, failure block, and continuation. A backend
rejects a failure block it cannot represent instead of silently dropping it.

Compile-time constants are IMPLEMENTED for deterministic integer and Boolean
expressions made from literals, earlier constants, unary operators, arithmetic,
and comparisons. Their evaluated value is retained as `compile_time_value` in
the AST, symbol facts, and lowering plan. Provider calls, runtime state,
unsupported expressions, overflow, and divide-by-zero are rejected during
constant evaluation. This is a bounded compile-time fact facility, not a
general compile-time programming language.

The runtime parser accepts `const name : Type(value)` and enum/variant
declarations and `when` over integer, enum, and variant values. Constants are
represented as `is_const` declarations and `mutability=const` symbol facts in
the frontend bundle. The structural AST still requires a `default` block for
`when`.

Variants have a target-neutral carrier in the semantic artifact. Member
discriminants are deterministic zero-based declaration-order values, and each
member records its canonical payload fields and types. LLVM currently lowers a
carrier as `{ i32 discriminant, i32 payload }`; integer and enum payloads are
implemented and tested, including payload extraction and arm-local bindings.
Multi-field, record, nested, and other non-`i32` payload layouts are retained
canonically but rejected by Flowlower with an explicit backend limitation.
TinyVM payload lowering is also **NOT SUPPORTED — BACKEND LIMITATION**. Backend
layout remains an implementation choice and is not part of Flowmini semantics.

## ABI and providers

An ABI block declares an external library, calling convention, structs, and
extern functions. Provider imports and ABI calls are the supported escape
boundary:

```flow
abi mathlib {
    library "./libmath.so"
    convention c
    extern fn answer(): c_int {
        symbol "answer"
        effect pure
    }
}
```

The declared library and symbol must exist. `print` is a convenient provider
operation, not proof of a complete standard library. There is currently no
general `unsafe` block or inline-assembly mechanism (**NOT SUPPORTED —
INTENTIONALLY EXCLUDED**). Flowmini source cannot embed foreign or opaque
regions. Machine-specific functionality must be an external provider/artifact
with declared identity, ABI, target constraints, inputs, outputs, effects,
resource obligations, provenance, and verification evidence.

`tools/flowbind-gen` is an EXPERIMENTAL Clang-backed C declaration generator.
It emits deterministic `flowbind.c_binding` JSON for scalar functions, enums,
opaque handles, and explicitly declared create/use/release contracts. It marks
unsupported carriers as `partial`; it does not parse C++ or execute foreign
calls. The real-library probe covers installed libm, zlib, sqlite3, and libcurl
headers.

## Flowcore model in programmer terms

Think of a program as:

```text
input -> handling/computation -> output
```

Inputs and observations become **facts**. Functions transform facts. A
**capability** authorizes an effect such as output, storage, or a device call;
a **provider** implements that capability for a target. **Policy** decides
which provider/effect is admitted. A state update is a revision with lineage,
not an unlabelled overwrite. A **projection** exposes selected facts to a
consumer; it does not become authority merely because it is convenient. ABI
and provider boundaries carry identity and **provenance**. Tests, captured
artifacts, diagnostics, and gates are **evidence** for readiness claims.

## Compilation pipeline

```text
source -> tokens/TokenTree -> structural AST -> symbol/fact projection
       -> Flowanalyst/Flowbind/Flowoptimize -> backend-neutral lowering
       -> provider/backend/runtime
```

The normal runtime parser is a separate path today. AST nodes, symbol tables,
Graph IR, lowering operations, provider calls, and runtime values have distinct
schemas and ownership. Integer and enum match lowering is tested. Variant
construction, discriminant selection, payload extraction, and provenance are
represented in the lowering plan before backend-specific layout is chosen.

## What to test next

Use focused tests while learning, then run `ctest --test-dir build
--output-on-failure`. The current root build and focused suite are green after
the semantic JSON producer fix and the parser/const/guard/provider gates. Two
mature probes combine lists, loops, guards, constants, enums, and math; run
the evidence reporter for current categorized-suite totals rather than copying
a frozen count. Their evidence and known workarounds are listed in
[`mature-program-probes.md`](../flowmini/mature-program-probes.md).

## Compact reference

```text
program name                 unit name
import "path"
main { statements }
fn name(a : Type): Return { statements }
type Name { field member : Type }
type Name refines Base { invariant expression }
abi name { library "path"; convention c; ... }
name : Type(initializer)       expression -> target
expression -> return           if expression { ... } else { ... }
while expression { ... }       guard expression { ... } else { ... }
break                          continue
list<Type>([values])           array<Type>[n, ...]([values])
record.field                   value[index]
```

Unsupported or uncertain behavior is recorded in
[`FUTURE_WORK_EVIDENCE.md`](../FUTURE_WORK_EVIDENCE.md) and the language
comparison documents. When a claim is marked DESIGNED or PLANNED, write a
probe before relying on it.

Implementation evidence used for this guide includes the AST definitions in
`Flowmini/flowmini_v25_symboltable_projection/include/flowmini_ast.h`, the
structural builder in `src/flowmini_ast_builder.cpp`, runtime parsing in
`src/main.cpp`, analysis in `flowanalyst/src/main.cpp`, and the executable
probes under `Flowmini/flowmini_v25_symboltable_projection/examples/bootstrap`.
