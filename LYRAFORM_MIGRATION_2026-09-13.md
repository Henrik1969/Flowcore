# Lyraform identity migration report

This report records the controlled migration from Flowcore to Lyraform.

## Identity

```text
pre-migration repository: Henrik1969/Flowcore
post-migration repository: Henrik1969/Lyraform
project identity: Lyraform
toolchain: Igor
primary branch: main
effective date: 2026-09-13
```

## Git and paths

```text
starting main SHA: 8b5f838fcd5b4add1094e462f23990f468c16d8e
migration branch SHA: 8495635c06612cd8d5ae0cce361f8a1845ff1f1d
final main SHA: 8495635c06612cd8d5ae0cce361f8a1845ff1f1d (verified migration checkpoint)
old active path: Flowmini/flowmini_v29_reusable_native_chain
new active path: Lyraform/compiler
Flowmini disposition: historical prototype/bootstrap lineage retained
```

The recovery bundle is preserved outside the repository at
`/tmp/Flowcore-pre-Lyraform-2026-09-13/flowcore-pre-lyraform.bundle` and was
verified before migration changes. The canonical checkout was clean; no local
uncommitted work required preservation.

## Tooling and compatibility

Igor commands introduced:

```text
igor build
igor check
igor test
igor run -- Lyraform/compiler/examples/pass/fn_demo.flow
igor doctor
```

Existing stage binaries such as `flowmini`, `flowanalyst`, `flowbind`,
`flowparallel`, `flowoptimize`, and `flowlower` remain available for explicit
stage use and compatibility. Serialized `flowcore.*` and `flowmini.*`
identifiers are deliberately retained because they are artifact contracts.
Source extensions are unchanged.

## Verification record

```text
pre-rename clean configure/build: PASS (146 targets, /tmp/flowcore-pre-rename-build)
pre-rename CTest: 81/81 PASS (35.74 seconds)
post-rename clean configure/build: PASS (146 targets, /tmp/lyraform-build)
post-rename CTest: 81/81 PASS (36.32 seconds)
final fresh-clone configure/build: PASS (146 targets, /tmp/lyraform-final-build)
final fresh-clone CTest: 81/81 PASS (35.27 seconds)
final fresh-clone Igor doctor/build/test: PASS / PASS / 81/81 PASS
old GitHub URL redirect verified: YES (old and new SSH remotes resolve to main)
```

## Scope protections

```text
FlowLFS touched: NO
FlowLFS merged: NO
master touched: NO
force used: NO
```

GitHub profile updated: YES (`Henrik1969` commit `31693a8`)
FrankenCore current links updated: YES (`FrankenCore` commit `b2da2f5`)
Lyraform repository description updated: YES
Historical links and records were not rewritten merely because the current
repository identity changed.

## Remaining known risks

The report commit that records these final facts advances `main` beyond the
verified migration checkpoint SHA above; its exact tip is available from
`git rev-parse HEAD` and the final Codex report. This avoids rewriting the
commit that was used for the fresh-clone verification.
