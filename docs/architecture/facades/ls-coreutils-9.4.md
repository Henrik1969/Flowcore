# `ls` facade compatibility profile: GNU coreutils 9.4

**Evidence date:** 2026-08-20  
**Observed executable:** `/usr/bin/ls`  
**Observed identity:** GNU coreutils `ls` 9.4, x86-64 ELF

This is an observed profile for the current Pop!_OS substrate. It is not a
universal promise for every implementation named `ls`.

The first compatibility prototype is built as
`Flowtools/facades/ls/franken_ls`. It uses the fixed `/usr/bin/ls` backend,
preserves ordinary calls through `execv`, and reports the explicit policy and
schema extensions as unresolved until their policy/record semantics exist.

## Compatibility floor

The facade must preserve the established GNU/POSIX call shape:

```text
ls [OPTION]... [FILE]...
```

The compatibility surface includes the standard listing, sorting, quoting,
color, recursive, timestamp, width, machine-output, help, and version
options exposed by this installed implementation. Important examples include
`-a`, `-l`, `-R`, `-x`, `-1`, `-C`, `-Q`, `--color`, `--sort`, `--time`,
`--zero`, `--help`, and `--version`.

The observed exit-status contract is:

- `0`: success;
- `1`: minor problems, such as an inaccessible subdirectory;
- `2`: serious trouble, such as an inaccessible command-line argument.

Output is context-sensitive. Terminal detection affects columns, quoting,
color, and control-character display. `LS_COLORS`, `QUOTING_STYLE`, locale,
terminal width, filesystem permissions, symlinks, and timestamps are part of
the observable behavior.

## Frankencore extension point

The legacy profile remains the default. An explicit extension may select a
policy and structured schema, for example:

```text
ls --Use_call_Policy=project-safe \
   --Use_call_Schema=frankencore.records.v1 \
   --zero
```

The extension may add provenance-rich records, policy-directed traversal,
structured sorting, or alternate projections. It must not reinterpret normal
`ls` calls silently.

## Required conformance evidence

Before installing a facade under the established name, compare native and
facade behavior for at least:

- empty, ordinary, hidden, and nested directories;
- files, directories, symlinks, broken symlinks, and inaccessible paths;
- terminal and non-terminal output;
- `-a`, `-l`, `-R`, `-x`, `-1`, `-C`, `-Q`, `--color`, `--zero`, and sorting;
- locale, width, quoting, and environment variations;
- exit status and signal interruption;
- machine-readable consumers and common shell scripts.

The native backend path and exact version used for comparison must be recorded
with the test result.
