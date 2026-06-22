# flowmini v16 — ABI pointer contracts

This version keeps the v15 ABI proof-of-concept, but starts formalizing pointer-shaped ABI values as sealed contracts.

The core rule:

```text
Pointers belong to the ABI boundary.
Normal Flowmini code may not construct, dereference, offset, cast, or free pointer-shaped values directly.
Pointer-like values are sealed capabilities that may only move through declared ABI contracts.
```

## Build

```bash
cmake -S . -B build
cmake --build build -j20
```

## ABI pointer type blocks

Inside an `abi` block, v16 can now declare ABI pointer-shaped contracts:

```flow
abi libc {
    library "libc.so.6"
    convention c

    type c_string {
        repr "const char*"
        ownership borrowed
        access read
        lifetime call
        nullable false
        terminator nul
    }
}
```

This describes the boundary contract. It does **not** make raw pointer manipulation available in normal Flowmini source.

Supported ABI type properties for now:

```text
repr        "..."        external ABI representation text
ownership   borrowed | owned | external | consumed
access      read | write | readwrite | opaque
lifetime    call | owned | external
nullable    true | false
terminator  <identifier> optional, e.g. nul
opaque      true | false optional
```

## Included pointer contract examples

```text
std/abi/libc.flow
std/abi/pointers.flow
```

`std/abi/libc.flow` declares `c_string` as a borrowed read-only `const char*` valid for the call duration, then binds:

```text
strlen(c_string) -> c_size_t
abs(c_int)       -> c_int
labs(c_long)     -> c_long
puts(c_string)   -> c_int
```

`std/abi/pointers.flow` declares future-facing sealed pointer abstractions:

```text
c_buffer_read
c_buffer_mut
c_opaque_handle
```

They are intentionally not usable directly yet; they establish the contract shape for later buffer and handle work.

## Demo

```bash
./build/flowmini examples/abi_pointer_contracts_demo.flow
```

Expected output on the test system:

```text
26
Flowmini pointer contracts
27
```

The final value is the `puts` return value and should only be treated as a system-specific non-negative success-ish result.

## Negative tests

```bash
./build/flowmini examples/bad_abi_pointer_direct_declare.flow
./build/flowmini examples/bad_abi_type_missing_contract.flow
```

Expected behavior:

```text
bad_abi_pointer_direct_declare.flow
    rejects direct construction/declaration of sealed ABI pointer-shaped c_buffer_read

bad_abi_type_missing_contract.flow
    rejects incomplete ABI pointer contract
```

## Existing v15/v14 capabilities remain

The language still supports:

- `import "file.flow"`
- imported `std/` libraries
- `type Name refines Base { invariant value <op> literal }`
- semantic integer contracts defined in `std/numbers.flow`
- `fn name(arg : int, ...): int { ... }`
- value-bound function ports
- `int`, `list<int>`, `array<int>[shape...]`
- rank-N `arr[i, j, k]` access
- loops, `if/else`, `break`, `continue`
- ABI scalar calls through `dlopen`/`dlsym`
- explicit `.flowir` files

## Deliberate limitations

Still not supported:

```text
raw pointer arithmetic
raw dereference
integer-to-pointer casts
malloc/free ownership modeling
struct layout contracts
callbacks
output pointer parameters
mutable buffers as callable values
variadic functions
errno/error-flow modeling
```

This version prepares the ABI/pointer abstraction layer without opening the foot-gun box.
