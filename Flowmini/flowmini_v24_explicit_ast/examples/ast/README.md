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
Canonical AST probes

The canonical probes are the files ending in:

*_probe.flow

These are expected to dump valid AST JSON with:

./cmake-build-debug/flowmini --dump-ast examples/ast/<name>_probe.flow

They are also covered by the golden AST test runner:

tools/run-flowmini-ast-golden-tests.sh

The matching expected outputs live in:

tests/expected/ast/*.ast.json

Current canonical probe set:

nested_body_probe.flow
expression_presence_probe.flow
expression_pool_probe.flow
expression_kind_probe.flow
call_expression_probe.flow
operator_expression_probe.flow
index_field_probe.flow
literal_expression_probe.flow
recursive_expression_probe.flow
precedence_expression_probe.flow

These files define the v0.24 AST golden regression gate.

The current `flowmini.ast.v2` dump uses arena-owned `statement_pool` and
`block_pool` collections. Conditional statements reference a mandatory
`then_block` and an optional tagged `else_arm`: either `else_block` with a
`BlockId`, or `else_if` with an `IfStmtId`. Else-if continuations are not
encoded as arbitrary nested statements inside an ordinary else block.

Exploratory / edgecase examples

Some examples are intentionally outside the current v0.24 accepted language surface.

These files may fail today.

That does not necessarily mean the compiler is broken.

It may mean the file captures a future language idea, an edgecase, or a design question.

Known exploratory file: ast_spacing_edgecases.flow

ast_spacing_edgecases.flow is not part of the current golden AST regression gate.

At the time of writing, it contains multiple root main blocks.

Current v0.24 rule:

a root program supports exactly one root main block

Therefore this file may fail with an error similar to:

multiple main definitions found in root file

This is expected for v0.24.

The file is still useful because it helped expose a future language-design direction:

one program may later define multiple named targets

The planned future model is documented in:

docs/language/named-targets.md

Future direction:

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

But this is not implemented in v0.24.

Rule for test tooling

Golden AST tests should not blindly run every examples/ast/*.flow file.

The checked-in golden files define the canonical regression set:

tests/expected/ast/*.ast.json

The runner should compare only those expected files against their matching source examples.

This allows the examples directory to contain both stable regression specimens and useful future-language specimens.
