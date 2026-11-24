# DQ0: the decisions archive, 2026-11-08

The agents' accounts of what they built, moved out of DECISIONS.md on 2026-11-24 (branch docs2) with their text
unchanged, as AGENTS.md's Records rule asks: DECISIONS.md holds the user's words only. Each section left a one-line
pointer there; where a section held the user's words too, those stay in DECISIONS.md, quoted, and the whole section
is here. Each section is marked with its lines in DECISIONS.md as at d35194df.

<!-- DECISIONS.md:2048-2060 at d35194df -->
### The dispatch table, stage 1 (2026-11-08, as built, for review: COMMANDS-CRITIQUE.md's §5.2, step L)

Which function carried out a verb depended on where there had been room: `s_command`'s switch (part 2), then its default's `chores` (part 2), then `more` (overlay 16), then `rarer` (part 24), each tried in turn, and "You can't ... in this game yet." if none took it.  Stage 1 puts a table in front of them and calls each verb's handler by its row, the handlers' work unchanged.  What's built:

- **A row a verb:** actions.c's `rows[]`, in overlay 11 (beside the resolver), in actions.h's order, four bytes each: its handler (dq0.h's `H_*`), the kinds of thing it takes (`TK_*`: so far only `TK_CLOSER`, a thing that closes, worked first, for open, close, unlock, lock and force), and two bytes kept for stage 2 (its refusals, refuse.c's rules[]) and stage 3 (its request, where `cmd.aux` holds one).  Written `ROW (VB_*, H_*, TK_*)`; the build holds the rows to actions.h (games/dq0/make.js), so a verb added without its row stops the build.
- **One dispatcher:** the end of `s_command`, after the resolver, the refusals and the puzzle: the row read (`verb_row`, an overlay stub), then a switch on the handler's number.  Part 2's handlers are called straight; overlay 16's through `run16 (h)` and part 24's through `run24 (h)`, each that bank's one entry, which switches on the number.  Not a table of addresses: that would need a stub a handler in the main RAM (6 or 11 bytes each, ~35 handlers), and the main RAM has ~75 bytes to spare.
- **The handlers:** `s_command`'s and `chores`' cases, made functions where they're more than a call (`do_go`, `do_door`, `do_examine`, `do_fight` ...; `do_portal` the door's work several end in, once the `portal:` label); `more`'s cases, now by number (`o_run16`); `rarer`'s (`s_run24`).  `chores` is gone; `more` and `rarer` are renamed `run16` and `run24` and dispatch nothing: each is called only for its own handlers' numbers.  The hide still calls overlay 16's work by number (`H_TIE`, `H_HIDDEN`), the door verbs `H_SHUT`, the climb part 24's `H_CLIMB`.
- **Dropped, unreachable:** chores' `VB_OBSERVE_OBJECT` (s_command's examine took it first); `more`'s open, close, unlock, force, lock and hide, and `rarer`'s climb, as verbs (s_command took each first: their work stays, called by number).
- **Verbs with no handler:** 50 rows say `H_NOT_YET`: the 42 the survey found, and 8 seen to before the table (step, set pace and pace, pace.c's; the target's three, resolve.c's; use, read, combine, the puzzle's).  Their answer is the same "not yet" as before.
- **Nothing a player sees changes** (the whole suite).  The cycles: the row costs an overlay call (~100 cycles) and the switch fewer compares than the old chain (which went through `chores`, an overlay call to `more` and a part swap to `rarer` before "not yet").  A verb of part 24's (a dig, a sway) no longer calls overlay 16 first.
- **Room:** the main RAM 2,133 bytes free to 2,122 (`verb_row`'s stub).  Part 2 905 to 1,059, part 24 3,238 to 3,342, overlay 16 146 to 184, overlay 11 1,367 to 738 (the table, 568 bytes).
- **For review:** the two bank entries (`run16`, `run24`) are where stage 3 will meet the question of a handler moved between banks: its `case` moves to the new bank's entry.  And whether the table should hold stub addresses after all once stage 3 has the handlers in few banks.

<!-- DECISIONS.md:2061-2064 at d35194df -->
### The dispatch table, stage 2: the refusals in the rows (2026-11-08, as built: COMMANDS-CRITIQUE.md's §5.2, step L)

refuse.c's list of refusals by verb is now part of the dispatch table: each verb's row says what it refuses for a thing named by a word and for a feature of the place, and which features the verb does.  One table says what a verb does, what it takes and what it refuses.  Nothing a player sees changed (the whole suite passes).

<!-- DECISIONS.md:2065-2075 at d35194df -->
### The dispatch table, stage 3: the handlers by meaning (2026-11-08, as built: COMMANDS-CRITIQUE.md's §5.2, step L)

The command handlers were where the old chain cut by size had put them: part 2 (main.c's `do_*` and one-line cases), overlay 16 (words.c's `run16`), part 24 (main.c's `run24`).  Stage 3 puts them in files by what they're about, each group's place its pragma's.  Nothing a player sees changed (the whole suite: all 426 tests pass).  What's built:

- **Five files, one a group** (cmds.h lists them): `hthings.c`, the things' verbs (take, drop, the handling verbs, hide, eat and drink) and their timed works (fill, look in, tie, a thing that closes, a hide begun); `hplaces.c`, the features' and the doors' (a door's work, climb, examine, search, a look one way, leave); `hmove.c`, moving (go, face, jump); `hdens.c`, the denizens' (a blow, the missiles, disengage, the sway verbs, give and trade); `hself.c`, the body's and the meta commands (the limbs, a feel about, a look, wait, stop, tend, dig, inventory, sheet and map, time, pause and save, help, quit).  dq0.h's `H_*` are in that order, each group's numbers together.
- **Where:** all in part 2, the commands' own (a handler there costs no copy of a part before its answer), but the timed works, in overlay 1 (overlay 16's till now, 131 bytes from full) through one stub, `things_work`, which the hide (part 18) calls too.  A group moved is its pragmas alone: the build routes a part's entry straight from inside its part and through its stub from outside.
- **The dispatcher:** the row read, then the group's entry by the handler's number (five compares), each entry a switch on it.  No table of stubs: the part-2 entries are called straight, and the overlay's is one stub.  `run16` and `run24` are gone, and with them part 24's copy for a dig, a jump, a climb, a sway, a gift or a trade.
- **The request byte:** the door verbs' rows carry their `PA_*`, put in `cmd.aux` by `verb_row` when the line gave none: lock's (the parser's 0), so the door's work and `H_SHUT` no longer test the verb for it.  Not filled where it doesn't pay: the handling verbs (wear and take off are two requests each), the sway verbs, search and examine.
- **Found, for review:** an empty name (`""`) an overlay gives `othings` is read through the window after `othings` has put the hub there: it was overlay 16's address, which falls past the hub's header, in the room bank 0 keeps free, where nothing is kept.  The moved timed works give `nil`, an empty name in the main RAM, so where they land doesn't matter.  resolve.c (overlay 11) does the same with IO_FIND, left as it is.
- **Room:** the main RAM 2,117 bytes free to 2,128.  Part 2 1,059 to 659, part 24 3,342 to 3,729; overlay 16 131 to 1,073, overlay 1 1,288 to 424, overlay 11 307 to 278.

<!-- DECISIONS.md:2076-2194 at d35194df -->
## One line of sight (2026-11-08, as built, for review: ENGINE-CRITIQUE.md's §4 and step 2, its first part)

ENGINE-CRITIQUE.md's §4 said there were three lines between two squares that might disagree. They did. What each did
before:

- **The look (dq0p/cast.s's ray, in its grid):** Bresenham's squares from the character's eyes (a step along if
  2e >= -|dy|, across if 2e <= |dx|), the eyes 5 feet above the floor (3 crouched, kneeling or sitting, 1 prone) and the
  line ending at the far square's floor.  A square stops it if it is rock or a wall, a shut door or window (any portal
  not open but a portcullis, cast.s's `rdoors`), or its floor is above the line.  A lowered portcullis is seen through,
  and so is glass.
- **A denizen's sight of the character, its smell past the sound layer, the lights the character sees far off
  (dq0p/eyes.c's `clear_line`):** other squares (from the denizen's end, x and y each truncated, i |dx| / dist), no
  heights at all, and its own rule: a door square stopped it if its portal wasn't open, *a lowered portcullis too*;
  *a window never did*, closed shutters and glass included.
- **A missile's line (dq0w/wshoot.c's `lof`, `passes`):** Bresenham's again but with the ties the other way (2e > -|dy|,
  2e < |dx|), tried from both ends, no heights, a wall's corner let through when the character saw the target, and its
  own portal rule: a portcullis's bars and a grate let a missile through, glass and closed shutters didn't.

So a denizen beyond a lowered portcullis could be seen and shot by the character and not see it, and one could see the
character through closed shutters the character couldn't see through, or over a rise.

**What's built.** One line, dq0w/line.inc (6502 assembly), assembled into the world (dq0w/line.s) and the senses
(dq0p/line.s), with C's view in line.h:

- `line (sn, x, y, tx, ty, mode)`: 0 clear, else what stopped it.  The modes: **LN_SIGHT** (the eye), **LN_FIRE** (a
  missile), **LN_AIR** (a smell; no heights, the air fills the place).  Bresenham's squares as cast.s steps them; the
  line from `ln_eye` feet above the first square's floor to the last square's floor, as cast.s works it out, for the eye
  and a missile; `ln_way`, if set, asked of each square (the world's: someone standing in the way).
- `ln_portal (o, mode)` and `ln_pass (sn, x, y, mode)`: one rule for what a square lets through (0 no, 1 yes, 2
  narrowly: a slit, a window, the world's cover).  No square, rock, a wall: nothing.  A door, a drawbridge, a gate, a
  hatch: open or broken down.  A portcullis or a grate: bars, up or down, let everything through.  An arrow slit:
  everything, narrowly.  A window: its shutters closed, nothing; its glass in, the eye alone; else everything, narrowly.
- **A line between the character and anyone is drawn from the character's end**, from its eyes by its body's position,
  as its look's lines are.  At a tie Bresenham's squares from one end aren't the same as from the other, so this is
  what makes a denizen see the character just when the character would see it (the light aside: each side's own light
  rules are as they were).  A line between two denizens (an ally's throw) is drawn from the shooter.
- **Its users:** the senses' `s_sense_actor` (a denizen's sight, LN_SIGHT; its smell past the sound layer, LN_AIR),
  `glows` (the lights far off, LN_SIGHT with LN_CACHED: a chunk not in the cache stops it, none made); the world's line
  of fire (`lof`, LN_FIRE, someone in the way by `ln_way`, cover by the square beside the target), a missile flying on
  past its mark (`on_through`: `ln_pass`), a smell's way (wtrace.c's `airy`: `ln_pass`, LN_AIR); the sound layer's
  doors and windows (sound.c's `kind_of`: `ln_portal`, LN_AIR); and the look's own doors and windows (cast.s's `rdoors`:
  `ln_portal`, LN_SIGHT).  `clear_line`, `passes`, `lof1` and the copies of the portal rule in sound.c, wtrace.c and
  part 68 are gone.
- **The look keeps its grid** (cast.s's ray: thousands of lines a look, reading the grid with no `at`): the same
  squares, heights and portal rule, worked out there (dq0looks holds the look's every answer to what it was, below).
- **Not yet in the paged ROM** (ENGINE-CRITIQUE.md's §1.4 wanted it there): the line reads the chunks through each
  task's own `at` (its cache in the hub's banks, the maker asked for a chunk not made), which a ROM module can't call
  back into.  It's a stage: one source, assembled into the two tasks that draw lines.  It goes to ROM with the chunk
  lookup.

**What the game now means by a clear line, mode by mode:**

| | Sight | Fire | Air |
|---|---|---|---|
| Rock, a wall, no square | stops | stops | stops |
| A door shut (a hidden one too) | stops | stops | stops |
| A door open or broken down | passes | passes | passes |
| A portcullis or a grate, up or down | passes | passes | passes |
| An arrow slit | passes | passes (cover) | passes |
| A window, shutters closed | stops | stops | stops |
| A window with glass | passes | stops | stops |
| A window open, or bars alone | passes | passes (cover) | passes |
| A floor above the line | stops | stops | (no heights) |
| Someone standing in it | (passes) | stops, unless the target's as far beyond again | (passes) |

**What a player sees change:**

- *A denizen sees through a portcullis's bars*, as the character does: one beyond a lowered portcullis that the
  character can see (and shoot) now sees the character too.  (dq0denportc's dwarf, drawn to the lowered portcullis,
  now sees the character through it, heaves it up and comes at it: the game runs on longer, the character knocked
  out, so its cycle budget went from 3,000M to 7,000M; it passes as before otherwise.)
- *Where a denizen stands out of the character's sight, it's out of the denizen's too*, so it behaves as one that
  hasn't seen the character.  (dq0tiring's dig at seed 3: a giant rat in the passage below saw the character before
  through a door the character's own look didn't, and stayed watching; now it doesn't, wanders in and out of view, and
  its stirring cut the first ten-minute spell short.  The test now sends the rats off as each dig begins, dq0Alone's,
  as other scenes do: its subject is the fatigue.)
- *A denizen no longer sees through a window's closed shutters*, nor does a smell pass them or glass.
- *A denizen's eyes and a missile are stopped by a floor above the line* (a rise, a ledge, steps), as the look is;
  and a crouched or prone character's lower eyes see, and are seen, over less.
- *The squares between are the look's*: at a slant a denizen may now not see the character where it did, or the
  reverse, just where the character's own look agrees.  At seed 3 (dq0looks's first game) a giant rat in the passage
  south of the start room, at 32718,32724 with the character at 32723,32722, saw it before through the room's door at
  32722,32723; from the character's end the line goes through the door at 32720,32723, shut, as the character's own look
  does, and it no longer does.  Another (32722,32727, the character at 32724,32722) saw it through the door at
  32723,32723; the character's line cuts the wall's corner at 32724,32723.
- *A missile's line is the character's look's* when the character shoots or is shot at: one way, from its eyes, no
  wall's corner let through (the look already decides what it sees), and a floor above the line stops it.  Glass and
  closed shutters stop a missile as before; bars and slits let it through as before.
- *The lights far off* are seen by the same line: closed shutters hide one, a portcullis doesn't, a rise does.
- Unchanged: the look itself (dq0looks: every reply the senses give the same as before up to the first denizen's
  answer that changed above, in two of its games, both at seed 3, and the rest byte for byte; tests/dq0/looks.ref
  written again for those two); a sound's way and a smell's trail through doors and windows (the same rule, now ln_portal's); a
  missile's flight on past its mark.

**Test dq0line** (tests.js's `dq0LineTest`): seed 1's guardroom (dq0tunnel's scene), the portcullis let down, the
character's lantern lit a step back from the bars, a dwarf miner held four squares off at the far end of the passage
beyond, the character hidden (its hiding poked, so it's silent: a denizen's hearing answers before its eyes are asked).
The character sees the dwarf through the bars; the dwarf's senses must say it would see the character (SN_LOOK).  On
the old code: `its senses [0]` (clear_line stopped at the portcullis): "the dwarf the character sees through the bars
wouldn't see it".

**Performance** (cycles, the same games on the old build and the new, a profile of the senses' task by routine):

| | Old | New |
|---|---|---|
| A look (`view`, seed 3 / seed 7 / seed 3 again) | 355,776 / 295,865 / 378,339 | 355,997 / 296,952 / 379,529 (+0.1-0.4%: `rdoors` calls `ln_portal`) |
| The lights far off (`glows`, a call) | 46,975 / 48,584 / 48,700 | 47,551 / 49,108 / 49,083 (+1%) |
| A denizen's senses (`sense_actor`, a call) | 92,610 / 129,342 | 97,689 / 128,929 |
| One line (`clear_line`, then `line`) | 3,783 / 4,529 | 4,413 / 5,035 (it reads the two ends' floors) |

The start (dq0starttime, its first look and the denizens' first senses in it): seed 1 225.4M cycles to 225.3M,
seed 123456789 302.4M to 302.7M.

**Room:** the world's main RAM 2,791 to 2,536 bytes free (ln_pass, ln_portal and the line's variables); its part 65
178 to 227 (the line itself, in place of lof1, lof and passes), part 68 972 to 1,192, part 29 50 to 281 (their portal
rules gone).  The senses' main RAM 2,634 to 2,435 (the whole line, in place of clear_line); part 1 1 to 4, part 5 4,028
to 4,136.  (Written in C first, cc65 made it 1,185 bytes; in assembly it's 766 in the world, 219 of them in the main
RAM and the line itself in part 65, and 892 in the senses, with the cache's checks.)
