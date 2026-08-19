#!/usr/bin/env bash
set -euo pipefail

EXAMPLE_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
FLOWMINI_ROOT="$(cd -- "${EXAMPLE_DIR}/../../.." && pwd)"
FLOWCORE_ROOT="$(cd -- "${FLOWMINI_ROOT}/../.." && pwd)"
KEEP_BUILD=0

for argument in "$@"; do
    case "$argument" in
        --keep-build) KEEP_BUILD=1 ;;
        -h|--help)
            sed -n '1,24p' "$EXAMPLE_DIR/README.md"
            echo
            echo "Usage: $0 [--keep-build]"
            exit 0
            ;;
        *)
            echo "unknown option: $argument" >&2
            echo "usage: $0 [--keep-build]" >&2
            exit 2
            ;;
    esac
done

FLOWMINI_BIN="${FLOWMINI_BIN:-${FLOWMINI_ROOT}/cmake-build-debug/flowmini}"
FLOWANALYST_BIN="${FLOWANALYST_BIN:-${FLOWCORE_ROOT}/Flowanalyst/build/flowanalyst}"
FLOWBIND_BIN="${FLOWBIND_BIN:-${FLOWCORE_ROOT}/Flowbind/build/flowbind}"
FLOWOPTIMIZE_BIN="${FLOWOPTIMIZE_BIN:-${FLOWCORE_ROOT}/Flowoptimize/build/flowoptimize}"
FLOWLOWER_BIN="${FLOWLOWER_BIN:-${FLOWCORE_ROOT}/Flowlower/build/flowlower}"

for tool in "$FLOWMINI_BIN" "$FLOWANALYST_BIN" "$FLOWBIND_BIN" "$FLOWOPTIMIZE_BIN" "$FLOWLOWER_BIN" clang; do
    if [[ "$tool" == "clang" ]]; then
        command -v clang >/dev/null || { echo "missing required tool: clang" >&2; exit 2; }
    elif [[ ! -x "$tool" ]]; then
        echo "missing executable: $tool" >&2
        exit 2
    fi
done

if [[ "$KEEP_BUILD" -eq 1 ]]; then
    tmpdir="$EXAMPLE_DIR/build"
    mkdir -p -- "$tmpdir"
    echo "preserving build artifacts in: $tmpdir"
else
    tmpdir="$(mktemp -d)"
    cleanup() { rm -rf -- "$tmpdir"; }
    trap cleanup EXIT
fi

"$FLOWMINI_BIN" --dump-frontend-bundle "$EXAMPLE_DIR/flowcat.flow" > "$tmpdir/frontend-bundle.json"
"$FLOWANALYST_BIN" < "$tmpdir/frontend-bundle.json" > "$tmpdir/semantic-report.json"
grep -q '"status": "ok"' "$tmpdir/semantic-report.json"
grep -q '"lowering_profile": "flowcat_argv_main"' "$tmpdir/semantic-report.json"

"$FLOWBIND_BIN" --policy "$EXAMPLE_DIR/policy.conf" < "$tmpdir/semantic-report.json" > "$tmpdir/binding-report.json"
grep -q '"status": "ready"' "$tmpdir/binding-report.json"

"$FLOWOPTIMIZE_BIN" < "$tmpdir/semantic-report.json" > "$tmpdir/optimization-report.json"
"$FLOWLOWER_BIN" \
    --emit-llvm "$tmpdir/flowcat.ll" \
    --binding-report "$tmpdir/binding-report.json" \
    < "$tmpdir/optimization-report.json" \
    > "$tmpdir/lowering-report.json"
grep -q '"status": "emitted"' "$tmpdir/lowering-report.json"

clang "$tmpdir/flowcat.ll" -o "$tmpdir/flowcat"
"$tmpdir/flowcat" alpha beta > "$tmpdir/stdout.txt"
cmp -s "$EXAMPLE_DIR/expected-stdout.txt" "$tmpdir/stdout.txt"

echo "flowcat example: PASS"
echo "  source: $EXAMPLE_DIR/flowcat.flow"
echo "  binary: $tmpdir/flowcat"
echo "  final output: alpha beta"
