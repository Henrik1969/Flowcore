# Flow Policy Envelope Pattern

## Summary

The **Flow Policy Envelope Pattern** is a pipeline pattern where data does not move through the system as naked payload.

Instead, each flow item is wrapped in an envelope:

```text
Envelope {
    payload
    context
}
```

The context then carries shared execution state:

```text
PipelineContext {
    policies
    diagnostics
    logging/output configuration
    runtime surroundings
}
```

A producer creates the first envelope.
Stages transform envelopes.
Sinks consume the final envelope.

```text
Producer => Stage => Stage => Sink
```

In C++ proof-of-concept form:

```cpp
stdinProducer
    >> stage
    >> stdoutSink;
```

The syntax is intentionally readable. It mirrors the intended Flowcore idea:

```text
source => node => node => sink
```

The important idea is not the `>>` operator itself. The important idea is the separation of roles.

---

# Core Roles

## Producer

A **Producer** creates the first envelope.

Examples:

```text
stdin
file
socket
generated data
timer
device input
```

In the examples, the producer is:

```cpp
StdinProducer
```

Its job is to read all input from `stdin` and create:

```cpp
Envelope<ByteBuffer>
```

So the producer converts external input into a typed flow item.

```text
external world -> Envelope<Payload>
```

## Stage

A **Stage** transforms one envelope into another.

```text
Envelope<InputPayload> -> Envelope<OutputPayload>
```

In these examples, the payload type stays mostly the same:

```cpp
Envelope<ByteBuffer> -> Envelope<ByteBuffer>
```

but the meaning of the bytes changes.

Examples:

```text
Base64Stage
HexStage
BitArtStage
```

Each stage reads policy values from the shared context, performs its transformation, and may add diagnostics.

## Sink

A **Sink** consumes the final envelope.

Examples:

```text
stdout
stderr
file
log
database
network
diagnostic sink
```

In the examples:

```cpp
StdoutSink
LogSink
```

`StdoutSink` writes the resulting payload.

`LogSink` writes diagnostics and fatal errors.

This is important: normal output and diagnostics are separate concerns.

---

# The Envelope

The envelope used in the examples is intentionally small:

```cpp
template <typename Payload>
struct Envelope {
    Payload payload;
    PipelineContext* ctx = nullptr;
};
```

This means:

```text
payload = the actual data being transformed
ctx     = the execution surroundings that follow the flow
```

The payload is what the stage transforms.

The context is what the stage consults or reports to.

The context is not the data.
The policy is not the data.
Diagnostics are not the data.

They travel with the data because they are part of the execution environment.

---

# The Context

The context contains:

```cpp
struct PipelineContext {
    PolicyBag policies;
    std::vector<Diagnostic> diagnostics;
};
```

This is the carried runtime state.

It gives stages access to policy, and it gives them a place to report non-fatal complaints.

This avoids hidden global state.

Instead of every stage reaching into a singleton, the context is explicitly carried through the flow.

```text
good:
    Envelope carries context

less good:
    every stage secretly reads global singleton state
```

---

# PolicyBag

The policy bag is a generic key/value policy store:

```cpp
using PolicyValue = std::variant<bool, int, std::string>;
```

The policy object stores keys such as:

```text
base64.mode
base64.strict
hex.width
hex.uppercase
bitart.width
bitart.one
bitart.zero
```

Each stage reads only the policies it understands.

Example:

```cpp
const std::string mode =
    ctx.policies.getStringOrDefault(
        "hex.mode",
        "encode",
        ctx.diagnostics,
        "hex"
    );
```

This means:

```text
Try to read hex.mode as string.
If missing, use "encode".
If present but wrong type, use fallback and emit diagnostic.
```

So policies can be wrong without always being fatal.

That is a key idea.

---

# Diagnostics

Diagnostics are carried as flow-side information:

```cpp
struct Diagnostic {
    Severity severity;
    std::string stage;
    std::string message;
};
```

A stage can complain without stopping execution:

```text
warning in hex: policy 'hex.width' expected int; using fallback
```

A fatal problem is different.

Fatal problems throw:

```cpp
DiagnosticError
```

So we distinguish:

```text
recoverable complaint -> diagnostic carried forward
fatal condition       -> exception
```

This makes the pipeline robust.

Not all wrong input is death.
Some wrong input is merely a complaint plus a safe fallback.

---

# Example 1: base64.cpp

## Purpose

The Base64 example demonstrates a simple text/binary transformation.

```text
bytes -> base64 text
base64 text -> bytes
```

## Pipeline

```text
StdinProducer
    => Base64Stage
    => StdoutSink
```

## Policy

Typical policies:

```text
base64.mode = encode | decode
base64.strict = true | false
base64.ignore_whitespace = true | false
base64.wrap = 0 | 76 | ...
```

## What it proves

The Base64 example proves the basic pipeline model:

```text
Producer creates envelope
Stage transforms payload
Policy controls behavior
Sink writes output
Diagnostics are separate
```

Example:

```bash
echo "Hello World" | ./base64 --mode encode
```

Output:

```text
SGVsbG8gV29ybGQK
```

Roundtrip:

```bash
echo "Hello World" | ./base64 --mode encode | ./base64 --mode decode
```

Output:

```text
Hello World
```

## Character of this example

Base64 is a good first example because it is simple, recognizable, and demonstrates encoding/decoding.

But it does not strongly show ABI or record structure. It is mostly a transform.

---

# Example 2: hex.cpp

## Purpose

The Hex example demonstrates binary-safe transformation and formatted output.

```text
binary bytes -> readable hex notation
hex notation -> binary bytes
```

Example output:

```text
42 41 52 54 01 00 08 00
```

## Pipeline

```text
StdinProducer
    => HexStage
    => StdoutSink
```

## Policy

Typical policies:

```text
hex.mode = encode | decode
hex.width = 80
hex.uppercase = true
hex.strict = true
hex.ignore_whitespace = true
```

## What it proves

The Hex example proves that the payload path is binary-safe.

This matters because output may contain zero bytes, non-printable bytes, or arbitrary data.

So `StdoutSink` must write with:

```cpp
out_->write(
    env.payload.bytes.data(),
    static_cast<std::streamsize>(env.payload.bytes.size())
);
```

not just:

```cpp
*out_ << env.payload.bytes;
```

The hex program also demonstrates formatting policy:

```text
hex.width = 80
```

This is not part of the data itself. It is presentation/execution policy.

## Roundtrip test

```bash
cat base64 | ./hex --mode encode | ./hex --mode decode > Base64
diff base64 Base64
```

If `diff` prints nothing, the result is byte-identical.

## Character of this example

Hex is stronger than Base64 as a binary-flow proof.

It demonstrates:

```text
binary-safe sink
formatting policy
lossless roundtrip
human-readable representation of bytes
```

It still does not define its own file ABI. It only transforms data into another representation.

---

# Example 3: bart.cpp

## Purpose

The BART example demonstrates a tiny ABI-framed binary format.

It converts ASCII art into packed binary bits with a header, then converts it back again.

Input art:

```text
..##....
.####...
######..
.####...
..##....
```

Packed BART output:

```text
[ BART header ][ packed bit payload ]
```

Unpacked output:

```text
..##....
.####...
######..
.####...
..##....
```

## Pipeline

```text
StdinProducer
    => BitArtStage
    => StdoutSink
```

## Policy

Typical policies:

```text
bitart.mode = pack | unpack
bitart.width = 8
bitart.one = "#"
bitart.zero = "."
bitart.strict = true
bitart.ignore_whitespace = true
```

## ABI Header

The BART format has a small binary header:

```text
offset size field
0      4    magic: "BART"
4      1    version: 1
5      1    flags: 0
6      2    width, little-endian
8      2    height, little-endian
10     4    bit_count, little-endian
14     N    packed payload
```

This is an ABI/format contract.

The stage does not guess how to unpack the payload. It reads the header.

Example encoded output viewed through `hex`:

```text
42 41 52 54 01 00 08 00 05 00 28 00 00 00 30 78 FC 78 30
```

Decoded:

```text
42 41 52 54   magic "BART"
01            version 1
00            flags 0
08 00         width = 8
05 00         height = 5
28 00 00 00   bit_count = 40
30 78 FC 78 30 payload
```

The payload bytes are:

```text
30 = 00110000 = ..##....
78 = 01111000 = .####...
FC = 11111100 = ######..
78 = 01111000 = .####...
30 = 00110000 = ..##....
```

## What it proves

The BART example proves something the Base64 and Hex examples do not:

```text
A payload can have a declared representation contract.
```

The BART header is not policy.

The BART header is part of the data representation.

It says:

```text
this payload is BART version 1
width is 8
height is 5
bit_count is 40
payload is packed bits
```

That is different from policy.

---

# Policy vs ABI

This is one of the most important lessons from the three examples.

## Policy

Policy controls behavior.

Examples:

```text
hex.uppercase = true
hex.width = 80
base64.strict = false
bitart.one = "#"
bitart.zero = "."
```

Policy answers:

```text
How should this stage behave?
How strict should it be?
How should output be formatted?
Which notation should be used?
```

## ABI / Format

ABI describes representation.

Examples from BART:

```text
magic = "BART"
version = 1
width = 8
height = 5
bit_count = 40
endianness = little-endian
payload bits are packed MSB-first
```

ABI answers:

```text
What is this data?
How is it laid out?
How should it be read?
How many bytes/bits are meaningful?
Which version of the format is this?
```

## Short rule

```text
Policy is execution law.
ABI is representation law.
```

That distinction is central.

---

# Sameness Across All Three Programs

All three programs share the same architecture.

## Same producer concept

Each program starts by reading from standard input:

```text
stdin -> StdinProducer -> Envelope<ByteBuffer>
```

## Same envelope concept

Each program sends an envelope through the pipeline:

```cpp
Envelope<ByteBuffer>
```

## Same policy mechanism

Each program stores behavior settings in:

```cpp
PolicyBag
```

## Same diagnostic mechanism

Each program can emit diagnostics into:

```cpp
ctx.diagnostics
```

## Same fatal error mechanism

Each program can throw:

```cpp
DiagnosticError
```

## Same sink mechanism

Each program writes output through:

```cpp
StdoutSink
```

## Same orchestration style

Each program has a readable main pipeline:

```cpp
stdinProducer
    >> stage
    >> stdoutSink;
```

This is the C++ proof-of-concept version of:

```text
source => node => sink
```

---

# Differences Between the Programs

## Base64

Base64 is a codec stage.

```text
bytes <-> base64 text
```

It demonstrates:

```text
simple transform
mode policy
strict/permissive decode behavior
roundtrip
```

It does not define a custom file format.

## Hex

Hex is a representation stage.

```text
bytes <-> hex notation
```

It demonstrates:

```text
binary safety
formatted output
line width policy
human-readable byte representation
lossless roundtrip
```

It also does not define a custom file format.

## BART

BART is a tiny format stage.

```text
ASCII art <-> ABI-framed packed binary
```

It demonstrates:

```text
custom binary header
magic/version fields
little-endian integer fields
bit packing
format contract
policy-controlled rendering
lossless roundtrip
```

BART is the most Flowcore-like of the examples because it separates:

```text
text notation
semantic bits
binary representation
rendering policy
ABI contract
```

---

# Why This Matters for Flowcore

These examples show that Flowcore can lower cleanly to C++.

Flowcore concept:

```text
Producer => Node => Node => Sink
```

C++ proof-of-concept:

```cpp
stdinProducer
    >> flt_Lex
    >> flt_BuildAst
    >> flt_Syntax
    >> flt_Lower
    >> flt_Semantic
    >> outputSink;
```

The examples are tiny, but they demonstrate the same structure:

```text
Producer creates an envelope.
Stage transforms an envelope.
Sink consumes an envelope.
Policy travels with the envelope.
Diagnostics accumulate in context.
Fatal errors throw rich exceptions.
ABI describes payload representation.
```

This maps well to Flowcore:

```text
Flowcore Producer -> C++ Producer class
Flowcore Node     -> C++ Stage class
Flowcore Sink     -> C++ Sink class
Flowcore Signal   -> C++ Payload type
Flowcore Policy   -> PolicyBag / PolicyView
Flowcore Wire     -> typed pipeline connection
Flowcore ABI      -> explicit representation contract
```

---

# Important Design Principle

The envelope must stay narrow.

It should carry what must follow the flow.

It should not become a suitcase full of random global convenience.

Good envelope/context contents:

```text
policy
diagnostics
source location context
artifact path
logging sink
runtime execution settings
```

Bad envelope/context contents:

```text
unrelated global state
random caches
everything the program might ever need
hidden dependencies
```

Rule:

```text
Carry what must follow the flow.
Do not carry what merely happens to be convenient.
```

---

# Error Handling Model

The examples use two levels of error handling.

## Non-fatal complaint

A stage may recover and continue:

```text
policy value invalid -> use fallback -> emit warning
```

Example:

```text
hex.width expected int; using fallback
```

## Fatal diagnostic

A stage may stop execution:

```text
invalid BART magic
unsupported BART version
payload shorter than header declares
```

This throws:

```cpp
DiagnosticError
```

Top-level orchestration catches it:

```cpp
catch (const DiagnosticError& err) {
    log.writeFatal(err);
    log.write(ctx);
    return EXIT_FAILURE;
}
```

The top level decides how diagnostics are printed.

Stages do not directly decide presentation.

---

# The Pattern in One Sentence

The **Flow Policy Envelope Pattern** carries payload, policy, context, and diagnostics through a typed producer-stage-sink pipeline, allowing each stage to transform data under declared execution policy while preserving clean separation between data, behavior, diagnostics, and representation.

---

# Short Version

```text
Producer creates.
Stage transforms.
Sink consumes.

Payload is data.
Policy is execution law.
ABI is representation law.
Diagnostics are flow.
Fatal errors stop the route.
```

---

# Minimal Mental Model

```text
cat input
  | producer wraps it
  | stage transforms it according to policy
  | diagnostics travel beside it
  | sink writes the result
```

Or in C++:

```cpp
stdinProducer
    >> stage
    >> stdoutSink;
```

Or in Flowcore spirit:

```text
stdin => stage => stdout
```

The three programs prove the same pattern from three angles:

```text
base64  proves simple transformation
hex     proves binary-safe representation
bart    proves ABI-framed payload format
```

Together they form a small reference lab for Flowcore-light in C++.

