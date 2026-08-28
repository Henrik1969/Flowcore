# Flowtools

Flowtools contains projections of the Flowcore capability chain for developer
tools. It does not parse or compile Flowcore independently.

## CLion/CMake/Ninja project

Open this file in CLion:

```text
Flowtools/examples/flowcat-clion/CMakeLists.txt
```

Select Ninja as the CMake generator. The project exposes:

- `flowcore_flowcat_analyze` — FlowMini, Flowanalyst, Flowbind, and
  Flowoptimize reports;
- `flowcore_flowcat_lower` — authorized LLVM lowering and native ELF output;
- `flowcat_run` — runs the generated application against sample files.
- `flowcore_architecture_check` — checks dependency boundaries and contract
  inventory;
- `flowcore_conformance` — runs the first executable constitutional laws.
- `Frankencore::Provenance` — reusable C++20 mutation-provenance API; its JSON
  output is an inspectable projection.

The CMake cache variables `FLOWCORE_FLOWMINI`, `FLOWCORE_FLOWANALYST`,
`FLOWCORE_FLOWBIND`, `FLOWCORE_FLOWOPTIMIZE`, and `FLOWCORE_FLOWLOWER` point to
the stage executables. CLion can override them without changing source files.

## Plugin

`clion-plugin/` contains the first IntelliJ Platform projection. It registers
`.flow` files as Flowcore files and deliberately leaves semantic truth to
Flowanalyst. Build it with Gradle using the instructions in its README.
