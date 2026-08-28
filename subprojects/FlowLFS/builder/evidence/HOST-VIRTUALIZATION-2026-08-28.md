# Host virtualization evidence — 2026-08-28

- CPU advertises VMX.
- `kvm` and `kvm_intel` kernel modules were already loaded.
- User `henrik` was already a member of group `kvm`.
- Sysfs exposed the KVM misc device as major/minor `10:232`.
- The installed udev rule requires `/dev/kvm` as `root:kvm`, mode `0660`.
- `/dev/kvm` was initially absent; `udevadm trigger --action=add
  /sys/class/misc/kvm` restored that dynamic device node.
- A two-second QEMU `-accel kvm` probe with no disks initialized successfully
  and remained running until the test timeout terminated it.

This remedy changed runtime device state only. It installed no package and
changed no host user, group, symlink, persistent rule, or system configuration.
