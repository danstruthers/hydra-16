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
11. **Search.** A denizen's reaches one square, finds no doors or traps, and is never told.
12. **The character** can't build, rob or filch a living denizen, dart past a body, or slam a door.

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
