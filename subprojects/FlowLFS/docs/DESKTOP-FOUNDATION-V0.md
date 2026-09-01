# FlowLFS desktop foundation v0

## Outcome

This profile descends from, but never modifies, the sealed minimal Wayland v0
artifact.  It adds the smallest graphical application substrate that makes the
existing FlowLFS command environment directly usable from the local display:
a font system, canonical font data, and a native Wayland terminal.

## Capability boundaries

- `terminal.graphical` is a capability. Foot 1.27.0 is its initial provider.
- Weston remains the compositor provider and retains its canonical desktop
  shell and launcher configuration interfaces.
- Fontconfig retains canonical `/etc/fonts` and per-owner configuration.
- No application owns compositor, shell, package-store, or owner state.
- Provider defaults live under `/etc`; `/etc/skel` receives only additive,
  editable XDG defaults when a real user session is introduced.

The first slice deliberately disables optional shaping, IME, SVG glyphs, and
UTMP integration. Those are named later capability layers, not hidden host
dependencies. UTF-8 terminal operation and the canonical upstream interfaces
remain present.

## Production gates

1. Sources are captured from the canonical release authorities and digest
   locked before entering the VM.
2. Every build runs offline as `flowbuilder` and produces an immutable object.
3. The sealed `wayland-minimal-v0` image remains byte-for-byte unchanged.
4. A new qcow2 must cold boot, start Weston, enumerate its font, open Foot from
   the desktop launcher, and accept keyboard and pointer input.
5. The terminal provider must be replaceable without changing application or
   compositor contracts.
