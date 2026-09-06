# UTF-8 source-reader contract v1

**Status:** Stage 0 reference boundary

This contract defines the first language-closure slice for a Flow-written
source reader. The current implementation is C++ in the active Flowmini
frontend. It is a bootstrap reference, not a permanent implementation
requirement.

## Input and output

The reader consumes an exact byte string. It emits an ordered sequence of
records:

```text
scalar value:    Unicode scalar value, 0..0x10ffff excluding surrogates
byte offset:     zero-based offset of the first byte
byte length:     one through four bytes in the original input
```

The reader also emits deterministic diagnostics. Each diagnostic has a stable
code and the byte offset at which the malformed sequence begins.

## Validation

Accepted input is strict UTF-8. The reader rejects unexpected continuation
bytes, invalid leading bytes, truncated sequences, invalid continuation bytes,
overlong encodings, surrogate scalars, and values outside the Unicode scalar
range.

After a malformed sequence the reference reader advances one byte. This keeps
diagnostic discovery deterministic and allows later independent malformed
bytes to remain visible. Consumers must not use partially decoded values as
valid source scalars when diagnostics are present.

## Authority and evolution

The C++ implementation and its tests are Stage 0 evidence. A Flow
implementation must consume the same captured byte corpus and reproduce the
same scalar records and diagnostic codes. If a clearer semantic contract is
adopted later, the contract and corpus change first; the C++ reference is then
updated or retained as historical evidence, and Flow receives a differential
test for the revised behavior.

The existing lexer is intentionally unchanged in this checkpoint. It still
operates on source bytes and remains outside this validated reader boundary.

## Flow probe status

`Flowmini/flowmini_v25_symboltable_projection/examples/bootstrap/utf8_source_reader_probe.flow`
is the first Flow-side consumer. It reproduces the valid two-byte scalar
calculation from the captured fixture using ordinary list values, indexing,
arithmetic, and a function call. The probe is intentionally not called a full
reader: general byte/file input and structured diagnostic collection are still
language/library gaps to close before a Flow implementation can replace the
C++ reference.
