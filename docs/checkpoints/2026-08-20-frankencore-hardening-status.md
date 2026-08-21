# Frankencore hardening checkpoint — 2026-08-20

## Scope

This checkpoint covers the first contract, substrate-observation, policy-
adapter, language-map, requirement, and executable-facade slices.

## Results

- architecture check: PASS;
- constitutional conformance: PASS;
- mutation-provenance check: PASS;
- clock contract: PASS;
- default CTest suite: 14/14 PASS;
- ConfigResolve-enabled CTest suite: 15/15 PASS;
- JSON syntax validation: PASS;
- `git diff --check`: PASS;
- ASan/UBSan execution with leak detection disabled: PASS;
- Valgrind probes for new contract/language/requirements/facade code: PASS;
- native-vs-facade `ls` differential test: PASS.

## Known test-environment limitation

LeakSanitizer cannot run in the managed test harness because the environment
uses ptrace mediation. It reports its documented fatal limitation rather than
a program leak. Leak coverage was therefore performed through Valgrind, while
AddressSanitizer and UBSan remained enabled for the executable test pass.

## Hardening changes

- native APT delegation uses a fixed default backend and shell-quotes any
  configured executable path;
- APT provider failures and malformed rows remain explicit diagnostics;
- package and APT observations are deterministically ordered;
- contract status vocabularies reject unknown values;
- localized moniker collisions remain unresolved rather than guessed;
- unsupported version-range syntax is rejected;
- ordinary `ls` facade calls delegate without changing behavior;
- unresolved policy/schema facade extensions fail closed.

## Remaining hardening gates

- full JSON-schema validation with a selected validator dependency;
- signed map/policy/release verification;
- source-map-preserving localized parser;
- full chain-policy resolution against observed capabilities;
- facade signal, terminal, locale, and machine-output matrix expansion;
- package mutation dry-run and recovery tests;
- fuzzing malformed manifests, sources, maps, and policy expressions.
