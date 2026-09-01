# FlowLFS factory proof: ripgrep v0

ripgrep 15.2.0 is the second package driven through the data-only Cargo
factory. Its canonical default build passed 114 unit and 332 integration tests
offline. Rollback and reforge reproduced the exact object
`sha256-b778ffaef4a6f8f31f7a6f37954ef62cd4fe8c1fae5f2e8d522119540a08fd5c`.

## Factory lesson

The host had a `/tmp/.git`, which correctly changed ripgrep's repository
detection and exposed ambient build-state leakage. Forge v0.2 gives every build
private `HOME`, `TMPDIR`, and Cargo home plus neutral global/system Git config.
No upstream source mutation or package-specific test exception was needed.

The v0.2 closed recipe also supports bounded `generate` rows. The compiled `rg`
binary generated its canonical man page and Bash, Zsh, and Fish completions.
No system or owner configuration was installed; optional PCRE2 remains a
separate future capability.

Verify inside the VM:

    /usr/local/sbin/flowprofile-factory-proof-ripgrep-v0 verify
    rg --version

Sealed runnable image:
`artifacts/FlowLFS-v0.1-factory-proof-ripgrep-v0.qcow2`, SHA-256
`69c8083a1ac68b041054c92b451dc602593d40fc586a577f860895910b2892fb`.
