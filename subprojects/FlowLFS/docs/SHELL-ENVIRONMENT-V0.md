# Shell environment profile v0

Status: active experiment on the writable Flowcore twin

## Purpose

`flowlfs.shell-environment.v0` supplies safe, comprehensible Bash and Zsh
defaults to users who have no personal dotfiles. It is composable with the
source-forge profile and does not select a mandatory login shell.

Canonical global policy lives under `/etc/flow-shell` and `/etc/zsh`. New Bash
users receive small writable adapters from `/etc/skel`; their personal files
may restrict or extend the defaults without becoming canonical system state.

## Inspection of Henrik's setup

The reusable ideas were:

- idempotent user-local PATH construction;
- explicit `EDITOR`, `VISUAL`, pager, history, completion, prompt, alias, and
  keybinding policy;
- separation between shell-neutral environment and interactive Zsh behavior;
- dependency-aware loading and collision avoidance.

The following remain personal or dependency-bearing and were not promoted to
global defaults:

- Zinit and startup-time repository cloning;
- Powerlevel10k and the personal `.p10k.zsh` projection;
- fzf, fzf-tab, zoxide, eza, bat, Neovim, and Git integrations while their
  providers are absent;
- Court widgets, monikers, greetings, keybinding constitution, and machine
  navigation;
- command response capture, cursor, history, and shell-journal observers;
- Henrik-specific paths, aliases, caches, histories, and presentation state.

No history, credential, token, SSH, GPG, cache, or secret-bearing material was
read or copied into FlowLFS.

## Capabilities

- shell-neutral, idempotent environment setup;
- pleasant root and ordinary-user prompts without external themes;
- bounded history policy stored in each user's home;
- Zsh completion cache stored in each user's home;
- useful aliases that do not replace `ls`, `cat`, or other base semantics;
- Bash and Zsh interactive startup;
- materialized, writable new-user Bash adapters;
- zero network activity during shell startup;
- package rollback independent of user identity.

## Identity and template boundary

An account is not a package and a home directory is not canonical global
state. `/etc/skel` is a template projection:

```text
immutable package object
        ↓ verified materialization
/etc/skel/.bashrc and .bash_profile
        ↓ account creation
ordinary user-owned writable files
```

This deliberately differs from normal package paths, which are symlinked into
the immutable store. Copying a store symlink into a home would make an existing
user depend on future store retention and would prevent normal customization.

## Anonymous acquisition direction

“Anonymous” means a person with no pre-existing FlowLFS account, local package
database, or relationship with the publisher. It does **not** mean an
unauthenticated remote login.

The first safe delivery shapes are:

1. a VM-runnable image containing the profile and its complete evidence;
2. an offline profile bundle attached as ISO or read-only virtual disk;
3. later, a static HTTPS content-addressed package store with a signed catalog.

The bootstrap flow should be:

```text
obtain public image or bundle
  → verify artifact digest/signature
  → inspect profile and source envelopes
  → boot into local owner-claim boundary
  → create a local identity
  → resolve selected composable profiles
  → reuse verified objects or rebuild from admitted source
  → project global policy
  → materialize user templates
```

The catalog only advertises identities and locations. It does not grant trust,
execution authority, account authority, or projection permission. Binary
objects may be offered as a cache, while canonical source, recipe, adaptation,
derivation, and evidence remain available for inspection and rebuilding.

## Next package layers

The visual environment should be added as separate capabilities rather than
growing this package implicitly:

- `flowlfs.modern-cli`: Git, curl/downloader, certificates, fzf, zoxide, eza,
  bat, and later Neovim;
- `flowlfs.zsh-presentation`: a reviewed prompt and completion presentation;
- `flowlfs.henrik-shell`: optional Court and personal workflow projection;
- `flowlfs.user-bootstrap`: local owner claim, account creation, and selected
  profile materialization.

No plugin manager should clone code during interactive startup. Dependencies
must enter through the package store and ordinary trust/admission gates.

Future presentation or shell semantics need not remain external scripts. Once
their contracts are understood, FlowLFS may add them directly to a
Flowcore-derived Zsh, Bash, readline, terminal, or supporting tool revision.
Such changes follow the source mutation law: canonical ancestry is retained,
the Flowcore source revision is named separately, and compatibility is tested
rather than implied.
