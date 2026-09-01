# Flow zoxide integration

This package changes no shell startup file and creates no owner state.
The owner explicitly opts in with one of:

    eval "$(flow-zoxide-init bash)"
    eval "$(flow-zoxide-init zsh)"

The emitted integration is zoxide's canonical shell projection. Canonical
`cd`, Bash, Zsh, and `/etc/skel` behaviour remain unchanged until that action.
