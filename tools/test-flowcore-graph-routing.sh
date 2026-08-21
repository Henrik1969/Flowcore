#!/bin/sh
set -eu

flowmini=${FLOWMINI_BIN:?FLOWMINI_BIN is required}
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT

cat > "$tmpdir/ports.flow" <<'EOF'
program ports

producer start : start.record
node probe : record.port_probe
sink halt : halt.record

main {
    one : int(1)
}

wire start.out => probe.left
wire probe.out => halt.in
EOF

"$flowmini" --trace true "$tmpdir/ports.flow" > "$tmpdir/stdout" 2> "$tmpdir/stderr"
grep -Eq 'route start\.out => probe\.left \[wire:[0-9]+\]' "$tmpdir/stderr"
grep -Eq 'route probe\.out => halt\.in \[wire:[0-9]+\]' "$tmpdir/stderr"

echo 'Flowcore graph routing: PASS'
