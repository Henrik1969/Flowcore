# Installed shape and Debian packaging boundary

Status: implementation started; packaging metadata not yet frozen.

The repository is a development workspace. Its source-tree build directories,
test providers, generated reports, and historical stages are not an installed
runtime contract. The canonical installation is the CMake staged install. A
Debian source package will consume that install and split it into ordinary
runtime, development, and documentation packages.

## Runtime payload

The runtime package installs the executable capability stages in `/usr/bin`:

```text
flowmini
flowanalyst
flowbind
flowoptimize
flowlower
flowkernel
flowparallel
franken_ls
```

These remain composable Unix tools. Their command-line contracts and policy
files are not replaced by Debian packaging.

## Development payload

The development package owns headers and static implementation libraries:

```text
/usr/include/flowcore/flowmini/...
/usr/include/flowcore/flowparallel/...
/usr/include/flowcore/frankencore/...
/usr/lib/<triplet>/flowcore/lib*.a
```

The namespace prevents Flowcore headers and libraries from colliding with the
host substrate or with future ABI generations. The first package must not
pretend that the current static libraries are a stable shared ABI.

## Data, policies, and bindings

Versioned language capability declarations are data, not C/C++ headers:

```text
/usr/share/flowcore/std/...
/usr/share/flowcore/examples/flowcat/policy.conf
```

The example policy is documentation/test data. It is not installed as an
active administrator policy. Future global policy defaults belong under
`/etc/flowcore/` and must be introduced through ConfigResolver semantics with
explicit precedence and provenance.

Displayer and provider shared objects, when their ABI is mature, belong under
the multiarch library directory in a Flowcore-owned subdirectory. No `.so`
plugin directory is claimed by this first install pass because the provider
contracts are not yet frozen as a stable binary ABI.

## Documentation

The first staged payload places stable user-facing documentation under:

```text
/usr/share/doc/flowcore/
```

Man pages will be installed under the normal `/usr/share/man/man<section>`
locations once the individual CLI contracts are frozen. Markdown remains the
source/documentation artifact; man pages are a projection of it, not a second
canonical specification.

## Debian split

The intended initial split is:

| Package | Contents |
|---|---|
| `flowcore` | runtime binaries and runtime data |
| `flowcore-dev` | headers, static libraries, CMake package metadata later |
| `flowcore-doc` | manuals, architecture, language, and provenance documentation |

CUDA provider support remains optional. The base package must install and work
with the CPU path when CUDA development libraries are absent; CUDA-specific
development dependencies must not become accidental dependencies of the basic
runtime package.

## Staging and verification

```sh
cmake -S . -B build/package -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/package
DESTDIR="$PWD/build/stage" cmake --install build/package
find build/stage/usr -type f -o -type l | sort
```

The packaging gate must verify that the staged tree contains no build-tree
paths, source-only test provider, generated artifacts, or undeclared runtime
dependency. Debian metadata and `dpkg-buildpackage` are the next layer; they
must consume this shape rather than invent a second layout.
