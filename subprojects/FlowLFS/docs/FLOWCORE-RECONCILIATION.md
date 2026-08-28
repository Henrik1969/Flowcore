# Flowcore documentation reconciliation

Date: 2026-08-28

## Reviewed scope

The live Flowcore documentation set was reviewed before creating this project:
206 Markdown files, approximately 110,000 words, excluding the explicitly
historical `_archive/` implementation snapshots. The review included project
entrances, active component declarations, architecture and ADRs, bootstrap and
self-hosting plans, checkpoints, development policy, language documentation,
provider boundaries, subprojects, and the completed TinyVM mission.

Historical documents remain provenance, not current authority. The active
TinyVM mission is complete and `.codex-run-state` is `DONE`.

## Binding consequences for FlowLFS

- The LFS command sequence is the factual baseline; Flowcore analysis cannot
  retroactively change what was built.
- Every meaningful boundary should become a durable, independently inspectable
  artifact with identity, revision, provenance, and structured failure.
- Source discovery and local availability do not authorize substitution.
- Target choice is policy and must not be inferred from host accidents.
- Build prerequisites must become explicit rather than remaining hidden
  environmental assumptions.
- Providers implement governed mechanics; they do not acquire semantic
  authority merely because the baseline uses them.
- Build, install, boot, runtime, and representation concerns remain separate.
- A new application or system must not require compiler dispatch based on its
  name, source filename, or fixture identity.
- Flowcore's source graph, `=>` relation, and `->` value placement remain
  distinct from shell command sequencing used to construct the baseline.

## Reconciliation with LFS

LFS supplies valuable experimental controls: a pinned source set,
cross-toolchain isolation, staged host independence, repeated toolchain sanity
checks, explicit test expectations, a chroot transition, configuration,
kernel, bootloader, and a bootable end state.

Flowcore should first observe those stages as transformations and capability
dependencies. It must not replace the commands with speculative orchestration
before the exact baseline succeeds. The first useful Flowcore overlay is an
evidence graph describing inputs, effects, outputs, checks, failures, and
provenance for each LFS step.

## Open questions deliberately not decided

- The public schema for a system-construction graph.
- Package and source capability contract names.
- Whether Flowmini can express the complete build graph at its present stage.
- The eventual image builder, boot provider, and ISO assembly provider.
- Which LFS mechanisms remain baseline-only and which become long-lived
  providers.
