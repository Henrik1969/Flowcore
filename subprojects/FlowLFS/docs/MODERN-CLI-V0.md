# Modern CLI profile v0

Status: first complete realization on the writable Flowcore twin

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

## Resolved v0 core

The resolved v0 core contains ten independently admitted objects: libtasn1,
p11-kit, make-ca, a frozen Mozilla policy input and its derived stores,
libunistring, libidn2, nghttp2, libpsl, curl, and Git. The realization lock
names every exact object and activates them in dependency order.

Git is built with upstream's `NO_RUST=1` capability constraint because no Rust
toolchain has yet been admitted. fzf, zoxide, eza, bat, Neovim, and a
Rust-enabled Git are optional unresolved extensions. Naming them does not
pretend they exist; each must pass the same source-forge process before an
owner can add it to a later realization.

## Realization interface

The guest-side owner interface is `/usr/local/sbin/flowprofile-modern-cli`:

```sh
flowprofile-modern-cli preflight
flowprofile-modern-cli activate
flowprofile-modern-cli verify
flowprofile-modern-cli status
flowprofile-modern-cli deactivate
```

Activation checks the exact ten-object lock, projects in dependency order, and
performs local verification without network access. Online HTTPS is exercised
only by the separate profile verification operation. Deactivation rolls back
in reverse order.

The completed lifecycle acceptance removed the whole profile, demonstrated
that SSH and the independently admitted shell layer survived, and reconstructed
the profile from `/flow/store` without rebuilding or downloading anything.
The permanent evidence capsule and its digest are recorded under
`flowpkg/evidence/modern-cli-v0`.
