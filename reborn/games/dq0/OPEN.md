# DQ0: open questions

The questions still the user's, as of 2026-11-24. Under WORLD-MODEL.md's section 9 these are presentation, balance,
the plug-ins' boundary and the scope of content. Each is asked as a rule: the rule proposed, the cases it settles, and
the choice against it. Open questions that a rule of WORLD-MODEL.md already answers are in ASSUMED.md, as answers the
user may veto. Engineering and tests are the engineer's (section 8): they're in the backlog at the end, not asked.

**How it was made.** On 2026-11-24 (branch docs2), OPEN.md's 91 entries as at d35194df were triaged against
WORLD-MODEL.md:
- 64 settled by a rule went to ASSUMED.md;
- 11 that are the user's became the five questions below;
- 16 that are engineering or tests are in the backlog.

The old entries and their sources: `git show d35194df:reborn/games/dq0/OPEN.md`.

**Keeping it.** When a question is answered, the user's answer goes in DECISIONS.md, and the question leaves this
file. A new question comes here only if AGENTS.md's Scope rules say to ask it, under the rule it would add or change.
The "as built" reports in built/ end with their open questions. Triage them against WORLD-MODEL.md when the branch is
merged:
- settled ones go to ASSUMED.md;
- the user's come here, merged into rules;
- engineering goes to the backlog.

**Not here:**
- **Unbuilt work** that needs no decision. It's in:
  - GAME-CRITIQUE.md's section 10;
  - the last "Left:" note of each row of ENGINE-CRITIQUE.md's status table (its section 11);
  - COMMANDS-CRITIQUE.md's plan;
  - ASSUMED.md's answers, which are now work to build.
- **Rules and builds awaiting the user's review**, with no question put:
  - the old "as built, for review" entries, now in built/<date>-decisions-archive.md;
  - the 48 rules HOUSE.md marks *(new, for review)* (`grep -n "new, for review" HOUSE.md`).

---

## The rule-level questions

- Q1-Q4 were answered on 2026-11-24 (DECISIONS.md, "The rule-level round"); their rules are in WORLD-MODEL.md (P6, S1, section 7).
- **Q5. Tuning by play** (old U2, U3; difficulty and balance).
  - **Proposed rule:** a number the book doesn't give is a realistic house figure, marked tunable, which the engineer
    sets now (AGENTS.md's Scope). The user tunes them later, by play:
    - each occupation's worth in the game, not only its points (U2);
    - how far each sound carries and how well each actor hears, by ear, once the real environmental sounds are in (U3).
  - **It settles:** U2 and U3. Till then the house figures that ASSUMED.md lets stand (D5's, H18's, R2's, R5's) are
    the stand-ins.
  - **The question:** is that the rule? And for U2: should each occupation be evened out to the same worth in play, or
    left as the book makes them?
  - Sources: HOUSE.md:4287-4292; DECISIONS.md:526, 729; SOUND.md:128.

## Engineering backlog

The engineer's, under WORLD-MODEL.md's section 8, and not asked: listed so they aren't lost.
- **P3.** Flags per action (NEEDS_REACH, NEEDS_HELD, NEEDS_LIGHT ...) with a handler per class of thing
  (COMMANDS-CRITIQUE.md's 4.4). The shared paths do those steps for every verb now. Source:
  built/2026-11-15-decisions-archive.md:29.
- **K7.** The sound task goes silent at the hub's pc_out as it reads it, so the knockout blow's own thud may go
  unplayed. The blow is heard before the black-out (B3), so it should always play. Source:
  built/2026-11-14-decisions-archive.md:83-84.
- **K9.** numb (main.c), the test of each event against a black-out, is called on every event: in main RAM, or in an
  overlay at a cost in time. Source: built/2026-11-14-decisions-archive.md:85-86.
- **T1, T2.** The reach is not planned whole ahead (each step walks the tree again), and rs_after_on and rs_using stay
  flags. Source: built/2026-11-12-decisions-archive.md:421-431.
- **T4.** A job come due during a free pass's own pass rides that pass. The single give-way rule on one clock
  (2026-11-17) is to be built over it. Source: built/2026-11-14-decisions-archive.md:442-443.
- **M2.** Seed 70002's ledger clue no longer fits under linegen's THEME_SIZE, so that theme has one clue fewer. The
  clue is what the player sees (P7): win the room back without changing another seed's dungeon. Ask only if that can't
  be done. Source: built/2026-11-17-text.md, its open question 3.
- **W9 (its bug).** The ruleset isn't carried to the next dungeon: without -r, a later dungeon goes back to
  HarnMaster. Source: built/2026-11-21-world1.md.
- **G17.** dq0answer's things lines at 81-87%: a speed branch of their own (ix_pick, o_pacing)? A referee's blow giving
  way to a typed line? Source: speed13 1, 2.
- **G18.** 4-byte stubs in the UI too (194 bytes, where it's 99 over its floor). Source: room7 1. (Its poke race was closed
  by scene1's scenes, 2026-11-24: dq0scene proves it.)
- **R7.** Room and time:
  - a spare of its own for the minds' and the UI's banks;
  - the most-read said rows in the first two banks;
  - 4-byte stubs in dq0harn and dq0p;
  - s_pace_cmd in a smaller part ("step forward" 86.7%, "put the whetstone" 87.4%);
  - the minds' part 14 split (door_way, take_way, about 31K a door).

  Source: room8 1-5.
- **A1.** Game code in the paged ROM, for good: "tried for one module, measured, then decided" (DECISIONS.md:2274).
  It is bank placement, the engineer's under section 8: run the trial, then report the measure and the choice.
- **S2c. The tests' scenes, what's left** (built/2026-11-24-scene1.md, its open questions 1 and 2):
  - the senses get a scene part, by the rule that every task owning state a test sets has one (dq0heading pokes the
    senses' RAM; dq0p is 242 bytes over its floor);
  - blindness a referee fact a scene sets (RS_SCENE), so dq0blind and dq0darklook stop overriding the summary;
  - dq0saveout's and dq0ahead's raw pokes.
- **E1, E2.** HyBASIC's banner (a version, or the free memory?), and docs/01-overview.md's examples (BASIC's shows its
  banner, hylang's leaves its own out). These are the languages' and the emulator's tools, not the game's. Source:
  built/2026-11-21-emu1.md, open questions 1 and 2.

## On hold: the between-dungeons work

The town, the scholar's house, the account and prices are on hold (GAME-CRITIQUE.md:31). Their open questions are in
the archived accounts:
- built/2026-11-12-decisions-archive.md:560-568, "dq0acct split: the account and the ruleset's hooks": **For review /
  left**.
- built/2026-11-13-decisions-archive.md:134-151, "The town and the scholar's house: the ruleset's hooks": **For review
  / open questions**.
- built/2026-11-13-decisions-archive.md:421-438, "Town prices, local": **For review / open questions**.
- ENGINE-CRITIQUE.md:683-686, and its status table's row 9 (line 826): the prices of the time between, "Left:".
