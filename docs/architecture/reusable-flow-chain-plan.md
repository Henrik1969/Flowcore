# Reusable Flow compiler-chain plan

## Objective

The built Flowcore tools must compile new Flow source files without adding a
C++ lowering profile or changing the toolchain binaries. Application semantics
belong in Flow source. C++ supplies reusable language machinery, substrate
bindings, providers, and backends.

```text
Flow source
  -> Flowmini frontend bundle
  -> Flowanalyst semantic report
  -> Flowbind capability authorization
  -> Flowparallel execution plan
  -> Flowoptimize transformed plan
  -> Flowlower backend IR
  -> native executable
```

## Current facts

- The chain is executable and regression-tested, but many native paths are
  selected by source-unit names and handwritten `lowering_profile` lists.
- Flowanalyst already knows call sites, resolved symbols, argument expression
  identities, result symbols, ABI requirements, effects, and provenance.
- Flowbind already authorizes provider/library/symbol/signature combinations.
- The missing reusable artifact is a structured lowering plan connecting those
  facts without fixture names.

## Migration gates

1. Emit `flowcore.lowering_plan` v1 beside the legacy profile field.
2. Make Flowbind authorize plan operations by exact capability/signature facts.
3. Make Flowparallel and Flowoptimize preserve plan operation identity and
   provenance while transforming it.
4. Make Flowlower consume generic value operations and external calls.
5. Add general control-flow/state lowering, then move `flow_less` paging logic
   into Flow source.
6. Convert existing fixtures one at a time and delete profile dispatch only
   after positive, negative, adversarial, sanitizer, and ELF tests pass.

## Non-negotiable invariants

- A new program must not require a C++ source edit or a new profile name.
- `->` value placement and `=>` graph connection remain distinct.
- Provider and capability authority are explicit and inspectable.
- Unsupported lowering fails with a structured report and provenance.
- Legacy compatibility is temporary and must not become the generic contract.

## First implementation slice

Flowanalyst emits one operation per resolved call site. An operation records its
callee, resolved provider symbol when present, ABI signature/effect facts,
argument expression IDs, result symbol, and source identity. This is additive to
semantic report v1 and is the input contract for the generic lowerer migration.
