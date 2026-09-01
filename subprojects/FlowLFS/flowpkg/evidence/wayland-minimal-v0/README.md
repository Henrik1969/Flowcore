# Minimal Wayland v0 evidence

The runnable artifact is
`subprojects/FlowLFS/artifacts/FlowLFS-v0.1-wayland-minimal-v0.qcow2`.
Its SHA-256 is
`e308a8f9dcfe19e1d0afaea097ded6aa1fb6c782fc68d5bc04a513f6febc9637`.

`wayland-minimal-v0-host-evidence.tar.xz` contains the reviewed source and
build manifests, profile realization declaration, image checksum, factory
build serial log, and final graphical-launch serial log. Package-local setup,
build, test and install logs remain in `/flow/evidence` inside the image, and
the exact active object identities remain at
`/flow/store/profile-realizations/wayland-minimal-v0/active.tsv`.

The final independent cold-boot checks reported:

```
FLOW_WAYLAND_DEVICE_PASS drm=/sys/bus/pci/drivers/virtio-pci input=5 compositor=drm-pixman
FLOWFACTORY_WAYLAND_VERIFY_PASS image=subprojects/FlowLFS/artifacts/FlowLFS-v0.1-wayland-minimal-v0.qcow2
FLOWLFS_WAYLAND_READY ssh=root@127.0.0.1:2300
```

The standalone launcher uses a temporary QEMU snapshot, discovers the
canonical `wayland-*` socket selected by Weston, and leaves the sealed image
unchanged.
