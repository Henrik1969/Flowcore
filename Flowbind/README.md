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
for a later lowering profile; the `flowcat_argv_main` example demonstrates this
The `flowcat_file_main` example uses exact `libc.so.6` grants for `open`,
`read`, `write`, and `close`.

Ready reports also contain a `capabilities` array. Each entry preserves the
declared contract, provider library, symbol, calling convention, effect, ABI
types, and authorization status for downstream inspectors and lowerers.

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
