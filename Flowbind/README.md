# Flowbind

Flowbind is the first external-world boundary after Flowanalyst. It consumes a
`flowanalyst.semantic_report` and verifies the report's declared external
library and symbol requirements with the host dynamic loader.

The v0.1 provider performs discovery only:

```text
FlowMini → Flowanalyst → Flowbind
```

It uses `dlopen` and `dlsym` to prove that a declared library and symbol are
available. It also verifies the v0.1 supported C signature family
(`c_int`, `c_long`, `c_ulong`, `c_size_t`, `c_string`, and `c_pointer`) and reports host
layout facts.
It never calls a foreign function. A ready report is the authorization input
for downstream generic lowering. The profile-free `flowcat` example uses exact
`libc.so.6` grants for `open`, `read`, `write`, and `close`.

Ready reports also contain a `capabilities` array. Each entry preserves the
declared contract, provider library, symbol, calling convention, effect, ABI
types, and authorization status for downstream inspectors and lowerers.

Resolution is an explicit deployment policy on the same binding contract:

```text
flowbind --resolution dynamic report.json   # runtime provider loading
flowbind --resolution linked report.json    # conventional link-time artifact
```

Both modes use the host loader during verification so missing libraries,
symbols, and unsupported declarations are rejected before lowering. The
`provider.resolution` field records the intended deployment mode; Flowbind
does not perform the final static link and does not execute a foreign call.

An optional `--abi-manifest manifest.json` consumes provider-owned aggregate
layout evidence. Flowbind reports `aggregate_abi: verified` when the manifest
matches the semantic aggregate declaration, but aggregate calls remain blocked
until aggregate lowering is separately implemented and tested.

Required CLI invariants:

```text
-h, -?, --help
-a, --about
-v, --version
```

The binding report is a versioned consumer boundary. A ready report means only
that provider discovery succeeded; `execution` is explicitly
`not-performed`.

## C binding generator prototype

`tools/flowbind-gen` is an EXPERIMENTAL, deterministic prototype for
C-compatible headers. It delegates declaration parsing to Clang's AST JSON
output and emits an inspectable `flowbind.c_binding` artifact. The current
subset covers scalar carriers, enums, function declarations, opaque pointer
handles, and explicit resource metadata supplied with repeatable options such
as:

```text
--resource sqlite3_open:sqlite3_handle:sqlite3_close
```

Unsupported declarations remain in an `unsupported` array and make the
artifact `partial`. Generator tests exercise installed libm, zlib, sqlite3,
and libcurl headers; this is binding evidence, not a claim that Flowbind calls
or fully models those libraries. C++/Rust/Zig/Mojo use remains an external
C-compatible adapter/provider decision.

External use is denied unless an exact capability grant is supplied:

```text
allow libc.so.6 strlen c pure c_string c_size_t
allow libc.so.6 puts c io
```

The optional final two fields bind a grant to the declared parameter and
return ABI types. Older four-field grants remain accepted for compatibility,
but do not make a signature-specific claim.

The ABI summary reports carrier-type support as `carrier_types_supported`.
Provider-exact signature evidence is explicitly reported as `not-provided`
until a provider manifest supplies it.

Pass the policy with `--policy path`. The policy is intentionally small and
explicit; environment and configuration discovery belong to a later policy
boundary.
