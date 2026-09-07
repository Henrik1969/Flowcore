# Flow `when` lowering plan

Flowmini currently executes `when` directly in its record runtime. The
semantic and backend chain needs a first-class control operation before enum
and tagged-variant matching can cross into LLVM or TinyVM.

The lowering plan now carries one generic operation for integer matching:

```json
{
  "kind": "match",
  "selector_expression": 17,
  "selector_type": "DecodeOutcome",
  "function_symbol_id": 12,
  "block_id": 4,
  "cases": [
    {
      "label": {"kind": "variant_member", "type": "DecodeOutcome", "member": "scalar", "tag": 0},
      "body_block_id": 5,
      "payload_bindings": [{"name": "value", "type": "int", "field": "value"}]
    },
    {
      "label": {"kind": "variant_member", "type": "DecodeOutcome", "member": "diagnostic", "tag": 1},
      "body_block_id": 6,
      "payload_bindings": [
        {"name": "code", "type": "int", "field": "code"},
        {"name": "offset", "type": "int", "field": "offset"}
      ]
    }
  ],
  "default_block_id": 7,
  "join_block_id": 8
}
```

Integer and enum labels use the same arm shape with `literal` or `low` and
`high` fields. Labels remain ordered source evidence; consumers validate
duplicate and overlapping coverage before lowering. A closed enum or variant
may omit `default_block_id` only when the arm labels cover every declared
member. Open integer and externally supplied selectors require a default.

`payload_bindings` are semantic facts, not backend name lookup. A backend
receives the selected variant tag and field paths explicitly, so it cannot
accidentally read a payload belonging to another member. The operation joins
all non-terminating arms at `join_block_id`.

The migration is staged: Flowanalyst emits integer `match` operations alongside
existing `branch` operations; Flowparallel and Flowoptimize preserve their case
order and identity; Flowlower emits inclusive integer comparisons and explicit
case/default/join edges in both LLVM and TinyVM. Named enum matches are admitted
to the semantic contract. Tagged-variant labels are preserved in the artifacts,
but Flowanalyst rejects variant lowering until their tag and payload
representation is target-neutral. Boolean `if` uses `branch`; `guard` retains
its dedicated frontend identity and failure-block provenance.

Variant routing follows a separate boundary. The target-neutral representation
is a discriminant integer plus an arm-local payload record:

```text
variant value = { tag: 1, payload: { code: int, offset: int } }
```

Tag-only matches may consume `tag` once this layout is carried by the backend
artifact. Payload bindings require the selected arm's field map and remain
unsupported until that map is explicit; a backend must reject them rather than
load a field from another arm by name or slot coincidence.
