# Flowkernel

Flowkernel is the first isolated Linux-kernel boundary probe brick. Version
0.1 deliberately covers only:

- read-only `getpid`, `clock_gettime`, `uname`, and `getrandom` probes;
- a private `/tmp` directory transaction using `openat`, `write`, `read`,
  `lseek`, `unlinkat`, and `rmdir`.

It performs no privileged operations, device access, mounts, networking,
namespace creation, cgroup changes, reboot, or host shutdown operations.

Every report states its format, version, probe, effects, individual results,
and status. The temporary filesystem probe cleans up its own directory and
file before returning.
