# Modern CLI profile v0

Status: active construction on the writable Flowcore twin

## Purpose

`flowlfs.modern-cli.v0` is an opt-in composition of acquisition and inspection
capabilities. It is not part of the recovery base and is not implied by the
shell-environment profile.

The profile is deliberately factored so an owner can select useful parts
without accepting every tool or every trust decision.

## Capability graph

```text
libtasn1
   -> p11-kit
      -> make-ca program
         -> selected certificate-policy snapshot
            -> HTTPS downloader
               -> Git

independent presentation tools
   -> fzf / zoxide / eza / bat / Neovim
```

The program that processes trust anchors and the selected set of trusted
anchors are separate capabilities. Installing TLS libraries does not grant a
certificate authority trust. Installing a downloader does not grant it
network, source-admission, execution, or projection authority.

## Control specification

The first trust tranche follows the captured BLFS systemd book revision
`r13.0-1379`, retrieved 2026-08-30:

- libtasn1 4.21.0;
- p11-kit 0.26.5;
- make-ca 1.16.1;
- a separately captured Mozilla certificate-policy input.

Versions remain fixed until an explicit profile revision changes them.
“Latest” is not a reproducible dependency selector.

## Opt-in law

- The recovery base boots and accepts local/SSH maintenance without this
  profile.
- Each package is independently admitted and projected.
- Profile resolution is an explicit owner decision.
- Trust-policy data is versioned and reviewable independently of code.
- Automatic network refresh is disabled by default.
- A refresh produces a candidate policy object; it never mutates an admitted
  object or silently changes the active projection.
- Local owner anchors are mutable owner state and are not folded into the
  upstream policy object.
- Rollback must restore both projected files and derived shared registries.

## First construction gate

The first executable gate is libtasn1. It proves that a shared-library package
can be built unprivileged, admitted immutably, projected reversibly, made
visible through the dynamic-linker registry, and consumed by the following
p11-kit build.

