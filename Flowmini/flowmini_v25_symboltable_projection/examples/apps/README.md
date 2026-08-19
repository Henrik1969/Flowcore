# Application examples

These examples are complete, reproducible vertical slices through the current
Flowcore language and compiler chain. Each application directory owns its
source, capability policy, expected output, and runner.

## Directory contract

- `*.flow` — application source;
- `policy.conf` — exact Flowbind capability grants;
- `expected-*.txt` — checked-in observable output;
- `run-*.sh` — reproducible pipeline runner using temporary intermediates;
- `README.md` — scope, stage contracts, and known limitations.

## Applications

- [`flowcat`](flowcat/README.md) — typed argv entry point with policy-authorized
  output lowering.
