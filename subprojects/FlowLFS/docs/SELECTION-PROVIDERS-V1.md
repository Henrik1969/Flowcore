# Selection providers v1

This immutable correction preserves the `flowselection.cli.v1` contract and
the native/fzf provider boundary from v0. It changes only the native provider's
presentation projection.

Native now writes a complete candidate list to `/dev/tty`, marks the selected
item with `> `, and emits a new complete state frame after Up or Down changes
the selection. It deliberately uses neither terminfo nor terminal cursor
addressing. This makes the baseline projection readable on ordinary, unknown,
and log-like terminals; richer in-place redraw remains the fzf projection's
concern. Standard output still contains only the final selected item.

The source-built Go 1.26.5 and canonical fzf 0.74.1 objects are unchanged.
`flow-selection` 0.1.1 is the new immutable object
`sha256-ef7c586436891172f0c820fbc507204646e1561d2e908f5797e8c58a7a6c9199`.
Its build gate drives Down+Enter through an invented-terminal PTY and requires
the initial list, updated marker, and normalized `beta` result.

Runnable image: `artifacts/FlowLFS-v0.1-selection-providers-v1.qcow2`

SHA-256: `a5c245619257f9bd79b764423a7616cc25b47703cebc451f7abd4680fae08e77`

Launch with `scripts/launch-selection-providers-v1.sh`; SSH defaults to port
2236.
