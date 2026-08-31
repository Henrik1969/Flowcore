# FlowLFS basis v1 evidence

`basis-v1-evidence.tar.xz` preserves the construction logs, pseudo-terminal
tests, object manifests and derivations, active realization, and rollback
realization from the disposable construction VM.

SHA-256: `f5442b02e000163458f7c175929d38abfb4b65a4e97d408a9a696ca46055bd3c`

Acceptance passed for canonical `useradd -m -k /etc/skel`, user ownership of
copied templates, ordinary-user Bash and Zsh startup, `/usr/local/sbin` PATH
reachability, private history placement, moniker navigation, ordinary-user
`sel`, recovery Bash, SSH survival, complete reverse rollback, and offline
store-only reprojection. The independently selected modern CLI profile remained
valid across the basis lifecycle.

The `sel` evidence includes canonical Flow source and the complete preserved
frontend, semantic, optimization, binding, lowering, LLVM, and assembly chain.
The build records the single `.addrsig` compatibility adaptation required by
GNU `as` and performs final assembly/link inside FlowLFS as `flowbuilder`.
