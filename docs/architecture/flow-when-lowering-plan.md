# Flow `when` lowering plan

Flowmini currently executes `when` directly in its record runtime. The
semantic and backend chain needs a first-class control operation before enum
and tagged-variant matching can cross into LLVM or TinyVM.

The next lowering-plan revision adds one generic operation:

```json
{
  "kind": "match",
  "selector_expression": 17,
  "selector_type": "DecodeOutcome",
  "function_symbol_id": 12,
  "block_id": 4,
  "arms": [
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

The migration is staged: Flowanalyst emits `match` alongside existing `branch`
operations; Flowparallel and Flowoptimize preserve its arm order and identity;
Flowlower adds structured emission; TinyVM receives the same operation through
its target-neutral lowering. Existing `branch` remains canonical for Boolean
`if` and `guard` until all consumers accept `match`.

