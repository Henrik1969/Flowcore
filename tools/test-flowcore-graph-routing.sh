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
grep -Eq 'route start\.out => probe\.left \[wire:[0-9]+\] \[signal:[0-9]+\]' "$tmpdir/stderr"
grep -Eq 'route probe\.out => halt\.in \[wire:[0-9]+\] \[signal:[0-9]+\]' "$tmpdir/stderr"

cat > "$tmpdir/fanout.flow" <<'EOF'
program fanout

producer start : start.record
node left : record.port_probe
node right : record.port_probe
sink left_halt : halt.record
sink right_halt : halt.record

wire start.out => left.left
wire start.out => right.right
wire left.out => left_halt.in
wire right.out => right_halt.in
main { marker : int(1) }
EOF

"$flowmini" --trace true "$tmpdir/fanout.flow" > "$tmpdir/fanout.stdout" 2> "$tmpdir/fanout.stderr"
left_route=$(grep -E 'route start\.out => left\.left \[wire:0\] \[signal:[0-9]+\]' "$tmpdir/fanout.stderr")
right_route=$(grep -E 'route start\.out => right\.right \[wire:1\] \[signal:[0-9]+\]' "$tmpdir/fanout.stderr")
left_signal=$(printf '%s\n' "$left_route" | sed -E 's/.*\[(signal:[0-9]+)\].*/\1/')
right_signal=$(printf '%s\n' "$right_route" | sed -E 's/.*\[(signal:[0-9]+)\].*/\1/')
test "$left_signal" = "$right_signal"

cat > "$tmpdir/unconnected.flow" <<'EOF'
program unconnected
producer start : start.record
main { marker : int(1) }
EOF
"$flowmini" --trace true "$tmpdir/unconnected.flow" > "$tmpdir/unconnected.stdout" 2> "$tmpdir/unconnected.stderr"
grep -Fq 'no wire connected from start.out; dropping envelope' "$tmpdir/unconnected.stderr"

cat > "$tmpdir/bad-port.flow" <<'EOF'
program bad_port
producer start : start.record
sink halt : halt.record
wire start.missing => halt.in
main { marker : int(1) }
EOF
if "$flowmini" "$tmpdir/bad-port.flow" >/dev/null 2> "$tmpdir/bad-port.stderr"; then
    echo 'invalid source port unexpectedly accepted' >&2
    exit 1
fi
grep -Fq 'source endpoint start.missing is not an output port' "$tmpdir/bad-port.stderr"

echo 'Flowcore graph routing: PASS'
