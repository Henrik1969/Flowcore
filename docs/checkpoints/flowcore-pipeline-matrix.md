---
title: Flowcore pipeline matrix
status: active-development-gate
---

# Flowcore pipeline matrix

The matrix exercises the complete process boundary:

```text
FlowMini → Flowanalyst → Flowoptimize → Flowlower
```

It distinguishes a successful semantic path from a deliberate semantic block.
This prevents unfinished language areas from being mistaken for regressions.

## Current cases

| Fixture | Purpose | Expected result |
|---|---|---|
| `call_expression_probe` | names, calls, arguments, return types | accepted through Flowlower boundary |
| `control_flow_unary_probe` | control flow and unary expressions | accepted through Flowlower boundary |
| `expression_pool_probe` | expression ownership and local resolution | accepted through Flowlower boundary |
| `refined_contract_probe` | refined bases and invariant bindings | accepted through Flowlower boundary |
| `target_projection_probe` | named targets and entrypoints | accepted through Flowlower boundary |
| `abi_contract_probe` | ABI type semantics | blocked by incomplete ABI type resolution |
| `type_reference_probe` | generic, qualified, and shaped types | blocked by incomplete type-family resolution |

The empty `main` trial remains the only case that currently emits LLVM IR and
produces an executable. The accepted matrix cases prove semantic and boundary
continuation; they do not claim that general lowering is implemented.

## Exploratory AST-family scan

The broader v0.25 AST fixture family was also scanned without promoting every
result to a gate. The scan exposed these current gaps:

```text
generic/container/shape types:
  list<int>, array<int>, optional<...>, result<...>

ABI contract types:
  c_int, c_long, c_size_t

qualified/domain types:
  Point, vendor.Domain, collection.list<int>

cross-unit and recursive call visibility:
  some imported or recursively described call names are not yet projected
  into the resolver's visible scope chain
```

These are semantic implementation gaps, not failures of the structural
FlowMini export. They are the next candidates for expansion of the accepted
matrix. The matrix should gain a case only when the corresponding semantic
contract is implemented and tested.

Run it with:

```sh
tools/run-flowcore-pipeline-matrix.sh
```
