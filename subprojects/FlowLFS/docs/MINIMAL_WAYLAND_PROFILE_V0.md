# FlowLFS minimal Wayland profile v0

## Purpose

This profile is the first bulk-produced graphical FlowLFS system. It provides
one small, locally runnable Wayland desktop while keeping presentation choices
outside the functional base and preserving the canonical interfaces of every
upstream component.

## Admitted surface

- Wayland client/server ABI and protocol scanner.
- Canonical stable and staging Wayland protocol descriptions.
- Linux DRM/KMS output, evdev input, keyboard data and seat mediation.
- Weston as the reference compositor with its desktop shell.
- Pixman software rendering, so graphical correctness does not depend on a
  host GPU, proprietary firmware, LLVM, or a hardware-specific Mesa driver.
- A QEMU launch projection with VirtIO display, keyboard, pointer, networking,
  serial evidence, and the existing SSH callback.

The compositor is a provider selected at session start. It does not own user
configuration, shell configuration, applications, or the package store.

## Deliberate exclusions

Mesa/GPU acceleration, Vulkan, Xorg, Xwayland, a display manager, GTK/Qt,
audio, printing, browsers, portals, remote desktop, and opinionated per-user
configuration are not part of v0. They remain additive providers or later
profiles. A terminal is the first client projection to follow; it must not be
made private authority of the compositor profile.

## Production order

`Flowforge/bulk/wayland-minimal-v0.tsv` is the reviewed dependency order. Its
XML parser is an explicit build dependency of the Wayland scanner rather than
an assumed host facility. Hardware identity data required to interpret display
metadata is likewise its own package authority. The display-info provider is
version 0.3.0 because Weston 16 declares the
compatible range `>=0.2,<0.4`; the newer BLFS 0.4 provider belongs to a later
Mesa-oriented profile and is not forced across that public boundary. Each
row must become an immutable source envelope, isolated build, admitted object,
and reversible projection. The realization is valid only when:

1. every manifest row has exactly one admitted object;
2. builds run from captured sources without network access;
3. rollback and re-forge reproduce the same object identity;
4. a cold boot exposes the declared Wayland libraries and Weston;
5. the VM reaches a visible Weston desktop through its VirtIO display;
6. input is proven through the VM keyboard and pointer path; and
7. the sealed qcow2 passes `qemu-img check` and has a recorded SHA-256.

## Canon and mutation policy

The dependency and build interpretation begins with the captured BLFS 13.1
systemd book dated 2026-08-30. Current upstream releases are accepted only from
their canonical release locations and are digest-locked before execution.
Build-option changes may remove unclaimed providers, documentation, examples,
or tests from the runtime projection; they may not silently change an upstream
interface. Any required source mutation is a separate, named FlowLFS patch in
the source envelope with its own digest and rationale.

## Session contract

The system remains canonically bootable to a text login. The additive command
`flow-session wayland` starts the graphical projection for the current local
seat and creates `XDG_RUNTIME_DIR` through the system's session provider. SSH
is a control and evidence channel, not an implied owner of the graphical seat.
The default configuration lives under `/etc`; user overrides follow canonical
Weston/XDG locations and are never written without owner action.
