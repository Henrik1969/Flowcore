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
    while index < length(args) {
        open/sendfile/close the source-selected path
    }
}
```

The real source spells out argv traversal, nested transfer loops, external
calls, error branches, cleanup, mutation, and return values; the abbreviated
snippet above only shows the shape. Each process argument after the executable
name is a path. The explicit `file_io` ABI contract authorizes
`open`, `sendfile`, and `close` through the adjacent `policy.conf`;
Flowbind verifies the exact C symbols before LLVM is emitted. The generated
program transfers each file in bounded chunks to stdout. A short transfer
advances the kernel-managed input offset and the Flow loop continues, avoiding
implicit pointer arithmetic.

The current gate rejects an open, transfer, or close failure with exit status
1. It does not yet preserve errno details, metadata, file permissions, or a
configurable transfer size; `sendfile` also makes this example Linux-specific.

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
