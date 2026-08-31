# FlowLFS basis v2: terminal insulation

Basis v2 replaces `sel`'s direct ncurses dependency with the semantic
`libflowterminal.so.1` ABI. The functional program neither reads `TERM` nor
consults terminfo. The initial provider supplies a conservative POSIX byte-I/O
and normalized-key-event floor and degrades without selecting a terminal brand.

The provider preserves the exact incoming termios state. Normal cleanup is
explicit and statically governed; `atexit` covers ordinary process exit; an
independent guardian restores the state if the application crashes, disconnects,
or receives `SIGKILL`. Close-on-exec and direct parent-liveness observation keep
restoration independent of inherited descriptors.

The historical ncurses ABI and `sel_ncurses.flow` remain available as an
optional compatibility projection. They are not dependencies of basis v2.

Exact profile objects are recorded in `flowpkg/profiles/basis-v2.json`.
Construction proved complete reverse rollback, canonical Bash/SSH survival,
and offline store-only reprojection before the image was sealed.

Runnable artifact: `artifacts/FlowLFS-v0.1-basis-v2-terminal.qcow2`

SHA-256: `5d69ff9ffb5bf445215560a085e18cb4d5d8a960a8ffc672c73b6dbf90d6244f`

Run with `scripts/launch-basis-v2-terminal.sh`, then connect using
`ssh -p 2230 root@127.0.0.1`.

## Navigation correction

Interactive testing revealed that basis v2 normalized arrow events correctly
but `sel` consumed only one event and did not mutate its selection. The sealed
v2 image remains unchanged as evidence. Basis v2.1 adds the missing functional
navigation loop and portable byte-stream cursor feedback without adding ANSI,
terminfo, ncurses, or terminal-brand dependencies.

Corrected artifact:
`artifacts/FlowLFS-v0.1-basis-v2.1-terminal-navigation.qcow2`

SHA-256: `17ee68613a0a58ececa4985e9a0f3f850e90131d2e34cdd6526bbac9e171bad9`

Launch with `scripts/launch-basis-v2.1-navigation.sh`; its default SSH port is
2232.
