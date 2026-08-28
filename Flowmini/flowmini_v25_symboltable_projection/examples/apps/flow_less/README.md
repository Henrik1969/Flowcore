# flow_less application example

`flow_less` is the first pager-shaped Flow application. Its source contains no
terminal library calls, cursor arithmetic, or provider-specific state. It reads
an actual text file and connects a pager provider to a plain renderer through
ordinary graph contracts:

```text
pager provider -> page record -> renderer -> sink
```

The deterministic slice is source-composed from `pager.input.fake`, the
provider-neutral `pager.navigate` state transition, `pager.render`, and a
terminal sink. Lines and commands belong to the injectable input provider;
page selection belongs to the separately wired navigation stage rather than
the provider. The interactive application slice uses
`pager.input.file.ncurses`, which opens the path declared by
its policy, reads the file, and renders pages in a pseudo-terminal. The source
template uses `__FLOW_LESS_PATH__`; the runner substitutes the concrete file
under test, keeping the checked-in source portable.

The provider contract is deliberately separate from navigation and projection.
`pager.input.file.ncurses` reads the file and maps terminal keys to provider-
neutral commands; the same `pager.navigate` node used by the fake input applies
`q`, arrows, PageUp, PageDown, Home, and End behavior. It loads the existing
ncurses provider dynamically; Flowcore does not reimplement ncurses. The
terminal test feeds `q` through a pseudo-terminal and checks the rendered
contents of a temporary text file.

Run the deterministic slice with:

```sh
./run-flow-less.sh
```

After building Flowmini, page any text file in the real terminal provider with:

```sh
./run-flow-less-ncurses.sh /path/to/a/text-file
```

Use `q` to quit, the arrow keys to move by line, PageUp/PageDown to move by
page, and Home/End to jump to the first or last page.
