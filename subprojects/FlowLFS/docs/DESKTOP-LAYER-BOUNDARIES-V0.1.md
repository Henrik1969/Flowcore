# FlowLFS desktop layer boundaries v0.1

The desktop is a composition of authorities, not a monolithic environment.

| Order | Layer | Owns | Must not own |
|---|---|---|---|
| 00 | Kernel and hardware | VirtIO, DRM/KMS, evdev devices | locale, key meaning, UI policy |
| 10 | Canonical data | tzdata, glibc locales, XKB symbols | active user policy or application behavior |
| 20 | Functional | libinput normalization, libxkbcommon interpretation, Wayland contracts, Flow capability commands | terminal brand or compositor appearance |
| 30 | Projection | Weston surfaces and Foot terminal rendering | application semantics or owner state |
| 40 | Protection/policy | selected locale, timezone, keyboard map, provider choice, owner override precedence | rewriting canonical upstream interfaces |

The default policy mirrors the development machine: `da_DK.UTF-8`,
`Europe/Copenhagen`, and XKB `pc105/dk/winkeys`. Locale selection and keyboard
mapping remain separate controls. The system seed is visible under
`/usr/share/flowcore/defaults`; canonical mutable files under `/etc` remain
administrator-editable. Weston is merely the current projection of the XKB
selection. The same functional key symbols may be projected by another
compositor without changing applications.
