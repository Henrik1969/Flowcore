# FlowLFS execution baseline

This tree is distinct from the immutable LFS 13.0-systemd control baseline.
It pins one coherent official LFS systemd development snapshot after the 13.0
security corrections.

```text
book/       exact execution book snapshot
manifests/  matching official source list and checksums
sources/    verified payload cache, intentionally untracked
```

The development snapshot is not described as a stable LFS release. Its exact
book revision, retrieval time, hashes, and differences from the control
baseline are mandatory evidence.

The execution contract is the book, download list, and checksums from the same
retrieval event. Do not combine this package set with instructions from the
13.0 control book.

## Reproduce and verify

```sh
scripts/fetch-execution-snapshot.sh
scripts/fetch-execution-sources.sh
scripts/compare-execution-inputs.sh
```

The first command intentionally advances to the newest official development
snapshot and therefore changes the pinned contract. It is a maintainer action,
not part of an ordinary reproducible build. Ordinary builds use the committed
snapshot and run only the source retrieval and verification step.

See [MUTATION-LEDGER.md](MUTATION-LEDGER.md) for the authority boundary and
classification of the differences.
