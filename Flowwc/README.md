# flowwc

`flowwc.flow` is the first Flowmini application pressure probe for a
word-count-like utility. The counting state machine is written in Flowmini:
it consumes `stdin.bytes()`, counts newline-delimited lines, counts maximal
non-whitespace byte sequences as words, and reports the consumed byte count.

Run it through the hosted Flowmini runtime with:

```bash
flowmini Flowwc/src/flowwc.flow < file.txt
```

The stdin provider is a mechanism boundary; counting and application policy
remain in this Flowmini source. Path-based file input is recorded in the
pressure ledger before adding a broader provider/carrier contract.
