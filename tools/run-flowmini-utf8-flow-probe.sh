#!/usr/bin/env bash
set -euo pipefail
exec "$(cd "$(dirname "${BASH_SOURCE[0]}")/../Flowmini/flowmini_v25_symboltable_projection/tools" && pwd)/run-flowmini-utf8-flow-probe.sh"
