<!-- BEGIN FLOWCORE AUTONOMOUS REUSABLE-CHAIN POLICY -->

## Autonomous reusable-chain work

The active autonomous mission is defined by
`docs/tasks/typed-artifact-contract-v028.md`. Read it completely before making
task changes. Treat it as the authoritative objective and definition of done.

### Continuation contract

- Continue through the first unfinished gate after every coherent checkpoint.
- An intermediate green build, successful commit, plan update, context
  compaction, or completed implementation slice is not completion.
- Recover after a fresh session by reading, in order: this file, the task file,
  `docs/architecture/typed-artifact-contract-plan.md`, the current maturation
  ledger, `.codex-run-state`, recent Git history, and the worktree.
- Keep `.codex-run-state` as exactly one of `CONTINUE`, `BLOCKED`, or `DONE`.
- Leave it as `CONTINUE` while any task gate remains unfinished.
- Set it to `BLOCKED` only when all safe progress is prevented by a genuine
  semantic decision, missing credential, unavailable authority, or external
  dependency. Record concrete evidence and the required decision in the ledger.
- Set it to `DONE` only after every definition-of-done gate passes, the final
  checkpoint is committed and pushed, and the worktree is clean.

### Execution discipline

- Inspect before editing and preserve unrelated user changes.
- Prefer small, reversible architectural slices over broad rewrites.
- Do not add application names, source-unit names, or fixture names to compiler
  dispatch as a substitute for generic language machinery.
- Run focused tests while iterating and the complete canonical gate at major
  boundaries.
- When a test fails, diagnose and repair it; failure alone is not a reason to
  stop.
- When ordinary implementation alternatives exist, choose the smallest option
  consistent with documented contracts, record the assumption, and continue.
- Keep confirmed semantics, temporary compatibility behavior, and proposals
  explicitly distinguished in code and documentation.

### Git authority and limits

- Normal commits and pushes to the currently checked-out development branch are
  authorized for this mission.
- Commit coherent, buildable checkpoints with descriptive messages.
- Update the maturation ledger before each major checkpoint, then push and
  continue immediately.
- Never force-push, rewrite published history, merge a pull request, delete a
  branch, change repository settings, or modify unrelated issues or pull
  requests.
- Do not discard or overwrite pre-existing user changes.

### Legitimate stopping conditions

Do not stop for ordinary uncertainty, a large remaining workload, an
intermediate report, a failed test that can be investigated, or a choice that
can be isolated behind a reversible assumption.

Stop only when the complete mission is done or when all safe progress is
genuinely blocked. A blocked report must identify the exact fact, attempted
alternatives, preserved repository state, and smallest decision or authority
needed from Henrik.

<!-- END FLOWCORE AUTONOMOUS REUSABLE-CHAIN POLICY -->
