# flowmini v5 — sweet `.flow` frontend over explicit `.flowir`

This version keeps the previous primitive graph language as the lowered form and adds a first human-facing sugar layer.

## Source layers

```text
.flow      human source with sugar
.flowir    explicit primitive graph IR
```

The old explicit syntax is still valid. The new sugar is parsed and lowered into the same `ModuleSpec` before validation and execution.

## Hard initialization rule

Declarations must create initialized state. No declaration may create undefined/random storage.

Valid:

```flow
one : int(1)
arr : list<int>([1,2,3])
```

Invalid:

```flow
one : int
```

Diagnostic:

```text
fatal in parser: declaration of 'one' requires initializer
```

Arrow placement is assignment into an existing contract-bound target. It does not create new names.

Invalid:

```flow
1 -> one
```

Diagnostic:

```text
fatal in lowerer: cannot assign to undeclared target 'one'
```

This preserves the semantic split:

```text
declaration = creates contract + initialized state
assignment  = checks expression against existing contract
```

## New sugar supported in v5

### Compact atom node declarations

```flow
parse : parse.int.to_record(out="n")
gt : int.gt(lhs="n", rhs="zero", out="cond")
route : route.bool(path="cond")
```

Lowers to:

```flowir
node parse : parse.int.to_record
policy parse.out = n
node gt : int.gt
policy gt.lhs = n
policy gt.rhs = zero
policy gt.out = cond
```

`stdin.text` is inferred as a producer and `halt.record` as a sink in the sugared form:

```flow
stdin : stdin.text()
halt : halt.record()
```

### Initialized declarations

```flow
zero : int(0)
one : int(1)
arr : list<int>([1,9,3,5,8,2,2,6,4])
```

These lower to primitive atoms:

```flowir
node one : const.int
policy one.out = one
policy one.value = 1

node arr : list.from_ints
policy arr.out = arr
policy arr.values = 1,9,3,5,8,2,2,6,4
```

### Wire chains

```flow
stdin => parse => zero => one => gt => route
route.true => print => dec => gt
route.false => halt
```

Lowers to explicit wires:

```flowir
wire stdin.out => parse.in
wire parse.out => zero.in
wire zero.out => one.in
wire one.out => gt.in
wire gt.out => route.in
wire route.true => print.in
wire print.out => dec.in
wire dec.out => gt.in
wire route.false => halt.in
```

The chain rule uses default `out`/`in` ports. Explicit ports remain available where meaning would otherwise be ambiguous.

### Early placement/assignment checker

The parser recognizes simple assignment forms such as:

```flow
foo + bar -> foo
1 -> foo
foo -> bar
```

For v5 this mainly establishes the semantic rule: the right-hand target must already be declared and type-compatible. More complete statement sequencing and expression lowering will come next.

## Build

```bash
cmake -S . -B build
cmake --build build -j20
```

## Run

```bash
echo 5 | ./build/flowmini examples/countdown_sugar.flow
```

Expected:

```text
5
4
3
2
1
```

```bash
echo 0 | ./build/flowmini examples/sum_list_sugar.flow
```

Expected:

```text
40
```

Old explicit examples still run, for example:

```bash
echo 0 | ./build/flowmini examples/bubblesort_list.flow
```

Expected:

```text
1
2
2
3
4
5
6
8
9
```

## Emit lowered `.flowir`

```bash
./build/flowmini --emit-flowir - examples/countdown_sugar.flow
```

or:

```bash
./build/flowmini --emit-flowir lowered.flowir examples/countdown_sugar.flow
```

`--emit-flowir` lowers and exits without executing.

## New examples

```text
examples/countdown_sugar.flow
examples/sum_list_sugar.flow
examples/bad_uninitialized.flow
examples/bad_undeclared_assignment.flow
```

The bad examples are intentional semantic/diagnostic tests.
