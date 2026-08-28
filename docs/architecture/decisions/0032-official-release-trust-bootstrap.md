# ADR-0032: Official release trust bootstrap

**Status:** accepted direction, implementation future
**Date:** 2026-08-20
**Scope:** Frankencore distribution and release acquisition

## Decision

The ordinary distribution establishes initial trust through an official,
project-controlled release site publishing hashes and RSA-signed manifests.

```text
official release site
    release manifest + artifact hashes + public signing key/reference

consumer
    obtain artifact and manifest
    verify hash
    verify RSA-PSS signature
    resolve key trust and release policy
    admit, isolate, or reject
```

HTTPS/TLS protects transport, but the release signature and independently
known publisher key provide the artifact trust anchor. RSA is used for signing
manifests and assertions; it is not treated as a generic key-exchange phrase.

## Release manifest

A release manifest should identify:

- project identity and release version;
- artifact names, kinds, sizes, and cryptographic hashes;
- supported platforms and substrate profile;
- signing key identity and algorithm parameters;
- policy/profile revision;
- release provenance and build evidence where available;
- expiry, supersession, or revocation state.

Consumers must verify both the artifact hash and the signed manifest. A hash
without an authenticated source is only an integrity check; a signature with
the wrong or untrusted key is not an authority claim.

## Key distribution and recovery

The official site may publish the public key, but key trust should be
reinforced through additional obtainable channels where practical, such as
the source repository, signed package metadata, release documentation, or a
previously trusted release. Key rotation must publish successor evidence.

Key compromise, revocation, expiry, and emergency replacement must produce
explicit diagnostics and a recovery path. A consumer must not silently accept
a replacement key merely because it appears on the same site.

## Established reference pattern

This decision follows the established distribution pattern used by Debian and
Linux Mint, while keeping the Frankencore contract provider-neutral.

Debian's APT chain signs repository metadata rather than relying on a
signature embedded in every downloaded package: package hashes are recorded
in package indexes, index hashes are recorded in a `Release` file, and the
archive signs that file. APT verifies the signature and then the hashes. Its
archive keys are distributed and updated through `debian-archive-keyring`,
and unsigned repositories are rejected by default. See the Debian
[`apt-secure(8)` documentation](https://manpages.debian.org/trixie/apt/apt-secure.8.en.html).

For installation media, Debian and Linux Mint use the same useful two-stage
shape: a project-signed checksum manifest authenticates the published
checksum list, and the checksum list authenticates the downloaded image.
Debian documents signed checksum files in [Verifying authenticity of Debian
images](https://www.debian.org/CD/verify). Linux Mint documents the
corresponding `sha256sum.txt` and detached-signature workflow in [Verify your
ISO image](https://linuxmint-installation-guide.readthedocs.io/en/latest/verify.html).

The lesson adopted here is deliberately limited: a valid signature proves
that an authorized signing key authenticated the manifest and that the
artifact matches its authenticated digest. It does not prove that the
publisher's build infrastructure was uncompromised, that the software is
bug-free, or that the release is safe for every operation. Those questions
remain provenance, policy, isolation, and admission decisions.

Accordingly, FrankenPOP should eventually provide two related workflows:

1. Release media and standalone artifacts use a signed manifest plus hashes.
2. Package repositories use signed repository metadata plus hashes of indexes
   and packages.

The same public-key trust store, key-transition rules, revocation handling,
and ConfigResolve arbitration should serve both workflows. The format may
differ for compatibility with an ecosystem, but the semantic contract must
remain the same.

## Substrate adaptation rule

Both release manifests and repository metadata are valid substrates. The
consumer must not impose one universal transport or packaging format on every
system. Instead, it discovers the substrate and selects the corresponding
adapter:

- Debian-family package installation uses the native APT repository metadata
  and keyring mechanisms.
- Release media and standalone artifacts use the signed-manifest and digest
  workflow.
- A future substrate may provide another native mechanism, provided its
  adapter can produce the same semantic results: authenticated origin,
  artifact integrity, key state, provenance, expiry/revocation state, and a
  clear admission outcome.

The adapter translates substrate evidence into the Frankencore trust and
provenance model. It must not silently downgrade a failed or unavailable
substrate verification into acceptance. When no compatible adapter exists,
the result is `unknown`, `quarantined`, or `rejected` according to resolved
policy.

## Practical boundary

This mechanism requires no secret component or special hardware. Enhanced
hardware-backed or independent trust anchors may strengthen the profile but
remain optional.

## Revisit triggers

Revisit when defining the manifest schema, release signing workflow, key
rotation, mirror trust, reproducible-build evidence, or package-manager
integration.
