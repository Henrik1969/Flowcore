# Flowselection

Flowselection defines an interactive single-selection capability independently
of its presentation provider.

## CLI contract v1

`flowselect [--provider native|fzf] ITEM...` writes exactly the selected item
and a newline to standard output. Exit 0 means selected, 1 means cancelled, and
2 means contract/provider error. Items containing newline or NUL are rejected.

The owner policy is `/etc/flowcore/selection/provider`. Optional fallback is
separately authorized by `/etc/flowcore/selection/fallback`; provider presence
alone never changes behavior.

`flowselection-provider set fzf` records an owner decision in mutable
`provider.local`; `reset` returns to the immutable native default.

Providers receive candidates as arguments, not shell source. The fzf adapter
clears ambient fzf command/options variables, supplies candidates on standard
input, and admits no preview, history, reload, transform, execute, walker, or
shell-integration authority.
