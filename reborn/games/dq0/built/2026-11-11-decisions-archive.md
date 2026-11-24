# DQ0: the decisions archive, 2026-11-11

The agents' accounts of what they built, moved out of DECISIONS.md on 2026-11-24 (branch docs2) with their text
unchanged, as AGENTS.md's Records rule asks: DECISIONS.md holds the user's words only. Each section left a one-line
pointer there; where a section held the user's words too, those stay in DECISIONS.md, quoted, and the whole section
is here. Each section is marked with its lines in DECISIONS.md as at d35194df.

<!-- DECISIONS.md:3142-3199 at d35194df -->
## The UI's room won back (2026-11-11, as built, for review: ENGINE-CRITIQUE.md's §1)

The UI (games/dq0) was at the edge: its main RAM 2,069 bytes free against the floor of 2,048, part 2 (the commands'
handlers) 102 bytes from the swap area's 4,352, its overlay banks some 4K free in pieces of 500 at most.  The ask: at
least 3,000 bytes of main RAM, a margin in part 2, and room in the banks for a new part of 3-4K.  Nothing the game
does or says changes; what's built:

- **The overlays' stubs 6 bytes, not 11** (bank.s's OVFN and its new ovcall): a stub is `jsr ovcall` and its two
  numbers after it (the overlay, the function's address), as the swapped parts' stubs have been since 2026-11-04;
  ovcall reads them off the return address, puts the overlay in (ovl_in's work, done in it, not called), calls the
  function, and puts the bank back (ovl_out), its arguments in .A, .X and its value as they were.  93 stubs, some 465
  bytes, less ovcall's 50.  The cost: some 30 cycles a call more (a stub's work was some 60 before, its function's own
  aside); with ovl_in called from ovcall it was 60 more, and the start at seed 123456789 took 5M cycles more, a sign how
  many calls there are.  The same in the referee's (dq0harn, 52 stubs) and the maker's (dq0m, 23) bank.s.
- **The status line out of the main RAM, into a 27th overlay bank** (dq0.cfg's OV27, filled; bank.s's OVMAX 27; dq0.h's
  ovbank[27]): main.c's status is o_status (its stub status, called as ever), term.c's o_t_status beside it, and the
  line as shown and as it's to be (sl, sn: 256 bytes) the bank's own data, last in it (OV27BSS: the overlays' check,
  dq0OvlBanks, stops there, as at OV22BSS).  An overlay's code can't read the hub in the window, so o_status reads its
  few bytes through hub_mem (theme.c's way, and bar_text's already); its left side is the character's name itself, not
  a copy (o_t_status is in the same bank, so the name's literal in it reads right); its right side stays in the main
  RAM (bar_text, overlay 3's, writes it).  A side effect, for review: called from overlay 3 (a pause, task.c's o_meta)
  the old status read the hub's fields from the window as it was, overlay 3's bytes, till the next second's redraw;
  through hub_mem it reads them right.  s_t_init's "draw it whole" is t_status (0, 0).
- **Buffers that needn't be the main RAM's:** s_size's 100 (the console's settings read as the screen's set up) on
  the C stack (called from the main loop and the start only, shallow); start's path (56) and hub_spawn's arguments
  (64) part 1's own (`bss-name` SW1RODATA: nothing else is swapped in while they're used; part 1 207 bytes free to 94).
  (character's two, part 7's, can't: it calls hub_run and t_init, part 1's, with them.)
- **Part 2's room:** the climb (s_climb, 229 bytes: hplaces.c, part 24's till stage 3) and the powers refused
  (s_powers, 73: hself.c) are part 24's, the commands seen to less often, through their stubs.  A climb or a spell
  refused swaps part 24 in and part 2 back (some 50,000 cycles, under a fiftieth of a second at 3.58 MHz); none of the
  timed commands (dq0answer's, the letters' tests') is either.
- **Measured** (the build's room table):

  | | before | after |
  |---|---|---|
  | dq0's main RAM | 2,069 | 3,378 |
  | part 2 | 102 | 404 |
  | part 24 | 1,520 | 1,218 |
  | part 1 | 207 | 94 |
  | overlay 16 (o_t_status's till now) | 842 | 1,179 |
  | overlay 27 (new) | - | 7,032 |
  | the overlays' free bytes, all told | 9,834 | 17,090 |
  | dq0harn's main RAM | 2,135 | 2,341 |
  | dq0m's main RAM | 2,879 | 2,940 |

  The bank packer placed some parts' images anew as the banks changed (the room table's "parts not in parts.txt's
  banks"), as it does.  The world (2,336), the denizens (2,184) and the senses (3,122) have no overlay stubs; their
  room is unchanged.
- **Not done, for later:** the swap area (4,352) is the biggest part's, part 4's (s_pace_cmd, one piece of 4,339, its
  pointer argument pinning it), so it can't shrink without splitting that function by hand; main.c's events and main
  loop (2,240 bytes) read the hub at every turn and stay; names.c's helpers shared by parts 5 and 6 (some 700) would
  want part-local copies (§1.3) and part 5 has 113 bytes free.
- **The start** (dq0starttime): seed 1 234.7M cycles to the first prompt, its budget 235M; seed 123456789 310.7M, its
  budget 315M.  The 27th bank's bytes are read in as it starts, and the stubs' calls cost a little more: seed 1 is
  0.3M from its budget, so the next bank added (or much more code in this one) will want the budget raised, which the
  user's said is fine (the start's time isn't a concern).
- **Tested:** all 442 tests pass.

<!-- DECISIONS.md:3200-3295 at d35194df -->
## Narration as data, stage 1 (2026-11-11, as built, for review: ENGINE-CRITIQUE.md's §3.2 and step 5, its first stage)

The words of what came of an action were written by hand, a function for each feature (33 of them), each its own
switch, its own strings and its own state.  This stage builds the generic mechanism, the critique's outcome event, one
renderer and a phrase table, and moves the first five wording functions onto it.  The words the player sees are the
same, letter for letter.

- **The outcome event (msgs.def's E_OUTCOME).**  Not a new kind but a head, as I_DEN is the denizens' intents': the
  outcome events include it (`E_DIG < E_OUTCOME; ...`), and the action is the event's kind.  Its fields: `who` the
  actor (the struct's own); w1's high byte the outcome, its low byte n1; b n2; x[0-1] the thing done to (obj) and x[2-3]
  the thing done with (instr), as handles (a thing's id, 0 none).  This is where E_DIG, E_CLIMB and E_TRAP already kept
  their outcome and their number, so those three needed nothing from the world; E_PORTAL's w1 was the other way round
  (PO_* low, the portal's kind high) and is now `pk:0-6 shows:7 po:8-15` (hub.h's PO_SHOWS 0x80, not 0x8000; its
  senders: dq0w.c's three and its revealed, workpart.h's tail and its told's denizen branch (`wk.out = w1 >> 8`),
  wroom.c's and wtrap.c's searches).  A kind names the head's fields its own way: hubgen.js lets a form's own field take
  the place of an included one in the same slot and within its bits (E_DIG's `how` is its outcome and `feet` its n1;
  E_TRAP's tk, tt and touch split n1; E_PORTAL's b is `room`, its search form's `touch`), so the tests read them by the
  names they always have (dq0dig's `how`, dq0portc's `po` and `pk`) and no test changed.  The JSON's new `outcome` lists
  the kinds that include the head.  E_PORTAL's a, b and w2 are declared now too (an opening's `what`, `lit`, `room`,
  `far`; a search's `reach` and `touch`, a form by its po).  **Choice, for review:** the action is the kind, not a byte
  of its own in a single E_OUTCOME kind.  That keeps the tests, the denizens' and the sound task's switches as they were,
  and leaves the struct's bytes for the handles; a new action is a kind (an enum member in hub.h) and a msgs.def line.
  **The handles are always 0 so far:** the world's dig works with the referee's tool names, not item ids, and a portal
  has no handle yet (COMMANDS-CRITIQUE.md's §4.8: features lettered later); the UI gives the words for them (below).
- **The phrase table (games/dq0/said.txt), made by the build** (games/dq0/saidgen.js, called by build.js's hubNumbers
  after hubgen.js, into obj/gen/dq0said.h; made again only when said.txt, saidgen.js or the headers' numbers change).
  A row is `KIND.OUTCOME[COND] "text"`: `DIG.begun`, `PORTAL.opened[pk=PK_WINDOW,PK_SLIT]`; the outcome's name is its
  enum member's (msgs.def's `@D_BEGUN` gives the prefix); a kind's own outcomes, rows the UI chooses itself, are declared
  (`own CLIMB rises falls`: SAID_CLIMB_RISES, a ledge come to).  `+"..."` says on after what's said, else the row is a
  line of its own (msg's); `once` says it as a look's sentences are.  In the text, in braces: `{feet}` a field by its
  msgs.def name (`far*SQ_FT`, `feet-121`, `feet>120`), `{feet?a|b|c}` a choice by its value, `{obj}` `{instr}` `{text}`
  the UI's words (capitalised `{Obj}`; `{instr?a|b}` by whether one's given), `{actor}` `{Actor}` `{s}` "you" or the
  denizen and the verb's ending, `{arg?...}` the UI's choice, `{a}` "a" or "an" by the word after it, `{rel F}` a
  heading from the way the character faces, `{room F}` `{light}` the theme's names, `{trap F G}` a trap's name, `{F!...}`
  said only as F changes (a dig's feet to go), `{forget}`, `{nl}`, and `{@NAME}` a piece (`@reveal`, "..., revealing a
  passage, 30 feet of it", the four openings' and the windows' rows).  The critique's `%`-names became braces: a
  template's letters and its codes are told apart without escapes, and choices nest.  123 rows, 11 pieces.
- **Packed:** a kind's rows its outcome, a condition (a field and the mask of its values), a flag and its text, the
  text's codes a byte each (a field by its place in a table of its byte, shift, mask and operation, so the renderer
  reads any kind's fields without knowing the kind), its pieces apart and called; and the 26 words most said (" the ",
  "portcullis", "shutters", " already" ...), chosen by the build by the bytes they save, a byte each.  7,107 bytes
  without the words, 5,153 with them: the overlay's room, and the bytes read in as the game starts.
- **One renderer (games/dq0/said.c, overlay 27: outcome_words).**  It finds the kind's row for the event's outcome (the
  first whose condition holds, then the one with none; none, nothing said), writes the text into lb, and says it with
  say, a `{nl}` as msg would, or with lb_said for a `once` row (lbsaid.s's no-sentence-twice rule a row's flag; no row
  moved yet wants it).  The UI's words for a row are `said_s` (obj, instr, text) and its choice `said_n`, in the main
  RAM, set by the caller; a field's last value for `{F!...}` is the main RAM's too (4 bytes, zero as the game starts, as
  dig_words' dig_feet was).  What's worked out for a row stays with its caller, the part that's truly the feature's own:
  main.c's s_news sets the dig's tool (dug_with) and the tool newly taken up (dig_with, "bare hands" its hands now, set
  in s_dig_used), and for a dig that won't begin which of six reasons (said_n, by what it carries: s_dig_used, part 14,
  which already refreshed the tools' list for it) and the better tool's name; a soft thing's name for a blow (PO_SOFT);
  a ledge's rise made a CLIMB row (`rises`, `falls`).  E_TRAP's words are said from the main loop as trap_words' were
  (untarget set there).
- **Moved and deleted:** dig_words (intro.c, overlay 19), climb_words (intro.c, overlay 12), portal_words (intro.c,
  overlay 10), fixt_words and its shows (fixt.c, overlay 4), trap_words (fixt.c, overlay 10): five of the 33, their stubs
  gone (dq0.h's CW_RISES and CW_FALLS too).  intro.c keeps the title and harm_words; fixt.c the portal looked at and a
  denizen in the way.  **The world's told copies:** none could go yet.  Each sends kinds not moved: workpart.h's (in five
  parts) sends E_NOISE, E_HARM and E_BEAT besides E_PORTAL and E_TRAP; wdig.c's and wport.c's are E_DEN (a denizen's
  digging, its door work: "denizens worded alike", stage 2), wfetch.c's and wshoot.c's (with told68) E_TASK, wfight.c's
  E_FIGHT.  Their only change is E_PORTAL's w1.
- **Room.** Overlay 27 is the renderer's and its table's: said.c some 2,400 bytes of code and the table 5,500, 282 bytes
  free.  The status line (main.c's o_status, term.c's o_t_status and its two 128-byte copies) moved from it to overlay
  19, into the room dig_words left (OV19BSS, its data last, dq0.cfg's; the stubs' overlay 19): **for review**, it was
  put in 27 this morning; 27 is the narration's now, for the stages after this.  The build's room table:

  | | before | after |
  |---|---|---|
  | dq0's main RAM | 3,378 | 3,399 |
  | overlay 27 | 7,032 | 282 |
  | overlay 19 | 297 | 1,452 |
  | overlay 4 | 54 | 594 |
  | overlay 10 | 304 | 49 (part 18's image put there by the packer) |
  | overlay 12 | 1,263 | 236 (part 17's image put there) |
  | the overlays' free bytes, all told | 17,090 | 18,138 |
  | part 14 (s_dig_used) | 1,821 | 1,218 |
  | part 11 (s_news) | 3,259 | 3,270 |
  | dq0w's main RAM | 2,336 | 2,336 |
  | dq0w's part 8 | 3 | 0 (E_PORTAL's packing, a few bytes in a few parts) |

  The packer placed some parts' images anew as the banks' room changed (20 and 25 have 2,898 and 5,111 free now).
- **The start** (dq0starttime): seed 1 234.1M cycles (234.7M before, budget 235M), seed 123456789 311.0M (310.7M,
  budget 315M): the words packed, the bytes read in as it starts fewer than the five functions'.
- **Checked:** the suite's output kept test by test before the change (a temporary dump in sim/test.js, not committed)
  and after it, its sentences compared: the same, but where the status line's redraws fall.
- **Left (stage 2 and on):** the other 28 wording functions (task_words, beat_words, harm_words, limb_words,
  throw_words, fight_words, shot_words, water_words, felt_words, sky_words ...) and the 277 inline strings; the world's
  told copies as their kinds move; SA_'s refusals as rows; the first words (D17) from a command's *begun* row; the
  denizens' actions by the same rows with the actor swapped (E_DEN's DE_* as outcomes: `{Actor} dig{s} into the wall.`);
  the sound task's event-to-sound mapping a column of the table (dq0s.c's world_sound); the handles filled (a dig's tool
  by its item id, a portal's once features have them) and `{obj}` worded from them.  Overlay 27's 282 bytes won't take
  much more: the next kinds want the renderer's code smaller (it's cc65's C; in assembly a third of it) or the table
  in a bank of its own, read through hub_mem.
- **Tested:** all 442 tests pass.  Every dq0 test's output before and after: no sentence of the five functions' said
  differently (the only sentences new or gone are the real-time fights' rolls in dq0antidote and dq0denpace, which go
  by the clock, and lines typed ahead).

<!-- DECISIONS.md:3296-3329 at d35194df -->
## The tests: split by area, scenes, the runner (2026-11-11, as built, for review: ENGINE-CRITIQUE.md's step 10, §9)

The suite took 14 to 17 minutes on the 8-core desktop (442 tests, 8 at a time), and every merge waits on it.  Measured first (obj/testtimes.json, and a dq0 test's lines' cycles one by one):

- **Where the time goes.**  6,809 test-seconds in all (each test's process's wall time, summed); the dq0 tests 6,132 of them (90%), 286 of the 306 billion cycles.  The longest are long for what they do, not their start: dq0hold, dq0housefaults, dq0looks, dq0packsteps (2 to 6 billion cycles, or a watcher on every instruction) take 90 to 140 s each.  A typical dq0 test (dq0parse, 417M cycles, 6.3 s alone): the boot 11M, hmchar 48M, **the game's start (its first dungeon grown) 254M, 61%**, its 20 commands about 100M.  Every test also spent about a second before its first cycle making its ROM image (romimg.build's ROM disk volume, the same for nearly every test), and a fifth of a second loading the runner and tests.js in its own process.
- **The runner already ran in parallel** (base/sim/test.js: a process a test, -j the cores, the longest started first, the reports in the list's order): 856 s of wall for 6,809 test-seconds is 8 processes busy to the end.  So what's left to win is work, not waiting.

What's built:

- **tests.js split** (tests/suites/*.js by area: the system's, HyForth's, BASIC's, hylang's, the numbers', the sound's, the PC tool's, the screen's, hmchar's; tests/suites/dq0/*.js the game's by subject, by areas.js's NEAR: map, parser, gear, fight, denizens, senses, body, time, save, theme, screen).  The helpers more than one suite uses are in tests/lib/common.js and tests/lib/dq0.js, a suite's own at its top.  tests.js is the index: the suites in order, and the same exports.  Every test's name, fields and functions are as they were (compared one by one against the old file); the order is the suites' (the system's first, then the game's by subject).  The selector reads the helper modules too (select.js: a helper module's line runs the tests whose entries use it, as a suite's own helper's does), and every test's areas and helpers came out the same as from the one file.
- **Scenes** (sim/lib/scenes.js).  The whole machine (machine.js's state(): its memory, the CPU, the devices, the output so far, the SD card's blocks written) and the test's watchers (the hub's recorder, the hub writers' check) are saved at two points of a test's input: the prompt waits just before and just after its first dq0 line (booted and the character made; the game begun, its first dungeon grown).  The next test whose input is the same up to there is taken up from it.  This is exact, not an approximation: while the input waits for a prompt, nothing after the wait reaches the machine (acia.js looks only at the wait), so the machine at that moment is every such test's.  A scene's key is the input to the point, the images' sha1 (the ROM's, the BIOS's, the SD card's), the seed, the chip, the RAM modules, the core, HUBOWN, and the emulator's and the runners' sources; a rebuild makes new scenes, and obj/scenes keeps the newest 400 (some 0.75 MB each).
- **Why the machine and not the game's saved games.**  A game carried on (dq0 -d) isn't the same game as one begun: other words, other dice, the start's events missing.  The tests would have to change, and some couldn't.  A snapshot of the emulator changes nothing the test sees, and it skips the boot and hmchar too.
- **A test's watchers.**  A test with start(m) (dq0Poke and the like) saves scenes of its own (its name and its suite's sources in the key), so its pokes are never left in another test's machine.  It can be taken up from its own or from one a test without watchers saved.  As it's taken up, its watchers are made and given what the serial port gave before the point (each byte read and shown, at its cycle; the hub not there before it was made).  A test with budgets, send, a /pc or a machine of its own is never taken up, nor one marked scene: false.
- **Held to runs without.**  SCENES_TRACE=DIR writes each dq0 test's cycles, output hash, failures and notes.  For all 292 dq0 tests, cold (173 from their start, 92 taken up at rc's prompt, 27 in the game) and warm (279 in the game): the same, to the cycle and the byte, but for two.  dq0layers and dq0looks have watchers that see the hub's writes as the game grows, which a scene can't give them.  They're scene: false and start from the beginning as before.
- **The runner** (base/sim/test.js, sim/test.js).  A test's image is kept in obj/imgcache, by its inputs' sha1 (the modules, the init, the hardware test, the BIOS, the ROM disk's files, the image tools' sources): made once and read after.  The parallel runner starts the longest first by each test's last wall time (obj/testtimes.json's `all`), not its cycle budget.  A process for each test stays: no test's state can reach another's.

**Measured** (the whole suite, 442 tests, 8 at a time).  Other agents' suites shared the machine through every run, 1.5 to 4 times over, so the walls are rough.  The CPU-seconds (each test's process's own, now in obj/testtimes.json) hold whatever else runs.

| | CPU-seconds | that, over 8 cores | wall measured |
|---|---|---|---|
| before | some 4,450 (the run without scenes, and 0.55 s a test for its image) | 556 s | 856 s (14m 16s) |
| split, images kept, no scenes (SCENES=0) | 4,209 | 526 s | 1,436 s (the machine 3 times over) |
| scenes cold (images new: scenes made as the run goes; 93 taken up at rc, 33 in the game) | 4,012 | 502 s | 1,963 s (4 times over) |
| scenes warm (nothing rebuilt since the last run: 277 of the 292 dq0 tests taken up in the game) | 2,899 | 362 s | 1,256 s |

Warm, the dq0 tests' CPU time is a third less (2,626 s, from 3,929 without scenes); cold, 5% less.  A dq0 test of some 400M cycles (dq0parse) runs in 1.5 s from its scene, 6.2 s from its start.  The longest tests (dq0housefaults, dq0packsteps, dq0hold, dq0looks: 1 to 2.5 minutes each) are long for what they do after their start, so a whole warm run on a quiet machine is bounded by the busiest core, not by the scenes.

**For review:**

- **Cold runs gain less than warm ones.**  After a merge the images are new, so the run is cold, and in a cold run a scene only helps tests that share a start with one run before them (most share the boot and hmchar; few share a dungeon).  The lead's run after the merges is cold, its RULES=null run after it (the same images, another dq0 line, `-r null`) gains only the rc point.  An agent's runs while it works, and every rerun, are warm.
- **Disk.**  obj/scenes holds some 300 MB at its fullest (400 scenes).
- **The emulator's snapshot** (base/sim/lib) refuses what it doesn't hold: the VERA, the input controller, the RTC, the sound, a /pc host, CA1's pulses, the debugger's things.  A test using one runs from its start.
- **Next** (§9's): scene constructors (a room, its things and its denizens declared, not reached by commands), and the recorder's offsets from the schema.

<!-- DECISIONS.md:3330-3418 at d35194df -->
## The placers' table and the keyed dice (2026-11-11, as built, for review: ENGINE-CRITIQUE.md's §5 and step 8)

The maker placed each fixture in code of its own with dice of its own: the doors their own hash (pass.c), fixtures.c
its `dseed`/`dr` (windows, portcullises, the fortified doors, the portals' make, the traps from a local array),
sconce.c a copy of them, and stock.c the rooms' marks and the kitchens' wells in a loop of its own.  This step gives
the maker one keyed dice API and one table of placers.  **Every seed's dungeon is byte for byte what it was:** the
maps, the fixtures and their places, the things.

- **The dice (games/dq0m/place.c).**  `dkey (purpose, where)` keys them for chunk L, `dr (n)` rolls (0 to n - 1) as
  often as the decision wants; a purpose (grow.h's DP_*) is what a number decides: DP_DOOR, DP_MAT, DP_WINDOW,
  DP_PORTC, DP_TRAP, DP_DTRAP, DP_MAKE, DP_FORT, DP_SCONCE.  **Each purpose's key is defined as its placer's old
  stream:** `((cx << 20) ^ (cy << 8) ^ (a << 28) ^ where) * 2654435761 + c`, a the old decision's number and c the old
  constant (the doors' a 0 and c 1: pass.c's own hash was the same formula; the fixtures' c 7).  The old streams were
  already keyed by place, never by call order, so none had to be kept apart.  DP_SCONCE's key is DP_MAKE's (sconce.c's
  copy keyed its rooms with fixtures.c's sixth number, the portals' make's): two purposes, one derivation, as it always
  was.
  **The old keys are the chunk's place alone**, not the stratum's seed: two strata's chunks at the same place have
  the same fixtures' numbers (a door on the same square the same lock), as before; kept, for byte-identity.  A purpose
  from DP_SEEDED on has its stratum's seed and its own number in its key, so a new placer gets fresh numbers that move
  nothing else and differ from stratum to stratum.  **For review:** whether to move the old purposes onto the seeded
  key some day (every dungeon would change, and the tests' scenes with it).
- **The streams (`rstream`).**  The growers' (built.c, caves.c: a layout's rolls one after another, call order by
  design) and the stock's (stock.c's things lying about) keep their dice.h streams; their seeds are made in place.c
  now (RS_GROW: the stratum's seed ^ (cx << 16 | cy) * 40503; RS_STOCK: ... * 69069 ^ 0x5EED), as make.c and stock.c
  made them.  They stay streams: their numbers depend on the order of the layout's work, which a key can't reproduce
  cheaply.
- **The placers (`placers[]`, `place (stage)`).**  A row: its stage (PG_WALLS and PG_TRAPS fixtures.c's, PG_STOCK
  stock.c's, PG_SCONCES sconce.c's), its sites (SI_ROOM; SI_WALL, a room's four sides; SI_DOOR, a portal that's a
  door), its dice's purpose (DP_NONE none), its odds (a chance in `sides` of a byte of the style, by its offsetof), what
  a site mustn't be (a room's R_* flags, a door's PS_* state), the room's use it wants (U_ANY any), qualifiers
  (PQ_DEFENCE: always in a fortified stratum's gatehouse and guardrooms; PQ_WATER: only where the chunk has water),
  where in the site it goes (WH_*), what it makes (KI_*, PK_*, or 0: the hook's choice) and its hook.  The one placer
  goes over a stage's rows, each over its sites: the site filtered, its key set, its odds rolled, the hook called.
  Rows of a stage that follow one another over the same sites are placed site by site (a room's mark, then its well),
  as the old loops ordered them, so the stock list's order is the same.  Seven rows:

  | row | sites | dice | odds | what it makes |
  |---|---|---|---|---|
  | window | a room's side | DP_WINDOW | windows in 20 (a fortified gate's always) | a window or a slit (put_window) |
  | portcullis | a door, not hidden | DP_PORTC | portc in 20 | a portcullis (put_portc) |
  | room trap | a room | DP_TRAP | traps in 20 | a pit, or a plate's or a wire's trap (put_trap) |
  | door trap | a door, closed | DP_DTRAP | d_trap in 20 | a door's trap (put_dtrap) |
  | mark | a room, its middle | none | always | its use's mark (put_mark: uses.c's) |
  | well | a kitchen, by its west wall, where there's water | none | always | KI_WELL (put_mark) |
  | sconce | a room not lit by its own light | DP_SCONCE | its hook's (kept, or built with them) | a sconce and its torch (put_sconce) |

  The well is data now: uses.c's US_SIGN 0x80 case (a kitchen's well where there's water) is gone, the row's use,
  PQ_WATER and KI_WELL saying it.  The hooks are the old code, each the part that's truly the fixture's own (a
  window's square along its wall, a trap's square with a walk round it, a sconce's square beside a wall, and their
  makes).  **Not rows:** the doors (placed by the grower's own stream as it joins the rooms, pass.c; only their
  details keyed); the materials, the fortified stratum's doors and the portals' make, which are decisions over every
  room or portal rather than placements: steps of fixtures.c's, between its two stages, with their own purposes.
- **A C table, not a text file.**  The rows hold hooks and the style's offsets, which a generator would have to write
  out as C anyway, and seven rows read plainly as C.  If the fixtures grow (COMMANDS's furniture with compartments, a
  fountain, a shelf), a placers.txt read by the build as said.txt is would be the next step.
- **Room.**  The placer, the table, `dkey` and `rstream` are overlay 9's (sky.c's, which had the room), called through
  bank.s's stubs (place, dkey, rstream, defended; the hooks put_window, put_portc, put_trap and put_dtrap in overlay
  5, put_mark in 7, put_sconce part 3's stub); `dr` is the main RAM's (it's called most).  A hook in another overlay
  can't read the table in overlay 9, so the row it's called for is copied to the main RAM first (`pr`, PR).  First
  built all in the main RAM, the placer in cc65's C was 1,477 bytes and left 1,424 free, under the 2,048.  The build's
  room table:

  | | before | after |
  |---|---|---|
  | dq0m's main RAM | 2,940 | 2,737 |
  | overlay 2 (pass.c) | 623 | 947 |
  | overlay 5 (fixtures.c) | 28 | 845 |
  | overlay 6 (the parts' images) | 66 | 508 |
  | overlay 7 (stock.c, uses.c) | 11 | 165 |
  | overlay 9 (sky.c, place.c) | 1,908 | 477 |
  | part 3 (sconce.c) | 2,043 | 2,485 |

  Overlays 1, 3, 4 and 8 and parts 1 and 2 as they were; no other task touched.
- **The start** (dq0starttime): seed 1 233.8M cycles (234.1M before, budget 235M), seed 123456789 311.4M (311.0M,
  budget 315M).
- **Checked byte for byte** (a scratch script, not committed): every run of bytes each task wrote to the hub's shared
  banks (the maker's chunks into the cache, its stock lists, the seen maps; the world's and the denizens' writes too),
  hashed, as a sorted list per seed, over 21 seeds (1, 3, 5, 7, 8, 11, 13, 21, 42, 77, 99, 117, 256, 374, 517, 1000,
  2026, 4242, 31337, 70000, 123456789), each in twelve games (as it is, -T, -T -T, -T -T -T, -M, -A, -D, -V, -P, -M -T,
  -A -T -T, -V -T -T -T: the top stratum and those under it; built, natural and mixed; the diagonal passages, the
  rooms at 45 degrees, the great caverns, the long slopes): 252 games, 902 chunks made (8,109 doors, 892 windows, 233
  slits, 915 portcullises, 770 traps in 489 chunks), 29,202 runs of bytes.  Before and after: the same, every seed.
- **Left:** the styles' proportions (uses.c's styles[4]) into the theme (§5's step 3); the private hashes that aren't
  placers' dice (caves.c's caverns, water and rubble, sky.c's skylights, plan.c's water, uses.c's ring, make.c's mixed
  third) as purposes of `dkey`'s, each defined as its own formula (they'd move nothing, but each wants its own
  derivation, or the seeded key and new dungeons); the hooks' square-finding as a few shared finders (a square along
  a wall, one in the open, one by a wall) once more fixtures want the same squares; a text source for the table if it
  grows.

<!-- DECISIONS.md:3419-3542 at d35194df -->
## Narration as data, stage 2 (2026-11-11, as built, for review: ENGINE-CRITIQUE.md's §3.2 and step 5, its second stage)

Stage 1 built the renderer and the phrase table and left overlay 27 with 282 bytes.  This stage makes room for the
table to grow, moves four more wording functions and seven of task_words' own sentences onto it, and takes out the
world's told () copies that only sent events.  The words the player sees are the same, letter for letter.

- **Room: the table's rows in a bank of their own (overlay 28), read a byte at a time.**  dq0.cfg's OV28 holds
  obj/gen/dq0said.h's said_rows alone (OV28RODATA).  The renderer (said.c) stays in overlay 27 with the small tables
  (the kinds, their fields, where the rows and the pieces begin, the words most said), and reads a row's bytes through
  bank.s's said_b: 21 bytes of main RAM that put bank 28 in the window, read the byte and put the window back, some 40
  cycles a byte with the call.  A row now begins with its length, so the renderer steps from row to row without reading
  their texts.  Before, PORTAL's 2.5K of rows were walked a byte at a time.  **Chosen over** two others:
  - the renderer in assembly: a third of its 2.4K back, but the table, 5K and growing, is what fills the bank;
  - the renderer as a swapped part: the window would be free while it runs, but every sentence would cost a part's
    copy (some tens of thousands of cycles), with part 11 swapped out and back around it.

  **Loaded as the game starts, not lazily.**  Measured: seed 1 234.1M to 234.3M, seed 123456789 311.0M to 310.9M
  (budgets 235M, 315M).  The first build of this stage, with the same bank and fewer rows, measured 233.5M and 310.6M.
  The bank adds bytes to read in, but the four functions' code and strings are gone and the texts are packed tighter.
  Seed 1 rising while seed 123456789 falls is the game's own start, not the loading, which is the same for both.

  **The packer (parts.js's banks) puts no part in a bank whose config line says "no parts"** (OV28's).  The first build
  had put part 16's image there, so the bank's room went to the parts and not the table.
- **More words most said.**  The 26 one-byte codes were spent.  The build now goes on choosing words, as SC_WORD and a
  byte, while one saves any (up to 200; 115 now).  Their letters are in overlay 27 (said_dw 668 bytes, said_do two bytes
  each).  The rows are 7,565 bytes without them and 6,280 with them.
- **Kinds older than the head (said.txt's `kind KIND FIELD`).**  E_BEAT, E_HARM and E_TASK keep their bytes where they
  have always been: their outcome in w1's low byte, not E_OUTCOME's high one.  A `kind` line names the field that is
  their outcome (`kind BEAT beat`), and the renderer reads each kind's outcome by its field (said_ko).  **For review:**
  the alternative was to move each onto E_OUTCOME's layout.  That would have changed every sender of three of the
  world's busiest kinds, and the tests' readings of them, without changing any word said.  The head stays the shape
  for new actions.  Two forms were declared in msgs.def for the rows' fields: E_BEAT:water (wet, runs) and E_TASK:shot
  (shot, arms).  Nothing moved; the tests read beat and te as they did.
- **New template pieces:**
  - `{kind F}`: a thing's kind's name, from the catalogue (the critique's handles, for a kind: "dagger");
  - `{name F}`: an actor's name as it is, by a field (E_BEAT's whose);
  - `{-s}`: the word before made singular ("1 arrow");
  - a row for any outcome (`HARM.any`), tried after the kind's other rows;
  - a condition on one value past 7 (`TASK.loaded[arms=KI_CROSSBOW]`).
- **Moved and deleted:**
  - limb_words (main.c's part 23): the limbs' news, every actor's, worded by its body;
  - water_words (water.c, overlay 4: the file is gone);
  - harm_words (intro.c, overlay 12);
  - shot_words (shots.c, overlay 1: the file is gone);
  - their stubs (bank.s's OVFN harm_words, water_words and shot_words) and dq0.h's lines.

  What stays with the callers is what's truly theirs.  s_news (part 11) gives:
  - the body (the row's arg);
  - a fall not seen (the UI's own outcome, BEAT.unseen);
  - the referee's words of the wounds (text, at lb + 200: the referee's hurt[] is under 200 bytes);
  - whether the character is dead (arg).

  It calls state_words where harm_words did.  A body's position stays pose_words'.
- **Inline sites moved:** task_words' own sayf and msg sites for TE_STOPPED, TE_HIT, TE_NEAR, TE_ASK_SEEN, TE_ASK_HEARD
  and TE_ASK_TIRED (what it was doing is their text, copied into the main RAM), TE_CANT, TE_ASK_HURT, TE_NO_FOE and
  TE_FREE, and the missiles' four (TE_NO_SHOT, TE_LOADED, TE_GATHERED, TE_READIED).  That is 48 rows more: 171, and
  15 pieces.
- **A stage 1 bug found:** the renderer's denizen name ({actor}) was copied by hub_mem into the overlay's own BSS, which
  is in the window.  With the hub's bank in the window, the name went into the hub's header, and the renderer read back
  nothing.  No stage 1 row ever named a denizen (the world tells no actor's portal work), so nothing showed.  The limbs'
  rows do ("The unslings its buckler."), so the name now goes into lb, in the main RAM.
- **The world's told () copies.**  Gone:
  - workpart.h's, in five parts (wdoor.c, wblow.c, wwin.c, wtrap.c, wroom.c): the work's events, E_NOISE, E_HARM,
    E_BEAT, E_PORTAL and E_TRAP;
  - wfetch.c's;
  - wshoot.c's two (told and told68), both E_TASK.

  **One sender:** dq0w.c's event, now the world's and not static, its b in evb.  It now maps the hub's header itself,
  as each copy did.  Beside it is work_told, the work's sender (wk.m: its a; its w2 the character's y; E_NOISE's w1 its
  x).  wfetch.c's and wshoot.c's told are now macros of event.  workpart.h's told is work_told, except in the parts that
  also work an actor's door (W_FRONT: wdoor, wblow, wwin): there told only keeps the actor's outcome, and otherwise calls
  work_told; it no longer sends anything itself.  dq0w's main RAM went from 2,336 to 2,243 bytes free, the two senders
  now resident.

  **Left:**
  - wdig.c's and wport.c's told (E_DEN: a denizen's digging, its door work).  Each works out where, how far and whether
    seen in its own way, with its own copy of apart.  One resident den_told would cost the world's main RAM some 170
    bytes more, past its margin over the 2,048 floor.  They go with the denizens' actions worded by the rows (E_DEN's
    DE_* as outcomes), when E_DEN's shape changes anyway.
  - wfight.c's told is the only E_FIGHT sender: the blow's record, not a copy.
- **Room** (the build's table):

  | | before | after |
  |---|---|---|
  | dq0's main RAM | 3,399 | 3,396 (said_b) |
  | overlay 27 (the renderer) | 282 | 4,351 (the rows out; the words most said, 115 now, its) |
  | overlay 28 (the rows) | - | 1,912 (the rows 6,280 bytes) |
  | overlay 1 | 369 | 1,118 (shot_words gone) |
  | overlay 3 | 34 | 440 (task_words' lines) |
  | overlay 15 | 24 | 1,736 |
  | overlay 26 | 323 | 4,333 |
  | overlays 4, 12, 20 | 594, 236, 2,898 | 76, 66, 767 (the packer's: parts 7, 24, 17 placed anew) |
  | the overlays' free bytes, all told | 18,138 | 28,177 (overlay 28's 1,912 among them) |
  | part 11 (s_news) | 3,270 | 3,059 (the limbs' and harm's callers) |
  | part 23 | 2,084 | 4,007 (limb_words gone) |
  | dq0w's main RAM | 2,336 | 2,243 (event and work_told the world's) |
  | dq0w's parts 5, 6, 7, 8 (workpart.h's) | 311, 16, 239, 0 | 376, 81, 304, 90 |
  | dq0w's parts 66, 68 (wshoot.c's), 72 (wfetch.c's) | 2,150, 1,192, 720 | 2,212, 1,227, 775 |
  | dq0w's overlays' free bytes, all told | 7,044 | 7,571 |

- **Checked:** every test's output was kept before the change and after it (a temporary dump in sim/test.js, not
  committed), and their sentences compared, the status line and the start's progress left out.  No sentence of the
  moved words is said differently.  The sentences that are new or gone, in 21 tests, are:
  - the real-time fight in dq0antidote;
  - lines typed ahead, landing before or after "Time passes." or "Something stirs.";
  - three tests whose denizens move on the clock: dq0answer (a rat heard twice less), dq0jobs (a squeak "to your left",
    not "ahead to your left") and dq0skykin (a cave adder "gone from sight" three times more).  The UI's cycles changed
    (the renderer's reads, event's mapping), so the world's clock meets the lines typed at other moments.

  The first run of the moved words caught two bugs, both fixed: the name above, and a one-way choice that should have
  been two (`{stopped?| You stop ...}`).
- **Left (stage 3 and on):**
  - the other wording functions (the rest of task_words, beat_words, felt_words, fight_words, sky_words, throw_words,
    sway_words, state_words, robbed ...) and the inline strings;
  - wdig.c's and wport.c's told, with E_DEN;
  - the SA_ refusals as rows;
  - the first words from a *begun* row;
  - the denizens' actions by the rows, the actor swapped;
  - the sound's mapping as a column of the table.

  The rows' bank has 1,912 bytes left.  The next bank, when it's needed, is a line in dq0.cfg (OV29, no parts) and
  said_b choosing the bank by the row's place.
- **Tested:** all 442 tests pass.

<!-- DECISIONS.md:3543-3656 at d35194df -->
## Narration as data, stage 3 (2026-11-11, as built, for review: ENGINE-CRITIQUE.md's §3.2 and step 5, its third stage)

Stage 2 left the rest of task_words, the walk's beats, the denizens' doings and the last two told () copies.  This
stage moves those onto the phrase table, gives the rows a second bank, takes out the two copies, and words the
denizens' doings by the same rows as the character's, the actor swapped.  The words the player sees are the same,
letter for letter.

- **The rows' second bank (overlay 29).**  The rows grew to 9,879 bytes, past one bank.  dq0.cfg's OV29 is a bank of
  its own ("no parts", so the packer keeps out of it) whose MEMORY line starts at **$A000**: the second half of the
  rows (said_rows2, OV29RODATA) is linked there, so a row's offset past 8,192 is simply an address past $A000.
  bank.s's said_b (main RAM, 9 bytes more) reads an address under $A000 from overlay 28's bank, one past it from
  overlay 29's at that address less $2000.  The renderer is unchanged: it still adds an offset to said_rows.  saidgen
  places each kind's rows, and each piece, wholly in one bank, the first with room, in their order (bank 28 8,186
  bytes, bank 29 1,693: DEN's rows and later pieces there).  OVMAX is 29; the game reads one more bank as it starts.
  **Chosen over** a bank number in each row's offset (said_at would grow a byte a kind, and the renderer would carry
  the bank through every pointer it walks).
- **The world's told () copies: none left.**  wdig.c's and wport.c's (E_DEN: a tunneller's dig, a builder's work, a
  denizen's door work) are one sender, wport.c's `s_den_told` in part 80, called directly there and through its stub
  (`den_told`) from wdig.c's part 79.  What each copy did its own way are flags in its loud byte: `DT_SEEN` (a dug
  square seen as it was, before it changed: wdig.c's 0x80 before) and `DT_DOOR` (a door's work: what lies between
  measured from the door, and seen if the body's own square is: wport.c's).  Its event is on the C stack, as
  wport.c's was (not wbody.c's bev).  dq0w's main RAM 2,188 → 2,182 (the stub alone); part 79 622 → 929 (its copy out), part 80 181 → 122.
  **Not resident**, so stage 2's 170 bytes weren't spent.  **Not in part 79, though it was built there first:** part
  79's bank is overlay 32, one of dq0w.rop's three, read in only at ropes_in; a denizen's door work before that
  copied an unread bank in and the world hung (dq0allydoor, dq0daily's garrison, dq0builder, dq0hoard).  wbody.c's
  told_den stays: it's every step's (DE_MOVED), the bodies' own, and a part's copy a step would cost thousands of
  cycles each.  **E_DEN's shape is unchanged**: the sound task (dq0s) and the denizens read it as before, and the
  tests' fields are the same.
- **The denizens' doings worded by the rows** (said.txt's `kind DEN de`: E_DEN worded by its own outcome field, as
  BEAT, HARM and TASK are).  Each DE_* that's told seen has a row: rally, snore, talk, alarm, dig, dug, and the 23 door
  and portcullis doings (opens ... picks); a pastime seen is DEN.any, its words tell.c's as before.  A row's
  `{arg?seen|heard}`: its arg 0 the sentence, "\nThe dwarf miner hangs a new door.", the actor swapped in by `{Actor}`
  ("The giant rat", "Grub") and the verb's ending by `{s}`; its arg 1 what it sounds like ("hammering"), left in lb by
  said.c's new `said_q` (the words not said: the caller's), for caps.c's heard words.  **The character's own words and
  a denizen's share a row's piece where they are the same words:** `@open` is `{Actor} open{s} the door`, PORTAL.opened's
  ("You open the door, revealing ...") and DEN.opens's ("The bandit opens the door.").  That is the one doing whose
  words are already the same both ways today: the character "closes" a door where a denizen "shuts" it, unlocks "with
  its key", lifts a bar "out of its brackets"; making them one would change what the player sees.  The other DEN rows
  are written for any actor ({Actor} ... {s}), so a row the character comes to share will read right.  main.c's
  s_door_words (part 18) and noise_told's own strings are gone; noise_told (part 14) keeps the weighing and the heard
  words' way.
- **Moved and deleted:**
  - the rest of task_words' words (task.c, overlay 3): what's not doing, the stop with nothing to stop and between the
    reach's steps, engaged, backed away, fighting on and turned to, a disengage, a light low and out (its plural, one
    seen, "One of your torches"), an ally's errand (fetched[]), a timed work begun (a journey, the mechanism, tracks,
    the air, a climb, battering with a thing), a bare part's blow stopped short, "Keep at it?", a journey there, the
    way forgotten, lost bearings, astray, elsewhere, drawing learnt.  task_words keeps what's truly its own: the stop
    (parts_in, reach), the questions and their answer, what the foe, the place, the light are as they're said;
  - robbed (task.c, overlay 15) and its OVFN stub: its words TASK.robbed and the UI's own TASK.coins, its purse kept
    in task_words;
  - the reach's 25 first words by verb (main.c's s_reach_said, part 19: first[]), TASK.step with its verb as its arg
    (`@steps`, a piece: a row is at most 254 bytes), and the grope, the pack's place, a fire-maker's try (TASK.fumble,
    .packed, .kindled);
  - beat_words (task.c, overlay 16) and its stub: a walk's beats (BEAT.quiet ... BEAT.on, a bend's sixteen bearings);
  - felt_words' dark beats (task.c, overlay 17): a side's wall, setting out in the dark, what the hands meet, a thing
    underfoot, running into a wall, an edge, a swat, a thing used up.  felt_words keeps the look's two (FW_NEW, FW_TOUCH);
  - state_words (task.c, overlay 7) and its stub: HARM.state, said by s_news (part 11) as the old loop did;
  - main.c's s_door_words (part 18).

  The walk's beats now go through news () (part 11), where the limbs' already did: main.c's events() calls news for
  every E_BEAT of the character's.  In all, 86 rows more: 257 rows of 8 kinds, 20 pieces, 31 fields of 32.
- **The table's compiler.**  A field named in more than one of a kind's forms is now the row's outcome's form's (E_BEAT's
  fa, 8-15 in its met form, 8-11 in its hit form), else the kind's own form's (E_TASK's task, 8-15, not its light form's
  8-13 beside its seen and another bits): the forms' selectors are in the hub's JSON (`sel`).
- **Room** (the build's table):

  | | before | after |
  |---|---|---|
  | dq0's main RAM | 3,383 | 3,415 (said_b 9 bytes more; the stubs of robbed, beat_words and state_words gone) |
  | overlay 27 (the renderer, the words most said) | 4,351 | 911 (the words most said, 156 now, 115 at stage 2; their letters and the renderer) |
  | overlay 28 (the rows' first bank) | 1,912 | 6 (8,186 bytes of rows) |
  | overlay 29 (their second bank: new) | - | 6,499 (1,693 bytes of rows) |
  | overlay 3 (task_words) | 408 | 2,121 |
  | overlay 7 (state_words gone) | 387 | 722 |
  | overlay 10 | 49 | 2,761 (the packer's: part 18's image placed anew) |
  | overlay 15 (robbed gone) | 175 | 311 |
  | overlay 16 (beat_words gone) | 865 | 1,672 |
  | overlay 17 (felt_words' beats gone) | 128 | 329 |
  | overlays 4, 18, 20 | 1,020, 1,982, 320 | 4,086, 758, 2,898 (the packer's) |
  | part 11 (s_news: the beats, the state) | 3,061 | 2,349 |
  | part 14 (noise_told) | 1,097 | 1,643 |
  | part 18 (door_words gone) | 1,640 | 2,582 |
  | part 19 (reach_said) | 2,604 | 3,541 |
  | part 23 | 4,007 | 3,560 (the packer's) |
  | dq0w's main RAM | 2,188 | 2,182 (den_told's stub) |
  | dq0w's parts 79, 80 | 622, 181 | 929, 122 |
  | dq0w's overlays 24, 32 | 291, 1,086 | 232, 1,393 |

- **The start** (dq0starttime): seed 1 233.2M → 234.0M cycles (budget 245M), seed 123456789 311.4M → 310.6M: the one bank more read as it starts, the four functions' code gone.
- **Checked:** every dq0 test's screen kept before and after (a temporary hook in sim/test.js, not committed), their
  sentences compared (the status line dropped, wrapped lines joined, the order of whole sentences ignored).
  8 of 294 differ, all noise: the real-time fights in dq0antidote and dq0denpace (dq0denpace's new run shows the moved
  rows said right where its fight went another way: "The bandit goes through your things as you lie helpless, and
  takes what it finds.", "The torch gutters and goes out.", "You fight back against the bandit."); denizens met at
  other moments as the cycles moved, dq0answer (a scurrying heard once more), dq0jobs (a squeak "to your left" once
  where it was "ahead to your left"), dq0mine (a muffled scraping heard once less), dq0prompt (a giant rat "to your
  right", not "ahead to your right", as it goes from sight); dq0passes' "Something stirs." once more; dq0sway, a
  key typed ahead echoed at a line's end a moment sooner ("behind you._N", not "behind you.N").  No moved sentence is
  said differently, nor more or less often, in any test.  **The first run caught one fault, fixed:** s_den_told built
  in part 79 hung the world on a denizen's door work before the ropes were read (above).
- **Tested:** all 444 tests pass.
- **Left (stage 4 and on):**
  - fight_words (blows.c, overlay 6), sway_words, sky_words (time.c's sky[] and its sky2 by what shows the sky: wants
    E_BEARING's sky byte split, its phase and its opening, two fields), throw_words (resolve.c: a request's reply, not
    an event: wants the throw an outcome event, or a kind for the UI's own replies), the state's and the facing's
    sheet, and the other inline strings (s_came, s_pose_words, hide's, the background's);
  - the SA_ refusals as rows, and the first words from a *begun* row: both are the UI's own, not the world's events,
    and want a kind of the UI's (a row keyed by a refusal or a verb) before they move; not built;
  - the denizens' doings that the character does too in other words (shut and close, unlock "with its key"): one
    wording each, the user's call, then the rows shared (the user chose one wording: done, "One wording for the player
    and the denizens" below);
  - wbody.c's told_den, the bodies' step: kept resident in its part for its pace;
  - the sound's mapping as a column of the table (untouched, as asked).

<!-- DECISIONS.md:3657-3713 at d35194df -->
## The user's review of five defaults (2026-11-11, the user's)

The user reviewed defaults this file had marked "for review" and asked for five of them changed; two more were kept.
Each entry above now says so; what's built:

- **"g" is again's** (Infocom's convention), no longer give's.  commands.aliases: "g again" (beside "repeat again"),
  "hand give" and "offer give" (they were "hand g", "offer g": g was give's root), "give g" gone; commands.verbs'
  [words] "g = again" in place of "g = give" (the same record: dq0c's records as they were); commands.ebnf's
  `<GIVE> ::= "give"`.  Give must be spelt out ("give", "hand", "offer").  Nothing else read "g" as give (the grammar's
  `<GIVE>` was its only use; letters.c makes a lone "g" after a verb a carried thing's letter, "drop g", as before).
  term.c's `said_again` (overlay 16) keeps a line out of `t_last` when "g" is a part's first word (the line's start,
  after "," ";" "." or "then"), so "g" runs the line before it, never itself; a "g" elsewhere ("drop g") is kept.
  The corpus has "g" (VB_REPEAT).
- **A vow of fasting is carried in the save.**  No file grows and `SAVE_VER` stays 45: game.dat's header byte `plain`
  had one bit used (the map in plain letters); its bits 1-7 now hold the vow's time left as the game was kept, in
  steps of 12 minutes rounded up (life.h's `FAST_STEP`, 7,200 tenths: a day 120 steps), 0 none (an old save's).  As a
  game's carried on, the vow is set again on the clock it's carried on with.  Between dungeons: life.dat's head had six
  bytes of room; one is now `fast`, the vow's end in steps after the midnight that begins the head's date, dq0life's
  (`ended`: from the ended dungeon's save and its clock, world.dat's), 0 if it's over by then, and 0 once the time
  between (dq0acct's) has moved the date on a day or more (a day's vow can't outlast a whole day more).  The UI takes
  what's left of it after the hour the next dungeon begins (its seed's, as dq0life reckons it).  "stop" and "break my
  fast" end it as before.  Test dq0fastkept.
- **A sip can bring on the flux.**  Tasting still water (the water it stands in, or a skin filled from it) tells the
  referee it's drunk foul with no pints (RS_TIME's drink 0x80: hthings.c's taste, as feed.c's drink tells it).  The
  figure: *a sip is a tenth of a drink's dose, so the Contagion test comes only one time in ten* (d100, 10 or under:
  afflict.c; then the drink's own test, CI 5 times Endurance), some 2-5% where a drink is 25-50%; the null ruleset the
  same, one in ten (`random (10)`).  Spoiled food doesn't exist yet (nothing goes off in this game: HOUSE.md's "not
  yet"), so tasting food stays "still good"; when it does, its taste takes the same tenth.  The world's DG_TASTE is
  unchanged (it reports the wear).  Tests: dq0rules' three sip cases (a miss, its one die; the tenth come and caught;
  a pint from the UI, no tenth's roll), dq0things' sips (the referee told, the antidote's taste not).
- **The sheet names a dressing's cloth.**  pc_dr's spare bits 4-6 hold its cloth (dq0harn.h's `DR_MAT`, `DM_HEMP`
  ... `DM_SILK`; 0 not known).  The world's tools text gives a bandage's cloth after its grade ("bandage9S": R hemp,
  S linen, T wool, U silk; things.c's s_tools), the referee's equip_all keeps it in tqual's high bits, and the dressing
  takes the cloth of the best bandage (the first lot of that grade, the one the world uses up).  The sheet: "dressed in
  linen, clean" (the grade its cloth's whole), "worn" (0.7 of it or more), "soiled" (less, as it went on or by the
  days: linen worn after a day, soiled after two).  The "dressed" line, compatibly: a letter after a dressing's place,
  'q' plus its DM_* (r, s, t, u), none for a cloth not known; a line without it reads as before and its sheet's words
  are the old ones.  One reader and writer for the referee and the account (dq0harn/dressln.h); dq0life, dq0end,
  dq0schol and dq0town don't read it, nor any test's decoder.  Tests: dq0rules' four cloth cases, dq0dress's words.
- **The days between dungeons soil dressings.**  Every day between passes through dq0acct's `o_day` (at rest, on the
  road, the lull, the town's days, the work's), and there each dressing on a serious or grievous wound loses three
  grades, its cloth kept (`DR_SOIL`, the referee's heal.c's the same).  The account now reads the "dressed" line into
  its own pc_dr (sheet.c keeps the line whole in the main RAM's `dressed`; body.c reads and writes it, overlay 1:
  overlay 3 had 20 bytes) and keeps it in step as wounds heal and go (`by_drop`).  Untold in the account: the next
  dungeon's sheet says it.  The town sells bandages, and "change the bandage" in the dungeon puts a fresh one on as
  before.  Test dq0dressbetween (a serious stab dressed in clean linen, four days between, soiled to grade 1, its cloth
  kept, the line written back as the account ends in an infection's death: the account heals what it can before the
  next dungeon, up to ninety days, so a dressed wound seldom lasts to it).
- **Kept, reviewed:** generated chests brace doors (the maker's own chests and tables beside a door brace it, as a
  shoved one does: "The things' verbs"); and the looser hmchartongues check (hmchar.hl's native script among its
  scripts, not first: its dice can't be seeded; the cloth branch's change) is accepted.
- **Room:** dq0's main RAM 3,386 free (3,399 before); part 1 406 (the packer moved s_size to part 12), overlay 16
  418 (1,179), part 29 1,647 (1,804), part 14 1,097 (1,218).  The referee 2,264 (2,341: the tools' cloth), overlay 1
  194 (353), overlay 8 32 (69), overlay 9 1,683 (1,829).  dq0acct 2,152 (2,264: pc_dr and the line), overlay 1 319
  (1,194), overlay 3 9 (20: its line buffer now path).  dq0life 18,318; dq0null 5,035; the world's main RAM unchanged
  (2,336), its overlay 5 265 (328).  dq0c's records as before; the aliases 1,896 bytes (1,889).

<!-- DECISIONS.md:3714-3809 at d35194df -->
## The referee's skeleton (2026-11-11, as built, for review: ENGINE-CRITIQUE.md's §8 and step 9)

**The user's choice, 2026-11-11: option (a), swappable rules are a real goal.**  §8 found the referee interface
halfway: an indirection the game paid for, and HarnMaster everywhere behind it.  (b) would have said HarnMaster is the
house system and dq0null a test double.  The user chose (a): a referee skeleton in a library so a ruleset is its
answers, the interface trimmed, HarnMaster's lists behind the referee, and the meta programs talking to "the ruleset".
Built in five stages, each its own commit:

- **1. rs.h trimmed, P_ one thing.**  Ten requests no one made (RS_DROP_ACTOR, RS_TEST_MANY, RS_REST,
  RS_INITIATIVE, RS_EXPERIENCE, RS_POWER, RS_EXPORT, RS_IMPORT, RS_RESTORE, RS_QUIT: each checked, no sender and no
  answer in any task) and `struct rs_neutral` deleted, with their msgs.def lines; 17 requests left.  The tests that
  had request numbers typed in (denizens.js's and senses.js's RS_TEST = 4, RS_ATTACK = 7) take them from the layout
  by name.  `P_` now means only rs.h's purposes: theme.h's stratum purposes are `SP_*` (SP_HALLS ... SP_WORKS,
  SP_FORT), dq0town's places `TOWN_*`.
- **2. The skeleton.**  games/dq0/referee.h (the interface) and games/dq0/rf/rf.c, rf/sheets.c.  **Where it lives:
  a shared C source each referee compiles, not the paged ROM.**  The ROM library (modules/dq0lib) is assembly with no
  RAM of its own, called by XCALL at some 120 cycles; the skeleton is C, its loop and its `put` called constantly, and
  nearly all of it was each referee's main RAM already, so moving it to ROM would have meant rewriting it in
  assembly for little room (dq0harn's whole skeleton is some 1,300 bytes, much of it data and BSS it keeps
  anyway).  Each referee has a file of its own that #includes the shared one with its #pragmas first
  (dq0harn/rf.c in its main RAM, dq0harn/rfsheets.c in swapped part 2; dq0null/rf.c), the way hub.c already was.
  The skeleton: `main` (the hub attached, its dice seeded from the game's and the role, each slot answered as rung,
  the denizens' slot with DTEXT of text, the dice by who asks: dice.inc's `dice_to`, no DICE_BEGIN/DICE_IN macros),
  the answer's prologue (tp, tend, result 0), `put`/`putn` (dq0harn's: a string in the window read with its bank),
  `tools`/`tflag`/`tqual`/`equip_all`/`has` (dq0harn's, with its marks, qualities, versions and cloths: dq0null's
  `held[]` became `tflag & T_HELD`), `bseed`, `sheets_in` (the creatures' sheets read back once, a game carried on,
  as a creature's asked for) and the save's order (the character, then the creatures' sheets), `sheets_open`
  (beasts.dat).  **A ruleset is four functions:** `rf_start` (its arguments, its overlays and banks), `rf_begin`
  (its own beginning, once its dice are seeded), `rf_answer` (each request; RS_SAVE's the character alone),
  `rf_sheets` (its creatures' sheets in beasts.dat).  Each referee's own main, prologue, `den_slot`, `sheets_in`,
  `put`, `putn`, `equip_all`, `has`, `tools` and `bseed` went.  dq0null needs no HarnMaster header (it never
  included one; it now also has no copy of dq0harn's code), and passes the tests that run it (dq0antidote,
  dq0air's, dq0dressbetween's null games, dq0outfit's).  dq0null got `getbank`/`setbank` in its dice.s, and `num`
  as atoi (it doesn't load the game's library).
- **3. The house's rules as linked functions.**  rshouse.h is declarations; the code once each in games/dq0/rf/:
  dig.c (`digger`, `house_dig_tool`), notice.c (`house_notice`), state.c (`house_pace`, `house_watching`,
  `house_light`), bdice.c (`bdice`), want.c (`want_of`, rswant.h's), air.c (`air_of`, rsair.h's).  rswant.h and
  rsair.h are gone.  Each referee compiles the ones it uses where its room is, a file of its own that #includes the
  shared one, its #pragmas first: dq0harn's hdig.c (part 3, beside dig_of), hnotice.c (overlay 1, beside sheet.c),
  hbdice.c (overlay 5, beside bearing.c), hwant.c (overlay 7), hair.c (overlay 8); dq0null's house.c (its main RAM).
  The macro hooks became the ruleset's functions: `want_say` and `air_say` (its words into its news); AIR_LEVELS
  went (dq0harn sets `aff_breath` from the house's `air_lv` after `air_of`: they were always equal).  What's an
  expression stayed a macro, now unconditional: `HOUSE_SWING (k)` (what a thing does swung: the referee's gear.c and
  the world's two parts each their own use, as before), `QUIET_IN`/`OUT`, `JUDGE_IN`/`OUT`, `DIG_TESTED`.
- **4. HarnMaster's lists behind the referee.**  The outfit tags (`OT_*`: cultures, classes, wealth, sexes) are
  dq0harn.h's; rs.h says only that x[0], x[1] are the bits of outfits.txt's tag words, the ruleset's to set.  The
  strike locations (`BP_*`) are gone from rs.h: nothing outside the referees read them (an effect's x[0] is the
  ruleset's own number, HarnMaster's harn.h location, the null's 0; the game reads only the words), and t_rules names
  them for its cases itself.  **The grades are a generic scale:** `G_BADLY`, `G_FAIL`, `G_PASS`, `G_WELL` (were
  G_CF, G_MF, G_MS, G_CS, HarnMaster's level names), the mapping dq0harn's `grade ()`; renamed everywhere (the UI,
  the world, the denizens, the maker, both referees, t_rules' cases), the values the same.  **The ruleset's own
  stance:** `D_COUNTER` is `D_OWN` (HarnMaster's counterstrike; a ruleset with none takes it as D_AUTO).
  `C_STUNNED`'s meaning is the ruleset's (stunned: in no state to do more than rest or walk slowly away;
  HarnMaster's shock).
- **5. The meta programs' interface.**  games/dq0/rsmeta.h: **the rulesets' table** (`RULESETS`: each referee and its
  own program for the time between dungeons, "/../acct/dq0acct" for dq0harn, none for dq0null) and **the sheet as a
  meta program sees it** (`meta_line`: a line of the sheet read, what the ruleset keeps of it, the rest to the
  program's own; `meta_ml (MK_*)`: a mastery by what it's for, MK_LANGUAGE, MK_SCRIPT, MK_FOLKLORE, MK_RECKON,
  MK_ASTROLOGY, MK_HERALDRY; `meta_taught`: what a master's month opens a tongue or a script at; `META_TONGUES`,
  `META_SHEET`).  dq0life runs the program the save's ruleset names (`accounts_of (sv.referee)`), not dq0acct when
  the referee is "dq0harn".  dq0schol includes rsmeta.h, not harn.h nor dq0harn.h: HarnMaster's implementation is
  dq0harn/meta/meta.c (in a folder of its own, so the referee doesn't compile it), compiled in by dq0schol's rsmeta.c.
  **dq0acct is HarnMaster's own account program** (the table names it as the ruleset's): it still includes
  dq0harn's headers, as part of that ruleset.

**Room** (main RAM free, before → after): dq0harn 2,264 → 2,209; dq0null 5,036 → 4,605 (dq0harn's fuller
`equip_all` and `put`); dq0life 18,318 → 18,198 (the table and its lookup); dq0schol 5,405 → 5,333 (its overlay
591 → 630); the UI (3,383, 3 less than before stage 1: the request numbers), the world (2,243), the denizens, the
account and the others as they were; dq0harn's overlays 1 194 → 170, 3 110 → 174, 7 11 → 47, 8 32 → 22, part 2
1,574 → 1,564.  The start at seed 1: 233.3M cycles (233.9M at stage 2); at 123456789, 310.6M (budgets 245M and
325M).

**A race found on the way: dq0reachstop.**  Its "halt" and "stop" are typed as a step's words show, at the turns pace;
whether they land in that step or the next turned on some thousands of cycles between the parser's answer (some 1.3M
cycles after Enter) and the world's next turn.  Each stage's few cycles flipped it (stage 1 game 7, stage 3 game 2).
Holding the turns pace while a busy-time line is classified (`ui_giving` from its Enter) didn't help (the step had
ended before the UI's loop saw the Enter), and holding from the first key is what the turns pace was made not to do;
left for review (see below).  **So the suite: 444 of 444 at stage 2; 443 at stages 1, 3, 4 and 5, dq0reachstop the one
(at stage 5 its game 2: "halt" lands in the reach's next step, "You stop there.", the rations taken).**

**For review:**
- The skeleton is a shared source, not the ROM library: worth moving `equip_all`/`put` to dq0lib only if the
  referees' main RAM gets short (some 400 bytes in dq0harn).
- The world's worn places are still harn.h's locations (dq0w/kinds.c, items.h's WP_*, worn.c's words): the largest
  piece of HarnMaster left in the engine.  The UI's "counter" word and its "in shock" words, outfits.txt's skill
  sections and tag words, the bestiary (RULESETS.md) are the others.
- dq0schol's d100 test is still the book's (its own `test`), and its scribes' and sages' figures.
- dq0reachstop's race: a test that types as words show at the turns pace depends on the parser's speed; it wants a
  scene that holds the world while the line's parsed, or a pace that waits on a line entered.
  **The user's, 2026-11-11: accept either step.**  Game 2's check passes the stop landing in the step after the one
  it was typed in (the rations taken in hand, then "You stop there."), the pack still at its feet and nothing after it
  done (tests/suites/dq0/gear.js); the game unchanged.
  **The user's, 2026-11-12: game 7 the same.**  Its "stop" typed at "take all"'s first "Taken." may land one thing
  later (the record store's timing flipped it): the tinderbox reached for and taken, then the stop, the pack in hand,
  no third thing taken.

<!-- DECISIONS.md:3810-3892 at d35194df -->
## The step planner and the activity stack (2026-11-11, as built, for review: ENGINE-CRITIQUE.md's §3.3, §7 and step 6)

**What the survey found.**  The reach's steps were already planned from the containment tree, lazily: each time a
handling is asked (the UI's command again, after a step's wait), things.c's `s_reach_step` walks the thing's
containers out from the hand (`rs_walk`) and begins the next step owed (a pack on the back off, a level reached into,
held open, the thing got out), and the step's end (`RO_END`, `lwork`'s `LW_REACH`) does what it changes.  So the edges
were there (`rs_do`, thirteen kinds); what was hand-coded was (1) the step's record, spread over six globals, three
of them other jobs' scratch bytes aliased (`rs_k` was `nin`, `rs_c` `dropped`, `rs_t` `far_y`); (2) its words: 33 LT_*
kinds (LT_PACKOFF ... LT_UP2), nine of them a pair's other half picked by a ternary (GETOUT/GETNEW, PUTAWAY/PUTOLD,
TAKEUP/TAKENEW/UP2, SETDOWN/DOWNNEW, BLOWOUT/BLOWOLD, LIGHTFROM/LIGHTNEW, LETFALL/LETOLD, DIP/DIPOLD); and (3) what's
done after the work, a bit set (`rs_redo`'s RR_STOW, RR_RELIGHT, RR_D1, RR_D2 with `rs_down`, `rs_down2`, `rs_two`)
read in `rs_after`'s fixed order.  "What the PC is doing" besides `pc_task.kind`: `move.on` (the walk, a spell of
digging); the queued continuations `take_n`, `gath`, `pfoe`, `srch`, `lwork`, `trav` (the main loop's free-again
chain, in that order); the long task's own going on, `fwant` (and LT_BATTER's `batter_on`); its stop, `pc_cut`; and
two that aren't the character's at all but periodic work, `burn_ask` (the lights' minute) and `chart_late` (the map
making's late job).

**Built in three stages, each its own commit, the suite whole at each:**

- **(a) The step record and the first queue.**  things.h's `struct step {edge, say, what, thing, apart, time}`; the
  step under way `rstep` (was `rs_do`, `rs_k`, `rs_what`, `rs_c`, `rs_wn`, `rs_t`: no more aliased scratch bytes);
  the edges `SE_*` (were RS_OFF ... RS_SLING, the referee's prefix): a pack taken off, a level reached into, put on,
  let fall or set down, a spark struck, blown out, lit from another, stowed in the pack, taken up, a tool got out to
  hand, put back, put in its place (a sconce, a sheath), slung.  **The first family on a queue of step records:**
  what a redip or a new light sets down to free a hand is taken up again after the work by a step pushed on a stack
  (`aside[2]`, `naside`) as it's set down and popped as the work's over, the last set down first; the other of two
  alike marks the first's entry ("You take up the other torch.") as it's pushed.  Gone: RR_D1, RR_D2, `rs_down`,
  `rs_down2`, `rs_two`.
- **(b) The step kinds out of LT_*.**  The 33 reach kinds are gone from the LT enum (it ends at LT_DIP).  A step's
  say is a verb and a word: hub.h's `SV_*` (25 verbs, `LT_STEP` 0x20 set, so a long task's kind past LT_DIP is a
  step) | `SM_*` (`SM_NEW`, `SM_OLD`, `SM_OTHER`).  The nine variant kinds are a verb and a word (GETNEW is
  `SV_GETOUT | SM_NEW`, PUTOLD `SV_PUTAWAY | SM_OLD`, UP2 `SV_TAKEUP | SM_OTHER` ...), the ternaries a bit or'd.
  **The word is the first words' alone:** the long task's kind (I_WAIT's task, `pc_task.kind`) is the verb, as
  E_TASK's light form carries 6 bits of it (its 14 and 15 its own flags), and the world's own step's TE_BEGUN takes
  its say from `rstep` (rs_for's RF_TELL is bit 15, SM_OLD's place).  The UI: reach_said's 25 sentences (33 before),
  the word put into the name the way the AW_* words ("lit", "spent") are and never with them, as before; task.c's
  doing words by verb (`DOING`); "stop sharpening" names SV_HONE, a dip SV_DIP.  The event bytes are the same but for
  a step's kind byte.  Kept as verbs of their own, being other sentences: SV_DOWN2 ("You set the other torch down."
  isn't "down at your feet" with a word), SV_LAYLIT2 and SV_INPACK2 ("too"), SV_OFFDOWN, SV_BLOWDARK, SV_LETBURN; and
  the other hand's words in s_room (`SV_LAYLIT ? SV_LAYLIT2 : SV_INPACK ? SV_INPACK2 : SV_SETDOWN ? SV_LETFALL`), a
  verb's choice, not a word's.
- **(c) The activity.**  games/dq0w/activity.h's `struct pc_activity act`: what's queued after the long task, `take`,
  `gath`, `foe`, `srch`, `work`, `trav` (were `take_n`, `gath`, `pfoe`, `srch`, `lwork`, `trav`), and the long task's
  own `fight` (fighting on, was `fwant`) and `cut` (its stop, was `pc_cut`): the flags of four files and two statics
  in one record, every reader and writer by name (dq0w.c, things.c, wgame.c, wshoot.c, wfight.c, wblow.c, wport.c,
  wroom.inc).  **The free-again chain is a pop:** dq0w.c's `act_next` takes the first queued, `take` and then the
  bytes `gath` ... `trav` in their order, each its handler from a table (`next_[]`), where the main loop had
  `take_n ? take_up () : gath ? ... : trav ? travel_on () : 0`.  LT_BATTER's next blow and fighting on stay after it,
  the long task's own going on.

**What it isn't yet (for review):**
- **The queue is an order, not a stack pushed.**  The continuations are set by their own handlers and popped in a
  fixed order, the chain's.  A real stack (each pushed as it's planned, popped last first) is the same only if no two
  are ever queued out of that order, and the order matters where two are set at once (a dig's tool put away, `work`, before
  the journey's next leg, `trav`).  Moving each to a push is the next step, one at a time.
- **The reach is still planned a step at a time** (each ask walks the tree again), not a whole queue up front: the
  world changes between steps (a denizen takes the pack, a grope fails) and the handling's own checks come first.
  The after-work take-ups (stage a's) are the one family queued ahead; the pack put back on and a tool put away
  (`rs_after_on`, `rs_using`, RR_STOW, RR_RELIGHT) are still flags `rs_after` reads in order.
- `burn_ask` and `chart_late` are periodic work, not the character's: stage (d), §7's scheduler table, wasn't built.
- The denizens: their minds post intents the world carries out; `struct pc_activity` and `struct step` don't name
  the character, so a denizen's record would be the same shape, but none was moved.
- Containers (COMMANDS-CRITIQUE.md's §4.9): the edges are the planner's; the compartments aren't there (a thing's
  `where`/`x` its holder alone), so the walk (`rs_walk`, `level_of`) still knows a container by K_CONTAINER, KF_BACK,
  IS_CLOSED and K_HANG.  With §6b's compartment table the walk reads `(holder, compartment)`, each compartment's
  `closes` and noun pick the edge (open, reach in) and its words, a row per verb in said.txt (the narration's)
  rather than reach_said's table.

**The words.**  Every dq0 test's screen was saved before and after each stage (sim/test.js's TXDIR hook, not
committed) and compared sentence by sentence (the status line and the typing row dropped, wrapped lines joined, the
order of whole sentences ignored, as the timing noise moves them).  Stage (a): 6 of 294 differ, all noise
(dq0antidote's real-time fight; dq0jobs' and dq0skykin's denizens met at other moments; dq0stopwords' "Something
stirs."; an Initiative improvement told in dq0robrest and not in dq0restbusy, a denizen's timing).  Stage (b): 5
(dq0answer's scurrying, dq0robrest's improvement, dq0skykin, dq0stopwords, and dq0reachstop's race: game 2's "halt"
lands in the step it was typed in, "You stop getting out the rations.", the side the user accepted on 2026-11-11).
Stage (c): 10, all noise (dq0antidote's and dq0denpace's real-time fights; dq0answer's, dq0jobs' and dq0mine's sounds heard from denizens met at other moments; dq0skykin; dq0passes' and dq0stopwords' "Something stirs."; dq0robrest's improvement; dq0reachstop's race on the side the user accepted).  No step's words changed: each reach sentence, each word ("the old torch", "the other torch", "the new torch") and each "You stop ..." is said as often as before in every test but dq0reachstop's race.

**Room.**  dq0w main RAM 2,243 → 2,220 (a: the step record and the stack) → 2,220 (b) → 2,188 (c: the pop's table and
loop, 32 bytes more than the ternary chain); its part 86 234 → 212, 87 2,203 → 2,188; overlays 17 134 → 119, 26
114 → 92.  The UI (dq0) 3,383 throughout; its overlays 171 bytes freer and its parts 209 at (b) (the sentences;
ovlpack's placing).  The start at seed 1: 233.3M → 233.5M (a) → 233.7M (b) → 233.2M (c) cycles (245M the budget);
at 123456789 310.6M → 311.0M (b) → 311.4M (c).

<!-- DECISIONS.md:3893-3955 at d35194df -->
## The meta programs' library, their state record, one theme compiler (2026-11-11, as built, for review: ENGINE-CRITIQUE.md's §9 and step 11)

The programs between dungeons (dq0life, dq0acct, dq0town, dq0schol, dq0end; themegen and themec beside them) each had their own copy of the same helpers, passed the town's state to each other through long argument lists and a file of their own, and the theme compiler was written twice, in JS for the build and in C for the Hydra, the tests holding the two to the same bytes.  Four stages, each its own commit:

**1. libdq0meta** (games/dq0/meta/meta.c).  One C source a program compiles in, a section at a time:

    #define META_WORDS
    #include "../dq0/meta/meta.c"
    #undef META_WORDS

so each program takes only what it calls and puts each section where it wants it (dq0town's ask, width_read, seed_of and launcher in its overlay 1; dq0acct's width_read in its overlay 2, value_of in 4, the launcher in 5), the names a section reads (path, ans, asks, said, tw, rbuf ...) declared by the program first.  The sections: fail (META_NAME's "NAME: why", exit 1), in_ (a path), num, seed_of, the words by the console's width (say, sayn, amount, say_flush; width and ob, META_WB and META_OB the buffers' sizes), width_read, ask, words_of and words_read (the "key | value" text, META_TEXT_ON/OFF round each reading of it: dq0town's bank), value_of (a rumour's value: the first, or with META_VALUE_N the n'th), and the launcher (META_LAUNCH: the system's SPAWN and WAIT themselves, named META_SPAWN and META_JOIN: the UI's hub_run and hub_join are the library's; META_RUN: meta_run, both; META_ARG, META_ARGN: arg1, arg, argn).  META_LINK, empty or static: shared with the program's other files (dq0acct's words, its overlays call them) or not.

| helper | was in | now |
|---|---|---|
| say, sayn, amount, the words' buffer and wrap | dq0acct, dq0town, dq0schol (the town's and the scholar's 6 of 68 lines apart: the buffers' sizes, static or not, say_flush) | META_WORDS (META_WB, META_OB) |
| the launcher | dq0life run, dq0acct's town.c arg/argn and SPAWN/WAIT, dq0town run and arg, the UI's hub_run and hub_join | META_LAUNCH, META_RUN, META_ARG, META_ARGN |
| words_of, the text read | dq0town (its bank put in and out), dq0schol | META_KEYS (META_TEXT_ON/OFF, META_LINE, META_TEXT_MAX) |
| value_of | dq0town (the n'th value), dq0acct's quest.c (the first) | META_VALUE (META_VALUE_N) |
| num | dq0life, dq0end | META_NUM |
| in_ / in | dq0life, dq0acct, dq0town, dq0schol, dq0end; themec, themegen | META_PATH |
| fail / die | dq0life, dq0acct, dq0town, dq0schol | META_FAIL |
| seed_of | dq0life, dq0town | META_SEED |
| width_read | dq0acct's o_width, dq0town, dq0schol | META_WIDTH |
| ask | dq0town, dq0schol | META_ASK |

Kept local, as they differ in what they do: dq0acct's ask (dq0auto, " > "), dq0end's words (its spaces kept, an indent stands), dq0life's out (no wrap), hmchar's in (another signature, under its floor), the rumours' files read (dq0acct's and dq0town's: the town counts themegen's).  Every program's words the same: where the copies differed it's a parameter or the difference did nothing (dq0schol's ask's word_out before out_flush).  One change in what a program does: value_of now stops at a key line with no bar inside the rumour's own lines (dq0town's way; dq0acct's read on past the rumour's end), which no rumour file has.

**Why a shared C source, not the paged ROM's library** (modules/dq0lib).  Measured against what the ROM would save: a ROM call is an XCALL to an assembly entry (dq0lib has no RAM of its own: the words' buffer, their column and the console's width would be the caller's, passed in), its stub in each program (dq0lib.inc, as the tasks have it) some 130 bytes of main RAM, and the helpers here are 100 to 400 bytes a program.  The meta programs aren't the game's tasks: but dq0acct (its main RAM at 2,152, its words in main RAM for its overlays to call, the same as before) they have thousands of bytes to spare, and a shared source puts each helper where its program had it, so nothing moves.  The UI's hub_run and hub_join the library's too, its image byte for byte the same.

**2. One state record** (games/dq0/records.def, struct between, between.dat).  What the account (dq0acct) gives the town (dq0town) was seven arguments (-t -i -p -k -h -r -n: the town, the inn, the days passed, the keep so far, the habit, Rhetoric's and Intrigue's EMLs), the town gave the scholar's house (dq0schol) its name and first command as arguments (-t, -x), and town.dat (struct townrec, in dq0acct/town.h) went from the town to the account and between the town and the house.  Now one record in the save's folder, declared in records.def (a sibling of msgs.def): its fields, their types, what each is, and the file's name, magic and version.  hubgen.js reads it with the headers: it writes obj/gen/dq0rec.h (the struct, BETWEEN_FILE, BETWEEN_MAGIC, BETWEEN_VER and the defines it wants), lays it out with cc65 with the rest (dq0hub.json's S.between) and records each field's type (dq0hub.json's records), so the tests' decoder is the same pass's: `load ().record ('between', bytes)` gives { town, inn, passed, keep, ..., learnt: [..], ok } (a char array its string, an array its numbers, ok the magic and version).  The flow: the account fills it and writes it, runs `dq0town -d folder -l lib`; the town reads it as it starts (none, or another magic or version: it fails, said), writes it before it runs `dq0schol -d folder -l lib` (the first command in it) and reads it back after, and writes it as it's left; the account reads it and removes it.  The scholar's house reads it (none: a stay begun, all 0) and writes it back; its -t, -b, -c, -w (the tests' scenes, run alone) set the town, the town's kind, the money and the writings over the file's, so dq0scholar types what it did.  An old folder's town.dat is passed by (read by nothing now; it was the town's, removed by the account as it read it).  dq0life's (-d, -s, -l), dq0acct's (-d, -l), dq0end's and themegen's arguments are where things are or what to make, not state passed on: as they were.

**3. One theme compiler: themec's (C).**  What uses it: themegen runs it on the Hydra as a game begins (hy_exec in its place) to make a game's theme.dat from the theme.txt it just wrote, the history's, from any seed; a hand-written theme is tried with it from the shell.  So the game needs a compiler on the Hydra, and the only one it can have is the C one: the JS one could be the only compiler only if themegen made the record itself (a compiler in C again).  Kept: themec, games/themec/comp.c.  The build runs it in the emulator (tools/themec.js; no host C compiler is on the build's machines, and the emulator is the build's already: hysnap's snapshot): a paged ROM of the modules and t_rc, an SD card made for the run (themec, bestiary.dat, the themes' texts), "themec -l /sd/0 -o tN.dat tN.txt" typed for each, its status and words read off the console and the records off the card (hydrafs).  Each answer is kept (obj/themec, by the hash of themec, the bestiary, the modules and the text).  Who runs it:

- the build, for the stand-in themes: games/dq0/make.js's after (build.js calls a make.js's after once every game's built: dq0's make.js runs before themec is built), obj/games/dq0t.dat themec's records with their count and file's name put in, as make.js did;
- the build, for the goal kinds: games/themegen/make.js holds each goal block made a theme to themec (compiled and checked, no record), after it writes bestiary.dat;
- the tests: tests/lib/dq0.js's THEMEC (themes) where they compiled with make.js's: dq0themegen (themegen -s 3's theme compiled and checked, theme.dat held to its record, byte for byte), dq0history (theme.dat's story), dq0why and dq0handtheme (the theme.txt a theme); dq0themec holds the records compiled on the Hydra to the build's (dq0t.dat's, their count 1 and no file's name: make.js's standIns), the same three, byte for byte.

Gone: make.js's compile and check (some 300 lines) and bestiary.  The stand-ins' records came out byte for byte the same (themec's in the emulator held to make.js's before it went; obj/games/dq0t.dat the same as before), and every image (the paged ROM's chips, the SD card) the same as stage 2's.  The build's time: some 3 s more for the emulator's run (the three stand-ins, measured on a quiet machine; the six goal kinds about as much), nothing when the themes, themec, the bestiary and the modules haven't changed (kept); the whole build 2m 27s.

**4. tools/dq0build.js.**  The readers the make.js files shared by copy: keyLines (the "key | value, indented continuation" reader: dq0/make.js's lines, themegen/make.js's read), blocks (dq0d/make.js's, on keyLines), rows (the one-liner: dq0d's, dq0s's, themegen's bestiary's), and packOnly (dq0harn's, dq0m's and dq0p's make.js, the same three lines: each now `module.exports = require('../../tools/dq0build.js').packOnly('dq0harn')`).  The build's outputs byte for byte the same.

**Room** (bytes free; before, after):

| program | main RAM | overlays |
|---|---|---|
| dq0 | 3,415, 3,415 (its image the same) | the same |
| dq0acct | 2,152, 2,157 | 4: 254, 250; 5: 2,166, 2,008 |
| dq0life | 18,198, 18,095 | |
| dq0town | 3,211, 3,379 (its arguments' parsing gone) | 1: 6,493, 6,443 |
| dq0schol | 5,333, 5,143 (the record's 208 bytes, the first command's 64 among them) | 1: 630, 630 |
| dq0end | 20,959, 20,959 (the same) | |
| themec | 902, 902 (the same: under the floor, a warning, as it was) | |
| themegen | 712, 712 (the same: as above) | |
| hmchar | 559 (untouched) | |

**Held to:** every dq0 test's transcript before and after each stage, sentence by sentence (the scratch's sent.js, a hook in sim/test.js not kept): RESULTS.  The stand-ins' records, themec's in the emulator against make.js's before it went: the same bytes.  The suite: all 444 tests pass after stage 1 and after stage 4.

**For review:**

- **The record's reach.**  between.dat holds what passes in the town.  The other files the programs share (life.dat, kit.dat, townkit.dat, kit0.dat, clues.dat, heard.txt, rumours.txt, quest.txt, lives.dat) are as they were: life.dat is life.h's struct life (the game's too), the kit files the world's; records.def could declare them next (life.h's struct life is the first candidate, its layout read by the tests by hand now).
- **The emulator in the build.**  The stand-ins and the goal kinds are compiled in the emulator: a build with themec, the bestiary, a goal or a stand-in changed runs it (some 3 s each); one without reads the kept answers.  A host build of comp.c would be quicker, if a C compiler is ever on the build's machines: the same source (it reads its files with open and read).
- **The tests' theme oracle** is now the same compiler as the game's: a theme compiled on the Hydra is held to themec's run alone, not to a second compiler's.  What the two compilers' agreement checked before (that each read the format the same) has no second reader to disagree with now; comp.c is the format's statement.

<!-- DECISIONS.md:3956-4075 at d35194df -->
## One wording for the player and the denizens (the user's, 2026-11-11; the choices as built, for review)

**The user's decision:** an action has one wording, the character's and a denizen's, said with the actor swapped
("You open the door." / "The bandit opens the door.").  Stage 3 shared one row's piece (@open); every other doing the
character and a denizen both do was worded twice.  Now each is one piece of said.txt, used by the character's row
(PORTAL's, DIG's, TASK's, BEAT's) and the denizen's (DEN's, BEAT's), and the duplicates are gone.  The player's
wording was kept where it could take an actor (all but one, a thing dropped); where it couldn't ("The lock gives with a click.", no one doing it),
the actor was put into it, the player's words kept after it; the denizen's was taken where it was the more exact.

- **The renderer** (said.c, saidgen.js):
  - `{its}`: the actor's, "your" or "its" (SC_ITS, 0xEF); `{'s}` after `{Actor}`: "r" or "'s" ("Your", "The
    bandit's": SC_POSS, 0x7F).  Codes, not fields: the table's 31 of 32 fields are as they were.
  - A kind's actor may be a field of its own: said.txt's `kind BEAT beat whose` (said_ka, a byte a kind): E_BEAT's
    whose (0 the character, else the actor plus 1), not the event's who.  `{Actor}`, `{s}`, `{its}`, `{'s}` and
    `{actor?a|b}` read it.
  - A piece two kinds use is compiled for each (its fields are the kind's), and kept once where the bytes are the same
    (saidgen.js): @open, @close ... are PORTAL's and DEN's, one copy.
- **The UI:** task.c's TE_BEGUN gives what's forced or heaved at as obj (noun: "the window"); s_noise_told (part 14)
  clears obj for a denizen's row (its piece's "the door"); s_news (part 11) gives the character's limbs' rows its
  body, BD_TWO (said_n 0: it was left as the last row's).

**The pairs, before and after** (the character's row, the denizen's; *chosen* the one now said by both):

| action | the character's, before | a denizen's, before | one wording now (piece) |
|---|---|---|---|
| open a door | You open the door{, revealing ...}. | The bandit opens the door. | unchanged (@open, stage 3's) |
| close a door | You close the door. | The bandit shuts the door. | {Actor} close{s} the door. (@close: the player's) |
| unlock with its key | You unlock it with its key. | The bandit unlocks the door. | {Actor} unlock{s} the door with its key. (@unlock) |
| lock with its key | You lock it with its key. | The bandit locks the door. | {Actor} lock{s} the door with its key. (@lock) |
| bar a door | You set the bar in its brackets: the door is barred. | The bandit drops a bar across the door. | {Actor} set{s} the bar in its brackets: the door is barred. (@bar: the player's) |
| lift its bar out | You lift the bar out of its brackets, and set it down. | The bandit lifts the door's bar. | {Actor} lift{s} the bar out of its brackets, and set{s} it down. (@lift: the player's) |
| forcing begun | You set to forcing the door. (TASK.begun's) | The bandit strains at the door. | {Actor} set{s} to forcing {the door}. (@forcing: the player's) |
| forced | With a crack the door bursts open{, revealing ...}. | The bandit bursts the door open. | With a crack {actor} burst{s} the door open{...}. (@burst) |
| forcing held | The door holds. | The bandit can't force the door. | The door holds against {actor}. (@held) |
| broken down by blows | It gives way, and crashes open{, revealing ...}. | The bandit breaks the door down. | {Actor} break{s} the door down: it crashes open{...}. (@broke) |
| picking begun | You set to picking the lock. (TASK.begun's) | The bandit works at the lock. | {Actor} set{s} to picking the lock. (@picking: the player's) |
| picked | The lock gives with a click. | The bandit picks the lock. | {Actor} pick{s} the lock: it gives with a click. (@picked) |
| heaving begun | You set to heaving at the portcullis. | The dwarf heaves at the portcullis. | {Actor} set{s} to heaving at the portcullis. (@heaving: the player's) |
| winding begun | You set to winding the winch. | The dwarf winds the portcullis's winch. | {Actor} set{s} to winding the winch. (@winding: the player's) |
| raised (winch, by hand) | The portcullis grinds up, and catches, raised{...}. / The portcullis rises, and catches, raised{...}. | The dwarf raises the portcullis. | {Actor} raise{s} the portcullis (@raise), the character's rows adding ": it grinds up, and catches{...}." / ": it rises, and catches{...}." |
| lowered | You loose the winch: the portcullis crashes down. | The dwarf lets the portcullis fall. | {Actor} loose{s} the winch: the portcullis crashes down. (@lower: the player's) |
| bending begun | You set to bending the bars. | The dwarf strains at the bars. | {Actor} set{s} to bending the bars. (@bending: the player's) |
| bent apart | The bars bend apart: there's room to get through. | The dwarf bends the bars apart. | {Actor} bend{s} the bars apart: there's room to get through. (@bent) |
| digging begun | You set to digging: some 12 feet of it, by the look of it. | The dwarf miner digs into the wall. | {Actor} set{s} to digging (@digging: the player's; its feet the character's row's) |
| the roof come down | The rotten rock splits, and part of the roof falls into the hole. | The dwarf miner is showered with rock from the roof. | The rotten rock splits over {actor}, and part of the roof falls into the hole. (@roof) |
| a thing dropped | Your sword falls from your hand. | The bandit drops its sword. | {Actor} drop{s} {its} {thing}. (BEAT.drop: the denizen's) |
| a thing broken | Your sword breaks! | The bandit's sword breaks! | {Actor}{'s} {thing} breaks! (BEAT.broke: the same words) |
| up, not up, fallen | You get to your feet. / You struggle, but can't get up. / You fall. | The bandit gets to its feet. ... (by its body) | {Actor} get{s} to {its} feet ... (BEAT.risen, .not_up, .fallen: the same words; a creature's body its own) |

**The choices, for review:**

- **"with its key", not "with your key".**  The key is the door's (a key fits its own lock alone), and a chest's are
  said so already ("You unlock the chest with its key.", hthings.c's): "with your key" would have told the door and the
  chest two ways.  The pronoun is there (`{its}`) if the user wants "your key" / "its key".
- **The door named, not "it".**  "You unlock it with its key." read after "unlock the door"; a denizen's needs the
  door named, so the character's names it too ("You unlock the door with its key.").
- **An impersonal sentence given its actor.**  "The lock gives with a click.", "The door holds.", "With a crack the
  door bursts open", "It gives way, and crashes open", the portcullis's "grinds up": no one in them to swap.  Each keeps
  its words and takes the actor: "You pick the lock: it gives with a click.", "The door holds against you.", "With a
  crack you burst the door open", "You break the door down: it crashes open", "You raise the portcullis: it grinds up,
  and catches" ("raised" gone: the verb says it).  The shutters' (a window's, the character's alone) are as they were.
- **A timed work begun is the character's words** ("sets to forcing the door", not "strains at the door"): the
  character's are said by TASK.begun's rows, now one for each of the five a denizen does too ([task=LT_FORCE],
  LT_PICK, LT_HEAVE, LT_WIND, LT_BEND), their words the same as the general row's were.
- **A raise is one doing for a denizen** (E_DEN's DE_RAISES, by the winch or by hand), two for the character: the
  piece is the clause they share, "{Actor} raise{s} the portcullis", the character's rows adding how it went.
- **Lifting the bar is PORTAL.lifted's**, not PORTAL.unbarred's ("You lift the bar and open the door"): a denizen lifts
  a bar on its side first (dq0d.c's PA_LIFT), then opens the door, two doings told each.
- **A thing dropped is the denizen's words** ("You drop your sword.", not "Your sword falls from your hand."):
  WB_DROP is a fumble, but also a weapon let go to take up a bow (wshoot.c's), which "falls from your hand" misreads;
  and the table has no verb agreement by the thing, so the first build's "The soldier's rations falls from its hand."
  (dq0denhands) read wrong, where "drops its rations" can't.  The player's own words had the same fault for a plural
  thing ("Your rations falls from your hand.").
- **A named denizen's limbs** now read "Grub gets to its feet." (was "The Grub gets to its feet.": the row put "The"
  before the name).

**Not shared** (one side's alone, so one wording already): a rally, a snore, talk, the alarm, a door slammed as its
denizen flees (the character closes), a denizen's blow at a door (the character's blows are told by what they do:
"Your blows begin to tell on it."), a door hung, a way walled up, a pastime; a way dug through, told from where it opens
(its digger may be out of sight: "A fresh tunnel opens ahead to your left."); the denizens' thing snatched up, drawn,
readied, eaten, handed or thrown (the character's are its commands' replies, not the table's); the character's own
refusals and its windows.  TASK.away ("Quicker, the rat backs away from you.") and TASK.break ("You back away from the
rat.") already say the same doing the same way.

- **Room** (the build's table):

  | | before | after |
  |---|---|---|
  | dq0's main RAM | 3,415 | 3,415 |
  | overlay 27 (the renderer, the words most said) | 911 | 695 (SC_ITS, SC_POSS, the actor by kind, said_ka) |
  | overlay 28 (the rows' first bank) | 6 | 0 (8,192 bytes of rows: the packer fills it, a kind's rows wholly in a bank) |
  | overlay 29 (their second bank) | 6,499 | 6,577 |
  | the rows, all told | 9,872 bytes, 257 rows, 19 pieces | 9,807 bytes, 262 rows (TASK.begun's five), 38 pieces (20 shared, kept once), 31 fields |
  | overlay 3 (task.c: what's forced or heaved at) | 2,121 | 2,081 |
  | part 11 (s_news: the character's body) | 2,349 | 2,346 |
  | part 14 (noise_told: obj cleared) | 1,643 | 1,637 |
  | overlays 15, 18 (the packer's) | 311, 758 | 308, 752 |
  | dq0w's main RAM | 2,182 | 2,182 (the world untouched) |

- **The start** (dq0starttime): seed 1 234.0M → 233.4M cycles (budget 245M), seed 123456789 310.6M → 311.4M.

- **Checked:** every dq0 test's screen kept before and after (a temporary hook in sim/test.js, not committed), their
  sentences compared (the status line dropped, wrapped lines joined, the order of whole sentences ignored).  41 of 294
  differ.  Every sentence new or gone is one of the rewordings above: a door broken down (dq0batter, dq0battering,
  dq0doorfar, dq0highkinds, dq0strike, dq0wordsfix), forced (dq0moving, dq0pure, dq0timed, dq0traps, dq0watch), locked
  and unlocked with its key (dq0denkey, dq0foundkey, dq0keysteps), a portcullis raised (dq0portc), bars bent (dq0break),
  the roof come down on a dig (dq0dig), a thing dropped (the character's, in 19 tests: "You drop your torch."), and the
  denizens' (dq0allythrow, dq0denportc: "sets to heaving at the portcullis"; dq0denkey: "sets to picking the lock",
  "sets to forcing the door"; dq0tunnel: "sets to digging").  The rest is the known noise: the real-time fight in
  dq0denpace; denizens met at other moments (dq0answer, dq0jobs, dq0mine, dq0prompt), dq0answer's a key typed ahead
  echoed after a sentence's stop ("some bones.."); "Something stirs.".  **The first build** said "The soldier's rations
  falls from its hand." (dq0denhands): a thing dropped is the denizen's words since (above).
- **Tests' strings changed** (the words the decision changed, each check's meaning kept): tests/lib/dq0.js's
  DQ0_REPEATS (a dig's "The rotten rock splits over you ..."; a fumble's "You drop your ...", "... drops its ...", a
  named one's too); denizens.js (dq0tunnel's and dq0mine's "sets to digging", dq0denportc's "sets to heaving at the
  portcullis", dq0denkey's "You lock the door with its key.", "sets to picking the lock", "sets to forcing the door");
  parser.js (dq0wordsfix's "You break the door down: it crashes open"); map.js (dq0foundkey's "You unlock the door with
  its key", the old words' alternative gone).
- **Tested:** all 444 tests pass.

<!-- DECISIONS.md:4076-4190 at d35194df -->
## One record store (2026-11-11, as built, for review: ENGINE-CRITIQUE.md's §6 and step 7)

**What the survey found.**  Not five stores paging by region but seven, in three tasks, and each its own table, its
own least-lately-wanted loop and its own copy of vfile.inc (the one thing they shared: the save folder's versions):

- the world's: the change records (store.c, wfile.c: `rgSXXYY.dat`, pages of the hub's change banks, their table
  the world bank's `cpage`), the strata's records (sfile.c: `stSS.dat`, a slot each), the traces (wtrace.c,
  wtrfile.c: `trSXXYY.dat`, a directory of chunks, `THD`'s `ch`), the item store's paging (ipage.c: `itSXXYY.dat`,
  `IPG`'s `r`); the traces and the things both keep a region's file of chunks' groups (each its own `others ()`);
- the denizens': their regions (dq0d.c: `dnSXXYY.dat`, `hd`'s `reg`, and an unused copy of its lookup in part 4);
- the maker's: the seen maps (changes.c: `smSXXYY.dat`, `sreg`), the change records as it lays them over a chunk
  (rg read), the big plans' file's name (`plSS0000.dat`); and the chunks' cache's slot for a new chunk (make.c's
  `pick`);
- every reader of the chunks (chunk.c's `find`: the world's, the denizens', the senses'): the cache's lookup.

The UI has none.  Their eviction agreed (the least lately wanted, its 16-bit stamp the oldest, round as it wraps,
`s - t > 0x8000`), but for the chunks' cache, which compares plainly (`<`): kept as it was (RTF_PLAIN), as changing it
would change which chunk is made way for once the clock wraps, and so the seen maps' writes.  What each keeps besides
differs: a free entry first or not, the one at hand, a box about the character (two chunks each way, one, or a region
each way; with dq0 -C none, or only its own), an actor's things (ipage's `kept_for`), a slot a task's reading or one
stocked and not yet marked (the cache's).

**What's built: the record store in the paged ROM's library** (modules/dq0lib's `RP_*`, games/dq0/rp.h its C side,
dq0lib.inc's `rp_x`: a block of the caller's a call, its first byte the entry):

- **One file-naming scheme and one save-and-restore path** (`RP_NAME`, `RP_OPEN`, `RP_MAKE`, `RP_DONE`; games/dq0/rp.inc
  the four calls in C, `rp_name`, `rp_open`, `rp_make`, `rp_done`, a swapped part's copy as vfile.inc's was): a
  region's file `preSXXYY.dat`, written whole as a new version over a slot that's neither the save's nor (three slots)
  the newest, its header last; read as the newest valid version by the hub's vsaved and vbase.  The save is the
  stamps moved on (wfile.c's `store_commit`, unchanged).  **Each kind's count its own** (`struct rpk`: vlast,
  vcount, the main RAM's), as each store's copy of vfile.inc had its own, so every stamp, and so every file's bytes,
  is as it was; one count a task would have given the same files other stamps.
- **One lookup** (`RP_FIND`): an entry by its key (a mask of an entry's first eight bytes, as the tables are laid out:
  their layouts, in items.dat, seen.dat and dens.dat, unchanged), its stamp made the clock's next if the store wants
  it, a free one among the first n, from an entry on (a region's pages one after another).
- **One eviction policy** (`RP_OLDEST`): the least lately wanted, round or plainly, but the key's own (`RTF_HAND`), a
  box about a centre (`r` each way, one byte or two, on its stratum: the near rule each store had), and those the
  store's own routine keeps (a callback into the caller's code, from the ROM: ipage's `kept_k`, make.c's `busy`); a
  free one first (`RTF_FREE`).
- **A region's file of chunks' groups** (`RP_GROUPS`): the groups read past, each but the chunk at hand's copied into
  the new version, or to the chunk at hand's: the things' and the traces' `others ()`.

Each store keeps only its own logic: what its records are and where (pages, buckets, a bank of denizens, maps), what
a record replaces, what's kept besides (the regions known to have no things, an actor's region), the records read,
renumbered and written.  `rp_dirty`, `rp_flush` and `rp_save` as ENGINE-CRITIQUE.md drew them aren't calls: what's
dirty and when it's written is each store's (the pages' `dirty`, the traces' `TC_DIRTY`, the things written as a
chunk goes out), and the save is the stamps' commit, already one.

**Where it lives, and why the ROM.**  The paged ROM's library (§1.4), not a C source compiled into each task: vfile.inc
was compiled six times (four of the world's parts, the denizens' part 21, the maker's main RAM), and with it the C
library's file calls it took in; the tables' loops a dozen times.  In the ROM they're one copy and no task's room: the
library is assembly, its scratch the caller's block (a part's own data, or the main RAM for a table read in play),
its kernel calls its own (the fds the cc65 library's, the same numbers).  Its bank has room.  The cost is an XCALL
each call (some 120 cycles), nothing beside a file's work.

**Measured.**

- **The chunks' cache** (the one called in play: a reader's `at` the long way, as its square's in another chunk than
  its last): the world's `find`, entry to return, in a game of seed 5's (34 calls): median 1,195 cycles before, 1,326
  after (least 619 and 1,084: the XCALL and the block made), 0.06M and 0.08M cycles of a 461M game.  RP_FIND built
  for it: the key's bytes as offset and value pairs in the call registers first, an entry some 60 cycles to the C
  loop's some 155, so the ROM's quicker on a long search and slower by its call on a short one.  dq0lookway 357.0M
  cycles to 358.0M, dq0travel 2,727.6M to 2,728.8M.
- **The start** (dq0starttime): 234.0M cycles at seed 1 before (310.6M at 123456789), 232.3M after (308.6M); the
  maker's main RAM's code the less.
- **Room** (bytes free):

  | | before | after |
  |---|---|---|
  | dq0w main RAM | 2,182 | 2,236 |
  | dq0w part 17 (ipage.c's table) | 1,222 | 1,529 |
  | dq0w part 18 (wfile.c's files) | 422 | 1,791 |
  | dq0w part 19 | 638 | 654 |
  | dq0w part 20 (ipage.c's files) | 120 | 1,518 |
  | dq0w part 22 (sfile.c) | 1,554 | 2,811 |
  | dq0w part 28 (wtrace.c's store) | 483 | 682 |
  | dq0w part 30 (wtrfile.c) | 329 | 1,713 |
  | dq0w part 38 (store.c's put) | 1,860 | 1,853 |
  | dq0d main RAM | 2,184 | 2,379 |
  | dq0d part 1 / part 21 | 811 / 897 | 907 / 2,369 |
  | dq0m main RAM | 2,737 | 4,635 |
  | dq0p main RAM | 3,122 | 3,176 |

  The world's overlays as the packer placed the parts (7: 130 to 2,897, 10: 328 to 1,712, 21: 48 to 1,725; 17: 119
  to 22, 27: 114 to 23); the UI, the referee and the rest as they were.

**Checked byte for byte** (a scratch script, not committed): every save-folder file each task wrote (each OPEN,
CREATE, WRITE's bytes hashed, SEEK, by file: the kernel's fd table, TA_FD, read at each call to name an fd), in order
per task, over 32 games: 8 seeds (3, 5, 7, 42, 99, 1000, 31337, 70000), each with -C -T -T -T, -C -T, -T -T and -C (the
small caches, so every store makes way; strata below), a save folder, a fixed random player's commands (the soak's
`choose`, its own stream, not the game's words: the same commands whatever the timing), quit, carried on and saved
twice: some 1,200 to 2,500 region files' versions a game.  The baseline itself isn't the same at each power-up (the
tasks' timing: a walk in the dark interrupted by a sound a step sooner or later, so its later commands land
elsewhere), so it was run at four power-ups; each stage's every game is one of the four's byte for byte (life.dat left
out: its date the host's clock).  One game at stages 5 and 6 was another of its timing's outcomes (that build at
another power-up gave the baseline's bytes); at the end, 31 games the first power-up's, one the third's.  The
sentences (the transcripts, by sentence) the same but that one's.

**The suite: 443 of 444; dq0reachstop's race flipped (for review).**  Its game 7 types "stop" as "take all"'s first
"Taken." shows, at the turns pace, and wants the take ended before the next thing (the tinderbox) is reached for.
That's the race "The referee's skeleton" found (whether a line typed as a step's words show lands in that step or the
next turns on some thousands of cycles; the user accepted either step for its game 2, not yet for game 7).  From
stage 6 on (the denizens' regions on the record store) the next thing's reached for first ("You reach for the
tinderbox. Taken. You stop there."), at five power-ups of five; the baseline, and this build with dq0d.c alone put back
as it was, pass at five of five.  Not a change of what the denizens do: their files, every other store's and the
sentences of the 32 games are the same; their work's timing is (the regions' lookup and the part it's in quicker), and
with it how soon the UI goes on to the next thing.  Not made to pass: neither the test loosened nor a delay put in.
Wanted: the user's word on game 7 as on game 2, or a pace that waits on a line typed as words show.

**Left.**  The plans' pages (changes.c's `plt`, its own least-lately-wanted loop over a file that isn't versioned) and
the senses' caches of their own (none paged to a file) aren't on it; the chunks' cache's plain comparison
(RTF_PLAIN) could go round as the others do, a behaviour change (which chunk's made way for after the clock wraps) to
be taken as one; the item store's `items.dat` and the traces' flush at the save stay each store's; an `rp_save`
walking every store's dirty entries would want the stores in one task.
