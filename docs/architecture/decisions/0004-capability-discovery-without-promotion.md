# ADR-0004: Capability discovery without automatic promotion

**Status:** accepted direction, provisional implementation
**Date:** 2026-08-20
**Scope:** Frankencore capability vocabulary and discovery

## Decision

Frankencore separates capability discovery from capability authority.

```text
project metadata
    authoritative local meaning and policy

capability notice
    informative advertisement that a reusable candidate exists

system contract
    promoted meaning with explicit shared authority
```

A project may advertise a capability or status that could be useful outside
its current scope. That advertisement must not promote the capability or
pollute a global vocabulary. Independent consumers may discover the candidate
and evaluate whether a shared contract should later be designed.

## Non-promotion law

Discovery must not change authority. A notice is not a registry entry, does
not redefine another project's status, and does not authorize consumers to
use the capability. Promotion requires an explicit architectural decision at
the broader scope.

## Notice requirements

A capability notice should identify:

- source project and owner;
- local capability/status identity;
- scope and intended consumers;
- contract and version, if any;
- provider and policy references;
- whether the notice is informative only;
- evidence location for inspection.

Consumers may use notices to avoid reinventing work, compare designs, and
propose promotion. They must still validate the local contract before use.

## Catalog projection

Notices are projected into a visible, generated, project-local catalog:

```text
frankencore/catalog/capabilities.jsonl
```

The catalog is optimized for browsing and search. It is explicitly
`informative_only`, may contain current, superseded, withdrawn, and suggested
entries, and must link each entry to its source project, provider, contract,
and evidence. It is not itself an authority or a replacement for project
metadata or provenance history.

The catalog may later be indexed across projects for discovery, but an index
does not promote its entries into universal contracts.

## Catalog lifecycle and levels

There are two related catalog levels:

```text
project catalog
    born with the project and stored as a project artifact

user-local discovery index
    derived by the toolchain from the projects known to that user
```

Before a project exists, no project catalog is required. The first Flowcore
project creates its catalog naturally as part of the toolchain cycle. Later
projects contribute additional entries to the user's local discovery index.

Project lifecycle changes update discovery state:

```text
created       discoverable
deprecated    discoverable but non-current
removed       absent from the current index
```

The user-local index is a convenience and may be rebuilt from project
artifacts. It is not an authority, must follow XDG data-location policy, and
must not become a hidden replacement for project-local metadata or history.
Historical deprecation/removal facts remain in project provenance; the
current catalog projection may remove entries that are no longer current.

## Materialized-view lifecycle

The user-local catalog is a materialized discovery view over projects and
their artifacts. Project acquisition is itself an ingestion event: cloning,
importing, restoring, or otherwise obtaining a project makes its catalog and
capability notices eligible for indexing.

State-changing operations naturally update or invalidate the view:

```text
acquire / clone / restore   ingest project metadata and notices
change / transmute          refresh affected project entries
deprecate                   mark entries non-current
remove                      remove from the current view and retain history
rebuild                     explicitly reconstruct from available projects
```

The toolchain should update the view as part of these operations and expose an
explicit rebuild operation for repair, migration, policy changes, or user
request. Rebuild must be deterministic from the available project artifacts,
must diagnose missing or malformed inputs, and must not silently invent
capabilities.

The current view may omit removed projects, but lifecycle history remains
available through project provenance and catalog-maintenance diagnostics while
the relevant project artifacts are available.

## User-local inventory database

The toolchain maintains a user-local database recording the projects known to
the user and the materialized capability view derived from them. The first
backend is SQLite, stored according to XDG data policy:

```text
$XDG_DATA_HOME/frankencore/catalog.db
```

If `XDG_DATA_HOME` is unset, the implementation follows the XDG fallback
location. The database records project identity, acquisition/source metadata,
current lifecycle state, known artifact locations, and indexed capability
entries. It is a searchable inventory, not a universal authority.

Project files, project metadata, and project provenance remain the sources of
truth. The database may be discarded and rebuilt. Rebuild consults the
registered projects and their available artifacts, reports missing or
malformed inputs, and never manufactures capability entries.

Database updates should be transactional with catalog refreshes. A failed
refresh must leave the previous valid index available and expose the failure
for diagnosis rather than publishing a partial view.

## Registry boundary

The database is a municipal register, not a second project archive. It records
enough official information to identify, locate, and summarize a project and
its discoverable capabilities, while authoritative details remain in project
artifacts and provenance.

The minimum initial model has three logical tables:

```text
projects       identity, owner, source, location, lifecycle, revision, digest
artifacts      project reference, kind, location, revision, digest, availability
capabilities   project reference, meaning, scope, contract, provider/policy,
               state, evidence reference
```

The registry must not copy source files, complete project metadata, full
provenance history, generated binaries, or credentials. A later `scan_runs`
record may explain when and how a view was derived without duplicating the
project's provenance.

## Project identity and registration

Like a person having a permanent identifying number, a project carries its
own canonical `project_id` in project metadata. The ID is generated by the
Frankencore identity facility and travels with the project across acquisition,
cloning, backup, and restoration.

The user-local database does not assign or replace that identity. It records a
local registration/observation of the project, including where it was found
and when it was last inspected. The same project acquired twice therefore
remains one project with multiple observations or locations.

Lineage is explicit:

```text
clone / restore       preserve project_id
fork                  new project_id, parent reference
transmutation         new project_id, source and transformation reference
independent project   new project_id
```

The identity is a neutral ULID, not a personal or machine-sensitive value.

## Rationale

Scope isolation prevents accidental global coupling, while informative
discovery prevents needless reinvention. The model preserves both properties
without turning every useful local idea into a universal dependency.
