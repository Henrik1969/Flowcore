#!/usr/bin/env bash
set -euo pipefail

ROOT="."
APPLY=0
OVERWRITE=0

usage() {
    cat <<USAGE
Usage: $0 [options]

Options:
  --root DIR     Repository/project root. Default: current directory.
  --apply        Actually create files. Default is dry-run.
  --overwrite    Replace existing files. Default preserves existing files.
  --help         Show this help.
USAGE
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --root) ROOT="$2"; shift 2 ;;
        --apply) APPLY=1; shift ;;
        --overwrite) OVERWRITE=1; shift ;;
        --help|-h) usage; exit 0 ;;
        *) echo "unknown argument: $1" >&2; usage >&2; exit 2 ;;
    esac
done

ROOT="$(cd "$ROOT" && pwd)"

write_file() {
    local rel="$1"
    local path="$ROOT/$rel"
    local dir
    dir="$(dirname "$path")"

    if [[ "$APPLY" -eq 0 ]]; then
        if [[ -e "$path" && "$OVERWRITE" -eq 0 ]]; then
            echo "would keep:  $rel"
        else
            echo "would write: $rel"
        fi
        cat >/dev/null
        return
    fi

    mkdir -p "$dir"

    if [[ -e "$path" && "$OVERWRITE" -eq 0 ]]; then
        echo "keep:        $rel"
        cat >/dev/null
        return
    fi

    cat > "$path"
    echo "wrote:       $rel"
}

echo "bootstrap documentation structure"
echo "root: $ROOT"
[[ "$APPLY" -eq 1 ]] && echo "mode: APPLY" || echo "mode: DRY RUN"
echo

write_file "CHANGELOG.md" <<'EOF'
# Changelog

This repository is a language-design and implementation lab. The numbered folders are intentionally preserved as standalone snapshots.

## Current checkpoint

- Current Flowmini active version: `Flowmini/flowmini_v22_unit_kinds`
- Suite baseline: `75 / 75`
- Source unit law introduced: `program` vs `unit`
- Examples categorized into `pass/`, `fail/`, `support/`, and `docs/`
- Environment-aware suite runner added.

## Flowmini v22_unit_kinds

- Added explicit source unit roles:
  - `program` = executable/root source unit
  - `unit` = defining/importable source unit
- Enforced import law:
  - units may be imported
  - programs may not be imported as ordinary units
  - imported files must not define `main`
  - root execution requires a program with `main`
- Categorized examples.
- Added expected stdout and diagnostic substring suite support.
- Suite passes `75 / 75`.

## Flowmini v21_structural_bridge

- Integrated TokenTree as a structural token-tree inspection path.
- Integrated SymbolTable as a structural symbol projection path.
- Preserved existing execution behavior.

## Earlier Flowmini snapshots

Earlier snapshots remain in place under `Flowmini/` and are indexed in `Flowmini/VERSION_INDEX.md`.
EOF

write_file "docs/index.md" <<'EOF'
# Documentation Index

This repository is organized as a lab notebook with project islands.

## Main areas

- [Flowmini](../Flowmini/README.md)
- [Subprojects](../subprojects/README.md)
- [Pattern explored](../Pattern_explored/README.md)
- [Architecture notes](architecture/README.md)
- [Flowmini docs](flowmini/README.md)
- [Session notes](sessions/)

## Current active Flowmini version

See:

- [Flowmini current version](../Flowmini/CURRENT.md)
- [Flowmini version index](../Flowmini/VERSION_INDEX.md)
- [Flowmini changelog](../Flowmini/CHANGELOG.md)

Current checkpoint:

```text
Flowmini v22_unit_kinds
suite: 75 / 75
```
EOF

write_file "docs/architecture/README.md" <<'EOF'
# Architecture Notes

Project-wide architecture notes belong here.

Current direction:

```text
source files
  -> import/source loading
  -> lexer
  -> TokenTree
  -> parser
  -> AST
  -> SymbolTable population
  -> semantic checker
  -> lowerer
  -> FlowIR
  -> runtime/backend
```

Current reality as of Flowmini v22:

```text
source files
  -> import/source loading
  -> lexer
  -> TokenTree inspection path
  -> current parser/lowerer
  -> FlowIR/runtime representation
  -> runtime interpreter
```

Planned architecture topics:

- pipeline boundaries
- TokenTree
- SymbolTable
- AST
- semantic checker split
- runtime backends
- bytecode
- effects and failure-flow
EOF

write_file "docs/flowmini/README.md" <<'EOF'
# Flowmini Documentation

Flowmini is the executable prototype/lab language used to explore Flowcore ideas.

Current active version:

```text
Flowmini/flowmini_v22_unit_kinds
```

Current baseline:

```text
build: OK
suite: 75 / 75
```

Documents in this directory:

- [Roadmap](roadmap.md)
- [Testing](testing.md)

Planned:

- programmer's manual
- language reference
- ABI notes
- examples guide
EOF

write_file "docs/flowmini/roadmap.md" <<'EOF'
# Flowmini Roadmap

## Current checkpoint

```text
v22_unit_kinds
build: OK
suite: 75 / 75
```

## Immediate

1. Commit expected suite outputs and diagnostics.
2. Clean compiler warnings.
3. Add programmer's manual draft.
4. Establish Valgrind baseline.
5. Establish GDB-on-failure baseline.

## Next structural versions

### v23_token_tree_parser_bridge

Move parser input toward TokenTree without changing language behavior.

### v24_explicit_ast

Introduce explicit AST as parser output.

### v25_symboltable_from_ast

Populate SymbolTable from AST declarations before lowering.

### v26_semantic_checker_split

Separate syntax parsing, symbol collection, semantic checking, and lowering.

## Later runtime work

- FlowIR bytecode format
- portable switch-dispatch VM
- GNU computed-goto threaded VM backend
- C++ emitter backend
- precompiled bytecode artifacts
- precompiled unit/program artifacts
EOF

write_file "docs/flowmini/testing.md" <<'EOF'
# Flowmini Testing

Flowmini uses a categorized integration test suite.

## Categories

```text
examples/pass/*.flow
    runnable programs expected to succeed

examples/fail/*.flow
    examples expected to fail with diagnostics

examples/support/*.flow
    importable units / support files

examples/docs/*
    documentation
```

## Runner

```bash
$TOP/tools/run-flowmini-test-suite.sh
```

Useful environment:

```bash
export TOP="/home/henrik/Projekter/scratchpad/flow_Policy_envelope_pattern"
export FLOWLAB_TOP="$TOP"
export FLOWMINI_ROOT="$TOP/Flowmini/flowmini_v22_unit_kinds"
export FLOWMINI_STDIN=5
```

Expected current baseline:

```text
total: 75
pass:  75
bad:   0
```

## Expected files

```text
tests/expected/stdout/<name>.out
tests/expected/stderr/<name>.err
tests/expected/diagnostics/<name>.contains
```

Diagnostic `.contains` files are substring checks. They should contain stable diagnostic phrases, not full path-sensitive stderr.

## Optional modes

```bash
FLOWMINI_VALGRIND=1 $TOP/tools/run-flowmini-test-suite.sh
FLOWMINI_GDB_FAILURES=1 $TOP/tools/run-flowmini-test-suite.sh
FLOWMINI_GDB_ALL=1 $TOP/tools/run-flowmini-test-suite.sh
```
EOF

write_file "docs/sessions/2026-06-flowmini-v22-unit-kinds.md" <<'EOF'
# Session Note: Flowmini v22 Unit Kinds

## Summary

Flowmini v22 introduced explicit source roles:

```text
program = executable/root source unit
unit    = defining/importable source unit
```

## Why

The old flat `examples/` layout mixed root programs, negative tests, support files, and documentation. Categorizing examples exposed hidden import/path assumptions.

## Decisions

- `program` files may contain `main` and may be root-executed.
- `unit` files may be imported and must not contain `main`.
- Ordinary imports accept units and reject programs.
- Program-as-callable-entrypoint remains speculative future work.

## Testing

The examples were categorized:

```text
examples/pass/
examples/fail/
examples/support/
examples/docs/
```

The suite runner is environment-aware and checks expected stdout and diagnostic substrings.

Baseline:

```text
suite: 75 / 75
```

## Next

- clean compiler warnings
- add programmer's manual draft
- v23 TokenTree parser bridge
EOF

write_file "Flowmini/README.md" <<'EOF'
# Flowmini

Flowmini is the executable prototype/lab language used to test Flowcore ideas.

The numbered directories are standalone snapshots. They are intentionally kept in place because each version represents a historical implementation step.

Current active version:

```text
flowmini_v22_unit_kinds
```

See:

- [CURRENT.md](CURRENT.md)
- [VERSION_INDEX.md](VERSION_INDEX.md)
- [CHANGELOG.md](CHANGELOG.md)
EOF

write_file "Flowmini/CURRENT.md" <<'EOF'
# Current Flowmini Version

Current active version:

```text
flowmini_v22_unit_kinds
```

Status:

```text
build: OK
suite: 75 / 75
bad:   0
```

Run the current suite:

```bash
export FLOWMINI_ROOT="$TOP/Flowmini/flowmini_v22_unit_kinds"
$TOP/tools/run-flowmini-test-suite.sh
```

Current important language law:

```text
program = executable/root source unit
unit    = defining/importable source unit
```
EOF

write_file "Flowmini/VERSION_INDEX.md" <<'EOF'
# Flowmini Version Index

The numbered directories are standalone historical snapshots.

| Version | Directory | Main purpose |
|---:|---|---|
| v1 | `flowmini_v1_pattern_introduction` | Initial pattern introduction |
| v2 | `flowmini_v2_general` | Generalization step |
| v3 | `flowmini_v3_layers` | Layering step |
| v4 | `flowmini_v4_lists` | Lists |
| v5 | `flowmini_v5_frontend` | Frontend work |
| v6 | `flowmini_v6_scopes` | Scopes |
| v7 | `flowmini_v7_if_else` | If/else |
| v8 | `flowmini_v8_list_indexing` | List indexing |
| v9 | `flowmini_v9_break_continue` | Break/continue |
| v10 | `flowmini_v10_compound_expressions` | Compound expressions |
| v12 | `flowmini_v12_fn_value_ports` | Functions with value ports |
| v13 | `flowmini_v13_imports` | Imports |
| v15 | `flowmini_v15_abi_bindings` | ABI bindings |
| v16 | `flowmini_v16_abi_pointer_contracts` | ABI pointer contracts |
| v19 | `flowmini_v19_comments` | Comments |
| v20 | `flowmini_v20_bool` | Bool / predicate results |
| v21 | `flowmini_v21_structural_bridge` | TokenTree + SymbolTable structural bridge |
| v22 | `flowmini_v22_unit_kinds` | `program`/`unit`, categorized examples, expected suite |

Missing version numbers are historical gaps or discarded intermediate experiments.
EOF

write_file "Flowmini/CHANGELOG.md" <<'EOF'
# Flowmini Changelog

## v22_unit_kinds

- Added explicit source roles:
  - `program`
  - `unit`
- Enforced import law.
- Categorized examples into `pass`, `fail`, `support`, and `docs`.
- Added expected stdout and diagnostic checks.
- Suite baseline: `75 / 75`.

## v21_structural_bridge

- Integrated TokenTree as a structural token-tree inspection path.
- Integrated SymbolTable as a structural symbol projection path.
- Preserved the existing execution path.

## v20_bool

- Added internal `Bool`.
- Added `true` and `false`.
- Comparisons produce `Bool`.
- `if` and `while` require `Bool`.
- No implicit integer truthiness.

## Earlier versions

See `VERSION_INDEX.md`.
EOF

write_file "Pattern_explored/README.md" <<'EOF'
# Pattern Explored

This area contains earlier pattern experiments and prototypes that informed Flowmini and Flowcore design.

These are not the current active Flowmini implementation, but they remain useful as implementation archaeology and design reference.
EOF

write_file "Pattern_explored/CHANGELOG.md" <<'EOF'
# Pattern Explored Changelog

This changelog is intentionally light for now.

Known purpose:

- preserve early Flow Policy Envelope and related implementation experiments
- keep the original pattern work separate from the Flowmini version line
EOF

write_file "subprojects/README.md" <<'EOF'
# Subprojects

This directory contains reusable project islands used by Flowmini/Flowcore.

Current subprojects:

- [TokenTree](TokenTree/README.md)
- [SymbolTable](SymbolTable/README.md)

Each subproject should keep its own README and changelog because it can evolve independently of Flowmini.
EOF

write_file "subprojects/TokenTree/README.md" <<'EOF'
# TokenTree

TokenTree is a reusable structural token tree library.

It is not the parser, semantic checker, or final AST.

Purpose:

```text
external tokens
  -> structural grouping
  -> traversal/mutation/inspection
  -> parser input boundary
```

Flowmini currently uses TokenTree as a structural bridge/inspection layer. A later Flowmini version should make the parser consume TokenTree more directly.
EOF

write_file "subprojects/TokenTree/CHANGELOG.md" <<'EOF'
# TokenTree Changelog

## tokentree_v1

- Renamed/purified from earlier AstLiib-style project.
- Provides a reusable structural token tree layer.
- Intended to stay policy-free and parser-agnostic.
EOF

write_file "subprojects/SymbolTable/README.md" <<'EOF'
# SymbolTable

SymbolTable is a reusable policy-free symbol fact store.

Core idea:

```text
SymbolTable      stores facts
ISymbolPolicy    interprets facts for a specific view
SymbolTreeView   exposes a filtered projection
Semantic layer   decides language-law
```

The raw table is not an enforcer. It stores facts. Policy/view/semantic layers decide what those facts mean in a language context.
EOF

write_file "subprojects/SymbolTable/CHANGELOG.md" <<'EOF'
# SymbolTable Changelog

## SymbolTable_v1_1

- Policy-free symbol table.
- Scope tree.
- Symbol insertion and lookup.
- Factoid attachment.
- SymbolTreeView projection.
- Example policy layer such as private-symbol hiding.

## SymbolTable_v1

- Initial version.
EOF

echo
if [[ "$APPLY" -eq 0 ]]; then
    echo "Dry-run complete. Re-run with --apply to write files."
else
    echo "Documentation structure bootstrap complete."
fi
