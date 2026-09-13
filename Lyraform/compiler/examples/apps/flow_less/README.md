# Native Flow pager

`flow_less.flow` owns command interpretation, page bounds, page extraction,
rendering and failure selection. Ordinary source functions receive native graph
activations through `source.out => navigate.in => display.in` (two wires).
Compiler tools contain no pager algorithm or application dispatch.

Separate provider libraries supply raw input batches and generic text/integer
output. The fake input reads `FLOW_PAGER_LINES` (pipe separated),
`FLOW_PAGER_COMMANDS` (comma separated) and `FLOW_PAGER_PAGE_SIZE` (default 2).
The terminal input reads `FLOW_PAGER_PATH` and a bounded batch of raw ncurses
key codes. Flow interprets down/up, PageDown/PageUp, Home/End and q. Down/up each
move one page. `q` stops processing the remainder of the delivered batch.

Build the root project, then run:

```sh
FLOWCORE_BUILD=/absolute/build/path ./run-flow-less.sh
FLOWCORE_BUILD=/absolute/build/path ./run-flow-less-ncurses.sh /path/to/text.txt
```

After `cmake --install BUILD --prefix PREFIX`, the scripts and source are under
`PREFIX/share/flowcore/examples/flow_less`. Set `FLOWCORE_PREFIX=PREFIX` to use only
the installed tools, generator and providers. Custom `lib64` installations can
supply explicit library paths.

The terminal default is one key; `FLOW_PAGER_KEY_COUNT` selects 1–4096 raw keys.
The provider closes ncurses before delivering the batch. Flow renders the final
page after processing it. This bounded example does not redraw between keys or
implement streams. Empty files render page 1/1 with an empty content line.

To retain native artifacts, use `build-flow-less.sh /tmp/native-pager` with the
same build environment. It generates evidence-bearing ABI imports and exact
policies, captures the source graph, runs the existing compiler stages, links
LLVM with the selected providers and graph runtime, and verifies unchanged tool
and provider hashes. Generated files remain outside the repository. The ncurses
source path is a symlink to the same Flow program; selection changes providers,
not application source. `FLOWPAGER_INPUT`, `FLOWPAGER_OUTPUT`,
`FLOWPAGER_NCURSES_INPUT` (terminal runner) and explicit stage binary paths support
injection without rebuilding compiler tools.

Input data stays valid until process exit after the single startup activation.
Source failures exit 70 with operation, activation, port, wire and signal evidence;
invalid page size, input failure, unknown command and output failure select codes
1, 2, 3 and 4 respectively. `FLOWCORE_GRAPH_TRACE=1` enables delivery traces.
Unconnected final output always emits a structured drop diagnostic.

This example targets Linux x86-64 with clang, a C++ linker and ncursesw.so.6.
The provider's dynamic ncurses dependency follows the platform loader; generated
binding evidence authenticates the selected provider bytes at bind time, not its
transitive dependencies or future process-time replacements.
