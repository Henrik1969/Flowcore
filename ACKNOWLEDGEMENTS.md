# Acknowledgements

Lyraform is developed with and alongside the following free software projects,
libraries, and tools:

- Git and GitHub for versioned collaboration and preserved project history.
- CMake, Ninja, CTest, GCC, Clang, and LLVM for the build, test, and native
  compiler toolchain.
- OpenSSL for the cryptographic provider used by selected artifact checks.
- ncurses for the terminal provider used by the `flow_less` acceptance path.
- Python and POSIX shell tooling used by reproducibility and adversarial test
  drivers.

The project also acknowledges the surrounding work recorded in the repository:

- FrankenCore provides the governance and constitutional architecture context.
- TinyVM remains an independent backend experiment and is not merged into the
  main Lyraform line.
- FlowLFS remains an independent experimental branch and project line.
- Flowmini is retained as the historical prototype and bootstrap lineage from
  which the current Lyraform compiler evolved.

This file records implementation influences and software dependencies. The
academic and intellectual reading list is maintained separately in
[REFERENCES.md](REFERENCES.md).
