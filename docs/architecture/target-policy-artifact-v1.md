# Target-policy artifact v1

`flowcore.target_policy` version 1 is the independent decision boundary between
a requested build target name and backend lowering. It is resolved before a
backend-neutral lowering artifact is prepared. The resolver does not inspect
source code, invoke a lowerer, discover a host backend, or choose a fallback.

Each policy records:

- backend and executable-artifact version;
- architecture word size and byte-order model;
- ABI identity and revision;
- required and admitted capabilities;
- slot and execution-step resource ceilings;
- startup and cleanup lifecycle laws;
- the contract/revision supplying compatibility evidence;
- an explicit fallback mode and target.

The initial `llvm-host` and `tinyvm-portable` policies both set fallback to
`none`. A missing file, malformed policy, path-like name, identity mismatch, or
unsupported backend is a hard structured diagnostic. Future explicit fallback
is policy data; it may never be inferred from local tool availability.

The public `flowtarget` resolver accepts a fixed policy root and one target
name, validates the named file through Flowcontracts, confirms that its internal
identity equals the requested name, and writes canonical JSON. Thus a build
driver can keep every input and tool fixed while changing only the target name.

`flowprepare --target-policy FILE` now captures a resolved policy into
`flowcore.backend_lowering_artifact` version 2. Version 1 remains readable for
historical captured replay; version 2 requires the complete policy. The source
program's named projection remains in `target`, while machine/backend selection
is exclusively `target_policy`, avoiding two meanings for the same field.

LLVM and TinyVM independently validate the captured lowering artifact and admit
only their exact backend, executable format, architecture, ABI and required
provider capability set. An incompatible policy produces an `unsupported`
result with exit 2 before an output file is created. Neither consumer invokes
the resolver or reads the source/optimization producer's files.
