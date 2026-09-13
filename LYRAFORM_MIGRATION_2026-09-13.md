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
migration branch SHA: TBD
final main SHA: TBD
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
pre-rename clean configure/build: PASS
pre-rename CTest: 81/81 PASS
post-rename clean configure/build: TBD
post-rename CTest: TBD
final fresh-clone configure/build: TBD
final fresh-clone CTest: TBD
old GitHub URL redirect verified: TBD
```

## Scope protections

```text
FlowLFS touched: NO
FlowLFS merged: NO
master touched: NO
force used: NO
```

The GitHub profile and FrankenCore current links will be recorded here after
the corresponding external updates are verified. Historical links and records
are not rewritten merely because the current repository identity changed.

## Remaining known risks

The GitHub repository rename requires valid GitHub account authority. The
local SSH clone credential is sufficient for Git transport, but the configured
`gh` account token is currently invalid; repository/profile updates remain
unverified until that authority is available.
