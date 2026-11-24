# One path for every actor: the plan

The user's rule (2026-11-17; DECISIONS.md, "One path for every actor"): "Please make sure that the player and any
agent's interaction with the world are handled the same, in all cases."

This plan comes from two audits of the code at 485f8b83, with the things branch's code read as well. They're kept
beside this file as the work list; every row there has its file:line:
- audit/2026-11-17-act.md, acting on the world: the body and moving, portals, things, lights, tools, the body's
  needs, traps and hazards, costs;
- audit/2026-11-17-sense.md: sensing, fighting, dealing with each other, telling, time.

## 1. Where it stands

| | Interactions compared | Already the same | Differ | One side can't at all |
|---|---|---|---|---|
| Acting on the world | 59 | 7 | 52 | 20 more, outside the table |
| Sensing, fighting, dealing, telling, time | 104 | 27 | 65 | 12 in the table, 14 more outside it |
| All | 163 | 34 | 117 | 46 |
| After stage 2's search and hide (op2b, 2026-11-21) | 163 | 38 | 113 | 45 |

(Stage 2's search and hide made the same: the act audit's C15 and G-7, the sense audit's F1, F3 and F5; the rest of
its area's rows narrowed, below.)

**What a player would meet:**
1. **Denizens sense only the character.** They know one another by straight distance, through rock, and a denizen's
   eyes have no facing.
2. **No denizen fights another.** Both referees refuse it, the minds target only the character, allies never strike,
   rival factions never clash.
3. **Traps and harm.** Denizens never spring a trap, underfoot or on a door, and the world can harm a denizen only by
   a blow.
4. **Moving.** Denizens can't use stairs, ladders or shafts on their own (an ally is put beside the character), can't
   climb, have no swimming test, never drown, choke or stumble in the dark; bad air is only a wall to them.
5. **The body.** Fatigue, encumbrance, useless arms, shock and the Rise test bind only the character. A creature
   dodges or counters without limit, against the one-foe-per-blocker rule.
6. **Hearing.** Three formulas. The character's door work is silent to the minds; a denizen's walk is half as loud as
   the character's; a denizen's armour never clanks.
7. **Light.** Denizens carry none; the character's torch counts as dark to them (`unlit_at`).
8. **Robbery.** Takings vanish to the lair; robbers pick locks without picks; a hostile is held for ever at a braced
   door.
9. **Dealing.** Talk, trade, orders and robbery go one way only, and no denizen speaks.
10. **Time.** Typing costs the denizens time while the character's steps and blows go on.
11. **Search.** ~~A denizen's reaches one square, finds no doors or traps, and is never told.~~ Done (stage 2, op2b,
    2026-11-21): below.
12. **The character** can't build, rob or filch a living denizen, dart past a body, or slam a door.

**Done: stage 2's search and hide** (branch op2b, 2026-11-21; built/2026-11-21-op2b.md):
- **The act table's rows** search, hide, conceal, sniff and track (games/dq0w/acts.txt), read by both sides: a search's
  minute, its test and its noise (the character's busy noise, an actor's told: E_DEN DE_SEARCHES, "The bandit starts
  searching about.", heard as rummaging); hiding oneself's ten seconds; a thing concealed's minute and Concealment test
  (its grade the hiding's steps, acts.h's HIDE_OF); a sniff's three seconds and a track's minute.
- **One search** (wsearch.c, part 94): a denizen's search is the character's: wtrap.c's search of the hidden doors and
  traps within its reach, by its own sight (the line from its eyes; by touch in the dark) and its own sheet; the hidden
  things lying there by the character's own test; one hiding that it sees (the character's hiding ended, "found"; a
  denizen not of its faction out of hiding) by the character's search's test of a hidden denizen.  What it finds is
  found for every one, as the character's finds are.  Hostile folk search where they lost the character, once, as
  robbers did (tailing the character: the user's "searching is anyone's").
- **A thing hidden:** a robber's stash at its home by its own Concealment test and the minute (it was hidden at 1,
  with no test and no time).
- **Sniff for anyone:** a hunter by smell's sniff of the traces' store is the character's sniff: the same test (its
  own nose and sheet, P_NOTICE_SMELL), its steps by the scent's strength (trace.h's TRACE_STEPS), its three seconds; a
  critical failure loses the trail.
- **Parity tests:** dq0srchact (a secret door and hidden crowns searched by the character and by a bandit: the same
  tests at the same steps, the minute, the noise, the finds as the tests went), dq0hideact (a thing hidden by the
  character and by a robber), dq0sniffact (the character's sniff and the rats' store sniffs).
- **Left in this area:** knowers of a find (a door or a trap a denizen finds is found for every one: B10's knowers,
  a lair's own secret doors); an actor's search all at once, not by the time it gets (F2: with an actor's timed
  work); a robber's takings carried home and hidden there, not at a distance (C10, C14); no denizen tracks by prints,
  and a hunter follows the character's trail alone (M2's any actor's trail: stage 3); smell without asking (M1, the
  senses'); watched while hiding by the watcher's own eyes (B1); hidden compartments searched by a denizen (C5).

**Done: stage 2's digging** (branch dig1, 2026-11-23; built/2026-11-23-dig1.md):
- **One spell** (dq0w.c's dig_spell, part 1): the character's spell (part 1's s_dig_on) and a tunneller's (wdig.c) are
  one code: the rock it's into, the referee's test by who, the referee's rate and wear, the rock's hardness, poor rock
  come down, a hidden door dug into and found. The act audit's E1 and G-13 (its tools: its own things, bare hands, a
  chisel with a hammer), H1's spell time and H2's noise.
- **The act table**: the dig and digbegin rows carry the spell's ten minutes (SPELL, both sides' own, before); new rows
  digfound (a hidden door found by a dig), hang and wallup (a builder's work: its time and its noise from the table).
- **The referee**: P_DIG answered for every actor by rshouse.h's house_dig (both referees): an actor's tools listed in
  the tools' own form, the house's table's new claws row (the world's own copy of the rates, wdig.c's, gone); the
  grade by the creature's own sheet, as before.
- **The minds**: a hidden door is dug at as the wall it seems (dq0d.c's dig_way: dug_as).
- **Parity test**: dq0digact (the character's dig and a dwarf miner's: the query as it begins, the spells' ten
  minutes, the noise, the test and the rate by the grade, the tool worn).
- **Left in this area**: ~~the character's building (hang a door, wall up a way)~~ done (build2, below); a denizen digging down (no mind wants a shaft yet: OPEN.md); the roof's harm on a denizen (the world's harm
  for an actor, G3: the step's branch); a denizen tiring by its spells (the one body clock, A6, Y9); an arm useless
  stopping a denizen's chisel; the time of a denizen's spell spent while it's interrupted (Y8).

**Done: stage 2's things and robbery** (branch things1, 2026-11-23; built/2026-11-23-things1.md):
- **The act table's rows** hold, put, takeout, don, doff, strip, give, drag, rob, filch (games/dq0w/acts.txt), read by
  both sides: the character's handling times (things.c's catalogue code, part 48's hold and doff, part 35's strip),
  an actor's (wthings.c, wtake.c, wfetch.c), the drag's time, noise and test (wdrag.c, every actor's: E_NOISE or E_DEN
  DE_DRAGS, the referee's P_FORCE by the actor's own sheet).
- **A denizen's things by the character's rules** (wthings.c, parts 114-116; I_ROB's errands FE_STOW, FE_PUT, FE_DON,
  FE_DOFF, FE_DRAG, FE_HAND): put away (coins its purse, a family's carrier, its pack), put in or on a thing within
  reach (o_fits, the character's), worn (a wearable alone), taken off, a heavy thing shoved or dragged, a thing handed
  to another by the receiver's take rule; each its row's time and its reach (part 56's, every actor's), told as seen
  (E_BEAT:things).  C6, G-9, G-10, G-11, C11's receiver.
- **Its take** (wtake.c): sees things in open containers lying in its sight at any depth (H17), takes them out by the
  take rule (its time the takeout row's and a level each; a worn kind from a body the strip row's more: looting), and
  carries them by the character's carry (coins to its purse, a hand, else its pack: C1, C9); its hands full is no
  longer IR_FULL while its pack or purse has room.  An ally's fetch the same, with its time (it had none).
- **The character robs and filches** (G-18): "rob the bandit" (one senseless beside it: what it holds but a light lit,
  and its coins), "filch from the bandit", "steal from" (one asleep, or senseless: its coins alone, the character's
  Stealth against its hearing, caught it wakes); the minds' SW_ROB, SW_FILCH (dq0d's robbed), the world's FE_ROBBED
  (wthings.c's den_robbed: the character's carry), each thing told.
- **Parity tests:** dq0thingact (put, takeout, don, drag), dq0dentakein (a take from an open sack), dq0pcrob.
- **Left in this area:** see built/2026-11-23-things1.md's open questions (C10's takings carried home, the grope for
  a denizen, worn places for a denizen, the minds' use of the new errands, the character's take-up time the UI's).

**Done: building** (branch build2, 2026-11-23; built/2026-11-23-build2.md; the user's "The character builds anywhere,
with materials"):
- **One build** (dq0w/wbuild.c, part 123): the character's "hang the door" and "wall up" (the things' IO_BUILD) and a
  builder's I_KIT KT_BUILD (wdig.c) are one code: a broken door's leaf hung again in any doorway, a passage walled up
  anywhere; a hammer (every actor's tool_of; a record's its outfit's); the leaf the broken door's own, the stones the
  way's own spoil or rubble beside it; the referee's Timbercraft or Masonry (P_TIMBER, P_MASONRY) by the actor's own
  sheet; the act table's hang and wallup rows' times and noise.  The act audit's G-17.
- **Outfits**: bandits, soldiers and cultists carry a torch and a tinderbox, dwarf miners a tinderbox (climb1's
  lighting then theirs).
- **Parity test:** dq0buildact (the character's hang and wall, refusals, and a dwarf miner's, by the same tests, rows
  and effects).
- **Left in this area:** a door's leaf and stones as things carried; a new door hung in an empty doorway (a portal made
  where there was none); a doorway walled up; the work's time interrupted part way (the character's a wait, its work
  done as it begins).

**Done so far:**
- **Stage 1, the world's part** (act1, 2026-11-19): the act table (games/dq0w/acts.txt) and one sender each for noise and
  the referee's test (acts.h's act_noise, act_test).
- **Stage 2, closures** (op2a, 2026-11-21; built/2026-11-21-op2a.md): doors, windows' shutters, portcullises and lids by
  one code and one table for both sides.
  - Portcullises and windows read the act table too (wind, heave, raised, lower, bend, bent, prise, wopen, wshut).
    wport.c's own noise table for a denizen's portcullis is gone, so every actor's winding is 5, and the character's
    heave, raise and bent bars are now heard as a denizen's were.
  - One tool rule (wport.c's tool_of): a key that fits, or lockpicks, carried by whoever acts. A robber picks a door's
    lock only with lockpicks (bandits now carry them in the purse; kobolds and orcs don't), and picks a lid's lock with
    them before forcing it (den_pry). B4, G-8.
  - Denizens bar doors (den_bar: the widest bar it carries or has within reach, the character's rule; wk.link is no
    longer the picking flag). They also work a window's shutters, bend its bars and work its pane out: den_portal uses
    s_work's routing. G-5, G-6 (climbing through is left to the climbing branch).
  - Slamming is one act (PA_SLAM, PO_SLAMMED): "slam the door" for the character, and a fleeing denizen's slam is its
    mind's PA_SLAM. G-20.
  - A door that won't open though it's neither locked nor barred (braced) is tried twice, then forced if the denizen is
    hostile, else given up. B8.
  - A door opened as the character walks through is heard. A door with a set trap is not opened on a walk; it is left
    for the door work, which springs the trap. B2.
  - An ally's errands: lock, bar, unbar, force and pick, at a door, a window or a portcullis.
  - Not yet: a denizen springing a door's trap (B9: needs the referee's harm for an actor), battering by who (B6), hidden
    doors known to their lair (B10), a denizen's key interruptible as the character's is (B3).

## 2. The design: the same by construction

One function per act, one sensing routine, one engagement, one exchange, one give-way. Each takes "who": 0 the
character, else an actor. Actors differ only in their data: the sheet (skills), the species (darkvision, nose), the
body (size, wounds, load), what they carry.

### 2.1 Acting (act audit, section 3)
- **The act table** (GAME-CRITIQUE.md's E6). One row per act: its time, test, noise, fatigue, whether it can be
  interrupted, its tool and hands. Generated from a data file into the paged ROM, so it costs no main RAM. The
  character's numbers (HOUSE.md's) are the reference.
- **One sender each, by who:** time spent (busy), noise, harm (the referee's RS_HARM with an actor), and the referee's
  test.
- **The character is actor 0** (the user's choice, 2026-11-17): its record moves into the hub's actor table, so
  every task reads one body shape for every actor. The pc_* fields (about 50, read some 1,577 times across the tasks)
  become actor 0's; compatibility names first, so each task's code moves over in its own time; the save converted.
- **Shared acts, area by area:** closures (doors, lids, locks: E5), things (take, put, stow, hands, purse; robbery as
  takes), the step (traps, water, rubble, bad air, the dark, loudness, fatigue), search and hide, digging, climbing
  and the links between strata, lights.
- **Last, one dispatcher:** an act, its roles and "who", the same record whether the UI's command (phase B's grammar
  recording roles) or a mind's rung (E15) posts it.

### 2.2 Sensing (sense audit, 3.1)
- **One routine, `sense (looker, target)`,** in the senses task, for every actor, the character included:
  - sight: range by the sheet, facing, any light (carried lights included), darkvision and the eyes' adaptation, low
    targets, the line, a grille both ways;
  - hearing: one formula for every hearer;
  - smell for everyone; "sniff" and "track" tested for anyone.
- **A senses record per actor:** sight, darkvision, hearing, nose, the eyes' rate, its light's reach, listening. About
  63 bytes for nine.
- **`unlit_at` goes:** the senses publish what's lit, or the world asks them.
- The character's look stays a grid (the map and the words need it), by the same rule per square.

### 2.3 Fighting (3.2)
- **The referee:** an attack between any two actors; defences counted per round for every side; a compact side record
  per sheet.
- **The world:** a blow between two bodies; zones for every actor hostile to the mover; the disengage, the grope and
  shock's walk for everyone.
- **The minds:** a target each (0 the character, or an actor). Allies take the character's attacker; rival factions
  engage by morale; a broken foe may yield.

### 2.4 Dealing (3.3)
- **One exchange,** `exchange (speaker, listener, how, thing)`: talk, ask, threaten, order, trade, rob, filch, between
  any two. The speaker tests by its own sheet; a word's reach is the hearing rule.
- **Talk remembers and tells** (the user's answers of 2026-11-17): patience, and denizens who challenge, warn,
  surrender and answer what they know.

### 2.5 Telling (3.4)
- **One wording by viewpoint:** every act told by the same rows, whether the character acts, is the target, or
  neither; told if seen, else if heard.
- **One sound table** for both queues, by act; each event carries its actor's square and loudness.

### 2.6 Time (3.5)
- **One give-way** in the paged ROM library, on real ticks, for the world, the minds and the senses.
- **Catch up in full** (the user's answer): no cap on a late denizen's catching up; it decides again in the same pass;
  steps stamped with their intent's time; each denizen's own dice.
- **One body clock** for fatigue, every body.

### 2.7 The capability gaps
All 46 are filled on the side that lacks them, the mind choosing whether to use them.
- **Denizens get:** stairs, ladders, shafts, ropes, ledges, swimming, windows, lights, barring, searching for doors
  and traps, picking lids' locks, stowing, wearing, dragging furniture, digging down, tending wounds, throwing at any
  mark, speaking, trading among themselves, fighting one another, looting, yielding.
- **The character gets:** building (hanging a door, walling up a way), robbing and filching a living denizen, darting
  past a body, slamming a door, rallying.

## 3. What has to come first: room and time

- **The referee** (dq0harn) is 19 bytes over its floor, and its fight overlays are full. Its HarnMaster library goes
  into overlays (E10) before fighting is opened to any pair.
- **The world's main RAM** has about 2,165 bytes free after phase A, about 117 over its floor. The shared acts' stubs
  come from it; the merged dispatcher is where room comes back. Several parts are full (35, 58, 59, 24, 86; part 81 is
  at 36).
- **The senses' part 6** moves into a bank of its own; the lit map needs 232 of SIGHT_BANK's 279 free.
- **The UI's main RAM** is 43 bytes over its floor.
- **Time:** dq0answer's normal-pace first letters (being fixed), and dq0traces' step past 90% of its budget. The
  walk's step wins its time back before the step's act moves.
- **Parity tests:** one scene per act, a denizen and the character doing the same thing, their time, noise, test and
  effect compared. The pattern: dq0denportal, dq0denfound.

## 4. Order of work

| Stage | What | In parallel |
|---|---|---|
| 0 | Room and time: the D17 targets rethought (the user's); the referee's library into overlays; the world's room and the walk's time; the senses' bank; the parity harness | yes, by task |
| 1 | The character as actor 0: the hub's actor table, every task, the save (the user's choice); then the act table and the one sender each (world); one sensing routine (senses); one give-way, each denizen's dice and the minds' per-denizen struct (library, minds) | actor 0 first, then by task |
| 2 | Shared acts, one branch per area: closures; things and robbery; the step and its hazards; search and hide; digging; climbing and links; lights. Each fills its area's capability gaps | two or three at a time, by files |
| 3 | Actors with each other: one engagement for any pair; one exchange; one telling and one sound table | yes |
| 4 | One dispatcher, met by phase B's role-recording grammar | after B |

This takes in phase E (denizens worth meeting: allies and factions fighting, talk that remembers and tells) and the
critique's E4, E5, E6, E7 and E15.

## 5. Transcripts and tests

- Each stage changes the denizens' outcomes and timing. The character's change only where its own path was the odd
  one (a walked-through door that skips the door's trap; useless arms).
- Each branch explains every changed transcript, as phase A's did, and adds tests for the new behaviour: a denizen in
  a pit, drowning, picking a chest, tipping a lid, two factions fighting.

## 6. The user's decisions

DECISIONS.md, "The one-path plan and phase A's questions: the user's answers (2026-11-17)":
- **The character becomes actor 0 first**, then every act is rebuilt on it.
- **Now, alongside phases B and C**; it takes in phase E's denizen work.
- **Denizens hear one another's footsteps along the straight line**; loud events by a real pulse; exact whenever the
  character is at either end.
- **The D17 speed targets are rethought**, their assumptions investigated and reset where they make little sense.
- **Searching is anyone's**, by each actor's own nature (exploring, tailing prey or the character, patrolling ...),
  not a robber's alone.
