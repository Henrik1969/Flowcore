# Flow shell defaults 0.1.0 evidence

Date: 2026-08-30

## Result

The owner-authored package was deterministically archived, SHA-256 verified,
syntax checked as `flowbuilder`, atomically admitted, projected, rolled back,
and reprojected on the writable twin.

Source archive SHA-256:

```text
76383d4759c348337afa1d87f1393231b6b5b6f2177d9c0ab2a7b7b48ce2c497
```

Store object:

```text
/flow/store/objects/sha256-2c940496de50227f5367fc982ed23eb581a12ed8db4ca4b7877e400f1e4d7da3
```

## Acceptance evidence

- Bash and Zsh source syntax: pass;
- source archive path safety and digest: pass;
- build ran as `flowbuilder`: pass;
- atomic store admission: pass after repairing the first incomplete attempt;
- global Bash login and interactive policy: pass;
- global Zsh environment, completion, history, aliases, and prompt: pass;
- package rollback and reprojection: pass;
- two clean builds resolved to the identical store object: pass;
- locked disposable `flowguest` account created with UID/GID 1000: pass;
- Bash login environment as `flowguest`: pass;
- interactive Bash defaults as `flowguest`: pass;
- interactive Zsh defaults and completion cache as `flowguest`: pass;
- user state was owned by `flowguest`: pass;
- `/etc/skel` files were materialized as regular mode-0644 templates: pass;
- new home adapters were regular, user-owned, writable mode-0644 files: pass;
- disposable test account and home removed after verification: pass;
- root login shell remained `/bin/bash`: pass;
- `sshd.service` remained active: pass;
- VM intentionally left running at the owner's request.

## Lessons

1. An executable file is not an executable capability when an ancestor store
   directory denies traversal. The prior Zsh object root was mode 0550, so
   root-only testing concealed failure for ordinary users.
2. Zsh was rebuilt into user-accessible object
   `sha256-35919dca...cd9b73`, whose object root is mode 0555 after sealing.
3. Admission must publish atomically. The first shell-default attempt moved
   its root and wrote a manifest before missing build evidence stopped it.
   That incomplete directory was preserved under `/flow/store/rejected` and
   the corrected admission now assembles a hidden incoming object before one
   final rename.
4. `/etc/skel` is a template boundary, not an ordinary symlink projection.
   Store symlinks copied into user homes remain coupled to object retention.
5. Materialization must restore the declared mutable mode. Blindly preserving
   store mode produced user-owned but read-only startup files.
6. Account lifecycle, home state, global shell policy, and package identity are
   separate capabilities and authorities.

See `object-manifest.tsv` and `object-derivation.txt` for the sealed object
facts.
