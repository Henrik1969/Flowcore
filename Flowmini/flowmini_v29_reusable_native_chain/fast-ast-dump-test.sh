#!/bin/env bash

 grep -R -n -E '(^|[[:space:]])main([[:space:]]|$)|"main"' examples/pass \
    | cut -d: -f1 \
    | sort -u \
    | head \
    | while IFS= read -r file; do
        {
            echo "===== $file ====="
            ./cmake-build-debug/flowmini --dump-ast "$file"
            echo
            echo "----- source -----"
            cat "$file"
            echo
            echo
        } >> tempresult.txt
    done
