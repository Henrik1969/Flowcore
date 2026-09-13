# Flowmini AST Examples

Status: v0.24 explicit-AST development examples

This directory contains Flowmini source files used while stabilizing the explicit AST.

Not every file in this directory is expected to pass today.

There are two classes of examples here:

```text
canonical AST probes
    stable examples used by golden AST regression tests

exploratory / edgecase examples
    useful language-design specimens, but not necessarily valid v0.24 programs
```

## Canonical AST probes

The canonical probes are the files ending in:

```text
*_probe.flow
```

These are expected to dump valid AST JSON with:

```bash
./cmake-build-debug/flowmini --dump-ast examples/ast/<name>_probe.flow
```

They are also covered by the golden AST test runner:

```bash
tools/run-flowmini-ast-golden-tests.sh
```

The matching expected outputs live in:

```text
tests/expected/ast/*.ast.json
```

Current canonical probe set:

```text
abi_contract_probe.flow
call_expression_probe.flow
control_flow_unary_probe.flow
else_if_chain_probe.flow
else_if_final_else_probe.flow
expression_kind_probe.flow
expression_pool_probe.flow
expression_presence_probe.flow
index_field_probe.flow
literal_expression_probe.flow
nested_body_probe.flow
nested_conditionals_probe.flow
operator_expression_probe.flow
ordinary_else_probe.flow
placement_statement_probe.flow
precedence_expression_probe.flow
recursive_expression_probe.flow
refined_contract_probe.flow
statement_assignment_probe.flow
statement_condition_probe.flow
statement_else_probe.flow
statement_initializer_probe.flow
statement_return_probe.flow
structured_value_probe.flow
type_reference_probe.flow
```

These 26 files define the current v0.24 AST golden regression gate.

`function_signature_gallery.flow`, `unit_symbol_projection_gallery.flow`, and
the refined/ABI contract probes also serve the separate SymbolTable projection
golden gate. The unit galleries are
inspectable through structural dump modes but remain non-executable as root
source, preserving the program/unit boundary.

The current `flowmini.ast.v2` dump uses arena-owned `statement_pool` and
`block_pool` collections. Conditional statements reference a mandatory
`then_block` and an optional tagged `else_arm`: either `else_block` with a
`BlockId`, or `else_if` with an `IfStmtId`. Else-if continuations are not
encoded as arbitrary nested statements inside an ordinary else block.

## Exploratory / edgecase examples

Some examples are intentionally outside the current v0.24 accepted language surface.

These files may fail today.

That does not necessarily mean the compiler is broken.

It may mean the file captures a future language idea, an edgecase, or a design question.

Known exploratory file: `ast_spacing_edgecases.flow`.

`ast_spacing_edgecases.flow` is not part of the current golden AST regression
gate.

At the time of writing, it contains multiple root main blocks.

Current v0.24 rule:

```text
a root program supports exactly one root main block
```

Therefore this file may fail with an error similar to:

```text
multiple main definitions found in root file
```

This is expected for v0.24.

The file is still useful because it helped expose a future language-design direction:

```text
one program may later define multiple named targets
```

The planned future model is documented in:

[`docs/language/named-targets.md`](../../../../docs/language/named-targets.md)

Future direction:

```flow
program toolset

target cli {
    main {
        ...
    }
}

target daemon {
    main {
        ...
    }
}
```

But this is not implemented in v0.24.

## Rule for test tooling

Golden AST tests should not blindly run every `examples/ast/*.flow` file.

The checked-in golden files define the canonical regression set:

```text
tests/expected/ast/*.ast.json
```

The runner should compare only those expected files against their matching source examples.

This allows the examples directory to contain both stable regression specimens and useful future-language specimens.
