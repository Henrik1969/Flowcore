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

For this first vertical slice, `flowcat` prints each process argument after
the executable name on its own line. The `puts` capability is granted only by
the adjacent `policy.conf`; Flowbind verifies that `libc.so.6` exports the
authorized C ABI symbol before LLVM is emitted.

This is intentionally an argv-output application, not yet a full file-content
`cat` implementation. File opening, reading, buffering, and error reporting
are the next standard-library capability boundary.

## Run it

Build all sibling projects first, then run:

```sh
./run-flowcat.sh
```

The runner writes all intermediate reports and LLVM into a temporary directory,
leaving the example directory clean. It verifies the final output against
`expected-stdout.txt`.

The runner accepts tool overrides for alternate builds:

```sh
FLOWMINI_BIN=/path/to/flowmini \
FLOWANALYST_BIN=/path/to/flowanalyst \
FLOWBIND_BIN=/path/to/flowbind \
FLOWOPTIMIZE_BIN=/path/to/flowoptimize \
FLOWLOWER_BIN=/path/to/flowlower \
./run-flowcat.sh
```
