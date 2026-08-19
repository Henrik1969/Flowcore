# Flowkernel

Flowkernel is the first isolated Linux-kernel boundary probe brick. Version
0.1 deliberately covers only:

- read-only `getpid`, `clock_gettime`, `uname`, and `getrandom` probes;
- a private `/tmp` directory transaction using `openat`, `write`, `read`,
  `lseek`, `unlinkat`, and `rmdir`.
- child-process IPC through `pipe2`, `fork`, `read`, `write`, and `waitpid`;
- local Unix-socket IPC through `socketpair`, `send`, `recv`, and `waitpid`.
- local-only TCP loopback through `bind`, `listen`, `poll`, `accept4`, and
  `connect` on an ephemeral `127.0.0.1` port.

It performs no privileged operations, device access, mounts, external networking,
namespace creation, cgroup changes, reboot, or host shutdown operations.

Every report states its format, version, probe, effects, individual results,
and status. The temporary filesystem probe cleans up its own directory and
file before returning.
