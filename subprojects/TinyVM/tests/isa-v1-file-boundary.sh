#!/bin/sh
set -eu
pack=$1 validate=$2 run=$3 build=$4
artifact="$build/tinyvm-isa-v1-boundary.tvm"
"$pack" "$artifact"
"$validate" "$artifact" | grep -q '"status":"valid".*"isa_version":1'
"$run" "$artifact" | grep -q '"status":"completed".*"carrier":3.*"result":42'
rm -f "$artifact"
