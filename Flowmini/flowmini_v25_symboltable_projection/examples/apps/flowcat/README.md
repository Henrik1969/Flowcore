# flowcat application example

This is the first complete application-shaped example in the Flowcore
compiler chain.

```text
Flow source
  -> FlowMini frontend bundle
  -> Flowanalyst semantic report
  -> Flowbind capability verification
  -> Flowoptimize optimization report
  -> Flowlower LLVM IR
  -> clang executable
```

The source declares the canonical typed entry point:

```flow
main(args : list<string>) {
    print args
}
```

For this vertical slice, `flowcat` treats each process argument after the
executable name as a path. The explicit `file_io` ABI contract authorizes
`open`, `read`, `write`, and `close` through the adjacent `policy.conf`;
Flowbind verifies the exact C symbols before LLVM is emitted. The generated
program reads each file in bounded chunks and writes the bytes to stdout.

The current gate rejects an open failure, read failure, close failure, or
short/failed write with exit status 1. It does not yet preserve errno details,
metadata, file permissions, or a configurable buffer size; those belong to
later capability slices.

## Run it

Build all sibling projects first, then run:

```sh
./run-flowcat.sh
```

By default the runner writes all intermediate reports and LLVM into a temporary
directory, leaving the example directory clean. It verifies the final output
against `expected-stdout.txt`.

To preserve the complete inspection build:

```sh
./run-flowcat.sh --keep-build
```

This leaves the generated binary at `build/flowcat`, with the frontend bundle,
semantic report, binding report, optimization report, lowering report, LLVM,
and captured output beside it. The `build/` directory is ignored and is not a
source artifact.

The runner accepts tool overrides for alternate builds:

```sh
FLOWMINI_BIN=/path/to/flowmini \
FLOWANALYST_BIN=/path/to/flowanalyst \
FLOWPARALLEL_BIN=/path/to/flowparallel \
FLOWBIND_BIN=/path/to/flowbind \
FLOWOPTIMIZE_BIN=/path/to/flowoptimize \
FLOWLOWER_BIN=/path/to/flowlower \
./run-flowcat.sh
```
