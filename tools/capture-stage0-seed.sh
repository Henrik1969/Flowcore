#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$root"

tracked_digest=$(
    git ls-files -z -- CMakeLists.txt Flowcontracts Flowmini/flowmini_v25_symboltable_projection Flowanalyst Flowbind Flowoptimize Flowlower subprojects/TinyVM \
        | sort -z \
        | xargs -0 sha256sum \
        | sha256sum \
        | cut -d ' ' -f 1
)

tool_version() {
    command_name=$1
    if command -v "$command_name" >/dev/null 2>&1; then
        "$command_name" --version 2>/dev/null | head -n 1
    else
        printf '%s' unavailable
    fi
}

jq -cn \
    --arg revision "$(git rev-parse HEAD)" \
    --arg tracked_digest "$tracked_digest" \
    --arg cmake "$(tool_version cmake)" \
    --arg cxx "$(tool_version c++)" \
    --arg cc "$(tool_version cc)" \
    --arg clang "$(tool_version clang)" \
    --arg jq_version "$(tool_version jq)" \
    '{format:"flowcore.bootstrap_seed",version:1,status:"captured",
      source:{revision:$revision,tracked_sha256:$tracked_digest},
      tools:{cmake:$cmake,c_compiler:$cc,cxx_compiler:$cxx,clang:$clang,jq:$jq_version},
      standards:{c:"C11",cxx:"C++20"},
      providers:["host-c-runtime","posix-shell","openssl-crypto","threads","dynamic-loader","llvm-toolchain"],
      deterministic_fields:["format","version","standards","providers","source.tracked_sha256"],
      environment_fields:["source.revision","tools"]}'
