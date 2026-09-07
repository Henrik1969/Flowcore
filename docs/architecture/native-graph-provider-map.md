# Native graph provider selection

`flowcore.graph_provider_map` v1 is an explicit, non-authorizing selection file.
Flowanalyst accepts it with `--graph-providers path`. An entry maps the arbitrary
implementation name already present in a graph declaration to one qualified
external function in the captured frontend symbol table:

```json
{"format":"flowcore.graph_provider_map","version":1,"providers":[
  {"implementation":"input.batch","source_callable":"host.read_batch",
   "activation":"startup_once","output_port":"out"}
]}
```

The bounded adapter requires a producer node, a zero-argument external function,
and one returned value on `out`. Startup invokes a producer once, consistent with
the existing serial interpreter's initial producer activation. It defines no
streaming, repeated output, asynchronous activation, persistent receiver state or
new source-function trigger. Provider behavior remains external and explicitly
selected; a source receiver still obeys the owner's fresh-single-input contract.

Selection resolves source callable identity, provider/library/native symbol,
convention, carrier/effect facts and generated evidence independently. Neither a
factory-like name nor successful resolution grants capability authority.
`source_graph.providers` retains resolved facts and `provider_selection` retains
the selection file for independent validation. Import aliases belong to the
source reference, never the native symbol. Unused selections have no activation.

The current checkpoint only captures and validates this evidence. All source
graphs remain non-executable at downstream compiler boundaries. The next slice
must authorize producer calls, publish graph scheduling with full endpoint,
wire and signal identities, and lower source receiver invocation. Removing the
existing graph refusal before that route works would silently change programs.
