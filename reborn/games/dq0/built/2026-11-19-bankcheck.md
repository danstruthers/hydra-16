# A build check for reads from another bank; one overlay count a task

As built, for review.  Branch bankcheck, 2026-11-19, from reborn-commands' 685f7bb4.

## What was built

- **Another bank's labels** (build.js's partsChecked, which already held each swapped part's code and data to its
  stubs).  It now also fails when an overlay's or a part's code or data names a label in another overlay's segment:
  a string literal, a table, a function or a global, read with the wrong bank in the window (e10's fight.c literals:
  cc65 puts a file's literals in the read-only data named at its end).  It reads each task's config for its overlays
  (OVn, and OVnT, the rest of OVn's bank).  The world's KIN and CAT banks aren't overlays: their callers put them in
  first (cat_map).  The main RAM's code may name an overlay's labels (bread, the stubs).
- **The sanctioned reads** are marked in the source, "bankcheck: banked NAME":
  - dq0's said_rows (overlay 28's, named by overlay 27's said.c): read only through RB, said_b putting in overlay 28's
    bank (29's, 30's) for each byte.  Confirmed: said.c's run and its row walk read the rows by RB alone.
  - dq0harn's bestiary (overlay 5's, named by overlay 2's fight.c): copied out by bread, its bank put in.
  Over every game nothing else is found.  Put back, e10's "your " literal is caught: "S0016 (OV2RODATA's, OV2) used
  from OV10CODE".
- **One overlay count a task** (build.js's overlaysCounted).  The config's OVn areas are the one source: the build gives
  the task's C and assembly OVLS (-D OVLS=n), and fails at a count its sources keep by hand (an assembly's OVMAX,
  OVL_BANKS or OVBANKS a number, or _ovbank's room; an extern ovbank[] of a number; OVLS, OVBANKS or ACCT_NOV #defined
  as one).
  - bank.s's OVMAX (dq0, dq0harn, dq0m), OVL_BANKS and _ovbank's room (dq0d, dq0p), OVBANKS (dq0w), things.h's
    OVBANKS and the accounts' ACCT_NOV are OVLS.
  - The C's ovbank[] declarations have no count (dq0harn.h's said 8 of its 10; dq0.h's 30, dq0d's 7 and grow.h's 10
    were right by hand).  dq0m.c's unused OVLS 10 is gone.
  - The town and the scholar's house read one overlay into one bank: they fail to compile with more.
  - dq0w's make.js checks that OVBANKS is OVLS and _ovbank OVBANKS + 4.

No sentence change: the games' images (40 files: .hyx, .ovl, .ovp, .cat, .rop) are byte for byte as at 685f7bb4.

## The suite

499 tests, with the guards on: all pass.  Every transcript is the same as e10's run (sim/tools/sentdiff.js: none
differ).  Start time (dq0starttime) as at e10: seed 1 241.1M cycles, seed 123456789 314.4M.

## Open

- The main RAM's code naming an overlay's label isn't checked: right through bread or a stub, wrong if read straight.
- Code in an overlay calling the world's catalogue bank (both in the window) isn't checked.
