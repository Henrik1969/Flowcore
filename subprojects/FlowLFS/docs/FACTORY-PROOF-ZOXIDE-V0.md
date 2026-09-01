# FlowLFS factory proof: zoxide v0

This image proves that the fabric can consume a closed, data-only Cargo recipe
and a pinned vendored source envelope, test and compile as the unprivileged
`flowbuilder` with Cargo offline, admit an immutable object, and project it with
the native projector.

The decisive rollback/reforge run produced the exact same zoxide object:
`sha256-ef684892b18d6698c8994909527c95255404ec7d2a0bef8a18be7ffb6dafdd72`.
All 16 upstream unit tests passed on both recorded builds.

## Authority boundary learned

A receipt copy of the recipe belongs inside the resulting object, but the
authoritative recipe cannot live only inside its product: product rollback
would remove the means to rebuild it. The locked authority therefore lives at
`/flow/forge/specs/zoxide-0.10.0.tsv`, independently of the projection.

## Additive shell use

Installation does not alter Bash, Zsh, `cd`, `/etc/skel`, or owner state. A user
chooses integration in their current shell:

    eval "$(flow-zoxide-init bash)"
    eval "$(flow-zoxide-init zsh)"

## Verify

    /usr/local/sbin/flowprofile-factory-proof-zoxide-v0 verify
    zoxide --version

Sealed runnable image:
`artifacts/FlowLFS-v0.1-factory-proof-zoxide-v0.qcow2`, SHA-256
`cb2be4e143caf92251b089c1b58c6b1c6c67a47277a18408a2cd8f5ce7b03c33`.

The Rust 1.97.1 object is explicitly an official bootstrap seed. It is not
represented as a compiler built from source; it enables the first source-built
Cargo workload and can later be replaced by a bootstrapped compiler chain.
