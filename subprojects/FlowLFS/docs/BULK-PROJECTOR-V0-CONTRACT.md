# Bulk projector v0 contract

## Scope

The v0 engine realizes an immutable object's ordinary directory and symlink
projection. It does not own package admission, profile selection, copied
templates, merged configuration, accounts, service state, or post-hooks such
as `ldconfig`. Package adapters retain those responsibilities explicitly.

Inputs are the exact object directory, a target root, and a projection-state
path. Production uses target root `/`; isolated tests use a temporary root.
Every input path is normalized and absolute. Projection transactions record
the exact object identity and cannot be replayed against another object.

## Project transaction

1. Traverse `OBJECT/root` in byte-sorted name order.
2. Build the complete directory and non-directory plan in memory.
3. Preflight every destination before filesystem mutation.
4. Refuse an active or interrupted transaction and every leaf collision.
5. Write and synchronize `links.list`, `dirs.list`, `object`, and `state` under
   `PROJECTION.incoming`.
6. Create only absent directories, then absolute links to immutable object
   entries, reporting bounded progress.
7. Mark the state active and atomically rename the transaction to `PROJECTION`.

The plan exists before mutation, so recovery does not depend on the process
having recorded each completed operation after the fact.

## Rollback transaction

Rollback first verifies the projection's object identity and **every** owned
link target. Any missing, replaced, or retargeted link refuses the complete
rollback before removal begins. It then removes links, attempts newly created
directories in reverse plan order, and atomically archives the projection
record at the caller-supplied path. Non-empty directories survive because they
contain state outside this projection's authority.

## Interrupted construction

`recover` reads the prepared incoming plan. Planned links are removed only when
their current target exactly matches the transaction's object entry. Absent
links are harmless; changed paths fail closed. Planned new directories are
removed in reverse order when empty. The prepared transaction is retained as
an interruption archive.

## Compatibility and acceptance

The native engine must produce the same destination-to-object link mapping and
the same empty target after rollback as the current shell reference projector.
Acceptance also requires:

- collision refusal before partial projection;
- changed-link refusal before partial rollback;
- exact-object transaction binding;
- forced-interruption recovery;
- deterministic plans and bounded progress counters;
- large-object rollback/reprojection benchmarking in the FlowLFS VM;
- survival of package and profile verification; and
- cold boot of the exact sealed VM bytes.

