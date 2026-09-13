#!/usr/bin/env bash
set -euo pipefail

# Generate expected stdout and diagnostic substring files from current behavior.
# Run from a Flowmini version root after a successful build.

ROOT="${1:-.}"
ROOT="$(cd "$ROOT" && pwd)"
cd "$ROOT"

BIN="${FLOWMINI_BIN:-$ROOT/build/flowmini}"
INPUT="${FLOWMINI_STDIN:-5}"

[[ -x "$BIN" ]] || { echo "missing executable: $BIN" >&2; exit 2; }
[[ -d examples/pass ]] || { echo "missing examples/pass" >&2; exit 2; }
[[ -d examples/fail ]] || { echo "missing examples/fail" >&2; exit 2; }

mkdir -p tests/expected/stdout tests/expected/stderr tests/expected/diagnostics
export LD_LIBRARY_PATH="$ROOT/build:${LD_LIBRARY_PATH:-}"

for f in examples/pass/*.flow; do
    [[ -e "$f" ]] || continue
    name="$(basename "$f" .flow)"
    printf '%s\n' "$INPUT" | "$BIN" "$f" > "tests/expected/stdout/$name.out"
done

for f in examples/fail/*.flow; do
    [[ -e "$f" ]] || continue
    name="$(basename "$f" .flow)"
    tmp_err="$(mktemp)"
    tmp_out="$(mktemp)"
    printf '%s\n' "$INPUT" | "$BIN" "$f" >"$tmp_out" 2>"$tmp_err" || true

    # Keep the stable core of the diagnostic. Avoid absolute paths and line/column suffixes.
    first_line="$(head -n 1 "$tmp_err")"
    first_line="${first_line//$ROOT\//}"
    first_line="$(printf '%s' "$first_line" | sed -E 's/ at line [0-9]+, column [0-9]+.*$//')"
    printf '%s\n' "$first_line" > "tests/expected/diagnostics/$name.contains"

    rm -f "$tmp_err" "$tmp_out"
done

# Hand-tighten known path-sensitive diagnostics into robust fragments.
cat > tests/expected/diagnostics/bad_import_cycle.contains <<'EOD'
import cycle detected
std/cycle_a.flow
std/cycle_b.flow
EOD

cat > tests/expected/diagnostics/bad_import_main.contains <<'EOD'
imported file defines main
only unit files may be imported
std/main_bad.flow
EOD

cat > tests/expected/diagnostics/bad_undeclared_assignment.contains <<'EOD'
root program has no main block
bad_undeclared_assignment.flow
EOD

cat > tests/expected/diagnostics/bad_uninitialized.contains <<'EOD'
root program has no main block
bad_uninitialized.flow
EOD

echo "generated expectations under tests/expected"
