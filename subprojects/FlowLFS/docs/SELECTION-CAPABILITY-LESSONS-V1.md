# Selection capability lessons v1

This checkpoint records what the basis-v2 terminal work and the two
Flowselection profiles taught us. It is guidance for later FlowLFS capability
work, not new authority over the sealed artifacts or their evidence.

## What the experiment proved

The selection capability can retain one semantic contract while changing its
presentation provider. Both native and fzf consume an ordered candidate set,
allow one choice or cancellation, and return the selected item through the
same CLI result contract. Provider installation does not select a provider;
the immutable policy and the owner's mutable decision remain distinct.

Terminal brands are not functional dependencies. Flowterminal normalized the
keys without consulting `$TERM` or terminfo, and the native selector worked
with an invented terminal name. fzf remains free to provide a richer terminal
projection because it is an explicitly selected provider rather than hidden
semantic authority.

The v0 provider was semantically correct but operationally incomplete: it
reported cursor state without presenting the candidate set. This was not a
key-input failure. It was a presentation-contract failure. The immutable v1
correction made the complete state visible without changing selection meaning.

## Permanent architectural lessons

1. **Name the semantic capability before choosing its interface.**
   Selection means ordered candidates, current choice, navigation, acceptance,
   cancellation, and a normalized result. A menu, fuzzy finder, GUI, speech
   interface, or remote service is a projection of that meaning.

2. **A projection must expose enough state to be usable.**
   Correct internal transitions are insufficient if a user cannot see the
   candidate universe and current selection. Observable presentation belongs
   in provider acceptance criteria.

3. **Keep semantic output separate from interaction.**
   Interactive frames go to `/dev/tty`; standard output contains only the
   selected value. Consequently `choice=$(flowselect ...)` remains stable even
   when presentations become richer.

4. **The least-assuming provider should be the recovery baseline.**
   Native uses ordinary text, normalized key events, and append-only state
   frames. It does not require cursor addressing, alternate screens, terminal
   databases, or a known terminal name. Rich providers may opt into those
   capabilities explicitly.

5. **Do not push projection requirements into semantic layers.**
   Kitty, Alacritty, xterm, ncurses, and ECMA-48 are presentation mechanisms.
   The selection state machine must neither identify nor special-case them.

6. **Provider presence grants no authority.**
   Installing fzf did not change the default. Owner selection is recorded in
   separate mutable state, fallback is separately authorized, and unknown
   providers fail closed.

7. **Constrain adapters at their authority boundary.**
   The fzf adapter clears ambient command/options state and withholds preview,
   execution, reload, transform, history, walker, and shell authority. Input
   candidates are data, never shell source.

8. **Test the actual interaction boundary.**
   Pipe-only tests cannot prove terminal behavior. The durable gate must use a
   pseudo-terminal, send real key sequences, inspect visible state, and verify
   the normalized result. The promoted image must repeat that test after a
   cold boot.

9. **Corrections are new immutable objects, not edits to history.**
   The inadequate v0 image, corrected v1 package, exact profile locks, evidence
   archive, Git commits, and tags form a lineage. The earlier image remains
   valuable evidence of what was learned.

10. **Snapshot inspection and persistent construction are different modes.**
    Public launchers use `-snapshot` to protect sealed images. Construction
    must begin from a verified clone, make only the twin writable, shut it down
    cleanly, seal it read-only, hash it, and cold-boot those exact bytes.

## Package-store performance lesson

The package model behaved correctly through full rollback and offline
reprojection, but the implementation processed Go's 15,027 links through
shell loops and one filesystem operation at a time. Under TCG, rollback took
roughly ten minutes and reprojection several more. Progress was real and
measurable, but the mechanism scales with shell/process overhead rather than
only filesystem work.

Later package-store work should preserve the current manifests and collision
rules while replacing per-entry shell execution with a bounded bulk projector:

- parse and validate the complete manifest before mutation;
- reject all collisions before creating any link;
- apply links and directories in one native process or equivalent batch;
- retain a transaction journal and exact reverse-order rollback;
- emit progress counters for large objects;
- make interruption recovery explicit and idempotent;
- differential-test the new projector against the current shell semantics;
- measure rollback and reprojection separately; and
- never optimize by weakening object immutability or ownership checks.

The observed delay is therefore a tooling-performance defect, not evidence
against the immutable object/projection model.

## Gates for the next capability provider

Before another provider enters a FlowLFS profile, require:

1. a provider-independent semantic contract and normalized exit/result model;
2. explicit data, control, filesystem, process, network, and shell authority;
3. immutable default policy separated from owner-local selection;
4. unavailable and unauthorized-provider failure tests;
5. a least-assuming recovery provider;
6. provider equivalence tests over the same candidate/input corpus;
7. a real interaction-boundary test where interaction exists;
8. complete rollback and offline reprojection;
9. profile-transition lineage from the prior sealed object;
10. a runnable, read-only VM artifact tested after cold boot.

## Sensible next work

The next presentation improvement should not put escape sequences into the
selection state machine. If in-place native redraw is desired, define a small
semantic frame contract and add an explicitly capable ECMA-48 projection while
retaining the append-only plain provider as fallback.

Before profiles become much larger, build and prove the bulk package projector.
The current implementation is trustworthy enough to serve as its differential
oracle, but too slow to remain the long-term realization engine.

