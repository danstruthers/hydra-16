# DQ0: working rules

The rules for anyone, person or agent, who changes the game. They hold whatever the task. README.md says how the game
is built; OPEN.md holds the open questions; ../../tests/TESTING.md says which tests a change can affect.

## Scope

- **Change only what your task says.** A behaviour change the task and the user's decisions don't cover needs the
  user's decision: don't build it; put it in OPEN.md (and in your report) as an open question.
- **Work in your own worktree and branch.** Several agents build at once on one machine; stay out of the files another
  branch owns, and keep edits to shared files small.
- **Shared tables: add at the end.** In said.txt, msgs.def and records.def, add rows at the end of a section rather
  than reordering, so branches merge cleanly. Kinds of thing (items.h, dq0w/kinds.c) and denizen kinds
  (dq0d/kinds.txt, dq0harn/beasts.c) are numbered by position, and the numbers reach the saves: append only.

## Files

- **Deletions.** Delete files only in reborn/obj (the build's own) or in a scratch folder of your own outside the
  repository. Never run `rm -rf` on any other path, or on a path with `..` in it.
- **Line ends.** The working copy's text files have CRLF line ends (this checkout's core.autocrlf is true; git stores
  LF). Keep them CRLF: edit with an editor, or with a node script that writes CRLF. Never use Git Bash's `sed -i` on
  them.
- **Plain ASCII.** said.txt's rows must be plain ASCII, or the build fails (saidgen.js). In the docs, characters
  past ASCII have been mangled before (DECISIONS.md's section signs and arrows, a plus-minus in HOUSE.md): prefer
  ASCII, and read a doc's diff for mangled characters before committing.
- **Generated files** (obj/gen, the .dat files, parts.inc, dq0cover.txt) are the build's: change their sources.
- **The images.** Don't commit reborn/bin's images (prom*.bin, sdcard.img) with a change: they are rebuilt and
  committed on their own once the work is merged (as c7a8c5b7 did).

## Memory

- **The zero page.** The world, the minds and the senses end their own zero page at $43; $44-$70 is the paged ROM
  library's fixed block (lnzp.inc). Add no zero-page variables to those tasks: one more fails their link.
- **The main RAM floors.** Main RAM free must stay at least:
  - 3,000 bytes in the UI (dq0);
  - 2,048 bytes in every other task;
  - 512 bytes in themec and themegen.

  At a46c5c38: dq0 3,043; dq0w 2,285; dq0d 2,988; dq0p 4,430; dq0m 3,852; dq0harn 2,067. The build fails a task
  below its floor (build.js's FLOORS). The UI's main RAM is the tightest room in the game (43 bytes over its floor at
  a46c5c38): put new code in overlays and swapped parts, not there.
- **The headroom rule** (the user's, 2026-11-17): a part or bank under 64 bytes free, or a measure past 90% of its
  target, has its room or time won back before the next feature. The build lists the first (its "headroom" lines),
  the suite the second (after the tests).
- **Overlays** are 8K banks in each task's .cfg, with bank.s's OVMAX (OVBANKS in the world) and ovbank[]. The three
  must agree. In the world, dq0w/make.js checks them; elsewhere nothing does (the UI's dq0.h still declares
  ovbank[27] against bank.s's OVMAX of 30).
- **The fill rule.** An overlay followed by another in the same file needs `fill = yes, fillval = $FF` in its .cfg
  line. Without it the banks after it load shifted. Only the world's make.js checks this, so check it in the others.
- **Swapped parts** are set in parts.txt, `#@SWn` lines and `#pragma code-name (push, "SWnCODE")`, and packed by
  pack.js. A function moves between parts by its pragma alone.
- **The bank window.** A call between an overlay and a part, or from an overlay into code that maps the hub, can
  return with the wrong bank in the window. It is the commonest bug class. The emulator's guards (sim/lib/guards.js,
  in every test) stop a test at an instruction fetched from the window with no code there, and at a store that names
  the hub's header landing in another bank; but only where a test goes. Before adding such a call, read
  GAME-CRITIQUE.md's section 6.1 and the fix in 79083a98 (way.c's look_ov: the window kept, the hub put in, the call,
  the window put back).

## Behaviour and transcripts

- **Every behaviour change is checked by transcripts.** Run the suite before and after your change with
  `TRANSCRIPTS=DIR` (each dq0 test's screen saved in DIR), then compare the two folders sentence by sentence with
  `node sim/tools/sentdiff.js BEFORE AFTER`.
- **Every sentence that differs must be intended, or known noise.** tests/noise.txt lists each noisy test and why:
  the real-time fights, "Something stirs." and "Time passes.", denizens met at other moments, the accepted races (a
  stop landing a step or a thing later in dq0reachstop and dq0tiring), dq0peoplegen's last prompt. A difference
  that is neither is a bug until shown otherwise.
- **The save folder.** Where a change touches what's saved, compare the files byte for byte over many seeds.
- Say in your report every sentence the player now sees differently, and why.

## Budgets

- **What fails the suite:** the D17 targets (rethought 2026-11-17, built/2026-11-17-d17.md; tests/lib/trends.js), at
  115,200 baud: every command's first letter within 720,000 cycles, a look's and the map's too; a reading's words
  (a look, the map, what's carried, a thing examined) with no pause longer than 1.79M. Then the main RAM floors, and
  a silent start. No measure over its target is let off, by a list or by its trend: one still over fails, named as
  time to win back.
- **The other cycle budgets are trends** (the user's, 2026-11-17): dq0answer's per-command budgets, dq0traces' step,
  the start's 245M and 325M, and the rest (tests/trends.json). They move with memory layout alone. Don't reshape code
  to pass one. A rise of more than 5% is noted, not failed: say it in your report. Take a measure as the new trend
  only on purpose (`node sim/tools/trends.js --update NAME`, ../../tests/TESTING.md), with the reason.

## Tests

- `node sim/test.js` runs all 460 tests, ten minutes or more alone, longer while others build: run it in the
  background. `node sim/test.js NAME ...` runs a few; `--near AREA` and `--changed REF` choose them
  (../../tests/TESTING.md).
- Run the relevant tests while you work, and the whole suite at the end. All must pass, the trends apart.
- **Loosen no check.** Change an expected string only where your change means it to change.
- **Add tests for what you build.**
- `node build.js` in reborn/ takes ten minutes or more: run it in the background too.

## Commits

- One commit for each coherent stage.
- **The message:** a subject of 72 characters or fewer; a blank line; the body, what changed and why; the
  measurements at the end (the room before and after, the start's time, the suite's result).
- An agent ends the message with its Co-Authored-By line.

## Records

- **DECISIONS.md holds the user's decisions only**, dated, in the user's words where there are some. Don't add
  anything else to it.
- **"As built" reports** go in games/dq0/built/<date>-<name>.md, one file a branch: what was built and why, what the
  player sees differently, the room and timing before and after, and the open questions.
- **Open questions** go in OPEN.md, under their area. When the user answers one, the answer goes in DECISIONS.md and
  the question leaves OPEN.md.
