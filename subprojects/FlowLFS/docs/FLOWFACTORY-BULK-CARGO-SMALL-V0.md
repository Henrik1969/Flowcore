# Flowfactory small Cargo bulk v0

One invocation of the stable `flowfactory` boundary resolved, vendored, built,
tested, admitted, rolled back, reforged, combined, sealed, and cold-booted two
canonical packages:

- fd 10.5.0: 268 passing tests per build; object
  `sha256-6b6f51ee73fb75a539759d1a91ab848ba3cbe382714389cd74f449dd31a7a466`.
- bat 0.26.1: 340 passing tests and four upstream-declared ignored tests per
  build; object
  `sha256-c086f612bcf22e2ebda0f9bc2b208483223e5c62be69999f76a1e8a50f6fd336`.

Both second builds reproduced their first object identity exactly. Neither
package installs configuration or owner state.

## Lessons promoted into the factory

1. SSH must detach from manifest stdin or it consumes later package rows.
2. Manifest count must equal realized-object count before sealing.
3. Build-output exclusions must be root-scoped; a broad `target` exclusion
   corrupted the legitimate vendored path `cc/src/target/llvm.rs`.
4. Failed working twins are automatically discarded while verified source
   envelopes remain reusable.
5. The reviewed `flowfactory` command is the single permission boundary for
   retrieval, VM transport, QEMU execution, and sealing.

The first incomplete fd-only run was rejected and removed; it was never kept as
the bulk artifact.

## Run

    subprojects/FlowLFS/scripts/launch-bulk-cargo-small-v0.sh
    ssh -p 2244 root@127.0.0.1
    fd --version
    bat --version
    /usr/local/sbin/flowforge-cargo-package-v0.2 /flow/forge/specs/fd-10.5.0.tsv verify
    /usr/local/sbin/flowforge-cargo-package-v0.2 /flow/forge/specs/bat-0.26.1.tsv verify

Sealed image SHA-256:
`08291b9e934c7b13a0ee3e667134fb1d5251442b3940247e859ef1378f71fb8d`.
