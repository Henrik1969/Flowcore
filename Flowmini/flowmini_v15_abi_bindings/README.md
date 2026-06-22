# flowmini v15 — ABI binding proof-of-concept

This version adds a deliberately narrow ABI bridge to shared libraries (`*.so`).

The point of v15 is not to bind the whole operating system. It proves that Flowmini can treat an external shared-library symbol as a node-like callable contract with typed input ports, a typed return port, ABI metadata, and declared effects.

## Added syntax

```flow
abi libc {
    library "libc.so.6"
    convention c

    extern fn strlen(text : c_string): c_size_t {
        symbol "strlen"
        effect pure
    }
}
```

ABI blocks may be placed in imported library files, for example:

```flow
import "../std/abi/libc.flow"
```

## Included ABI library

```text
std/abi/libc.flow
```

It declares:

```text
strlen(c_string) -> c_size_t
abs(c_int)       -> c_int
labs(c_long)     -> c_long
puts(c_string)   -> c_int
```

## Supported ABI scope in v15

Supported:

```text
C calling convention only
simple scalar integer-like ABI types
borrowed c_string input
c_int / c_long / c_size_t return values
dlopen + dlsym proof-of-concept
```

Not supported yet:

```text
structs
callbacks
output pointer parameters
malloc/free
buffers
arrays through pointers
variadic functions
errno/error-flow integration
ownership/lifetime contracts
```

Wrong ABI declarations can crash a real process in principle. In v15, ABI declarations are trusted external contracts.

## Demo

```bash
./build/flowmini examples/abi_libc_demo.flow
```

Expected output on glibc/Linux is similar to:

```text
5
42
123456
Hello from Flowmini ABI
24
```

The final number is the return value from `puts`. It is platform/library dependent except that non-negative normally indicates success.

## Regression examples

```bash
./build/flowmini examples/type_contracts_demo.flow
./build/flowmini examples/capable_typed_imported.flow
./build/flowmini examples/capable.flow
```

Negative ABI tests:

```bash
./build/flowmini examples/bad_abi_wrong_arg.flow
./build/flowmini examples/bad_abi_unsupported_signature.flow
```

## Model

An external function is a node-like ABI contract:

```text
input ports  = ABI arguments
output port  = ABI return value
implementation = foreign shared-library symbol
effect = declared external effect
```

This keeps the Flowcore model intact: ABI calls are not magic, they are contracted external nodes.
