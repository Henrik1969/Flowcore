# Byte, text, and outcome foundation v1

**Status:** design boundary for the next language-closure slice

Flow’s core language must be able to process source bytes on a hosted system,
in a bootloader, and in a freestanding system without changing semantic
meaning. A hosted standard-input call is therefore a provider, not the source
reader abstraction.

## Core values

The first target-neutral values are:

```text
ByteBuffer  bounded or explicitly dynamic sequence of uint8 values
Scalar      Unicode scalar value with a validated source encoding
Span        byte offset plus byte length into an identified source buffer
Diagnostic  stable code, severity, span, and optional structured payload
Outcome<T>  success(T) or failure with an ordered diagnostic collection
```

Byte offsets are authoritative for decoding. Scalar values are never used as
byte offsets. A span always identifies its source buffer, so diagnostics do not
silently refer to a different input after concatenation or transformation.

## Provider boundary

The language/library contract consumes a `ByteBuffer` or a declared source
provider capability. Hosted file and standard-input providers may populate that
buffer. A bootloader, firmware image, ROM window, or device provider may supply
the same value through a target policy. The decoder, tokenizer, and parser do
not depend on file descriptors, libc, terminals, or an operating system.

Providers must declare capacity, ownership, lifetime, and failure behavior.
Dynamic allocation is not implied by the abstract value; a bare-metal target
may admit only a fixed-capacity buffer and return an explicit exhaustion
outcome.

## Staged implementation

1. Preserve the C++ Stage 0 UTF-8 artifact corpus.
2. Add a Flow byte-buffer representation using currently admitted integer/list
   operations as a temporary reference projection.
3. Add structured diagnostic and outcome records without hiding allocation or
   cleanup behavior.
4. Add a hosted byte provider as an explicit capability and capture its input
   into the same artifact format.
5. Implement the decoder in Flow and compare scalar, span, and diagnostic
   artifacts against C++.
6. Add fixed-capacity and freestanding providers before declaring the facility
   generally available.

The current UTF-8 Flow probe is intentionally earlier than this boundary: it
proves arithmetic and iteration, while its hardcoded lists make the missing
provider, diagnostic records, and ownership semantics visible.
