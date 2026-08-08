# Flowcore / Flowmini Prerequisites

## Purpose

A `prerequisite` declares an external condition that must be satisfied before a
program, unit, ABI binding, test, runtime stage, or build profile can be used.

A prerequisite is not an import.

```text
import
    brings source declarations into scope

prerequisite
    declares environment/build/runtime requirements

The purpose is to replace hidden local assumptions with explicit contracts.
Motivation

A program should not fail with an unexplained low-level error such as:

dlopen failed for './build/libflowmini_testabi.so'

Instead, the program or build system should be able to report:

Cannot run abi_struct_demo.flow

Missing prerequisite:
    shared_library flowmini_testabi

Required by:
    std/abi/testabi.flow

Required version:
    >= 1.0.0
    <  2.0.0

Required symbols:
    point_sum
    point_weighted_sum

Hint:
    Build target flowmini_testabi or configure a provider for this prerequisite.

Conceptual Lifecycle

declare
    Source declares what external thing is needed.

resolve
    Build/runtime policy searches for matching providers.

verify
    Candidate provider is checked for kind, family, version, implementation,
    symbols, ABI, platform, permissions, or other constraints.

prepare
    Build/test tooling may fetch, build, copy, link, mount, or otherwise prepare
    the prerequisite.

execute
    Runtime starts only after required prerequisites are valid.

fail
    If a prerequisite cannot be satisfied, failure is structured and explanatory.

Initial Syntax Sketch

Simple form:

prerequisite shared_library "flowmini_testabi"
prerequisite command "ffmpeg" version >= "6.0"
prerequisite package "sqlite3" version >= "3.40"

Block form:

prerequisite shared_library testabi {
    family: "flowmini_testabi"
    version >= "1.0.0"
    version <  "2.0.0"
    implementation: "flowmini-testabi-c"
    symbols: [
        "point_sum",
        "point_weighted_sum"
    ]
}

Version Constraints

Initial supported comparison operators:

==
!=
>
>=
<
<=

Example:

version >= "1.0.0"
version <  "2.0.0"

Range sugar may be allowed later:

version in ">=1.0.0 <2.0.0"

but should lower internally to explicit comparisons.
Candidate Prerequisite Kinds

shared_library
static_library
command
file
directory
package
service
capability
abi
runtime
toolchain
compiler
platform
kernel_feature
device
network_endpoint
permission

Build-Time and Runtime Meaning

The same declaration can serve both build-time and runtime.

Build/test tooling may use prerequisites to prepare the environment:

build flowmini_testabi
copy or link the shared library
generate local config
prepare test data

Runtime may use prerequisites to verify readiness:

library exists
version is compatible
required symbols exist
permission/capability is available
ABI matches expectation

Current Flowmini ABI Example

Current hidden assumption:

std/abi/testabi.flow expects ./build/libflowmini_testabi.so

Future explicit model:

prerequisite shared_library testabi {
    family: "flowmini_testabi"
    version >= "1.0.0"
    version <  "2.0.0"
    symbols: [
        "point_sum",
        "point_weighted_sum"
    ]
}

Policy then resolves testabi to a concrete path such as:

cmake-build-debug/libflowmini_testabi.so

or a system-installed provider.
Rule

Hidden environmental assumptions should become explicit prerequisites.


Then commit it as an architecture note:

```bash
cd ~/Projekter/scratchpad/flow_Policy_envelope_pattern

mkdir -p docs/architecture

cat > docs/architecture/prerequisites.md <<'EOF'
# Flowcore / Flowmini Prerequisites

## Purpose

A `prerequisite` declares an external condition that must be satisfied before a
program, unit, ABI binding, test, runtime stage, or build profile can be used.

A prerequisite is not an import.

```text
import
    brings source declarations into scope

prerequisite
    declares environment/build/runtime requirements

The purpose is to replace hidden local assumptions with explicit contracts.
Motivation

A program should not fail with an unexplained low-level error such as:

dlpen failed for './build/libflowmini_testabi.so'

Instead, the program or build system should be able to report:

Cannot run abi_struct_demo.flow

Missing prerequisite:
    shared_library flowmini_testabi

Required by:
    std/abi/testabi.flow

Required version:
    >= 1.0.0
    <  2.0.0

Required symbols:
    point_sum
    point_weighted_sum

Hint:
    Build target flowmini_testabi or configure a provider for this prerequisite.

Conceptual Lifecycle

declare
    Source declares what external thing is needed.

resolve
    Build/runtime policy searches for matching providers.

verify
    Candidate provider is checked for kind, family, version, implementation,
    symbols, ABI, platform, permissions, or other constraints.

prepare
    Build/test tooling may fetch, build, copy, link, mount, or otherwise prepare
    the prerequisite.

execute
    Runtime starts only after required prerequisites are valid.

fail
    If a prerequisite cannot be satisfied, failure is structured and explanatory.

Initial Syntax Sketch

Simple form:

prerequisite shared_library "flowmini_testabi"
prerequisite command "ffmpeg" version >= "6.0"
prerequisite package "sqlite3" version >= "3.40"

Block form:

prerequisite shared_library testabi {
    family: "flowmini_testabi"
    version >= "1.0.0"
    version <  "2.0.0"
    implementation: "flowmini-testabi-c"
    symbols: [
        "point_sum",
        "point_weighted_sum"
    ]
}

Version Constraints

Initial supported comparison operators:

==
!=
>
>=
<
<=

Example:

version >= "1.0.0"
version <  "2.0.0"

Range sugar may be allowed later:

version in ">=1.0.0 <2.0.0"

but should lower internally to explicit comparisons.
Candidate Prerequisite Kinds

shared_library
static_library
command
file
directory
package
service
capability
abi
runtime
toolchain
compiler
platform
kernel_feature
device
network_endpoint
permission

Build-Time and Runtime Meaning

The same declaration can serve both build-time and runtime.

Build/test tooling may use prerequisites to prepare the environment:

build flowmini_testabi
copy or link the shared library
generate local config
prepare test data

Runtime may use prerequisites to verify readiness:

library exists
version is compatible
required symbols exist
permission/capability is available
ABI matches expectation

Current Flowmini ABI Example

Current hidden assumption:

std/abi/testabi.flow expects ./build/libflowmini_testabi.so

Future explicit model:

prerequisite shared_library testabi {
    family: "flowmini_testabi"
    version >= "1.0.0"
    version <  "2.0.0"
    symbols: [
        "point_sum",
        "point_weighted_sum"
    ]
}

Policy then resolves testabi to a concrete path such as:

cmake-build-debug/libflowmini_testabi.so

or a system-installed provider.
Rule

Hidden environmental assumptions should become explicit prerequisites.o
