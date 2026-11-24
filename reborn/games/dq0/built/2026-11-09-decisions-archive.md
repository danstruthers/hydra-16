# DQ0: the decisions archive, 2026-11-09

The agents' accounts of what they built, moved out of DECISIONS.md on 2026-11-24 (branch docs2) with their text
unchanged, as AGENTS.md's Records rule asks: DECISIONS.md holds the user's words only. Each section left a one-line
pointer there; where a section held the user's words too, those stay in DECISIONS.md, quoted, and the whole section
is here. Each section is marked with its lines in DECISIONS.md as at d35194df.

<!-- DECISIONS.md:2195-2244 at d35194df -->
## The game's own library in the paged ROM (2026-11-09, as built, for review: ENGINE-CRITIQUE.md's §1.4, §4 and step 2)

ENGINE-CRITIQUE.md's §1.4 wanted the code the tasks each had a copy of in a shared library of the paged ROM, called by
XCALL (some 120 cycles a call: for routines that do real work, or are called seldom).  Its first contents are the two
§4 copies that were simplest and safest: the overlays' loading and `num`.

**A module of its own, not an addition to unpack.**  modules/dq0lib (rom.txt, after unpack: a library module,
HT_LIBRARY, its jump table at `$A030`).  The unpack library is the system's (libraries.md lists it with numbers, math
and asm: packed banks read from any file, nothing of the game's in it); the game's own code, its overlays' files'
names, its later line and router, would make it the game's.  The cost is a bank of the paged ROM (a library has its
own `.cfg`, so the build doesn't pack it beside another module); the ROM has the room.  dq0lib calls unpack by XCALL
in its turn (found by MODINFO, as the tasks found it before): XCALLs nest, each putting its caller's bank back.

**How a task calls it** (games/dq0/dq0lib.inc, which each task's bank.s includes in place of ovunpack.inc, with
`OVL_BANKS`, `OVL_DOT` and `OVL_FILE` set before it):

- `unsigned char ovl_load (const char* dir)`: finds the library (MODINFO, its directory's entries read into the parts'
  area, nothing there yet), keeps its bank and page (two bytes), and calls its `OVL_LOAD`: the path made in the parts'
  area, the file opened, the banks had (a run), their numbers written to `_ovbank`, the unpack library called, the file
  closed, the window's bank put back.  0, or not 0.  The UI gives the fd find_dirs opened (its high byte 0) and a `.`
  after the first twelve banks (`OVL_DOT`: its start's progress, as before).  dq0harn's ovl_in takes its two banks
  more after it.  Every task calls it first, so the library's found before anything else calls it.
- `unsigned num (const char* s)`: `NUM`, a number's digits (32 bits in the library, 16 here).  dq0d's, dq0p's (its
  `s_num` in part 3) and dq0w's `num` and the UI's `dnum` (bank.s: `_dnum = _num`) are gone, and dq0m's `atoi` (with
  the C library's ctype that came with it).  dq0harn keeps `atoi` for its sheets' numbers; its arguments are `num`'s.
- Not moved: dq0life's and dq0end's `num` (32-bit): RAM programs with some 20,000 bytes free, which don't find the
  library (finding it would cost more than their `num`).

**Room** (the main RAM's free bytes, the floor 2,048): the UI 2,128 to 2,307; the world 2,536 to 2,624; the denizens
2,399 to 2,512; the senses 2,435 to 3,122; the maker 2,414 to 2,879; the referee 2,197 to 2,329.  More than §1.4's
measure ("a few hundred bytes a task"): a task's `ovl_load` was what brought the C library's `open`, `strcpy`,
`strcat` and the like into its main RAM, and the senses and the maker used them nowhere else; the maker and the referee
had a 64-byte path besides.  The parts: the world's part 3 84 to 166 and overlay 1 86 to 168, the denizens' part 23
2,473 to 2,547 and overlay 2 31 to 105, the senses' part 3 4,966 to 5,048 and overlay 3 2,209 to 2,291 (their `num`s).

**Performance:** the start (dq0starttime) 226.8M cycles to 226.6M at seed 1, 305.9M at seed 123456789; every task's
banks as ld65 made them (dq0OvlBanks).

**The line of sight and the chunk lookup, looked at, not moved** (ENGINE-CRITIQUE.md's §4 has the plan).  The obstacle
noted before ("a ROM module can't call back into the task's `at`") isn't the real one: a ROM routine can `jsr` to its
caller's RAM.  The real ones: the ROM can't link to the task's symbols, so `at` and the rest must be handed over at run
time; the line keeps some 45 bytes over its calls of `at`, which may change the call registers, and a library has no
RAM; it reads cc65's C stack, whose runtime is at another address in each task; and `at`'s quick way (some 80 cycles,
thousands a look) can't be an XCALL.  The plan: a fixed zero-page block for the line (`$50-$7F`: the tasks' own zero
page ends at `$41`) with the callbacks' vectors, laid out in one include for the ROM and the tasks; stubs in the task
that take cc65's arguments into it; the hub cache's search as the library's `CS_FIND`.  It would free some 770 bytes of
the senses' main RAM and the world's part 65, for some 150 cycles a line (3%).  Left for now: neither task is short of
room, the one line is already one source, and the router (step 2's next) needs the same contract of callbacks into
`at` from the ROM, so it's best designed once, for both.

<!-- DECISIONS.md:2245-2353 at d35194df -->
## One way-finder (2026-11-09, as built, for review: ENGINE-CRITIQUE.md's §4 and step 2, its second part)

ENGINE-CRITIQUE.md's §4 listed the step rule and the way-finding as copies that might disagree: chunk.c's `step_ok`,
the world's `step_by` (its own copy), ways.inc (the same source, compiled into the world and the senses), the minds'
`open_sq` and `taken`; the minds' `route` (a search) and `step_for` (greedy), the world's `ways`, `s_go_to`,
`s_path_ways`, `s_ways_go`, `s_step_to`.  What each decided before:

- **A step, a step at a time** (chunk.c's `step_ok`, slant.s's `slant`: the world's walks and steps, a denizen's body's
  step as the world carries it out, wbody.c, the minds' steps): a square can be stood on if its kind can (T_PASS: not
  rock, a wall, a window, a pit, a cut; rubble, steps, a stairway's square can), a door's only if open and not hidden;
  a rise or fall of STEP_MAX feet, STAIR_MAX on steps at either end; a diagonal only a diagonal way's, past two corners
  neither of which can be stood on (cornk: rock, a wall, a door, a window, a pit), one of them cut.  One source
  already, compiled into each task.
- **The minds' steps** (dq0d.c's `step_for`, `s_coarse`, `s_way`, three copies of one test): that rule, and the square
  within its reach of home, no one on it (`taken`: the character, where it's going, another actor, a denizen out near
  it), no water deeper than it wades unless it swims or flies, no bad air (`wades`).
- **The minds' way found** (`route`, part 18: a search over a 32-square field from the mark): the same kinds, heights
  and corners, a shut door a way for hands (worked as it gets there: door_way), not for a beast, a hidden one none;
  **no notice of water or bad air**; a diagonal past a door's corner refused once that door's square had been reached
  (the order the search came to it), let through before; and its fields **shared by every walker** bound for the
  same mark, whatever its rules.
- **The character's ways** (dq0w.c's `s_ways_go` and `step_by`, part 21: a search over the room's floor and walls, or
  25 squares about it in a passage): the same kinds, heights and corners (its own copy); a door's square a way in a
  room whatever its state (hidden too), none along a passage, open or shut; no square a denizen stands on, nor one
  with a trap it knows of, a hidden trap the way's first; its times by its pace.
- **The character's straight walk across a room** (`across`, part 33): any of the eight headings over the room's
  floor, a diagonal over open floor and past corners too; a door straight on opened if it's handled one.
- **The placing's `open_sq`** (where a denizen is put, an ally come along, a cornered one's dart): only floor, a
  passage, steps.

So they agreed on slopes, steps, rubble, windows, pits and the diagonal ways' corners everywhere a player can reach
(the world's copy of the rule was the rule).  They disagreed on water and bad air: **a denizen's way led into water
deeper than it wades, or a pocket of bad air, where its steps won't go**, and it stood at the edge for good (its way's
next step refused, the greedy step no nearer, the same way found again); a beast could follow a field found for one
with hands to a shut door; and the diagonal past a door's corner hung on the search's order.

**What's built.** One source, dq0w/walk.inc (C), compiled into the world (dq0w.c's part 21) and the denizens (dq0d.c's
part 18), not yet the paged ROM (it reads the chunks through each task's own `at`; it goes there with the chunk
lookup, as the line does):

- `wk_cell (t, q)`: a square's cell by the rule, for the walker's rules (`wk_rules`): WK_HANDS, a shut door (not a
  hidden one) a way, to be worked as it gets there; WK_NODOOR, no door's square a way.  The rule with neither is
  `step_ok`'s.  What else a walker leaves out its reader takes off the cell as it reads the squares in: the
  character's, a denizen's square and a trap it knows of (a hidden trap marked, WC_TRAP); a denizen's, water deeper
  than it wades and bad air (dq0d's `wet_ok`, `wades`'s own test) and a chunk not made.
- `wk_search ()`: the one search over a grid of 32 by 32 cells (a chunk's squares, the world's; a field about the
  actor and its mark, the denizens'), the floors beside them, from the cells queued, the four ways then the diagonal
  ways, the step's rise (STEP_MAX, STAIR_MAX on steps) and the diagonal's corners checked there, the first way to each
  square kept (the way it was come into); within a box (the room's, the world's), stopped at a square (the actor's,
  the denizens').  The world's times and its traps on the way by WK_REACHED, its hook.
- **The world**: `s_ways_go` reads the room's squares in (the character's rules: WK_HANDS in a room, WK_NODOOR along a
  passage) and searches; `step_by` gone; the ways' bank laid out anew (cells, floors, times, traps, a 512-entry
  queue, then the path and the travel's legs as before).
- **The denizens**: `route` reads the field's squares in by the actor's rules and searches; each field kept with the
  rules it was found by (WKEY: its water's, its hands), a field shared only by walkers whose rules are the same; the
  minds' one test of a step, `legal` (the rule) and `may` (its reach, no one there, `wades`), in place of their three
  copies in `step_for`, `s_coarse` and `s_way`.
- Unchanged, on purpose: `step_ok` and `slant` (the rule a step at a time, and already one source); ways.inc (the
  same source in the world and the senses: a passage's breadth, the ways on); `open_sq` (a stricter rule: where one is
  put, not where it may step); `across`'s diagonals over open floor (a straight walk, the character's own; see below).

**What a player sees change:**

- *A denizen goes round water deeper than it wades, and round bad air*, where its way led it in and it stood at the
  edge for good.  (dq0route: a hostile dwarf miner four squares off across the guardroom, a pocket of bad air on the
  square between, comes round it to the character; on the old code it stood beside the pocket, two squares off, for
  the three minutes the test waits.)
- *A beast doesn't follow a way found for one with hands*: one bound for the same mark finds its own (round, or none:
  its greedy step then, as before), not a way through a shut door it can't open.  The fields are found more often
  where walkers of different kinds go for the same mark (each at most every ten seconds, as before).
- *A diagonal way past a door's corner* is a denizen's way whatever order its squares were found in, as its step
  already allowed.
- *The character's ways*: unchanged but for one thing no player could see before: a hidden door's square is no longer
  a square of the room's ways (every caller had left hidden doors out already).
- Timing: the world's search now reads the room's squares once (each square's denizen and trap looked up once, not
  once for each of its eight neighbours) and searches the cells; the first letter of a walk's answer is a little
  later, the whole a little sooner (below).  As cycles move, so do the turns' dice in long fights: dq0antidote's
  adders bit the character senseless a third time within one answer ("Everything goes black again." told twice: the
  game's own wording, blows.c's harm_told, "again" after the first), which the repeats' check (DQ0_REPEATS) now lets
  pass with the fight's other falls.
- Left as they were (a difference kept): the character's straight walk across a room (`across`) goes diagonally over
  open floor, and between two corners in a cavern, which neither a denizen's step nor anyone's way does (a diagonal is
  a diagonal way's only); a dart and an ally come along want floor, a passage or steps (`open_sq`), not rubble nor a
  stairway's square.

**Test dq0route** (tests.js's `dq0RouteTest`): seed 1, dq0line's scene (the dwarf miners' guardroom), the character's
lantern lit; a dwarf miner made hostile four squares off along a row of floor (the others sent off: dq0Alone), a pocket
of bad air poked into the chunk on the square halfway (its reach 0: that square alone); three minutes waited.  The
dwarf must come within a square of the character and never stand in the pocket.  On the old code: it stood at
32708,32795, the pocket's edge, every time it was looked at ("the dwarf never came round the pocket of bad air").

**Performance** (cycles; the same games on the old build and the new):

| | Old | New |
|---|---|---|
| A denizen's decision (`decide`, a call: dq0denportc, its 15,219 decisions the same / dq0tunnel / dq0atbay) | 97,446 / 53,868 / 106,511 | 97,810 / 54,121 / 105,730 (+0.4%, +0.5%, -0.7%) |
| Its step (`step_for`, a call: the same three) | 5,671 / 6,516 / 6,917 | 5,737 / 6,426 / 7,148 (`may` and `legal` called, not inline) |
| A walk's answer (dq0answer: "walk to the door ahead to my left", "walk to the window"), its first letter | 623,716 / 481,606 | 633,207 / 488,461 (+1.5%: the room's squares read in first) |
| The same, the whole answer | 5,926,259 / 5,902,481 | 5,717,516 / 5,669,342 (-4%) |

The start (dq0starttime): seed 1 226.8M cycles to 227.2M, seed 123456789 302.2M to 305.2M (+1%: by the tasks, the
world +0.6M, the denizens +1.9M, in their own doings as the start's timing moved, more dice thrown and intents posted:
no way is found in the start).

**Room:** the world's main RAM 2,536 to 2,471 bytes free (the ways' lookups, way's and path_ways', by the new layout);
its part 21 1,641 to 1,402 free.  The denizens' main RAM 2,399 to 2,348 (`legal`, `may`, `wet_ok`: the three copies
of the step test gone from step_for's, s_coarse's and s_way's), part 18 95 to 89, part 17 2,718 to 2,870.  The senses
unchanged (2,435).

<!-- DECISIONS.md:2354-2424 at d35194df -->
## The hub's numbers, made by the build (2026-11-09, as built, for review: ENGINE-CRITIQUE.md's §2 and step 3, its first stage)

ENGINE-CRITIQUE.md's §2 found the hub's numbers kept in step across languages by hand: games/dq0c/hub.inc mirroring
twenty of hub.h's (held to them by dq0c's make.js), actions.h mirroring the catalogue (held by make.js), struct kind's
size, the attitudes, a square's feet, the sound's kinds and some sixty of struct theme's offsets typed again in the
make.js files with nothing holding them, the tests compiling C expressions with cc65 as they ran to find the structs'
offsets, and the writers' table a comment a test read.  Stage 1 makes each of them come from one place; it doesn't
yet type the messages' payloads (stage 2) nor change a field of the hub.

**The C headers stay the schema, read as the compiler sees them.**  games/dq0/hubgen.js, called by build.js
(`hubNumbers`) before the games are built and by `build.js prog`: it reads hub.h and the headers it includes (rs.h,
level.h, ../dq0w/world.h, items.h, theme.h) with dq0.h and life.h (the command tools' names), and commands.actions.
Every object-like #define and every enum's member by parsing; every named struct's fields by parsing (a field of a
struct type, or of an anonymous struct, flattened into its own paths: `tb.at`, `door.rel`, an array's first element;
a #define among the fields left out); then one C file of `const long vN = (long) (EXPR);` lines, one a name, a
field's offset and size, an array's element size, a struct's size and each region of the writers' table, compiled by
the build's own cc65 and read back from its `.dword`s.  A line cc65 can't make a constant of (a #define of code) is
dropped and the file compiled again: two of 2,166 (KEPT_CLOCK, FIGHT_LB).  2,164 names, 134 enums, 80 structs with
1,516 fields; some 0.3 s, made again only when an input's hash changes.  No second schema file: the headers are what
the C is compiled from, so they can't disagree with it, and stage 2's payload table is the place to decide whether a
declarative file should write the C instead.

**What it writes** (obj/gen, beside apigen's files, on the build's cc65 and ca65 include paths now):

- `dq0hub.json`: the names (`C`), the enums in order, the layouts (`S`: each struct's size, each field's offset, size
  and dimensions), the writers' map (`own`) and the tasks' names (hub.h's T_*).  `load()` reads it, with `off (struct,
  path)` (indices numbers or names: `faction[2]`, `mreq[ASK_UI]`), `size`, `words` (an enum's members as words, each
  checked its value its place) and `value`.  Read by dq0's theme compiler (make.js: struct theme's and struct kind's
  every offset and length, the TM_*, P_FORT, K_*/KF_*, TR_*, TE_*, END_*, G_THROUGH, TL_RUINED, AT_*, ST_* it typed),
  dq0w's make.js (struct kind's size, its name's, cap_weight's place, NKINDS, CQ_WORDS) and outfits.js (MT_*), dq0d's
  (ST_*, AT_*, SND_KINDS, P_*, SQ_FT) and dq0s's (SND_*) make.js, sim/lib/dq0meaning.js (its 400-odd named offsets and
  the names for its events), sim/lib/hubown.js (the map, the table, the tasks), tests.js (hubown's `evaluate`, nine
  calls, now offsetof, sizeof and names from the layout; four numbers it had typed) and commandwords.js (its own
  header parser gone: the names, CQ_REL, CF_*).
- `dq0hub_NAME.inc`, an assembly include per assembly file, each with the names it uses (hubgen.js's `ASM`: a header's
  name, `STRUCT.PATH` an offset, `sizeof STRUCT[.PATH]`), `.include`d where its hand-typed lines were: dq0c.s (the
  hub's places, the request's, its constants: hub.inc's), cmdmean.s (struct cmd's CM_*, struct cthing's T_*, CF_*,
  CQ_REL), line.inc (T_*, struct portal's kind, state and parts, PK_*, PP_*, PS_*), items.s (struct item's size and
  four fields, NITEMS, ITEM_BANK (0), ITEM_BANKS), letter.s (NLET, LT_REC, struct cix's size and let: an `.assert`
  that it's 4, its two shifts), pulse.s (SND_GRID, SND_W), at.s (CW) and lbsaid.s (dq0.h's TOLD).  The hardware's
  HUB_U and HUB_BANKREG, hub.c's literals, moved from hub.inc into dq0c.s.
- `actions.h`, from the catalogue: VB_NONE, its [actions] in order (each its `(* *)` words), NACTIONS.  The catalogue
  has the roles and the result codes, so it is the one list; games/dq0/actions.h is gone, and the C's `#include
  "actions.h"` finds the build's (VB_OBSERVE_IN's words moved into the catalogue).

**The writers' table** is the block between hub.h's `own:` and `own.` lines, in its comment as before (readable, a line
a region).  The build reads it (a line in the block that isn't an `own` line, a blank or a line's words going on
stops the build), works out each region's bytes in the same cc65 pass (the C expressions hubown.js compiled at test
time) and writes them into the JSON; hubown.js paints them and checks the writes as before.  The map was held to the
old one byte for byte (26 banks, 231 regions), as were the recorder's offsets and the 1,768 names the old parser found.

**Gone:** games/dq0c/hub.inc and dq0c's make.js checks (hub.inc to hub.h, cmdmean.s to hub.h, actions.h to the
catalogue), hub.h's HUBO_*/CQO_* and dq0c_fits, actions.h, things.c's (the world's and the senses') store_fits (items.s's
numbers to the headers), obj/hubown.json and obj/dq0meaning.json (the tests' caches of what they compiled).  Kept: the
C's own asserts (cmd_fits, theme_head: C to C, for the C), make.js's rowsSame (actions.c's rows are code, held to the
enum, now the JSON's), dq0c's limits (its make.js to dq0c.s's, the parser's own).

**No behaviour change:** every value the generator gives is the one that was typed (checked as each was replaced), and
the images came out byte-identical: bin/* (the ROMs, the SD card's image, the page) and the games' programs and data
in obj/games, 112 files (hydra.lib differs in ar65's file times alone, as it does build to build).  One difference
that isn't in the images: a theme's kind may now name every material and craft the game has (MT_*: wax, tallow, gold;
AP_*: rock), the theme compiler's lists being the enums' (they had stopped at `other` and `glass`).  The build took
1 min 23 s (1 min 27 s before).

**Left** (not the hub's, or not numbers): the denizens' own records in dq0d/make.js (dq0d.c's struct dkind, faction and
dkx, wtrace.c's struct tkind, lore.h's records: their headers aren't the hub's, and dq0d.c's structs are in a .c),
outfit.h's PL_* and places in outfits.js and dq0w/make.js, themegen.c's TK_*, line.h's LN_* in line.inc, and the
tables built from world.h's T_PASS (slant.s's t_kinds, cast.s's): the generator can take them as their headers are
added to its list.  `node sim/tools/commandmatch-test.js --phrases` doesn't link (resolve.c's letters, des_add and
the rest, not in cmdres.c's stubs: nothing of this change's), and isn't part of the suite or the tiers.

<!-- DECISIONS.md:2425-2494 at d35194df -->
## The messages' payloads, declared (2026-11-09, as built, for review: ENGINE-CRITIQUE.md's §2 and step 3, its second stage: the intents)

Every intent and event travels in one `struct msg { kind, who, a, b, w1, w2, t, x[4] }`, and what each slot meant for
each kind was written only in hub.h's comments, often packed by hand at both ends (I_WAIT's w2: `k << 8 | (n & 0x7F) <<
1 | 1` in the UI, `m->w2 >> 8` and `m->w2 >> 1 & 0x7F` in the world).  Stage 2 states each kind's fields once and makes
the rest from it, the layout untouched; this first part does the intents.

**A declaration file of its own, not a block in hub.h** (games/dq0/msgs.def).  The writers' table lives in a hub.h
comment because it reads as a table of regions beside the structs it names; a payload table is a grammar of its own
(slots, bit ranges, constants, enums, forms, inclusions) that would need escaping inside a C comment, and hub.h's long
comments beside each kind stay what the fields mean, msgs.def where they sit.  It is read by hubgen.js in the same pass
as the headers (its hash in the cache key), so it is checked against the very structs and constants cc65 compiles.  Its
syntax (its head says it at length):

    struct msg
    I_WAIT          a how; w1 tenths; w2(busy) task:8-15@LT_NONE what:1-7 quiet:0
    I_STEP          w1 head:0-2@H_N keep:8=STEP_KEEP; w2 squares
    I_STEP:den      < I_DEN; a heard@DE_MOVED; b loud; w1(way) head:0-2@H_N coarse:9=STEP_COARSE scent:10=STEP_SCENT;
                    w2(sense) one:0 feels:1=STEP_FEELS; x[2] nose

a line a kind (indented lines go on), each slot of the struct then its fields: a name alone is the slot whole, `name:N`
or `name:N-M` its bits; `=CONST` a one-bit field's constant from the headers (checked to be that bit: the hand names
STEP_KEEP and the rest stay, tied to the declaration); `@FIRST` an enum whose members name the values in the tests;
`SLOT(name)` a packed slot named whole too (to test it for none, copy it, or set it by its packer); `KIND:FORM` another
shape of the same kind (the denizens' I_STEP, a search's I_PORTAL, I_KIT's digs, I_ROB's errands); `< KIND` another's
fields included (I_DEN, the denizens' intents' head: doing and face, not a kind).  A slot of flags the headers already
name (I_WAIT's WA_*, I_KIT's KT_*, I_RISE's POS_*) is one field, its slot whole, and the code tests it by those names.
The same syntax is meant for the events and for struct req (a `struct req` line; its multi-byte x[] wants a slot range).

**What the build makes of it.**  obj/gen/dq0msg.h, which hub.h includes after struct msg (written before cc65 reads
hub.h, so the pass's own compile sees it): for each field an accessor `I_WAIT_TASK (m)` over a struct (a pointer's `*m`),
an lvalue when it's the slot whole (`I_GO_X (m) = tx;`); for each packed slot a packer, `I_WAIT_W2 (task, what, quiet)`,
its fields in the order written (one bit: its bit or none; the lowest field as given, the top one cut by the slot, one
between masked); for each form a builder, `I_PORTAL_SET (m, who, act, which, x, y, bar, strike, hand)`, setting kind,
who and each slot it names in the struct's order (a comma expression).  The generator stops the build on a slot the
struct hasn't, a field past its slot's bits or over another of its form, a name the headers have already, the same
name meaning two things in two forms, or a constant not its bit.  The macros were tried against cc65 first: `(&m)->`,
`(*m).` and `m->` are the same code, constant packers fold to their value (`I_WAIT_W2 (0, 0, 1)` is 1), a comma builder
is the same as its statements; one difference found by the images (a word's low byte as `w1 & 0xFF` loads both bytes,
`(unsigned char) w1` one), so a field of bits 0-7 of a word is accessed as the byte.  Into dq0hub.json go the payloads
by offset (`msgs`: each form's fields, offset, bytes, bits, enum), each slot's width held to cc65's; `load ().payload
(kind, form)` and `load ().decode (bytes, o, kind, form)` read a message's fields by name, a one-bit field a boolean, an
enum's value its member's name.

**Every intent declared and switched.**  The 31 kinds after I_NONE, in 45 forms.  The UI: its intents go out by
`intend2 (kind, w1, w2)` and `intend (kind, a)`, slot by slot, which stay (their slots plain values: the meaning is the
kind's line in msgs.def); a packed slot is made by its packer (`intend2 (I_WAIT, t, I_WAIT_W2 (k, n, 1))`, the quiet
waits `I_WAIT_W2 (0, 0, 1)`, `I_STEP_W1 (d, keep)`, `I_TRACE_W1 (TRC_SMELL, h)`), and the hand-built messages by the
builder (s_work's I_PORTAL_SET) or the accessors (s_intend, shot, reach_go's and place_go's walk, search.c's searches,
pace.c's journey): 61 lines in main.c, search.c, pace.c, way.c, hthings.c, read.c, tend.c, blows.c, feed.c.  The
denizens (dq0d.c, 177 lines): every `im.SLOT =` is its kind's accessor, the steps' packed slots by their packers.  The
world (184 lines): the receivers in dq0w.c (intent, intent_few, intent_more, task_word, close_go, fight_go,
disengage_go, passed, travel_go, a trace's work), wbody.c (the denizens' ring), wport.c, wtake.c, wfetch.c, wfight.c,
wdig.c, wlimbs.c, outfit.c, wgame.c, wtrace.c, and the world's own I_WAIT (things.c) and I_ASKs.  Left as slots, by
design: `kind` and `who` (every message's), the generic senders' bodies (intend2's, post's, act's head is I_DEN's), and
a few clears of slots a form doesn't use (`m.w1 = m.w2 = 0`).

**The tests.**  The recorder (sim/lib/dq0meaning.js) records the intents as the UI's ring (the hub's iq) and the
denizens' (di) show them: each `{ line, ring, kind, who, ...its fields by name, raw: { a, b, w1, w2, t, x } }`, a
denizen's by its :den form if its kind has one; dq0Turns gives each turn its `intents`.  dq0longwait reads "wait 8
hours"'s I_WAIT (`tenths` 480, `how` WA_MIN, `task` LT_NONE), dq0tend "tend my wounds"'s (`task` 'LT_TEND', `quiet`).
The events' decode merges a declared payload too, so declaring the events needs no change there.

**No behaviour change:** the images came out byte-identical (bin/*, and every program and data file in obj/games; the
objects differ only in their debug lines, hub.h two lines longer), and the suite passes.

**Left** (ENGINE-CRITIQUE.md's §2, "Left of stage 2"): the events (E_*: forms chosen by a field's value, E_TASK's TE_*,
E_BEAT's WB_*, in place of dq0meaning.js's hand switch; their names clear of the raw slots the event records keep at the
top), then the referee's requests (struct req: multi-byte slots, a request's fields and its reply's apart).

<!-- DECISIONS.md:2495-2576 at d35194df -->
## The events' payloads, declared (2026-11-09, as built, for review: ENGINE-CRITIQUE.md's §2 and step 3, its second stage: the events)

The intents' mechanism (the entry above) carried over to the events (E_*), the world's news to the UI, the denizens
and the sound task, in the same struct msg.  What an event's slots mean often hangs on one of them: E_TASK's b and
w1's high byte are the enemy's entry for TE_ENGAGED, the light's kind for TE_LOW, the robber and how for TE_ROBBED, the
errand's outcome for TE_FETCHED; E_BEAT's high byte is a side, a heading and a gait, a thing's kind or a search's step
by its WB_*.  dq0meaning.js's hand switch (forty lines of `if (e.te === ...)`) was where that was written down.

**A selector, `FIELD==CONST[,CONST]`.**  A form of a kind (msgs.def's `KIND:FORM`, as the intents' :den forms) is chosen
by a field's value:

    E_TASK          b named@LT_NONE; w1 te:0-7@TE_STOPPED task:8-15@LT_NONE
    E_TASK:engaged  b by:0-5+1 quiet:6=TE_QUIET still:7=TE_STILL; w1 te:0-7==TE_ENGAGED,TE_FAR,TE_AWAY,TE_FIGHT@TE_STOPPED
                    task:8-15@LT_NONE
    E_TASK:robbed   b by+1; w1 te:0-7==TE_ROBBED@TE_STOPPED purse:8 caught:9 asleep:10

`==` and not `=`: a one-bit field's `=CONST` is already its constant (`quiet:6=TE_QUIET`), checked to be that bit; a
selector names the field's value.  The tests' decoder (hubgen's `load ().decode`) reads an event by the first form whose
selector its field matches, else by the kind's own form (E_TASK's: TE_NOT's named, and the rest).  One selector a form,
of the form's own fields (not an included one's), none on a kind's own form; its values the headers' names, held to the
field's bits.  In the JSON a form has `sel: [field, [values]]`; the C has no use for it (the code tests the field).

**Its accessors, `KIND_FORM_FIELD`.**  The intents' rule is a name the same field in every form of a kind; the events'
forms reuse names for different bits (E_TASK's `task` is w1's high byte, but TE_LOW's and TE_OUT's its low six bits, two
flags above; `by` is b's low six bits, or four, or all of it), and the tests read them by those names.  So a chosen
form's field is `KIND_FORM_FIELD` (`E_TASK_LIGHT_TASK`, `E_TASK_ROBBED_BY`), unless the kind's own form has it the same,
slot and bits (`E_TASK_TE`, `E_TASK_TASK`, `E_BEAT_WHOSE`); its packers and builder are the form's as before
(`E_TASK_LIGHT_W1`).  A selector of one value isn't an argument: its constant is in its place
(`E_TASK_ROBBED_W1 (purse, caught, asleep)`, `E_BEARING_SKY_W1 (refusal)`), so a sender can't build a form with another
form's value; of several, it is (`E_TASK_ENGAGED_W1 (TE_FAR, LT_NONE)`).

**Three more marks, for what the hand switch worked out.**  `!` a signed byte (E_LEDGE's rise: the C accessor
`(signed char) (m).w1`, the tests' value negative); `+K` a value stored plus K (an actor's entry plus 1: the C reads and
writes it as stored, the tests read it less K: `by`, the entry); `@A,B` and `@a/b/c`, names of several kinds (E_FIGHT's
out, `@O_MISS,FO_GONE,FO_BLED,FO_BOUND`: the referee's O_* or the world's FO_*; TE_BREAK's how and TE_FREE's none,
`@back/flight/run`, words for 0, 1, 2, where the headers have no names).

**Every event declared:** the 21 kinds after E_NONE (the critique's 23 counted E_NONE and NEVENTS), in 41 forms:
E_TASK in ten (its own; engaged, hurt, nofoe, break, free, light, robbed, fetched, begun), E_BEAT in nine (its own; feel,
met, hit, thing, limb, rise, swat, search), E_HARM:struck, E_DIG:notool, E_BEARING:sky.  The field names are the hand
switch's where the tests read them (te, task, by, still, quiet, lamp, seen, ft, beat, side, thing, fa, head, gait, de,
def, out, ff, how, feet ...); a square's x and y are `sx` and `sy`.  Not declared: E_PORTAL's a and b (a search's reach,
an opening's what and room), E_TASK's b for the forms not given (TE_NO_SHOT's why, a journey's place: `named` as
before), E_BEAT's high byte for the water's and the walk's own beats (WB_WATER, WB_POSED: `side`, as before).

**The tests' records keep the raw slots at the top (the choice the critique left open).**  The intents' records moved
them under `raw`; the events' tests read `e.a`, `e.b`, `e.w1`, `e.w2`, `e.t` in some forty places (a denizen's DE_* by
`e.a`, a dug square by `e.w1`, an event's time by `e.t`, a fall's whose by `e.b`), so the events keep them where they
were, and no field of an event is named as a slot is (kind, who, a, b, w1, w2, t, x, line, den: dq0meaning.js's layout
stops on one).  The hand switch was kept beside the declared decoder for one run of the suite, every field it gave held
to the new one's.  The differences: fields it set where they meant nothing (E_BEAT's fa, thing, head and gait on every
beat past WB_FEEL, whatever it was; E_HARM's stopped on a fall; E_DIG's feet for D_NO_TOOL, what it's into, now
`into`); `by`, which the fights' and the robberies' tests read, first named otherwise (named so again); and values the better for it (E_DEN's
`seen` and TE_NO_FOE's `shock` the byte, not a boolean; E_BEARING's `brg` named with a refusal in w1's high byte;
E_FIGHT's FO_BOUND named).  Three test reads changed for them (dq0dig's two of D_NO_TOOL, `into`, dq0succession's `!!seen`);
then the hand switch went: dq0meaning.js's decode is the slots and `H.decode`.

**Switched:** 239 lines.  The world's senders (155: dq0w.c's `event (kind, who, w1)` by the packers, `E_CLIMB_W1
(K_BEGUN, cl.need)`, `E_TASK_W1 (why, HUB->pc_task.kind)`, `E_BEAT_THING_W1 (WB_SPENT, upk)`, the evb of a form by its
packer, `evb = E_TASK_ENGAGED_B (e, 0, 1)`; the hand-built ones by the accessors, `E_DEN_SX (m) = x`, and the packers:
wport.c's, wbody.c's and wdig.c's E_DEN, wfight.c's E_FIGHT and TE_ROBBED, wfetch.c's TE_FETCHED, wlimbs.c's, wroom.inc's
WB_SEARCH, the E_NOISEs, E_LIGHTs, E_VIEWs, E_LEFT, E_TAKEN, wtime.c's sky).  The UI's receivers (60: main.c's events
and heard, s_came, s_limb_words, s_news, s_noise_told; blows.c's fight words, caps.c's den_heard, search.c's
search_said).  The denizens' (dq0d.c, 10: E_NOWAY's N_DEN, E_NOISE, E_DEN, E_FIGHT), the sound task's (dq0s.c, 13) and
the senses' light layer (light.c, 1).

**Left as they were, by design or by the code they'd cost:** an argument that is a word whole (`task_words (m.w1, m.b)`,
`beat_words (m.w1)`, `trap_words`, `harm_words`, `pose_words`, `water_words`, rs_tell's and told's w: their callees read
it by hand), a whole-word test (`m.w1 == N_SHUT`, `m->w1 != PO_NOTHING`, `m->w1 == BRG_LOST`), and the shapes cc65
codes otherwise: the low byte as `w1 & 0xFF` inside a `||` or a `?:` or as an argument (the accessor's cast is shorter
code, so the images would change: E_ARRIVE's A_UP or A_DOWN, dig_words' and climb_words' feet), a field between two
others unmasked (E_HARM's band, WB_HIT's fa, WB_FEEL's head, E_PORTAL's pk, TE_ROBBED's and TE_OUT's flags packed by
hand: the packer masks it), and E_ARRIVE's sender (its link and flags by conditions).  The tests read nothing new; the
recorder's events are what they were, by declaration.

**No behaviour change:** the images came out byte-identical (bin/*, and every program and data file in obj/games: 154
files; the .s files the same less their .dbg lines; dq0's .lbl differs in one function's local labels' numbers alone,
its addresses the same), the UI's main RAM as it was, and the suite passes.

**Left** (ENGINE-CRITIQUE.md's §2, "Left of stage 2"): the referee's requests (struct req: multi-byte slots, a request's
fields and its reply's apart), being done beside this.

<!-- DECISIONS.md:2577-2661 at d35194df -->
## The referee's requests' payloads, declared (2026-11-09, as built, for review: ENGINE-CRITIQUE.md's §2 and step 3, its second stage: the referee's requests)

The referee is asked in `struct req { RQH; actor, other; steps; a, b, c; w1, w2; x[32]; grade, result; text }`, and
answers in the same slots: what each meant, the request's and the reply's, was rs.h's comments, and the code at both
ends read `r->w2`, `r->x[18]`, `*(unsigned long*) (r->x + 4)` by hand.  This part does for the requests what the first
did for the intents, by the same file and the same generator, extended where requests need it.

**The same file, a second struct.**  msgs.def's `struct NAME` line already named the struct its kinds travel in; a
`struct req` line after the intents starts the requests, each slot checked against cc65's struct req (its width, an
array's bounds, its offsets in dq0hub.json).  Four things are new, all in msgs.def's syntax and hubgen.js's reading of
it, nothing in the parts the events will want:

    RS_HARM         a cause@HC_FALL; b ground@GR_FLOOR; c how; w1 feet  -> result caught; b band; w2 out; x effects
    RS_TIME         a about; b pc; w1 minutes; w2 bleeders; x[0] fighting; ...; x[16-17] spent; x[18] aff@AK_NONE ...
                    -> result died; b health@H_FINE; c bound; grade news; w1 dead; w2 out; x[4-7] next
    RS_ATTACK:thing c run; w2(flags) shy:0; x[0-3] resist[]
                    -> result(struck) stop:0 part:1 short:2 broke:3; a drop; b aspect@AS_BLUNT; c worst; w1 harm; w2 out

- **A reply after `->`**, a form of its own (`RS_HARM:reply`, `RS_ATTACK:thing_reply`): the referee writes its answer
  over the request's slots (RS_ATTACK's a is the aim asked and whose fumble it was answered; its w1 the range of a shot
  and the two healths back), so the two are declared apart, each checked for overlaps on its own, and the accessors of
  both are the kind's (`RS_HARM_FEET`, `RS_HARM_BAND`: one name, one slot, or the build stops).  A reply with no request
  before it is a form of a reply alone (`RS_TEST:swim -> grade; w1 squares; w2 breath`: a swim's test is asked as any
  test is, its reply its own).
- **Slots spanning bytes**: `x[4-7]` an array's elements spanned, a word for two bytes and a long for four, the accessor
  a cast of its first byte's address (`RS_TIME_NEXT (m)` is `(*(unsigned long*) ((m).x + 4))`, the cast rswant.h wrote
  by hand), an lvalue; any other width, or a field written `name[]`, is its bytes, the address (`RS_ATTACK_RESIST (*r)[k]`
  in the loop that fills them); `x` alone the array whole (a reply's struct rs_effect list, a new actor's struct
  rs_summary: `(const struct rs_effect*) RS_ATTACK_EFFECTS (*r)`).  Bit fields go in a span of four bytes at most.  The
  overlap check knows spans: `x[4-7]` and `x[5]` in one form stop the build.  In dq0hub.json a field's seventh
  element is the events' signed mark (1: their `!` byte, and a signed slot whole, a reply's grade) or 2, a span's bytes
  (`name[]`), the decoder giving its bytes; the eighth the events' `+K`; a span's offset its first element's, its size
  the element's times their count.
- **A slot alone is its field of its own name** (`steps`, `grade`): the reply's grade is a grade, a test's steps steps.
- **No builder for a request.**  A request is filled slot by slot around what the code computes (the world's blow: the
  aim, then the creature's heart, then the flags by the two bodies' positions, the missile's afterwards), and a builder
  sets its slots in the struct's order: the stores would move, and the images with them.  hubgen.js makes a builder only
  for a struct with a `who` (struct msg).

**Every kind declared.**  All 27 RS_* kinds: 17 with fields, the 10 not asked yet (RS_DROP_ACTOR, RS_TEST_MANY,
RS_REST, RS_INITIATIVE, RS_EXPERIENCE, RS_POWER, RS_EXPORT, RS_IMPORT, RS_RESTORE, RS_QUIT) a `-`; 60 forms, 35
requests and 25 replies.  The forms by purpose, cause and target: RS_TEST's `:dig` (b the material; the reply's rate and
the tool's wear), `:appraise` (b the craft, w1 the thing's number; the craft's mastery back), `:read` (the writing's
number), and the replies of a swim, the bearings (the drift) and drawing (Drawing opened); RS_ATTACK's `:missile` (b
the band, c the kind shot, w1 the squares), `:thing` (at RS_THING: the blow's run, the thing's resistances; the harm it
takes, the part's hurt), `:rise` (O_RISE: still down); RS_HARM's `:strike` (HC_STRIKE, HC_FIRE) and `:struck` (the
striker's own part); RS_TREAT's `:done`.  Packed slots: RS_ATTACK's flags (w2, `shy:0` beside the RS_* and MS_* flags
the headers name), its reply's two healths (w1), RS_MORALE's leaders (w2: its band's head, its faction's), RS_TIME's
meal (x[14]: meals, a ration begun) and drink (x[15]: pints, a remedy taken, foul water), RS_DICE's a (seeded).  The
names are what rs.h's comments said: RS_TIME's `minutes`, `bleeders`, `fighting`, `drowned`, `aff`, `dose`, `air`,
`flame`; its reply's `died`, `health`, `bound`, `news`, `dead`, `out`, `next`.

**Every sender and receiver switched.**  The senders, hub_ask's callers: the world (dq0w.c's fall, a dig's test and its
query, the roof's and the air's harm, a swim's reply, the wield, the engage contest, a grope, a map drawn; things.c's
tests, wblow.c's blow at a thing, wfight.c's blow, wdig.c, wlimbs.c's RS_EQUIP and the harm's effects, wtend.c's
RS_TIME, outfit.c's RS_OUTFIT, wrope2.c, wrope3.c, workpart.h's harm's reply), the UI (main.c's character loaded, the
hello, the things to hand; read.c, tend.c, way.c, feed.c), the denizens (dq0d.c: a new actor, the stealth and notice
tests, the sway's tests, morale).  The receivers: the HarnMaster referee (dq0harn.c's answer and its own tests, fight.c,
missile.c, gear.c, heal.c, sheet.c, afflict.c, want.c, outfit.c) with the rulesets' shared rshouse.h and rswant.h, the
null ruleset's (dq0null.c), and the referee's unit tests (tests/ram/t_rules: RS_DICE's faces and sides, RS_LOAD_PC's
reply; its other requests are built from rules.txt's slot names, as data).  642 lines: the world's 147, the UI's 32,
the denizens' 42, the HarnMaster referee's 273 (rshouse.h and rswant.h with it), the null's 138, t_rules's 10.  Left as
slots, by design: `kind`, `actor`,
`other`, `text` (every request's: who asks about whom), the generic senders' bodies (the world's `ask_w (kind, a, b)`,
the UI's `ask_b`, the denizens' `ask`, workpart.h's `harm (a, b, c, w1)` for a fall or a strike alike), and the few
places an accessor would change the code: the minutes copied byte by byte (`x[16] | x[17] << 8`, rswant.h and
dq0null.c), dq0harn.c's swim using `result` as scratch, the refusals' `result = 1` and the defaults' `0xFF`.

**The tests.**  The recorder (sim/lib/dq0meaning.js) reads a request as it's posted (its state REQ_POSTED, every slot
written) and keeps its fields by name; each reply's record gains `rs` (its kind's name), `ask` (the request as it was
asked, its kind's form) and `reply` (its `:reply` form's), beside the raw slots the tests already read.  dq0well reads
the jump's fall as `r.ask.cause === 'HC_FALL'`, dq0swim the swimmer's time as `r.ask.about`, dq0hardthrow a throw's
steps as asked (`r.ask.steps`, not the reply's slot).  A form by a field's value (a dig's `mat`, read as RS_TEST's
`how`) is not chosen yet: the events' selector (`==`) would do it, but it renames a chosen form's own accessors
(RS_TEST_MAT to RS_TEST_DIG_MAT, RS_HARM_IMPACT to the strike's and the struck part's), and RS_ATTACK's missile (a bit
of its flags) and RS_TREAT's done (a 1) have no constant of the headers' to be chosen by.

**No behaviour change:** the images came out byte-identical (bin/*, every program and data file in obj/games, and
obj/tests/t_rules.hyx), and the suite passes.  The accessors were held to the code they replace: `((*r).b)` is `r->b`,
`(*(unsigned long*) ((*r).x + 4))` the hand cast, `&RS_TIME_NEXT (*r)` the address memcpy took, `((*r).result & 0x8)`
`r->result & 8`; RS_MORALE's packer `(l1) | (unsigned) (l2) << 8` the same as the hand's `l1 | l2 << 8`.

**Left** (ENGINE-CRITIQUE.md's §2, "Left of stage 2"): the requests' forms chosen by a field's value, a pass of its
own (the accessors it renames, the constants it wants).

<!-- DECISIONS.md:2662-2694 at d35194df -->
## Wear for what's worn (2026-11-09, as built, for review: the user's, "wear" restricted to articles that can be worn)

"Wear" was equip's: commands.verbs' `wear = don`, `don` the verb of `VB_EQUIP_OBJECT` with aux `IO_DON`, the world's
`IO_DON` (things.c's `don`): a thing of a kind that's worn (the catalogue's `K_WEAR`: clothes, armour, a cloak, a
backpack, a belt, a bandolier, a shield or a bow slung, a ring, a brooch, a necklace, a mask) put on; one that isn't,
but is held (`K_HAND`: a sword, a torch, a lantern), taken in hand instead ("wear the sword", "You take the sword in
your right hand.", or "You hold the sword in your right hand already."); anything else refused, "You can't wear the
sack." (the catalogue's `SA_NO_WEAR`).  "Wear what? [...]" (letters.c's) already offered only the `K_WEAR` kinds not
worn.  What's built:

- **Wear is a verb of its own, told apart by its aux.**  commands.verbs' `wear thing=VB_EQUIP_OBJECT unsaid=last
  aux=IO_DON_WORN`, `wear = wear`; equip, don, put on, ready, wield and brandish as they were.  `IO_DON_WORN` (items.h)
  is no request of the world's: hthings.c's `H_HANDLE` sends it as `IO_DON` with `to` `DN_WORN`, and the world's `don`
  refuses a thing that isn't worn whether or not it's held: "You can't wear the sword.", "You can't wear the lit
  torch.".  letters.c's "Wear what?" takes `IO_DON_WORN` as `IO_DON`.
- **Why the aux, not an action or a manner.**  The aux is the verb's own value and already tells the handling verbs
  apart within one action: wield is `VB_EQUIP_OBJECT` too, told from equip by `IO_HOLD`.  A `VB_WEAR_OBJECT` would be
  a row of the dispatch table, a line of commands.actions, the grammar's, the corpus's and the reference compiler's,
  and main.c's hand phrase and letters.c's tests of the action, for the same handler and the same request; the corpus
  ("wear the mail hauberk", `VB_EQUIP_OBJECT`) stays true.  A manner is how a thing's done (`MN_*`, the adverbs'), and
  no verb sets one.  `IO_WEAR` was taken (a tool's key worn by a body), so the name is `IO_DON_WORN`.
- **The refusal is the catalogue's, as it was.**  "You can't wear the %." (kinds.c's `said`, `SA_NO_WEAR`) is the
  world's, said after the thing's found by name, as the sack's always was; refuse.c's `said[]` and `say_no` are the
  line's refusals, before the world's asked, by a word, and whether a thing's worn is its kind's, which only the world
  knows of a thing named (one lying about too).  No new words.
- **Left as they are, for review:** "don" and "put on" (equip's still: a sword donned is taken in hand); "Equip
  what?" offers only what's worn, as before.  If "don" should be wear's, it's one line: `don = wear`.
- **Room:** the UI's main RAM 2,307 bytes free, as it was (the changes in overlays and parts).  The world's
  catalogue bank (its code at most 6,400 bytes, into the outfits' index) was 6,399 and went 6 over; take_from's and
  doff's tail (carried, into the pack or a hand, its words) made one routine, `carried`: 6,371.
- **Test:** dq0wearonly (a sword and the torch in hand, a sack: refused; "Wear what? [k]", the cloak's letter alone;
  the cloak put on by its letter; equip and wield the sword, taken in hand).

<!-- DECISIONS.md:2695-2713 at d35194df -->
## The verbs not done yet: the rows, the aliases and the powers' refusals (2026-11-09, as built, for review: the plan for the 50 H_NOT_YET verbs, its batch B0)

Fifty verbs' rows in the dispatch table (actions.c) said `H_NOT_YET`, "You can't ... in this game yet."  The user's rule: each verb is decided on its own, and each should do something in the world rather than say "not yet".  This is the shared groundwork, so the later batches (B1 the fights, B2 the noise, B3 the minds, B4 the things, B5 the self and the powers, B6 moving, B7 the line; grapple its own) can each work in files of their own.  What's built:

- **The rows seen to before the table** say so: the target's three (resolve.c's) and read and combine (the puzzle's, theme.c's: read keeps its row's refusals, asked before the puzzle) are `H_NOTHING`, never reached; "sequence" too (parts.c splits the line first).  The pace's verbs are pace.c's when it knows the words; a line that comes past it now does something: a step that names a thing ("step onto the stairs") goes to it (`H_GO`); "pace" alone, a pace it doesn't know ("double"), and "status" with more words than pace.c takes are a new `H_STATUS` in hself.c, pace.c's "status" words (its described (0)) by the same path.
- **"sneak north" moves**: pace.c set the creep and stopped there; now the sneak is one of the moves it lets on to the command (as "run north" was), so the pace is set and told, then the row's `H_GO` goes.  "sneak" alone still sets the pace alone.
- **The aliases are the parser's** (commands.verbs' [words], the verb a word says): "negotiate" is persuade's, "sense a lie" examine's, "activate" use's; the three verb lines that made `VB_NEGOTIATE`, `VB_JUDGE_SINCERITY` and `VB_ACTIVATE` are gone.  Their rows stay, for any line that still makes them, pointing where the alias goes (`H_SWAY` with persuade's `SW_PERSUADE` as its request, `H_EXAMINE`, `H_USE`).  "negotiate with the rat" is a persuasion now ("The giant rat is an animal: words won't reach it ..."); "sense a lie from the rat" a look at it ("The giant rat seems wary of you."); "activate the torch" the use's own words ("Use what, and on what? ...", the puzzle's).  Not added to commands.aliases (the player's listed shortcuts): [words] is enough for the parser, and the listing stays as it was.
- **The powers refused, true to the character** (the user's wording), as data: refuse.c's `said[]` has three refusals more, said by `say_no` from a new `H_POWERS` (hself.c), the refusal the row's request byte (`ROWP`'s, as the door verbs' `PA_*`: `A_NO_MAGIC`, `A_NO_PSI`, `A_NO_GODS`):
  - spells (cast, memorize, unmemorize, learn a spell, research, dispel, counter, release): "You've no magic: you never studied the convocations."
  - psionics (a talent, possess, link minds): "You reach for it; nothing stirs."
  - invocations (invoke, learn an invocation, a call for intervention): "You know no invocations of Peoni's.", the character's own deity; with none, "You know no invocations." (the refusal's nameless form, as "You see no %s here." has).
- **The deity's name is the referee's**: `RS_DESCRIBE` with b `RD_DEITY` (5) answers the name of the deity the character worships (HarnMaster's: its sheet's `att dty`, hmchar's choice, `harn_deity_name` through part 2's stub; "" none), the null ruleset "" always.  A call for intervention by the world's pantheon's name ("beseech Imma") would be refused by the character's own deity's: the refusal is about what the character knows.  For review: whether a call for intervention should be refused so, or be prayer's (B5).  Not tested: in the test's world "beseech ugrath" isn't understood (the parser's, the world's deities' words) and "call on X" is the names' ("That name is taken ..."), both as before.
- **Stubs for the batches**: each verb to come has a handler of its own (dq0.h's `H_*`, its row pointing at it), a function in its file by meaning that says "not yet" (`unknown (U_YET)`, the words as before) till its batch replaces it: hthings.c `H_USE` (use, and activate), `H_PUSH`, `H_PULL`, `H_TOUCH`, `H_TASTE` (B4); hplaces.c `H_KNOCK` (B2); hmove.c `H_SWIM`, `H_FOLLOW` (B6); hdens.c `H_CHARGE`, `H_THROW_WAY`, `H_ENGAGE`, `H_SPLIT` (B1), `H_SAY` (B2), `H_WAKE`, `H_TAUNT`, `H_ORATE` (B3); hself.c `H_PRETEND` (B3), `H_PRAY` (pray and pray for), `H_FAST` (B5), `H_AGAIN` (B7).  Grapple's row is left `H_NOT_YET`, the grappling's own.
- **The shared words**: hub.h's `I_NOISE` (a noise made: a its loudness, b what it is, w1 w2 its square, 0 the character's; declared in msgs.def; the world does nothing with it yet, its E_READY as any intent's), `SW_WAKE` and `SW_TAUNT` (after `SW_BEFRIEND`, so `SW_ATTACK` ... `SW_GIVE` ... move up two: dq0d.c's `cap[]` sized to `SW_TAUNT`, and its `s_sway` answers both `SR_CANT` till B3 builds them, never the attitude's table past its end); rs.h's `P_ACT` (Acting), `P_ORATE` (Oratory), `P_RITUAL` (the deity's Ritual), before `P_THEME`, with dq0harn.c's purpose table (`HARN_ACT`, `HARN_ORA`; Ritual by the sheet's deity, `HARN_RAGR` + it, a critical failure with none).
- **The count**: of the fifty, 28 are done (the 14 powers refused; the 11 rows seen to before the table or now somewhere, status and sneak with them; the 3 aliases), 21 are stubs for their batches, and grapple's row is its own.
- **Room**: part 2 (the handlers) has 237 bytes left of 659 (the stubs and the refusals' asking: some 17 bytes a stub), overlay 24 (the refusals) 132 of 292, part 4 (pace.c) 13 of 17; dq0's main RAM unchanged (2,307 bytes free), dq0harn's 2,203 (the stub and the Ritual's test), dq0null's 5,453.
- **The corpus**: no lines added.  commandcorpus.js checks the corpus's form and that each action has a case; its expected actions are the grammar's (commands.ebnf), not the meaning commands.verbs gives, so the aliases change nothing there and the rows-only verbs and the powers have their cases already.
- **Test dq0notyet**: the refusals' words (a spell, a talent, an invocation invoked and learnt, the deity's name and none), the aliases (activate, negotiate, sense a lie), "status" and "pace", and "sneak north" a creep and a move.

<!-- DECISIONS.md:2714-2789 at d35194df -->
## Grappling (2026-11-09, as built, for review: the verbs plan's B1g, the user's "full HarnMaster grappling")

The user, of the 50 verbs the game answered "not in this game yet": grappling gets HarnMaster's full rules, "grapple
attacks, holds, throws, pins, chokes, breaking free", by the Unarmed skill, the referee gaining them.  VB_GRAPPLE_OBJECT
was an H_NOT_YET row.  HOUSE.md's grappling has the rules as built; this is how, and where it's simpler than the book.

**How it's carried.**  A grapple is a blow of another kind, so it rides the fight machinery whole: the UI sends
I_ATTACK with its b `AIM_GRAPPLE | GM_*` (the move: hold, throw, pin, choke; the world's own GM_LETGO and GM_BREAK);
the world strikes it as a blow (wfight.c's blow: the same slot, ring, E_FIGHT, effects, morale, senselessness, death),
asking the referee RS_ATTACK with RS_GRAPPLE in its flags and the move in its a (msgs.def's new form,
`RS_ATTACK:grapple a move; c(hold) choked:0-6 pinned:7=GH_PIN`); the reply is a blow's, its outcome one of nine new
O_* (O_HOLD ... O_STRUGGLES, and the world's O_LETGO), so the UI words it (blows.c's grip_words, overlay 19) and the
denizens and the sound task take it as they take a blow.  The character fights it on as it fights a blow, round after
round, till its move is done (held, thrown, pinned, choked senseless: then its fight ends, the world's pc_cut), a hold
first if it doesn't hold that one already.  While it holds one, the character doesn't fight on by itself (the world's
fighting on, begun by a hostile one striking at it, isn't begun): the next move, a throw, a pin, a choke, a blow, a
let go, is the player's, or a hold would be beaten out of its hands by its own fists.  No new request kind, no new handler id (the row is H_FIGHT's, the
handler's first test the verb: the shared files' edits kept small, B0's in parallel).

**Who holds whom** is the world's (wfight.c's grip: the actor plus 1, GH_PIN; the rounds choked): one hold, the
character's.  A held denizen's intent (wbody.c's, through part 15's grip_held: part 14 has no room) is its try to
break free, its round, if it's anything but a wait or (not pinned) a blow: so it never walks away, and whatever its
mind wanted (to flee, to keep off, to rise) becomes the struggle.  Held but not pinned, its blows are four steps the
worse.  Let go: as either is knocked senseless or dies (the blow's, the bleeding's), as the held one leaves or is found
apart (its next intent), as the character moves off (the world's intent: a walk, a step, a dash, a climb, a take, a
close-in, a disengage, travel: told, "You let go of the giant rat."), at "let go".

**The referee** (dq0harn's grapple.c, a new overlay, the ninth: the others full; its two calls of fight.c's through
new stubs) tests each move as an opposed test, and the null ruleset answers each with a flat test.

**Where it's simpler than the book, and why:**

- **The book's grappling tables aren't in the repo** (no rulebook text: only the house's notes), so the moves are
  built from HarnMaster's grappling as the brief gives it, each an opposed test in the book's levels (CS, MS, MF, CF:
  the attacker's success must beat the defender's level; the defender has the ties) rather than a grappling table
  of outcomes.  Why: the opposed test is the book's general rule for one skill against another (Skills 5), and it
  needs no table the repo doesn't hold.
- **A hold is Unarmed against Unarmed or Dodge, the better;** a throw, a pin or a choke Unarmed against Unarmed or
  Strength (five times), the better; breaking free the held one's Unarmed or Strength against the holder's Unarmed.
  The book weighs Strength in grappling; five times it is the house's way of making an attribute a skill's peer (as
  the house's Stealth, a dig's).
- **Size two steps a difference,** both ways (the bigger the better), where the book bars throwing a much bigger foe
  outright.  Why: no table of what can be thrown; two steps keeps a bear hard and a rat easy.
- **A creature's Unarmed is its own attack's mastery** (its sheet's first weapon, its penalty off it): the bestiary
  keeps no Unarmed.  So a sword-armed bandit grapples at its sword's skill.
- **The throw's fall is two dice of blunt impact** (the sizes' difference a point a die) at a place the middle's strike
  table gives, its armour off it, as a blow's strike is; the book's fall impact by height isn't used (a throw is a
  fall from standing, too short for the falling table's bands).  The hold is let go as it's thrown (the thrower
  doesn't fall with it), so a thrown foe's next turn is its getting up, the book's Rise.
- **A choke is an Endurance test** (three times it) each round it's choked, ten less for each round before in this
  hold, a pinned one two steps the easier to choke; failed, it's knocked senseless as a blow's shock does (the book's
  shock rolls till it comes round, a minute at least: an unhurt one would come round the next round).  The book's fatigue from a choke isn't kept: the creatures' fatigue is tire.c's,
  by the clock, and a level more a round would knock it out no sooner.  Bones that move (BF_MINDLESS) can't be
  choked.
- **A pin** lays it down and holds it there; pinned, it can't strike (its blow too is a try to break free), four steps
  the worse to break free; the book's further holds from a pin (a lock, a choke's ease) are only the choke's two steps.
- **Held, its blows four steps the worse;** its dodge as ever.  The book's penalties for the held are by the hold;
  four steps is the house's one figure.
- **One hold at a time,** the character's: the denizens don't grapple the character (the brief's optional part, left:
  their minds would need a move to choose, and the character's intents a break-free of their own); a second hold on
  another lets the first go.
- **Grappling needs the foe beside the character** (out of reach: "go up to it first"), no close-in as a blow has: the
  grammar's own note ("the target must be engaged"), the book's a melee attack at one engaged.
- **A hold isn't kept in a saved game** (the world's own, not the save's): a game kept and taken up again has no hold.
- **The words "grapple", "wrestle", "tackle", "pin", "choke", "strangle", "throttle" are one verb** (commands.verbs'
  grapple), its move read from the line's first word (blows.c's grapple: p a pin; c, s and th a choke; t a throw;
  else a hold), as commands.aliases says a manoeuvre is: dq0c's records were full (255: five verbs made 258).  The
  grammar (commands.ebnf) gained "let go [of X]", "strangle" and "throttle"; sim/tools/commandcompile.js the same.  "Hold
  the goblin" and "throw the goblin" are a grapple when a denizen of that name is beside the character (else a thing
  held, a thing thrown, as ever); "let go" ("let go of the goblin") a verb of two words.  "Release the goblin" stays
  VB_RELEASE_PRINCIPLE's (a principle released, the magic's), its row B5's.

**The images** grew by the overlay (dq0harn.ovl's ninth bank), two of the UI's stubs, two of the referee's; the world's
part 15 the rest (its room's).  The tests: dq0grapple (a hold, a walk away failing, breaking free, a throw, a pin, a
choke to senselessness, "let go"); dq0rules' grappling cases; dq0parse's "grapple the rat" no longer "not yet".
