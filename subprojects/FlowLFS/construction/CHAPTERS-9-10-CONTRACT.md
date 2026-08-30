# Chapters 9–10 contract: configured bootable control

Authority is LFS r13.0-201-systemd, Chapters 9 and 10. Interactive choices in
the book are resolved here as explicit target policy rather than inferred from
the builder host.

## Declared target policy

- Identity: `flowlfs-control`
- Locale: `en_US.UTF-8`
- Timezone: `Europe/Copenhagen`; hardware clock stored as UTC
- Console: Danish keymap, Lat2-Terminus16 font, serial console enabled
- Network: QEMU Virtio interface `enp0s2`, IPv4 DHCP through systemd-networkd
- Root filesystem: ext4 label `FLOWLFS_ROOT`, single MBR partition; kernel root
  selected by that partition's stable MBR PARTUUID
- Firmware/boot: legacy BIOS, GRUB i386-pc installed to the target disk
- Kernel: Linux 7.1.8, x86_64 defconfig plus built-in Virtio block/PCI/network,
  ext4 root, devtmpfs, and 8250 serial-console capabilities

## Safety boundary

The installation script must run only inside the target chroot while the
builder exposes the target as `/dev/vdb`. It refuses any other root label or
disk identity. GRUB locates its files by filesystem label. The kernel command
line uses the partition's stable PARTUUID so the standalone artifact may boot
as `/dev/vda1` without embedding the builder device name or requiring an
initramfs.

## Exit evidence

- Configuration files contain the declared values.
- Kernel, System.map, and kernel configuration exist in `/boot`.
- GRUB's i386-pc core is installed and `grub.cfg` names both the filesystem
  label and the verified root PARTUUID.
- The target image is shut down, sealed, flattened, and booted without the
  builder disk before the baseline is certified.
