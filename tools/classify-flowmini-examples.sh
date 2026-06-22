#!/usr/bin/env bash
set -euo pipefail

# Classify Flowmini examples into test-oriented categories.
#
# Intended to be run from a Flowmini version root, for example:
#
#   cd Flowmini/flowmini_v21_structural_bridge
#   ../../tools/classify-flowmini-examples.sh --apply
#
# Default mode is dry-run. It prints the planned moves without changing files.
#
# Resulting layout:
#
#   examples/
#   ├── pass/       runnable root programs expected to succeed
#   ├── fail/       negative tests expected to fail
#   ├── support/    .flow support/library/non-root files
#   └── docs/       documentation files inside examples
#
# Classification rules:
#
#   bad_*.flow      -> fail/
#   *.flow with
#      root main {  -> pass/
#   *.flow without
#      root main {  -> support/
#   *.md            -> docs/
#   directories     -> support/
#
# It tries to avoid matching "main {" inside comments, but it is still a
# housekeeping classifier, not a compiler.

APPLY=0
ROOT="."
EXAMPLES_DIR="examples"

usage() {
    cat <<USAGE
Usage: $0 [options]

Options:
  --root DIR       Flowmini project root. Default: current directory.
  --examples DIR   Examples directory relative to root. Default: examples
  --apply          Actually move files. Default is dry-run.
  --help           Show this help.

Examples:
  tools/classify-flowmini-examples.sh --root Flowmini/flowmini_v21_structural_bridge
  tools/classify-flowmini-examples.sh --root Flowmini/flowmini_v21_structural_bridge --apply
USAGE
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --root)
            ROOT="$2"
            shift 2
            ;;
        --examples)
            EXAMPLES_DIR="$2"
            shift 2
            ;;
        --apply)
            APPLY=1
            shift
            ;;
        --help|-h)
            usage
            exit 0
            ;;
        *)
            echo "unknown argument: $1" >&2
            usage >&2
            exit 2
            ;;
    esac
done

ROOT="$(cd "$ROOT" && pwd)"
EXAMPLES_PATH="$ROOT/$EXAMPLES_DIR"

if [[ ! -d "$EXAMPLES_PATH" ]]; then
    echo "examples directory not found: $EXAMPLES_PATH" >&2
    exit 2
fi

if [[ "$APPLY" -eq 0 ]]; then
    echo "DRY RUN: no files will be moved. Add --apply to perform changes."
else
    mkdir -p "$EXAMPLES_PATH/pass" "$EXAMPLES_PATH/fail" "$EXAMPLES_PATH/support" "$EXAMPLES_PATH/docs"
fi

strip_comments_for_main_probe() {
    # Minimal classifier helper:
    # - removes // line comments
    # - removes /* ... */ blocks if they start/end on same line
    # This is not a full lexer; it is just good enough to avoid obvious false positives.
    sed -E 's://.*$::; s:/\*[^*]*\*/::g' "$1"
}

has_root_main_block() {
    strip_comments_for_main_probe "$1" | grep -Eq '^[[:space:]]*main[[:space:]]*\{'
}

target_for_path() {
    local path="$1"
    local base
    base="$(basename "$path")"

    if [[ -d "$path" ]]; then
        echo "support"
        return
    fi

    case "$base" in
        *.md)
            echo "docs"
            return
            ;;
        bad_*.flow)
            echo "fail"
            return
            ;;
        *.flow)
            if has_root_main_block "$path"; then
                echo "pass"
            else
                echo "support"
            fi
            return
            ;;
        *)
            echo "support"
            return
            ;;
    esac
}

move_one() {
    local path="$1"
    local bucket="$2"
    local base dest

    base="$(basename "$path")"
    dest="$EXAMPLES_PATH/$bucket/$base"

    if [[ "$path" == "$dest" ]]; then
        return
    fi

    if [[ -e "$dest" ]]; then
        echo "SKIP collision: $path -> $dest" >&2
        return
    fi

    printf "%-8s %s -> examples/%s/%s\n" "$bucket" "${path#"$ROOT"/}" "$bucket" "$base"

    if [[ "$APPLY" -eq 1 ]]; then
        mv "$path" "$dest"
    fi
}

echo "== classifying examples in $EXAMPLES_PATH =="

# Only classify current top-level contents to avoid re-moving already categorized files.
shopt -s nullglob dotglob
for path in "$EXAMPLES_PATH"/*; do
    base="$(basename "$path")"

    case "$base" in
        pass|fail|support|docs)
            continue
            ;;
    esac

    bucket="$(target_for_path "$path")"
    move_one "$path" "$bucket"
done

if [[ "$APPLY" -eq 1 ]]; then
    cat > "$EXAMPLES_PATH/README.md" <<'README'
# Flowmini examples

The examples directory is organized by test intent.

```text
examples/
├── pass/       runnable root programs expected to succeed
├── fail/       negative tests expected to fail with diagnostics
├── support/    support/library/non-root flow files
└── docs/       documentation related to examples
```

## Rules

- `pass/*.flow` should execute successfully.
- `fail/*.flow` should fail with a meaningful diagnostic.
- `support/*` is not run directly as a root program unless a test explicitly asks for it.
- `docs/*` is documentation and is ignored by the regression runner.

This split makes it possible to build a real test harness instead of treating every
file in `examples/` as a root executable.
README
fi

echo
if [[ "$APPLY" -eq 0 ]]; then
    echo "Dry-run complete. Re-run with --apply to move files."
else
    echo "Classification complete."
fi
