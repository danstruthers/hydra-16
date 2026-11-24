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

- **Q1. What gets a letter** (old P1, P2; presentation).
  - **Proposed rule:** everything the player can name in a look gets a capital letter, in the look and in "Which
    one?", as things and denizens do now. That includes features (a door, a window, a stair, a portcullis), and a part
    of the floor with things lying on it.
  - **It settles:**
    - P1: doors and other features lettered, in place of "Which door? (1) ... (2) ...";
    - P2: a zone of the floor lettered.
    
    The user's rule for doors, a numbered list unless one is directly ahead (DECISIONS.md:362-363), would then read
    "a lettered list".
  - **Against it:** letters for things and denizens alone, as built. Features stay named by their words and
    numbered, and floor places by the locatives ("on the floor", "in the corner"). Lettering features changes every
    room's words.
  - Sources: built/2026-11-07-decisions-archive.md:83, "The doors' 'Which door?' the same question";
    COMMANDS-CRITIQUE.md:454-457, 579-581.
- **Q2. The look and a change of light** (old L4; presentation).
  - **Proposed rule:** a look is told when what the character can see changes, and never otherwise (the user's rule of
    2026-11-14, "No look when the light's the same", taken as the general rule). When a light goes out and is lit
    again within one act (a reach's own steps, dq0packsteps' dip), the dark is told in a sentence as it falls. The
    light's return is told in a sentence too, with no look if what's seen is then the same.
  - **It settles:** L4. It confirms L1, L2 and L5, which ASSUMED.md answers by the same rule.
  - **Against it:** a whole look whenever a light is lit after none was carried, whether or not what's seen has
    changed. Or, as built: the dip not told at all.
  - Sources: DECISIONS.md, "No look when the light's the same" (2026-11-14), and its archived account
    (built/2026-11-14-decisions-archive.md); ENGINE-CRITIQUE.md's row 6, its last "Left:" note.
- **Q3. The plug-ins' boundary** (old W1, W5, W9, W11, W12; the plug-ins, presentation).
  - **Proposed rule:** the world owns every name, people, tongue and belief; the ruleset owns every number (P6, drawn
    to a line). So:
    - A creature's kind, name, faction and tongue are its world's; its sheet and stats are the ruleset's bestiary.
    - A coin's name is the world's; its worth is the ruleset's economy.
    - Any ruleset runs in any world; neither names the other.
    - A life keeps its world and its ruleset from its first dungeon to its last.
    - The world is asked at the start, as the pace is, with "first" the default.
  - **It settles:**
    - W1: the bestiary split by that line;
    - W5: the coins;
    - W9: the world fixed for a life;
    - W11: rulesets and worlds not paired;
    - W12: the world asked at the start.
    
    ASSUMED.md's W2-W4, W6-W8 and W10 follow from P6 as it stands. A different line here may reopen them.
  - **Against it:** rulesets and worlds paired (Harn's world with HarnMaster), so a world may carry its own numbers.
    And the world chosen by a command-line switch, as now, not asked.
  - Source: built/2026-11-21-world1.md's open questions; WORLDS.md.
- **Q4. Sight's detail by distance** (old U1; the user's own item; the scope of content).
  - **Proposed rule:** what a looker makes out falls with distance and light, by its own sight, unless it is blind:
    first movement, then a shape, then a kind (humanoid or not), then who it is. The look and the minds name only what
    is made out (S1), for every looker. Until it's built, a look one way reaches 31 squares at most.
  - **It settles:** U1. It also settles how a far or dimly lit creature is named everywhere ("something", "a
    figure", "a dwarf").
  - **The question:** is that the rule, and is it built now or later?
  - Sources: HOUSE.md:4283-4286; DECISIONS.md:769.
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
- **G14.** A denizen's test sent without RS_ASKER gets dice that repeat (the quiet dice; search in a loop may too).
  Every test must be fair and independent (section 8). Sources: climb1 10; build2 10; things2 12.
- **G17.** dq0answer's things lines at 81-87%: a speed branch of their own (ix_pick, o_pacing)? A referee's blow giving
  way to a typed line? Source: speed13 1, 2.
- **G18.** 4-byte stubs in the UI too (194 bytes, where it's 99 over its floor). dq0Poke re-pokes until its write takes
  (the dq0fightmid/dq0felled race). Source: room7 1, 3.
- **R7.** Room and time:
  - a spare of its own for the minds' and the UI's banks;
  - the most-read said rows in the first two banks;
  - 4-byte stubs in dq0harn and dq0p;
  - s_pace_cmd in a smaller part ("step forward" 86.7%, "put the whetstone" 87.4%);
  - the minds' part 14 split (door_way, take_way, about 31K a door).

  Source: room8 1-5.
- **A1.** Game code in the paged ROM, for good: "tried for one module, measured, then decided" (DECISIONS.md:2274).
  It is bank placement, the engineer's under section 8: run the trial, then report the measure and the choice.
- **X1.** dq0hold's own poke of burn_due is lost if a minute is under way as it lands. It should poke burn_ask, a
  sturdier probe. Source: built/2026-11-14-decisions-archive.md:447-448.
- **X3.** A test's `cycles` limit still acts as a budget where it sits close to the measure (dq0save: 940M against
  851M). Make each a generous timeout. Source: built/2026-11-17-guards.md, its open question 2.
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
