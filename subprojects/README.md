# Subprojects

This directory documents externally maintained or separately versioned components
used by the Flowcore superproject.

AstLiib currently lives beside this superproject:

```text
../AstLiib
```

The top-level CMake project includes it with `add_subdirectory` using the
`FLOWCORE_ASTLIIB_SOURCE_DIR` cache path. This keeps AstLiib's own public ABI,
tests, and implementation boundary intact while letting Flowcore build it as part
of the larger compiler chain.
