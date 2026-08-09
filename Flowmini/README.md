# Flowmini

Flowmini is the executable prototype language used to test and harden Flowcore ideas.

It is not the final Flowcore language. It is the laboratory where syntax, AST structure, semantic rules, lowering ideas, and tooling are made visible before they become larger Flowcore architecture.

## Current active stage

The current active implementation is:

```text
Flowmini/flowmini_v24_explicit_ast

Current stage theme:

Flowmini v0.24 explicit AST

The v0.24 line focuses on making the AST explicit, observable, regression-tested, and safe to deepen.

Current status

Flowmini is experimental and unfinished.

Current status:

lexer/token groundwork             usable
TokenTree/source structure          observable
explicit AST                        active and structurally deepening
shallow expression AST              first pass complete
semantic analysis                   mostly future work
type checking                       mostly future work
Graph IR lowering                   mostly future work
runtime semantics                   mostly future work

The current v0.24 AST can represent a shallow navigable expression graph for:

function calls
unary expressions
binary expressions
index expressions
field access
list literals
record literals

This is still not a complete expression parser. Recursive expression population, precedence, associativity, and semantic analysis are future work.

Build quickstart

From the active v0.24 directory:

cd Flowmini/flowmini_v24_explicit_ast

cmake -S . -B cmake-build-debug
cmake --build cmake-build-debug -j20

Adjust -j20 to match your machine.

Test quickstart

Run the AST golden regression tests:

cmake --build cmake-build-debug --target flowmini_ast_golden_tests

Run the full Flowmini suite:

cmake --build cmake-build-debug --target flowmini_suite

Expected current result:

AST golden tests: PASS (8)

total: 76
pass:  76
bad:   0
Useful commands

Dump an AST:

./cmake-build-debug/flowmini --dump-ast examples/ast/operator_expression_probe.flow

Run the AST golden helper directly:

tools/run-flowmini-ast-golden-tests.sh

Update AST goldens intentionally after an expected AST output change:

FLOWMINI_UPDATE_AST_GOLDENS=1 tools/run-flowmini-ast-golden-tests.sh

Always inspect the diff before committing updated goldens.

Important paths
flowmini_v24_explicit_ast/include/
    public headers

flowmini_v24_explicit_ast/src/
    implementation

flowmini_v24_explicit_ast/examples/ast/
    AST-focused source examples

flowmini_v24_explicit_ast/tests/expected/ast/
    golden AST JSON outputs

flowmini_v24_explicit_ast/tools/
    maintained helper scripts

flowmini_v24_explicit_ast/docs/
    status notes and implementation documentation
Archived earlier stages

Earlier Flowmini stages are preserved in the repository archive.

They are useful historical material, but they are not the active implementation line.

Current active development should happen in:

Flowmini/flowmini_v24_explicit_ast
Design direction

Flowmini follows the broader Flowcore principle:

model the system before mutating it

Important current distinction:

TokenTree remembers what the source looked like.
AST states what the source means.

The current v0.24 work is about making that AST layer real.

Documentation map

Recommended reading order:

Flowmini/README.md
Flowmini/flowmini_v24_explicit_ast/docs/v0.24-explicit-ast-status.md
Flowmini/flowmini_v24_explicit_ast/docs/v0.24-shallow-expression-ast-sitrep.md
Flowmini/flowmini_v24_explicit_ast/examples/ast/README.md
docs/language/named-targets.md
Warning

This project is experimental.

Do not use Flowmini for production, safety-critical, security-critical, financial, legal, medical, or operational workloads.

The implementation is a research/prototype line intended to make language and system architecture decisions visible and testable.
