# FlowLFS basis v1

Status: complete, store-reinstalled, and sealed as a runnable VM image

## Constitutional law

FlowLFS preserves canonically expected Linux behavior. Flowcore capabilities
are additive, explicit, inspectable, and removable. The recovery shell remains
Bash, `/etc/skel` remains the canonical new-account template, and `useradd -m`
continues to create an ordinary user-owned home.

## Realization

`flowlfs.basis.v1` resolves three immutable objects: Zsh 5.9.2, the owner-
authored `flow-basis` policy, and the native Flowcore `sel` proof tool. It does
not absorb the independently optional modern CLI profile.

The policy adds `/usr/local/sbin` and `/usr/local/bin` to the canonical PATH,
per-user Bash/Zsh history behavior, portable canonical home directories, and a
Zsh moniker mechanism. `/etc/skel` contains only portable home-relative
defaults. Account creation copies those templates; package updates and rollback
never traverse or overwrite an existing home.

The default moniker catalogue is a user-owned starting point, not global law.
Users may edit, add, remove, or replace every mapping. Convenience commands are
installed only when they do not collide with an existing command identity;
`go2 NAME` remains the unambiguous interface.

`sel` retains its canonical Flow source, frontend, semantic, optimization,
binding, lowering and LLVM evidence. Until the Flow compiler chain is admitted
inside FlowLFS, the package uses a recorded source-derived x86_64 assembly
bootstrap and performs the final assembly/link as `flowbuilder` with LFS GCC.
The sole compatibility adaptation removes Clang's metadata-only `.addrsig`
directive for GNU `as`; both forms and the exact diff are retained.

## Acceptance

- exact-object preflight;
- canonical `useradd -m -k /etc/skel` behavior;
- user ownership of all copied templates;
- ordinary-user Bash and Zsh startup;
- private history location and prefix search policy;
- editable moniker catalogue and navigation;
- ordinary-user `sel` pseudo-terminal execution;
- unchanged Bash recovery shell and active SSH;
- full reverse rollback followed by store-only reprojection.

## Runnable artifact

`artifacts/FlowLFS-v0.1-basis-v1.qcow2` is a standalone 40 GiB qcow2 image
(5.09 GiB stored when sealed) with no backing file. It passed `qemu-img check`
after clean shutdown.

SHA-256:
`1d09a2093a688cdc0b1c21a91f6030057989bf7aff718f226bbd537bc1e7ed60`

Run `scripts/launch-basis-v1.sh`; the default callback is port 2228 and the
launcher uses a disposable snapshot so experiments do not mutate sealed bytes.
