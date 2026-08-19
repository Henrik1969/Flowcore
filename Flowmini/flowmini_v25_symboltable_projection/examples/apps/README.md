# Application specimens

These examples are intentionally placed after the language-chain probes. They
are end-to-end application candidates, not yet promises of a complete runtime.

## `flowcat`

`flowcat` establishes the first application contract:

```flow
main(args : list<string>) {
}
```

The current chain accepts and analyses the typed entry point. The remaining
boundary is the standard-library and lowering contract for turning process
arguments into a Flowcore `list<string>` and streaming them to output. Until
that exists, this specimen is expected to stop at the semantic report rather
than claim to be an executable `cat` replacement.
