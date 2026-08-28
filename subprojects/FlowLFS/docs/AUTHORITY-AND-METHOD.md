# Authority and method

## Authority order

1. The pinned LFS 13.0-systemd book governs construction of the control
   baseline.
2. Flowcore binding architecture governs how meaning, contracts, provenance,
   policy, providers, targets, and derived artifacts are recorded.
3. This project records the reconciliation between them.

If the sources disagree, stop and record the conflict. Do not quietly rewrite
the LFS procedure or describe a future Flowcore mechanism as current fact.

## Two-track rule

Every LFS step is handled on two distinct tracks:

| Track | Question | Allowed result |
| --- | --- | --- |
| Baseline | What does the pinned book require? | An exact command, input, output, test, or configuration record. |
| Flowcore analysis | What capability, effect, dependency, policy, provider, or provenance boundary does the step reveal? | A proposal or observation that does not mutate the baseline. |

Only after the baseline is reproducible may a separately identified derived
image introduce a Flowcore adaptation.

## Flowcore laws applied to construction

- Source inputs, build products, target images, and observations are durable
  artifacts rather than hidden process state.
- Each transformation names its input identity, output identity, tool,
  revision, policy, and evidence.
- Package discovery is not package authorization.
- Host availability must not silently select a target or provider.
- Build-time, install-time, boot-time, and runtime requirements remain
  distinct.
- Canonical system state remains distinct from any future GUI, TUI, CLI, or
  document representation.
- The baseline remains independently inspectable and bootable after derived
  experiments begin.

## Initial Flowcore analysis vocabulary

These are analysis labels, not frozen public APIs:

- `source acquisition`
- `source verification`
- `build prerequisite`
- `cross-toolchain`
- `temporary tool`
- `filesystem projection`
- `identity and ownership`
- `configuration fact`
- `build policy`
- `boot capability`
- `service lifecycle`
- `diagnostic evidence`
- `artifact emission`

## Stop conditions

Stop before an action that would:

- mutate the production host's partitions, mounts, bootloader, users, groups,
  or system configuration;
- use an unverified or substituted source archive;
- hide a deviation from the pinned book;
- erase source/build provenance;
- conflate a container root with a boot-tested VM image;
- define a public Flowcore capability contract from conjecture;
- require credentials, publication, or external authority not already granted.
