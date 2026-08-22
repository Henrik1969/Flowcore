# Flowlower

Flowlower is the target-lowering sibling after Flowoptimize:

```text
source -> Flowmini -> Flowanalyst -> Flowparallel -> Flowoptimize -> Flowlower
```

The first target provider is `llvm`. The implementation supports narrow
source-derived profiles, including the executable `flowcat_file_main`,
`abi_kernel_getgid_main`,
`abi_kernel_geteuid_main`,
`abi_kernel_getegid_main`,
`abi_kernel_getpgrp_main`,
`abi_kernel_getpgid_main`,
`abi_kernel_getsid_main`,
`abi_kernel_getpriority_main`,
`abi_kernel_clock_main`, and
`abi_kernel_random_main` profiles, and emits valid
LLVM modules for them. General source lowering remains a later expansion; every
emitted profile must be explicit, policy-authorized where it uses external
capabilities, and executable-tested. Profile-free lowering currently covers
`c_int` calls with zero or one integer argument, typed result placement,
integer values and expressions, returns, and boolean/comparison branches.

The lowering report preserves the source path carried by the upstream semantic
and optimization reports. This keeps emitted IR attributable to its source
artifact without making the lowerer depend on Flowmini internals.

Multiplexed reports are preserved through Flowparallel and Flowoptimize. When
multiple named targets exist, Flowlower requires explicit selection:

```sh
flowlower --target cli < optimization-report.json
```

An absent or unknown target is rejected. A report without named targets keeps
the compatibility default `main`. This establishes target selection as a
separate artifact boundary. For the currently accepted `empty_program_main`
profile, independent target artifact emission is proven with separate
`--emit-llvm` paths. Each output contains an attributable target marker and
the lowering report records `artifact.target_specific: true`. Unsupported
target profiles remain blocked.
