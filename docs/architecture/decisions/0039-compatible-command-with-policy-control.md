# ADR-0039: Compatible command with explicit policy control

**Status:** accepted direction, future implementation  
**Date:** 2026-08-20  
**Scope:** Frankencore replacements for established Unix commands

## Decision

An eventual Frankencore replacement preserves the established command's
default and canonical call conventions, then adds an explicitly named
Frankencore control plane for callers that want different semantics.

Example:

```text
ls [the established options and operands]
ls [the established options and operands] \
   --Use_call_Policy <policy> \
   --Use_call_Schema <schema>
```

Without the added controls, the replacement behaves as the established
command for the declared compatibility profile. With them, the caller
explicitly asks for policy- and schema-directed behavior that may change
selection, ordering, projection, provenance, parallelism, or other rules of
the operation.

## Compatibility floor

The replacement must first implement or delegate the old command's canonical
behavior. Existing scripts must not acquire new semantics merely because the
command name resolves to FrankenPOP.

Compatibility includes the old options, operands, output modes, exit status,
environment behavior, diagnostics, and relevant filesystem semantics. Any
unsupported legacy behavior must be declared and delegated or rejected
according to policy; it must not be silently approximated.

## Explicit control plane

`--Use_call_Policy` selects a named or resolvable policy. It may govern what
the operation is allowed to inspect, how it handles errors, which providers
it may use, and whether stronger confirmation or isolation is required.

`--Use_call_Schema` selects the input/output contract for the extended call.
It may define structured records, fields, ordering, projections, and
consumer-facing output without changing the legacy default.

The exact option spelling and casing remain provisional until a common CLI
convention is finalized. The semantic requirement is that both controls are
explicit, discoverable, versioned, and separate from ordinary legacy options.

## Example capability shape

```text
legacy caller
    → ls
    → familiar listing behavior

Frankencore caller
    → ls --Use_call_Policy=project-safe \
         --Use_call_Schema=frankencore.records.v1
    → policy-directed records with provenance
```

The same pattern applies to commands such as `cat`, `find`, `grep`, package
tools, editors, and system utilities. The command name remains a projection
of a capability; the explicit policy and schema select the richer contract.

## Safety and provenance

An extended call must record the selected policy, schema, provider, and
resulting operation provenance. A caller cannot use an added option to bypass
constitutional laws, trust requirements, or owner-attestation rules. More
powerful behavior is available through explicit policy, not hidden inference.

## Revisit triggers

Revisit when defining the common option spelling, implementing the first
`ls`-compatible projection, or defining the first versioned structured-output
schema.
