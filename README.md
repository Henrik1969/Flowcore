# Flowcore

Flowcore is the controlling superproject for the Flowcore language toolchain.

The language is still being defined. This repository keeps the architecture open
and inspectable while the compiler chain evolves.

## Compiler Chain

The intended high-level chain is:

```text
flowlex | flowparse | flowoptimize | flowlink | flowemit
```

Stage intent:

- `flowlex`: turn source text into token signals.
- `flowparse`: turn token signals into structural or semantic graph artifacts.
- `flowoptimize`: transform graph artifacts without changing their contract.
- `flowlink`: combine artifacts and resolve cross-artifact contracts.
- `flowemit`: produce executable, object, bytecode, text, or other target output.

## Subprojects

AstLiib is included as the generic token-tree subproject. The superproject does not
take ownership of AstLiib internals; it builds AstLiib through its public CMake and
uses it as a component in the larger compiler chain.

By default the superproject expects AstLiib beside this directory:

```text
../AstLiib
```

Override it with:

```bash
cmake -S . -B build -DFLOWCORE_ASTLIIB_SOURCE_DIR=/path/to/AstLiib
```

## Build

From this directory:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

To build only the chain-level aggregate target:

```bash
cmake --build build --target flowcore_chain
```

## Architecture Rule

Flowcore keeps these layers separate:

- graph activation
- node interface
- node implementation
- ABI contract
- ownership and lifetime
- effects
- failure/status flow

For AstLiib specifically, this means token storage belongs to the host, tree storage
belongs to AstLiib, and grammar-specific parsing stays outside AstLiib.
