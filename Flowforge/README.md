# Flowforge

Flowforge turns a reviewed package specification and an immutable offline
source envelope into a tested FlowLFS package object. The first admitted build
class is `flowforge.cargo-package.v0`.

The controller recognizes a closed set of fields. It never sources the spec or
evaluates it as shell code. Package identity, source digest, toolchain provider,
test/build arguments, install mappings, runtime version, and authority notes
remain data. Object admission and projection use the existing FlowLFS store and
native projector.

The Cargo class requires `Cargo.lock`, an in-envelope vendor directory, Cargo
offline mode, a successful declared test command, and an exact runtime version
check. It does not grant shell initialization, user-state mutation, network, or
service authority.

Version 0.2 isolates each build behind private `HOME`, `TMPDIR`, and Cargo home
directories and neutral Git configuration. Closed `generate` rows allow the
built executable to produce documentation and completions into its staged
object without granting general shell hooks.

`scripts/flowfactory realize-bulk MANIFEST` is the stable host entry point. It
performs canonical retrieval, locked vendoring, isolated offline builds,
immutable admission, exact rollback/reforge comparison, combined realization,
VM sealing, and cold-boot verification behind one reviewed command boundary.
