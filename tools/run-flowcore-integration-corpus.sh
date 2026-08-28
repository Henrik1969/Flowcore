#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
flowmini=${FLOWMINI_BIN:-$root/Flowmini/flowmini_v25_symboltable_projection/cmake-build-debug/flowmini}
analyst=${FLOWANALYST_BIN:-$root/Flowanalyst/build/flowanalyst}
parallel=${FLOWPARALLEL_BIN:-$root/Flowparallel/build/flowparallel}
optimizer=${FLOWOPTIMIZE_BIN:-$root/Flowoptimize/build/flowoptimize}
corpus=$root/Flowmini/flowmini_v25_symboltable_projection/examples/integration
test -x "$flowmini"; test -x "$analyst"; test -x "$parallel"; test -x "$optimizer"

count=0
for source in "$corpus"/*.flow; do
    test -f "$source"
    report=$($flowmini --dump-frontend-bundle "$source" | "$analyst" | "$parallel" | "$optimizer")
    printf '%s\n' "$report" | jq -e --arg source "$source" '
      .format == "flowoptimize.optimization_report" and
      .status == "ready" and
      .source.path == $source and
      .state.canonical_graph == "unchanged" and
      .transforms[0].kind == "coo_deduplicate"' >/dev/null
    printf 'PASS %s\n' "${source##*/}"
    count=$((count + 1))
done
printf 'Flowcore integration corpus: %s programs passed the complete chain\n' "$count"
