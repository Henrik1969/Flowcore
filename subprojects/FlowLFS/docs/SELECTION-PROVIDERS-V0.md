# Selection providers v0

This profile is the first interchangeable higher-level FlowLFS capability.
`flowselect ITEM...` has one normalized contract and two providers: the minimal
native provider over `libflowterminal`, and canonical fzf 0.74.1.

Native remains the immutable default after fzf installation. The owner may run
`flowselection-provider set fzf`, inspect `status`, or `reset` to native. This
mutable decision is separate from package objects and is never inferred from
PATH or package presence. `--provider` supplies an explicit one-call choice.

The fzf adapter clears ambient fzf options and commands, provides candidates on
stdin, and grants no preview, history, execute, reload, transform, walker, or
shell-integration authority. Candidate newline/NUL ambiguity is rejected.

fzf was built from the signed-tag commit
`eae8d9d27eaeffc777699c01bf8f8b8c071908c1` with an offline `go.sum`-verified
vendor tree. Go 1.26.5 was built from canonical source using the official Go
1.24.6 bootstrap seed; a second source build produced byte-identical hashes for
all ten compiler/tool binaries before admission.

Runnable image: `artifacts/FlowLFS-v0.1-selection-providers-v0.qcow2`

SHA-256: `c451782c6ff82b548654403ba923fd063d8c2855e8cd624b8b779fb6c7274d35`

Launch with `scripts/launch-selection-providers-v0.sh`; SSH defaults to port
2234.
