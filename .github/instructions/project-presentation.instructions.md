---
applyTo: "README.md,Flowmini/README.md,docs/index.md,docs/presentation/**"
---

# Project-presentation instructions

These files form Flowcore's public-facing presentation layer. Keep them concise,
navigable, and evidence-bound.

- Use `docs/presentation/current-status-v1.json` as the current factual index.
- Link important claims to the checkpoint or architecture document that proves
  or defines them.
- Clearly label future direction. Flowmini is not self-hosted and FlowOpenOffice
  has not been ported.
- Describe LLVM and TinyVM as backends of a shared governed lowering chain only
  to the extent recorded by the current checkpoint.
- Keep build commands executable from a clean checkout and avoid machine-local
  paths.
- Never replace detailed evidence documents with promotional summaries.
