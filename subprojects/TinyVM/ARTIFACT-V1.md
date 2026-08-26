# `flowcore.tinyvm_artifact` binary format version 1

This is the first durable TinyVM execution boundary. It carries only recovered
ISA version 0. It is deliberately not yet the Flow-capable ISA or complete
backend contract described by the integration plan.

All integers are little-endian. Files consist of exactly one 512-byte header
followed by `code_count` 32-byte instruction words. No trailing bytes are
allowed.

| Offset | Bytes | Field |
|---:|---:|---|
| 0 | 8 | `FLOWTVM\0` magic |
| 8 | 4 | artifact format version (`1`) |
| 12 | 4 | ISA version (`0`, recovered prototype) |
| 16 | 4 | header size (`512`) |
| 20 | 4 | flags; must be zero |
| 24 | 1 | byte order (`1`, little-endian) |
| 25 | 1 | instruction size (`32`) |
| 26 | 6 | reserved; must be zero |
| 32 | 8 | entrypoint instruction index |
| 40 | 8 | instruction count |
| 48 | 8 | declared data words |
| 56 | 8 | declared stack words |
| 64 | 64 | artifact identity, zero-terminated |
| 128 | 64 | source identity, zero-terminated |
| 192 | 64 | target identity (`tinyvm-portable`) |
| 256 | 64 | lowering-plan identity |
| 320 | 64 | optimization identity |
| 384 | 32 | SHA-256 of the complete file with this field zeroed |
| 416 | 96 | reserved; must be zero |
| 512 | variable | four signed 64-bit fields per instruction |

Identity fields use printable ASCII letters, digits, `.`, `_`, `:`, `/`, `+`
and `-`. They must be non-empty and zero-terminated within their field.

Validation is complete-input and fail-closed. Version, target, digest, file
size, resources, entrypoint, opcode, register operands, reserved instruction
field, statically resolvable jump targets and final `HALT` are checked before
execution. Version 1 declares no imports, constants, strings, per-operation
provenance or instrumentation sections; those require a later format revision
rather than reinterpretation of reserved bytes.

`flowtinyvalidate` independently reports validity. `flowtinyrun` parses and
validates again and therefore never treats a prior validation report as
authorization.
