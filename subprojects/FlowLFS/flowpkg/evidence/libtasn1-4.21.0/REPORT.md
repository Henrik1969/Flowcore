# libtasn1 4.21.0 source-forge evidence

Date: 2026-08-31  
Profile: `flowlfs.modern-cli.v0`  
Selection: owner opt-in  
Target: writable Flowcore twin only

## Result

The canonical libtasn1 4.21.0 source archive was verified, built and tested as
the unprivileged `flowbuilder`, installed only into a staging root, atomically
admitted, transactionally projected, rolled back, and reprojected.

Source SHA-256:

```text
1d8a444a223cc5464240777346e125de51d8e6abf0b8bac742ac84609167dc87
```

Active store object:

```text
/flow/store/objects/sha256-7f9517391aa6d7b2ce0459e9a5b6bed7f0d9a8cba9eb462b7afe6cd1470a500c
```

## Verification

- captured BLFS MD5 matched: pass;
- detached signature cryptographically valid: pass, signing subkey
  `A3CC9C870B9D310ABAD4CF2F51722B08FE4745A2`;
- archive path-safety scan: pass;
- upstream test suite: 31 passed, zero failures and zero skips;
- unprivileged build and staged installation: pass;
- atomic object admission: pass;
- shared library visible through the regenerated dynamic-linker cache: pass;
- command executable by the unprivileged build identity: pass;
- rollback removed commands and library-cache resolution: pass;
- offline reprojection: pass;
- two clean builds converged on the same object identity: pass;
- root recovery shell remained `/bin/bash`: pass;
- `sshd.service` remained active: pass.

Compiler analyzer warnings are retained in the build evidence inside the VM;
they did not cause upstream test failures. Signature validity is recorded
separately from identity trust: the captured key validates the bytes, but the
key-distribution route is not silently elevated to owner trust.

## New shared-state lesson

The projected library files are immutable object content. `/etc/ld.so.cache`
is not package content: it is a derived registry resolved from the active
filesystem projection. Projection and rollback must regenerate it, and
verification must test both presence and absence rather than merely checking
that package symlinks changed.

See `object-manifest.tsv`, `object-derivation.txt`, and `guest/test.log` for
the permanent facts.
