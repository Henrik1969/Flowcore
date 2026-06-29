# Flowcore Type-System Foundation: Primitive Atoms, Contract Types, and Domain Types

## Status

**Locked baseline design.**

This note defines the current foundation for Flowcore’s type-system model. It should remain the default architecture unless a clearly better model appears.

The goal is to keep the primitive core small, finite, mechanically honest, and directly lowerable, while allowing every complex datatype to be built through explicit contracts.

---

## Core Principle

Flowcore separates types into three major classes:

```text
primitive type = machine-near value atom
contract type  = modeled meaning + representation rules
domain type    = semantic/algebraic laws + lowering rules
```

This distinction is fundamental.

Primitive types describe simple machine-near values.

Contract types describe structured data with explicit representation, ownership, validity, lifetime, mutation, and operation rules.

Domain types describe values whose behavior depends on mathematical, semantic, or algebraic laws.

---

## Primitive Core

The primitive core should remain small and finite.

### Boolean

```text
bool
```

### Signed integers

```text
int8
int16
int32
int64
int128
int256
int512
```

### Unsigned integers

```text
uint8
uint16
uint32
uint64
uint128
uint256
uint512
```

### Floating-point approximations

```text
float8
float16
float32
float64
float128
float256
float512
```

### Character/code-unit primitives

```text
char8
char16
char32
```

These are primitive character/code-unit/code-point sized atoms. Higher-level text behavior is not primitive.

---

## Basic Storage Mechanics

The language also needs basic representation mechanics:

```text
ptr<T>
ref<T>
array<T, N>
slice<T>
```

These are not high-level containers in the semantic sense. They are storage and access mechanics.

---

## What Is Not Primitive

Complex datatypes are not primitives.

Examples:

```text
String
Text
CString
Rational
BigInt
BigUInt
Decimal
Complex<T>
Quaternion<T>
Octonion<T>
Matrix<T>
Tensor<T>
Path
Regex
Date
SymbolName
NodeId
PortId
WireId
ScopeId
```

These are modeled through contracts.

The reason is simple: these types have representation choices, semantic rules, validity constraints, ownership questions, mutation policies, algebraic behavior, or special lowering rules.

They are not machine-near atoms.

---

## Complex Datatype Contract Components

A complex datatype is defined by a contract containing some or all of the following:

```text
representation
storage layout
size/alignment
ownership rules
lifetime rules
mutation rules
validity rules
encoding rules
indexing rules
allowed operations
error behavior
algebraic/semantic laws
ABI strategy
lowering strategy
runtime/helper-call strategy
optimizer rules
```

This lets Flowcore describe both what a type means and how it is lowered.

---

## Example: Text/String

A string is not a primitive.

At hardware level there is only memory:

```text
[address]     -> byte
[address + 1] -> byte
[address + 2] -> byte
...
```

A “string” may be represented as:

```text
C string:
    [c][c][c]...[0]

Pascal string:
    [len][c][c][c]...

Slice string:
    ptr + len

Owned string:
    ptr + len + capacity

Small-string optimized value:
    inline buffer or heap pointer

Rope:
    tree of chunks

Piece table:
    original buffer + add buffer + spans
```

Therefore `String`/`Text` must be modeled as a contract over:

```text
storage
encoding
ownership
lifetime
mutation
validity
indexing
operations
```

A possible modeled type:

```text
Text<UTF8, Slice<char8>>
```

This lowers to something like:

```text
struct {
    char8* ptr;
    size_t len;
}
```

plus UTF-8 validity and text-operation contracts.

---

## Example: Numeric Domains

Mathematical domains are not all primitive machine types.

```text
N  natural numbers
Z  integers
Q  rationals
R  reals
C  complex numbers
H  quaternions
O  octonions
```

Possible mapping:

```text
N -> uint<N> or BigUInt
Z -> int<N> or BigInt
Q -> Rational<T>
R -> float<N>, Decimal<T>, BigFloat, Interval<T>, etc.
C -> Complex<T>
H -> Quaternion<T>
O -> Octonion<T>
```

Important distinction:

```text
N and Z:
    exactly representable within fixed width until overflow

Q:
    exactly representable as numerator/denominator, but may grow

R:
    not generally exactly representable in finite binary storage;
    floating-point values approximate real numbers

C:
    modeled as two components

H:
    modeled as four components; non-commutative but associative

O:
    modeled as eight components; non-commutative and non-associative
```

This matters for compiler optimization.

For example:

```text
(a * b) * c
```

may not be equivalent to:

```text
a * (b * c)
```

for octonions.

Therefore algebraic law contracts must tell the compiler which rewrites are legal.

---

## Lowering Payoff

This architecture exists because it gives Flowcore a clean mechanical path from high-level type meaning to machine code.

The lowering chain becomes:

```text
source type
    ↓
semantic contract
    ↓
layout contract
    ↓
operation contract
    ↓
ABI/lowering strategy
    ↓
machine representation / helper calls / runtime calls
```

Examples:

```text
int64
    lowers directly to register/stack/memory scalar operations

int512
    lowers to 8 × uint64 limbs plus helper operations

Text<UTF8, Slice<char8>>
    lowers to ptr + len plus UTF-8 validity contract

Complex<float64>
    lowers to two float64 components plus complex arithmetic rules

Octonion<float64>
    lowers to eight float64 components plus non-associative algebra rules

NodeId
    may lower to a packed uint64 domain handle
```

---

## Flowcore Graph Relevance

This fits Flowcore’s graph/contract model directly.

```text
ports carry typed contracts
wires enforce representation compatibility
nodes operate on declared contracts
lowering selects concrete machine representation
runtime moves legal envelopes only
```

A value traveling through a Flowcore wire is not merely “some bytes.” It is a payload governed by a type/representation/semantic contract.

---

## Anti-Primitive-Creep Rule

Do not add a primitive just because it is useful.

Ask:

```text
Is it a machine-near value atom?
Is the size finite and mechanically clear?
Does it have one obvious representation?
Does it lack domain-specific semantic law complexity?
```

If not, it is not primitive.

It belongs in the contract/domain layer.

This prevents the primitive layer from becoming a junk drawer containing strings, dates, regexes, paths, matrices, tensors, big integers, decimals, and algebraic structures.

---

## Final Rule

The primitive core stays small, finite, and mechanically honest.

Complex datatypes are built from explicit representation and logic contracts.

This gives Flowcore a stable foundation for:

```text
type checking
ABI generation
storage layout
optimization legality
machine-code lowering
runtime helper calls
FFI boundaries
domain libraries
```

This is the locked baseline for the Flowcore type-system direction.

