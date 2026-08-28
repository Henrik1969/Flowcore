# ADR-0031: Practical assurance boundary

**Status:** accepted constitutional direction
**Date:** 2026-08-20
**Scope:** Frankencore security profiles and distribution usability

## Decision

Frankencore must remain usable within sane, obtainable conditions. The
baseline system may not require secret hardware, inaccessible institutions,
unobtainable components, private networks, or an elaborate trust ceremony
that ordinary users cannot reproduce.

```text
baseline assurance
    ordinary supported hardware, obtainable software, explicit local policy,
    inspectable provenance, and practical recovery

enhanced assurance
    optional hardware-backed keys, independent audit, measured boot, remote
    attestation, or stronger physical separation
```

Enhanced mechanisms improve assurance where available, but they are explicit
profile choices and never hidden prerequisites for the ordinary distribution.

## Capability honesty

The system must state:

- which assurance it can provide on the current hardware;
- which mechanisms are available or absent;
- which claims are self-reported or independently verified;
- which risks remain accepted;
- what stronger options would improve the profile.

It must never imply hardware-backed or independent assurance that is not
actually present.

## Trust boundary

Every security model ultimately depends on some trust anchor. The practical
goal is not impossible absolute trust; it is bounded, inspectable,
revocable, and proportionate trust using mechanisms users can obtain and
understand.

If an enhanced anchor is unavailable, the system downgrades the assurance
profile, chooses isolation or confirmation where possible, and explains the
trade-off. It does not invent a secret dependency or make the user discover a
fictional supply chain to operate the system.

## Revisit triggers

Revisit when defining distribution security profiles, hardware-backed key
support, measured boot integration, remote attestation, or high-assurance
deployment guidance.
