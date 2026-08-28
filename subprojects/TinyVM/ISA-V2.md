# TinyVM typed ISA version 2 execution inputs

ISA 2 preserves every ISA 1 opcode and carrier semantic unchanged. It adds one
execution-input convention without adding serialized host state:

- slot 0 is initialized to the process-style argument count as canonical
  `i32`;
- slot `1 + n` is initialized, when argument `n` exists, to opaque handle
  `0x03_00000000000000 | n`;
- absent argument slots remain uninitialized and fault if read;
- the artifact contains neither argument bytes nor host addresses.

The first argument is the artifact/program identity, matching native `argv[0]`.
Thus `flowtinyrun program.tvm selected` exposes count 2 and argument handle 1
for `selected`. A future governed string provider may resolve an argument handle
to bytes; the portable VM value remains an opaque identity.

Lowering computes the highest statically indexed argument, reserves the needed
slots, and emits a checked-count guard returning `i32(64)` before any indexed
access. Dynamic argument indexing is not admitted. Artifacts without argument
intrinsics continue to use ISA 1.

Portable-switch and computed-goto execution initialize identical values and
their complete post-execution states remain differentially tested.
