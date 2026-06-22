#!/usr/bin/env bash
set -euo pipefail

# Flowmini categorized test-suite runner with environment-variable defaults.
#
# Black-box integration runner:
#   build flowmini -> run categorized .flow examples -> capture exit/stdout/stderr
#   optionally Valgrind -> optionally GDB backtraces
#
# Env defaults:
#
#   FLOWLAB_TOP              optional project top, used only for convenience/messages
#   FLOWMINI_ROOT            default --root
#   FLOWMINI_BUILD_DIR       default --build-dir, relative to root unless absolute
#   FLOWMINI_EXAMPLES_DIR    default --examples, relative to root unless absolute
#   FLOWMINI_TESTS_DIR       default --tests, relative to root unless absolute
#   FLOWMINI_REPORT_DIR      default --report, relative to root unless absolute
#   FLOWMINI_STDIN           default --input
#   FLOWMINI_JOBS            default --jobs
#   FLOWMINI_TIMEOUT         default --timeout
#   FLOWMINI_NO_BUILD        1/true/yes/on disables build
#   FLOWMINI_VALGRIND        1/true/yes/on enables Valgrind
#   FLOWMINI_GDB_FAILURES    1/true/yes/on enables GDB for bad tests
#   FLOWMINI_GDB_ALL         1/true/yes/on enables GDB for all tests
#   FLOWMINI_RUN_SUPPORT     1/true/yes/on runs support files as expected-fail
#   FLOWMINI_KEEP_REPORT     1/true/yes/on keeps old report directory
#
# CLI options override env defaults.

truthy() {
    case "${1:-}" in
        1|true|TRUE|yes|YES|on|ON) return 0 ;;
        *) return 1 ;;
    esac
}

ROOT="${FLOWMINI_ROOT:-.}"
BUILD_DIR="${FLOWMINI_BUILD_DIR:-build}"
EXAMPLES_DIR="${FLOWMINI_EXAMPLES_DIR:-examples}"
TESTS_DIR="${FLOWMINI_TESTS_DIR:-tests}"
REPORT_DIR="${FLOWMINI_REPORT_DIR:-test-report}"
INPUT_VALUE="${FLOWMINI_STDIN:-5}"
JOBS="${FLOWMINI_JOBS:-${JOBS:-20}}"
TIMEOUT_SECONDS="${FLOWMINI_TIMEOUT:-${TIMEOUT_SECONDS:-20}}"

DO_BUILD=1
DO_VALGRIND=0
GDB_FAILURES=0
GDB_ALL=0
RUN_SUPPORT=0
KEEP_REPORT=0

truthy "${FLOWMINI_NO_BUILD:-}" && DO_BUILD=0
truthy "${FLOWMINI_VALGRIND:-}" && DO_VALGRIND=1
truthy "${FLOWMINI_GDB_FAILURES:-}" && GDB_FAILURES=1
truthy "${FLOWMINI_GDB_ALL:-}" && GDB_ALL=1
truthy "${FLOWMINI_RUN_SUPPORT:-}" && RUN_SUPPORT=1
truthy "${FLOWMINI_KEEP_REPORT:-}" && KEEP_REPORT=1

usage() {
    cat <<USAGE
Usage: $0 [options]

Options override environment variables.

Options:
  --root DIR          Flowmini project root. Env: FLOWMINI_ROOT. Default: .
  --build-dir DIR     Build dir. Env: FLOWMINI_BUILD_DIR. Default: build
  --examples DIR      Examples dir. Env: FLOWMINI_EXAMPLES_DIR. Default: examples
  --tests DIR         Tests dir. Env: FLOWMINI_TESTS_DIR. Default: tests
  --report DIR        Report dir. Env: FLOWMINI_REPORT_DIR. Default: test-report
  --input VALUE       Stdin value supplied to tests. Env: FLOWMINI_STDIN. Default: 5
  --jobs N            Parallel build jobs. Env: FLOWMINI_JOBS. Default: 20
  --timeout SEC       Per-test timeout. Env: FLOWMINI_TIMEOUT. Default: 20
  --no-build          Do not configure/build before running tests.
  --valgrind          Run tests under Valgrind.
  --gdb-failures      Rerun bad/unexpected results under GDB and save backtraces.
  --gdb-all           Rerun every test under GDB and save backtraces.
  --run-support       Run examples/support/*.flow as expected-fail.
  --keep-report       Do not delete old report before running.
  --print-env         Print useful export lines for this root and exit.
  --help              Show this help.

Expected layout:
  examples/pass/*.flow     must exit 0
  examples/fail/*.flow     must exit nonzero
  examples/support/*.flow  ignored unless --run-support

Optional expectations:
  tests/expected/stdout/<name>.out
  tests/expected/stderr/<name>.err
  tests/expected/diagnostics/<name>.contains

Typical lazy setup:
  export TOP="\$(pwd)"
  export FLOWLAB_TOP="\$TOP"
  export FLOWMINI_ROOT="\$TOP/Flowmini/flowmini_v21_structural_bridge"
  export FLOWMINI_STDIN=5

Then:
  \$TOP/tools/run-flowmini-test-suite.sh
USAGE
}

PRINT_ENV=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        --root) ROOT="$2"; shift 2 ;;
        --build-dir) BUILD_DIR="$2"; shift 2 ;;
        --examples) EXAPLES_DIR="$2"; shift 2 ;;
        --tests) TESTS_DIR="$2"; shift 2 ;;
        --report) REPORT_DIR="$2"; shift 2 ;;
        --input) INPUT_VALUE="$2"; shift 2 ;;
        --jobs) JOBS="$2"; shift 2 ;;
        --timeout) TIMEOUT_SECONDS="$2"; shift 2 ;;
        --no-build) DO_BUILD=0; shift ;;
        --valgrind) DO_VALGRIND=1; shift ;;
        --gdb-failures) GDB_FAILURES=1; shift ;;
        --gdb-all) GDB_ALL=1; shift ;;
        --run-support) RUN_SUPPORT=1; shift ;;
        --keep-report) KEEP_REPORT=1; shift ;;
        --print-env) PRINT_ENV=1; shift ;;
        --help|-h) usage; exit 0 ;;
        *) echo "unknown argument: $1" >&2; usage >&2; exit 2 ;;
    esac
done

ROOT="$(cd "$ROOT" && pwd)"

abs_or_rooted() {
    local p="$1"
    if [[ "$p" = /* ]]; then
        printf "%s" "$p"
    else
        printf "%s/%s" "$ROOT" "$p"
    fi
}

BUILD_PATH="$(abs_or_rooted "$BUILD_DIR")"
EXAMPLES_PATH="$(abs_or_rooted "$EXAMPLES_DIR")"
TESTS_PATH="$(abs_or_rooted "$TESTS_DIR")"
REPORT_PATH="$(abs_or_rooted "$REPORT_DIR")"
BIN="$BUILD_PATH/flowmini"

if [[ "$PRINT_ENV" -eq 1 ]]; then
    cat <<EOF
export FLOWMINI_ROOT="$ROOT"
export FLOWMINI_BUILD_DIR="$BUILD_DIR"
export FLOWMINI_EXAMPLES_DIR="$EXAMPLES_DIR"
export FLOWMINI_TESTS_DIR="$TESTS_DIR"
export FLOWMINI_REPORT_DIR="$REPORT_DIR"
export FLOWMINI_STDIN="$INPUT_VALUE"
export FLOWMINI_JOBS="$JOBS"
export FLOWMINI_TIMEOUT="$TIMEOUT_SECONDS"
EOF
    exit 0
fi

PASS_DIR="$EXAMPLES_PATH/pass"
FAIL_DIR="$EXAMPLES_PATH/fail"
SUPPORT_DIR="$EXAMPLES_PATH/support"

EXP_OUT="$TESTS_PATH/expected/stdout"
EXP_ERR="$TESTS_PATH/expected/stderr"
EXP_DIAG="$TESTS_PATH/expected/diagnostics"

[[ -d "$PASS_DIR" ]] || { echo "missing $PASS_DIR; classify examples first" >&2; exit 2; }
[[ -d "$FAIL_DIR" ]] || { echo "missing $FAIL_DIR; classify examples first" >&2; exit 2; }

if [[ "$KEEP_REPORT" -eq 0 ]]; then
    rm -rf "$REPORT_PATH"
fi
mkdir -p "$REPORT_PATH"/{stdout,stderr,diff,valgrind,gdb}

SUMMARY="$REPORT_PATH/summary.tsv"
MANIFEST="$REPORT_PATH/manifest.tsv"
FAILURES="$REPORT_PATH/failures.txt"

printf "status\tkind\texit\tstdout\tstderr\tdiagnostic\tvalgrind\tgdb\tfile\n" > "$SUMMARY"
printf "kind\tfile\tname\texpected_stdout\texpected_stderr\texpected_diagnostic\n" > "$MANIFEST"
: > "$FAILURES"

echo "== Flowmini test environment =="
echo "root:      $ROOT"
echo "binary:    $BIN"
echo "examples:  $EXAMPLES_PATH"
echo "tests:     $TESTS_PATH"
echo "report:    $REPORT_PATH"
echo "stdin:     $INPUT_VALUE"
echo "jobs:      $JOBS"
echo "timeout:   $TIMEOUT_SECONDS"
echo "build:     $DO_BUILD"
echo "valgrind:  $DO_VALGRIND"
echo "gdb bad:   $GDB_FAILURES"
echo "gdb all:   $GDB_ALL"
echo

# Preserve the current language lab assumption:
# imports/resources are resolved as if the user is standing in the version root.
cd "$ROOT"

if [[ "$DO_BUILD" -eq 1 ]]; then
    echo "== configuring/building Debug =="
    cmake -S "$ROOT" -B "$BUILD_PATH" -DCMAKE_BUILD_TYPE=Debug
    cmake --build "$BUILD_PATH" -j"$JOBS"
fi

[[ -x "$BIN" ]] || { echo "missing executable: $BIN" >&2; exit 2; }

# ABI/demo helpers often live beside the executable/build output.
export LD_LIBRARY_PATH="$BUILD_PATH:${LD_LIBRARY_PATH:-}"

have_timeout=0; command -v timeout >/dev/null 2>&1 && have_timeout=1
have_valgrind=0; command -v valgrind >/dev/null 2>&1 && have_valgrind=1
have_gdb=0; command -v gdb >/dev/null 2>&1 && have_gdb=1

[[ "$DO_VALGRIND" -eq 0 || "$have_valgrind" -eq 1 ]] || { echo "valgrind requested but not found" >&2; exit 2; }
[[ "$GDB_FAILURES" -eq 0 && "$GDB_ALL" -eq 0 || "$have_gdb" -eq 1 ]] || { echo "gdb requested but not found" >&2; exit 2; }

name_of() {
    local b
    b="$(basename "$1")"
    printf "%s" "${b%.flow}"
}

rel_of() {
    printf "%s" "${1#"$ROOT"/}"
}

safe_of() {
    local r
    r="$(rel_of "$1")"
    r="${r//\//__}"
    r="${r// /_}"
    printf "%s" "$r"
}

run_program() {
    local file="$1" out="$2" err="$3" vg="$4"
    local rel
    rel="$(rel_of "$file")"

    if [[ "$DO_VALGRIND" -eq 1 ]]; then
        local cmd=(valgrind --leak-check=full --show-leak-kinds=definite,indirect,possible
                   --errors-for-leak-kinds=definite,indirect,possible --error-exitcode=99
                   --log-file="$vg" "$BIN" "$rel")
    else
        local cmd=("$BIN" "$rel")
    fi

    if [[ "$have_timeout" -eq 1 ]]; then
        printf "%s\n" "$INPUT_VALUE" | timeout "$TIMEOUT_SECONDS" "${cmd[@]}" >"$out" 2>"$err"
    else
        printf "%s\n" "$INPUT_VALUE" | "${cmd[@]}" >"$out" 2>"$err"
    fi
}

run_gdb() {
    local file="$1" log="$2" input="$REPORT_PATH/gdb/input.txt"
    local rel
    rel="$(rel_of "$file")"

    printf "%s\n" "$INPUT_VALUE" > "$input"

    gdb --batch --quiet \
        -ex "set pagination off" \
        -ex "set confirm off" \
        -ex "run < $input" \
        -ex "bt" \
        -ex "bt full" \
        -ex "info threads" \
        -ex "thread apply all bt" \
        --args "$BIN" "$rel" >"$log" 2>&1 || true
}

check_exact() {
    local label="$1" actual="$2" expected="$3" diff_file="$4"
    if [[ ! -f "$expected" ]]; then
        echo "NO_EXPECTATION"
        return
    fi

    if diff -u "$expected" "$actual" >"$diff_file"; then
        echo "OK"
    else
        echo "MISMATCH"
        printf "[%s mismatch]\nexpected: %s\nactual: %s\ndiff: %s\n\n" "$label" "$expected" "$actual" "$diff_file" >> "$FAILURES"
    fi
}

check_contains() {
    local actual="$1" expected="$2" ok=1
    if [[ ! -f "$expected" ]]; then
        echo "NO_EXPECTATION"
        return
    fi

    while IFS= read -r needle || [[ -n "$needle" ]]; do
        [[ -z "$needle" ]] && continue
        if ! grep -Fq "$needle" "$actual"; then
            ok=0
            printf "[diagnostic substring missing]\nexpected substring: %s\nfrom: %s\nactual: %s\n\n" "$needle" "$expected" "$actual" >> "$FAILURES"
        fi
    done < "$expected"

    [[ "$ok" -eq 1 ]] && echo "OK" || echo "MISMATCH"
}

run_test() {
    local kind="$1" file="$2"
    local name safe out err vg gdblog expout experr expdiag
    local rc status outst errst diagst vgst gdbst

    name="$(name_of "$file")"
    safe="$(safe_of "$file")"

    out="$REPORT_PATH/stdout/$safe.out"
    err="$REPORT_PATH/stderr/$safe.err"
    vg="$REPORT_PATH/valgrind/$safe.vg"
    gdblog="$REPORT_PATH/gdb/$safe.gdb"

    expout="$EXP_OUT/$name.out"
    experr="$EXP_ERR/$name.err"
    expdiag="$EXP_DIAG/$name.contains"

    printf "%s\t%s\t%s\t%s\t%s\t%s\n" \
        "$kind" "$(rel_of "$file")" "$name" \
        "$([[ -f "$expout" ]] && echo yes || echo no)" \
        "$([[ -f "$experr" ]] && echo yes || echo no)" \
        "$([[ -f "$expdiag" ]] && echo yes || echo no)" >> "$MANIFEST"

    set +e
    run_program "$file" "$out" "$err" "$vg"
    rc=$?
    set -e

    outst="$(check_exact stdout "$out" "$expout" "$REPORT_PATH/diff/$safe.stdout.diff")"
    errst="$(check_exact stderr "$err" "$experr" "$REPORT_PATH/diff/$safe.stderr.diff")"
    diagst="$(check_contains "$err" "$expdiag")"

    vgst="NOT_RUN"
    if [[ "$DO_VALGRIND" -eq 1 ]]; then
        [[ "$rc" -eq 99 ]] && vgst="ERRORS" || vgst="OK"
    fi

    status="PASS"
    [[ "$kind" == "pass" && "$rc" -ne 0 ]] && status="UNEXPECTED_FAIL"
    [[ "$kind" == "fail" && "$rc" -eq 0 ]] && status="UNEXPECTED_PASS"
    [[ "$outst" == "MISMATCH" || "$errst" == "MISMATCH" || "$diagst" == "MISMATCH" ]] && status="EXPECTATION_MISMATCH"
    [[ "$vgst" == "ERRORS" ]] && status="VALGRIND_FAIL"

    gdbst="NOT_RUN"
    if [[ "$GDB_ALL" -eq 1 || ( "$GDB_FAILURES" -eq 1 && "$status" != "PASS" ) ]]; then
        run_gdb "$file" "$gdblog"
        gdbst="SAVED"
    fi

    printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
        "$status" "$kind" "$rc" "$outst" "$errst" "$diagst" "$vgst" "$gdbst" "$(rel_of "$file")" >> "$SUMMARY"

    [[ "$status" == "PASS" ]] && printf "." || printf "F"
}

echo "== running categorized Flowmini suite =="

shopt -s nullglob
for f in "$PASS_DIR"/*.flow; do run_test pass "$f"; done
for f in "$FAIL_DIR"/*.flow; do run_test fail "$f"; done

if [[ "$RUN_SUPPORT" -eq 1 && -d "$SUPPORT_DIR" ]]; then
    for f in "$SUPPORT_DIR"/*.flow; do run_test fail "$f"; done
fi

printf "\n\n== summary ==\n"

total="$(awk -F '\t' 'NR>1{c++} END{print c+0}' "$SUMMARY")"
passed="$(awk -F '\t' 'NR>1 && $1=="PASS"{c++} END{print c+0}' "$SUMMARY")"
bad="$(awk -F '\t' 'NR>1 && $1!="PASS"{c++} END{print c+0}' "$SUMMARY")"

echo "total: $total"
echo "pass:  $passed"
echo "bad:   $bad"
echo "report: $REPORT_PATH"

if [[ "$bad" -ne 0 ]]; then
    echo
    echo "bad results:"
    awk -F '\t' 'NR>1 && $1!="PASS" {printf "  %s  %s  exit=%s  stdout=%s stderr=%s diag=%s valgrind=%s gdb=%s\n", $1, $9, $3, $4, $5, $6, $7, $8}' "$SUMMARY"
    echo
    echo "failure details:"
    cat "$FAILURES"
    exit 1
fi
