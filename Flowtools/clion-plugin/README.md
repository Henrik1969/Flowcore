# Flowcore CLion plugin

This is the first CLion projection for Flowcore. It registers `.flow` files as
Flowcore source files and leaves project orchestration to the repository's
CMake/Ninja integration.

The plugin intentionally does not implement a second parser or semantic model.
Diagnostics and navigation will be added as external Flowanalyst consumers in
the next plugin slice.

## Build

The repository includes a Gradle 9.6.1 wrapper, so no system Gradle
installation is required. From this directory:

```sh
./gradlew clean buildPlugin
```

Install the generated ZIP from `build/distributions/` in CLion's plugin settings.

## Open the CMake project

Open this repository's:

```text
Flowtools/examples/flowcat-clion/CMakeLists.txt
```

Select Ninja as the CMake generator. The available targets include:

- `flowcore_flowcat_analyze`;
- `flowcore_flowcat_lower`;
- `flowcat_run`.

Override the `FLOWCORE_*` cache variables when the stage executables live in a
different build directory.
