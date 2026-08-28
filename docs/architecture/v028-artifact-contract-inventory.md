# v0.28 artifact-contract inventory

**Checkpoint:** `39804e1` plus this inventory  
**Scope:** required compiler chain and directly consumed provider/runtime evidence

## Contract map

| Artifact | Version | Producer | Required consumers | Authority carried | Current consumption at baseline |
| --- | ---: | --- | --- | --- | --- |
| `flowmini.frontend_bundle` | 2 | Flowmini | Flowanalyst; independent Python consumer | source, AST pools, symbols/scopes, origins, maps, structural identities | Flowanalyst parses JSON structurally but silently keeps the first duplicate key and often substitutes fallback values; Python consumer validates more cross-field invariants |
| `flowanalyst.semantic_report` | 1 | Flowanalyst | Flowbind, Flowparallel, Flowoptimize | status, source, targets, requirements, ABI types, effects, operations, lowering plan, regions, graph/matrix projection, provenance | Flowbind parses most payload structurally but checks top-level format/version/status by text search; Flowparallel and Flowoptimize search and slice text |
| `flowcore.lowering_plan` | 1 | embedded by Flowanalyst | Flowbind, Flowparallel, Flowoptimize, Flowlower | ordered operation/block identities, operands/results, providers, ABI/effects/resources, cleanup, source provenance | Flowbind and Flowlower have typed local models; middle stages copy an extracted object substring without validating it |
| `flowbind.binding_report` | 1 | Flowbind | Flowlower; pipeline tests | exact authorized capability tuple, provider, policy and ABI evidence, plan operation count | Flowlower parses structurally and checks exact capability tuples, but schema ownership is private to the lowerer and incomplete fields can become empty strings |
| `flowparallel.execution_plan` | 1 | Flowparallel | Flowoptimize; Flowparallel provider/planner experiments | preserved semantic authority, dependency summary, derived matrix, fallback/provider policy, runtime capability contract | Flowoptimize searches/slices text; several adjacent Flowparallel tools independently search text |
| `flowoptimize.optimization_report` | 1 | Flowoptimize | Flowlower | preserved authority, provider decision, derived projection, attributable transforms | Flowlower parses structurally with private definitions; unknown and missing-field policies are implicit |
| `flowlower.lowering_report` | 1 | Flowlower | independent users/tests | selected target, backend and emitted-artifact disposition | tests inspect text/JQ; no public C++ contract or independent validator |
| `flowcore.abi_manifest` | implicit current version | binding generator/provider | Flowbind | provider-owned aggregate sizes, alignments, ordered fields and offsets | structured Flowbind parser; exact version/unknown-field policy is incomplete |
| generated capability policy | line format | binding generator/user | Flowbind | exact library, symbol, convention, effect, carrier signature and evidence identity | dedicated line parser; legacy four-field wildcard remains explicit compatibility behavior |
| `frankencore.runtime_capabilities` / `flowcore.runtime_capabilities` | current experimental variants | runtime probes | Flowparallel runtime planner/providers | hardware/provider availability and runtime constraints | adjacent planners use text matching; not yet a required native-chain input but authority-bearing when provider selection runs |
| `flowparallel.graph_provider_decision` | 1 | Flowparallel graph planner | Flowoptimize | selected provider, representation, reason and CPU fallback | Flowoptimize searches text; nested fake fields and duplicates can affect acceptance |
| calibration artifacts (`flowparallel.matrix_benchmark`, `flowparallel.graph_calibration`, `flowparallel.graph_cuda`) | 1/current | benchmark/provider tools | Flowparallel planners | measured speedup, density, provider availability and calibration provenance | adjacent planners search text and use permissive numeric extraction |

## Required and optional authority by primary boundary

### Frontend bundle to Flowanalyst

Required authority: exact format/version, source path, AST pools and IDs,
SymbolTable scopes/symbols, origins, and references between pools. Optional
projection fields must be explicitly versioned. Duplicate object keys or entity
IDs are invalid. Baseline weakness: the C++ parser uses unchecked `map::emplace`,
so a duplicate JSON key is silently ignored; numeric fields are represented as
`double` and cast to `int` with fallback values.

### Semantic report to Flowbind/Flowparallel/Flowoptimize

Required authority: format/version/status, source, targets, binding requirements,
ABI type contracts, effect/resource facts, external operations, lowering-plan
format/version/status/operations, analysis graph and matrix dimensions/entries,
and source/operation identities. Optional analysis summaries do not authorize
execution. Baseline weakness: top-level acceptance can be triggered by nested or
escaped field-like text in all three consumers; Flowparallel and Flowoptimize
substitute `{}` or `[]` for missing/malformed authority.

### Semantic report and policy to Flowbind

Required authority: every provider tuple and carrier/effect/resource fact needed
by every external operation; exact policy grant; optional aggregate manifest
only when aggregate evidence is requested. Baseline strength: duplicate JSON
keys are rejected inside the local parser and lowering operations are validated.
Baseline weakness: format/version/status are checked on raw text before the
parsed root is used, and public schema rules are duplicated locally.

### Execution plan to Flowoptimize

Required authority: preserved source/target/operation/provider/ABI/effect/
resource/provenance identity, valid matrix dimensions and entries, fallback law,
and runtime/provider-policy boundary. Baseline weakness: every listed category
is located by text search or copied substring; malformed fields can collapse to
empty values while the report remains `ready`.

### Optimization and binding reports to Flowlower

Required authority: optimization format/version/status, target identity,
complete lowering plan, exact ready binding report and capability tuples,
operation reachability, cleanup disposition, carrier representations, and
source provenance. Baseline strength: strict complete-input parser, duplicate
key rejection, typed local operation/provider structures, and structured
reachability/cleanup checks. Baseline weakness: definitions are private,
integer representation/range rules are weak, and missing text fields can become
empty strings before later validation.

## Raw authority-decision inventory

### Required stage entry points

| Stage | Authority-search sites | Classification and required replacement |
| --- | --- | --- |
| Flowanalyst | local JSON parser plus fallback field helpers | Structural, but duplicate keys and typed/range/required-field policy are insufficient. Migrate frontend envelope and authoritative pools to public contracts. Ordinary type-string and AST-path substring processing is data interpretation, not JSON authority search. |
| Flowbind | `verify()` raw searches for semantic format/version/status | Authority bug. Parse once, validate the root envelope, and pass the typed root to all binding/lowering validators. Parameter-carrier splitting is ordinary contract-data parsing. |
| Flowparallel | `has_field`, `json_array_field`, `json_object_field`, `source_path`, marker counters, `number_after`, `matrix_entries`, `targets_projection`, direct version/matrix searches | All decide or preserve authority and must be removed. Missing/malformed fields must never default to empty authority. |
| Flowoptimize | `has_field`, `has_top_level_format`, array/object/source/target slicing, numeric/matrix scanning, provider-decision `string_after`, direct version/matrix searches | All decide authority or transformations and must be removed. Matrix validation must precede deduplication. |
| Flowlower | private strict parser and typed plan driver | Not raw authority search. Reconcile with the public component instead of rewriting emitter semantics. Carrier-list splitting is ordinary typed contract-data parsing. |

### Adjacent Flowparallel tools

`cuda_provider.cpp`, `graph_cuda.cpp`, `graph_planner.cpp`, and
`runtime_planner.cpp` also use format/status/numeric/matrix substring searches.
They are authority-bearing provider experiments and must migrate before the
complete v0.28 definition of done, after the primary execution-plan boundary is
typed. `matrix_benchmark.cpp`, `cpu_provider.cpp`, `cuda_execute.cpp`, and graph
reference code contain ordinary parsing or generated-report logic that must be
reclassified during their artifact slice rather than mechanically rewritten.

## Existing parsers and incompatible behavior

- Flowanalyst, Flowbind, and Flowlower each carry separate JSON value/parser
  implementations.
- Flowbind and Flowlower reject duplicate object keys; Flowanalyst does not.
- Escape support differs between parsers and none implements complete JSON
  Unicode escape handling.
- All three store every number as `double`, permitting precision loss and weak
  integer range/integrality checks.
- Required fields, unknown fields, and diagnostic paths are stage-local and
  inconsistent.
- Flowparallel and Flowoptimize do not parse JSON at all.
- Serialization is handwritten by every producer; object order happens to be
  stable in many outputs but has no public canonicalization contract.

## Existing tests and uncovered attacks

Existing positive coverage includes the complete 54-test pipeline, native
link/execution examples, generated bindings, resource cleanup, graph routing,
and the independent frontend-bundle Python consumer. Flowbind has malformed and
duplicate-key coverage; Flowlower has typed-plan and reachability refusals.

The following attacks are not covered consistently across the chain:

- nested fake format/version/status/provider fields;
- duplicate keys in Flowanalyst and every text-search consumer;
- legal reordering and arbitrary whitespace;
- escaped strings containing field-like text;
- truncated arrays/objects that become `{}` or `[]`;
- non-integral, overflowed, negative, or precision-lost integer identities;
- duplicate operation, target, block, provider, or provenance identities;
- out-of-range/conflicting matrix coordinates;
- semantically equal input with different textual representation;
- operation loss/reordering and mutation without derivation evidence;
- inconsistent transform counts and provenance.

## First vertical slice

Create a small `Flowcontracts` public library with strict JSON values,
complete-input parsing, duplicate rejection, integral integer representation,
JSON-path diagnostics, and deterministic serialization. The first complete ADTs
will be the common artifact envelope plus `flowanalyst.semantic_report` fields
required by Flowparallel, including the embedded lowering plan and analysis
matrix. Migrate Flowparallel first and prove:

```text
Flowanalyst producer -> shared validator -> typed Flowparallel consumer
```

This removes the largest raw-authority surface without changing application or
lowering semantics. Flowoptimize follows using the same semantic/execution-plan
types.
