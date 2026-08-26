# Flowcontracts

Flowcontracts is the public, compiler-stage-independent artifact contract
component introduced for Flowcore v0.28.

Current surfaces:

- strict complete-input JSON parsing;
- duplicate object-key rejection;
- signed 64-bit integer identity and range preservation;
- finite floating-point values;
- complete JSON string escapes, including Unicode surrogate pairs;
- JSON-path validation diagnostics;
- deterministic serialization with lexicographically ordered object keys;
- typed artifact headers;
- typed `flowanalyst.semantic_report` v1 consumption for Flowparallel;
- typed `flowparallel.execution_plan` and
  `flowparallel.graph_provider_decision` v1 consumption for Flowoptimize;
- lowering-plan identity and matrix dimension/coordinate validation.
- independently invocable `flowvalidate` support for every main-chain artifact,
  stable valid/invalid/blocked/unsupported exits, machine or human diagnostics,
  source attribution, and canonical JSON round-tripping.
- structural validation for current ABI manifests, both runtime-capability
  variants, and matrix/graph calibration evidence consumed by provider planners.

## Version-1 unknown-field policy

Version-1 artifact objects are additive: unknown fields are retained when the
containing `json::Value` is preserved and otherwise ignored by a consumer that
does not claim their semantics. Unknown fields never satisfy a required field,
select an artifact format, or authorize an operation. Authority-bearing nested
objects are validated through their public typed surface before use.

This policy permits additive evidence fields while failing closed on missing,
malformed, duplicate, unsupported, or conflicting authority.

The component intentionally does not depend on Flowmini AST internals,
Flowlower emitters, provider implementations, or application names.
