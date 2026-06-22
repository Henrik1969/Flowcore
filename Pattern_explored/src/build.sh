#!/usr/bin/env bash

set -u

for SOURCE in *.cpp; do
    TARGET="${SOURCE%.*}"
    LOGTARGET="${TARGET}.log"

    echo "Building $SOURCE -> $TARGET"

    if g++ -std=c++17 -Wall -Wextra -pedantic "$SOURCE" -o "$TARGET" >"$LOGTARGET" 2>&1; then
        echo "OK: $TARGET"
    else
        echo "FAILED: $SOURCE"
        echo "See: $LOGTARGET"
    fi
done
