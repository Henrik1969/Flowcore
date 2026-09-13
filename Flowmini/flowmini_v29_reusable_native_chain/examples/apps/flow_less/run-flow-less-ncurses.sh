#!/bin/sh
set -eu
example_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
FLOW_PAGER_PATH=${1:?usage: run-flow-less-ncurses.sh TEXTFILE}
if [ -n "${FLOWCORE_PREFIX:-}" ]; then
    FLOWPAGER_NCURSES_INPUT=${FLOWPAGER_NCURSES_INPUT:-$FLOWCORE_PREFIX/lib/flowcore/providers/libflowpager_ncurses_input.so}
fi
FLOWPAGER_INPUT=${FLOWPAGER_NCURSES_INPUT:-${FLOWCORE_BUILD:?set FLOWCORE_BUILD, FLOWCORE_PREFIX or FLOWPAGER_NCURSES_INPUT}/flow_less_providers/libflowpager_ncurses_input.so}
export FLOW_PAGER_PATH FLOWPAGER_INPUT
"$example_dir/run-flow-less.sh"
