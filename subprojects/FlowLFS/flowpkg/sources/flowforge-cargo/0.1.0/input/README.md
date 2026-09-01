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
