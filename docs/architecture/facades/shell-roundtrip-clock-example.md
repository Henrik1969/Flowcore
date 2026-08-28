# Shell round-trip experiment: English/Bash ↔ Danish/Flowcore

This experiment uses the existing
[`check-frankencore-clock.sh`](../../../tools/check-frankencore-clock.sh)
script as the canonical substrate example.

## Canonical Bash surface

The original script uses canonical Bash syntax and native commands:

```bash
clock_bin=${CLOCK_BIN:?CLOCK_BIN is required}
report="$($clock_bin --clock monotonic)"

if ! command -v jq >/dev/null 2>&1; then
    echo 'clock contract check: jq is required' >&2
    exit 2
fi

test "$(jq -r '.status' <<<"$report")" = "ok"
echo 'Frankencore Clock contract: PASS'
```

## Danish capability projection

This is a proposed localized source surface. It is intentionally not passed
directly to Bash; the moniker resolver would translate it to canonical Bash
before execution.

```text
ur_bin=${CLOCK_BIN:?CLOCK_BIN er påkrævet}
rapport="$(ur_bin --clock monotonic)"

hvis ikke kommando-find jq >/dev/null 2>&1; så
    skriv 'kontrol af ur-kontrakt: jq er påkrævet' >&2
    afslut 2
slut

test "$(jq -r '.status' <<<"$rapport")" = "ok"
skriv 'Frankencore ur-kontrakt: BESTÅET'
```

Illustrative moniker mappings:

| Danish surface | Canonical identity |
|---|---|
| `ur_bin` | `clock_bin` |
| `rapport` | `report` |
| `hvis` | `if` |
| `ikke` | `!` |
| `kommando-find` | `command -v` |
| `så` | `{` / block start |
| `skriv` | `echo` |
| `afslut` | `exit` |
| `slut` | `}` / block end |

The JSON fields remain canonical because they are schema identifiers, not
human-facing prose. The Danish diagnostic text may be translated without
changing the machine-readable contract.

## Canonical back-translation

Resolving the Danish surface produces the same semantic form:

```bash
clock_bin=${CLOCK_BIN:?CLOCK_BIN is required}
report="$($clock_bin --clock monotonic)"

if ! command -v jq >/dev/null 2>&1; then
    echo 'clock contract check: jq is required' >&2
    exit 2
fi

test "$(jq -r '.status' <<<"$report")" = "ok"
echo 'Frankencore Clock contract: PASS'
```

## Comparison

- **Semantic operation:** equivalent.
- **Capability identities:** equivalent after moniker resolution.
- **Arguments, control flow, exit status, streams:** preserved.
- **JSON schema:** unchanged.
- **Human diagnostics:** locale-specific by design.
- **Raw bytes:** not necessarily equal because names, comments, whitespace,
  and messages may differ.
- **Execution:** the canonical form passes `bash -n`; the Danish form requires
  the localized parser before it can execute.

This demonstrates the intended boundary: translation is an AST/capability
projection and round-trip, not blind text replacement. The canonical artifact
can be recovered mechanically, while local authors remain free to write the
capability-facing parts of the script in their own lingo.
