# The engine: a critique, and where it could be more modular, concise and generic

A companion to COMMANDS-CRITIQUE.md, which covers the command path. This document looks at the whole engine:
- the tasks and the hub that joins them;
- the world, the minds, the senses, the maker and the referee;
- the programs between dungeons;
- the build and the tests.

The aim is to find where the same idea is built many times, where per-feature code could be a generic mechanism
driven by data, and where structure is set by accident rather than design.

The numbers come from grep, wc and diff over the current tree (reborn/games, 2026-11-06). Treat counts as close,
not exact. Paths are relative to reborn/games.

---

## 0. The shape, and what's right about it

| Task | Lines | What it does |
|---|---|---|
| dq0 (UI) | 20.7K | the screen, the keys, commands, the words |
| dq0w (world) | 28.6K | the clock, the rules of the place, items, portals, fights, the PC's and denizens' bodies |
| dq0d (denizens) | 6.4K | minds |
| dq0p (senses) | 5.1K | perception, light, sound layer |
| dq0m (maker) | 5.4K | dungeon growth, the map |
| dq0harn / dq0null (referee) | 6.0K / 1.0K | the ruleset |
| dq0c (parser) | ~4.4K asm | the command grammar |
| dq0s (sound) | 0.6K | |
| dq0life, dq0acct, dq0town, dq0schol, dq0end, hmchar, themegen, themec | ~9K | between dungeons, characters, themes |

About 110,000 lines in all, for a 65C02 with 8K banks and a 2K main-RAM floor per program, joined through one
shared segment, the hub.

Several of the engine's decisions are sound and should be kept:

- **The tasks are split by concern**, and **each queue has one writer**, so nothing needs a lock.
- **Determinism at the turns pace.** Seed plus commands means the same game, which is tested with `-same` in the soak.
- **Perception comes first.** The UI never reads the dungeon, only what the senses report.
- **One intent structure** carries out the player's and the denizens' actions. wport.c's "one path for every actor"
  and wtake.c's take rule show it working.
- **Denizen kinds are driven by flags.** dq0d has no switch or string compare on kind identity at all; per-kind
  behaviour is `K_*` flags from kinds.txt. **This is the model for the rest of the engine.**
- **The catalogue and outfits are data**; burns, snaps and throws are tables.
- **The tests check what happened, not just the words**: the hub recorder (dq0meaning.js), the hub writers' check
  (hubown.js), and the soak runner.
- **LZ4-packed overlays** with the unpacker in a shared paged-ROM library. That's the important precedent for §1.

The problems below aren't bad ideas. **The same idea is re-implemented locally, again and again.** Two forces cause
that, and most of the findings trace back to one of them:

1. **Memory placement is done by hand, and copying code is the cheapest way out of a full part** (§1).
2. **There's no schema for the data the parts share**: hub messages, events, constants and records are defined in
   prose comments and kept in step by hand (§2).

Fix those two, and much of the rest becomes mechanical clean-up.

---

## 1. Memory placement dominates the code's structure

**The evidence.**

| Task | Swapped parts | SWFN stubs (hand-written) | OVFN stubs | cfg lines (hand-written) |
|---|---|---|---|---|
| dq0w | 102 | 227 | 0 | 392 (306 are SW entries) |
| dq0 | 26 | 76 | 70 | 186 |
| dq0d | 23 | 62 | 0 | 120 |
| dq0harn | 4 | 16 | 47 | 80 |
| dq0m | 3 | 5 | 23 | 82 |
| dq0p | 6 | 13 | 0 | 71 |

That's **399 SWFN and 140 OVFN stubs, and 851 lines of linker config, all hand-written.** Each function's part is
chosen with `#pragma code-name`, about 290 times across the tasks. Nothing generates the stubs or the config. Only
build.js checks them afterwards.

**What it causes.**

- **Copies of code, each justified as "the main RAM's room" or "part N's room".** About 54 comments in dq0w, 31 in
  dq0d, 7 in dq0p. Some of the copies:
  - the chunk-cache lookup four times in dq0d.c alone (`made`, `made9`, `made10`, `made16`);
  - `span`/`span67`/`span68` in one file (wshoot.c);
  - `abs_`/`abs6` and `fz`/`fz6` (dq0p/sight.c);
  - ways.inc compiled again in the senses under renamed symbols;
  - `put_down`/`put_down63`, `and_last`/`and_last36`/`and_last62`;
  - `near_foe`/`near_foe12`, `lof`/`lof1`, `trap_at`/`trap_at21`;
  - wkept.c's `kept71`, `room_lit71`, `dr71`.
- **Code moves as parts fill up**, and the comments record the moves: "part 11 → 36 → 40/61", "part 60's to 51",
  "part 2's till then, its room; overlay 29's". "Part N" is mentioned in comments 145 times in things.c, 154 in
  dq0w.c, 96 in dq0d.c, and 66 in main.c. Those references go stale each time code moves.
- **Dispatch is split by size, not by meaning.**
  - In the world, the PC's intents go `intent` (main RAM) → `intent_few` (part 53) → `intent_more` (part 37).
  - In the UI, commands go `s_command` → `chores` → `more` → `rarer` → `missiles`.
  - Where a case lives says how big the code around it was, not what kind of case it is.
- **The bank register is put back by hand.** There are 825 `hub_map(` calls in dq0w, 657 of them `hub_map(0)`.
  hub0.h exists only to save two bytes per call.
- **Hidden parameters.** Globals are set before a call because a stub can't cheaply pass more arguments: `evb`
  carries `event()`'s b; `tgx/tgy/want_/barw_` are set before `door_work`; `go_near` is set and reset around
  `go_to`. The UI has the same pattern (`tx/ty`, `shand`, `strike`, `hold_w`, `intend_a`).

**What I'd do: let the build do the placing.**

1. **A placement manifest instead of pragmas.** One file per task listing `function → part` (or a group name). The
   build generates bank.s's SWFN/SWPART/OVFN tables and the .cfg's MEMORY and SEGMENT lines from it. The `s_`
   naming convention and stubs become generated output. Moving a function is a one-line manifest change, and no
   code comment needs to say where it lives.
2. **Automatic placement, as a later step.** ld65's map file plus the compiler's call graph are enough for a packing
   tool:
   - give each function a size and a caller set;
   - bin-pack functions into parts so that callers and callees share a part where possible;
   - add a stub only where a call crosses parts;
   - honour pins for hot paths (the first-letter budget, D17).

   The partsChecked and headroom checks in build.js already compute half of this. Today a human solves that packing
   problem each time a part fills.

   *Built 2026-11-07 (games/dq0/pack.js, parts.js's banks).* The build reads each function's and table's bytes from
   the assembler's listing and what each refers to from the compiler's output, and groups them into pieces that
   can move: a stubbed function and what only it reaches. Calls are routed by part, so a function moved is its pragma
   alone. A part past the swap area moves out the piece with the fewest calls to the rest of it, to the part with
   the most calls to the piece whose bank has room, within the same file of banks. It doesn't move a piece whose
   calls across the new boundary would carry a pointer, one that stores an address in a global the other side
   reads, or one that assembly touches. A bank too full moves its smallest sufficient part to another bank. What
   it can't do: split a part that's one big function (the world's part 6, s_blows, 3,630 bytes); judge swap
   depth where the static estimate is already past SWDEPTH (the world's is 29 against 12, because its main loop
   serves requests as it waits); or make room in banks that are full (the world is bound by its banks, not its
   parts: one bank has more than 1,100 bytes free). Measured: with dq0d's and the world's parts squeezed
   (DQ0_PACK_SQUEEZE), ten functions moved and the whole suite passed.
3. **Copy leaf helpers by rule, not by hand.** Mark small pure helpers (distance, abs, chunk lookup, a table) as
   *part-local*, and have the build compile a private copy into each part that calls them. The copying the codebase
   already does by hand becomes one source and N mechanical copies, with no `made9`/`span68` names.
4. **A shared-helpers ROM library.** The LZ4 unpacker already lives in a paged-ROM shared library, so the mechanism
   exists. Put the engine's common leaf code there: distance and bearing, chunk-cache lookup, the square accessor
   (at.s), slant, the deterministic hash and dice (§5), name matching, `num`, word-list helpers, the generic
   record-page code (§6). Every task and every part could call it with no swap cost.
   *Measured later (2026-11-06), and smaller than first thought:* the copied helpers are tens of bytes to about 150
   each (dq0d's `made16` 151, dq0p's `fz`/`fz6` 53 each, the `ovl_load`s about 80 and run once), a few hundred bytes a
   task. What fills the overlays is the features' own code, not the copies. So the library is worth having for
   correctness (one line of sight, one path finder, one keyed dice: §4) and for having one copy to keep, not for room.
   Room comes from placement (points 1 and 2) and from data in place of per-feature code (§3).
   *Measured cost (base/kernel/sys.s, docs/09-programming/libraries.md):* an XCALL is about 120 cycles, plus whatever
   state the caller saves around it. That suits routines that do real work per call: line of sight, path finding,
   chunk lookup, name matching, keyed dice, paging. The tiny hot ones (`apart` and `abs`, ~30 cycles, called in
   loops) should stay as build-made part-local copies (point 3), not ROM calls.
   *Built 2026-11-09 (modules/dq0lib, games/dq0/dq0lib.inc; DECISIONS.md's "The game's own library in the paged ROM"):*
   a library module of the game's own, found by each task as it starts. Its first entries are the overlays' loading
   (`OVL_LOAD`: the path, the file, the banks, the unpack library called from it, the file closed) and `NUM`. The room
   came out more than measured above, because a task's `ovl_load` was what pulled the C library's `open`, `strcat`
   and the like into the main RAM: the main RAM's free bytes went from 2,128 to 2,307 in the UI, 2,536 to 2,624 in the
   world, 2,399 to 2,512 in the denizens, 2,435 to 3,122 in the senses, 2,414 to 2,879 in the maker and 2,197 to 2,329
   in the referee.
5. **Typed stub calls.** A generated stub can pass a small argument block (or use the C stack, as the copies already
   do), which removes the hidden-parameter globals.

*The UI's room, 2026-11-11 (DECISIONS.md's "The UI's room won back").* The UI's main RAM had 2,069 bytes free against
the floor of 2,048, part 2 (the commands) 102 bytes and its banks some 4K in pieces. Now: the main RAM 3,378 free,
part 2 404, and a 27th overlay bank with 7,032 bytes free (room for a new part's image of 3-4K and more). Where it
came from: the overlays' stubs 6 bytes each, not 11 (a call and its numbers, as the parts' are: ovcall, about 415
bytes in the UI, 220 in the referee, 80 in the maker: dq0harn 2,135 to 2,341, dq0m 2,879 to 2,940); the status line
(its words and its two 128-byte copies) out of the main RAM into the 27th bank, reading the hub through hub_mem
(about 650 bytes); three buffers that only part 1 uses, or that need not outlive a call, made the part's own or the C
stack's (220); and two of the rarer handlers (the climb, the powers refused) moved from part 2 to part 24.

---

## 2. The hub: one big hand-maintained header in place of a schema

**The evidence.**

- hub.h is **2,018 lines, 55% of them comments**, at HUB_VERSION 53. Every task includes it (82 cross-directory
  includes), so every change rebuilds everything and is checked by hand.
- **One untyped message for every intent and event.** `struct msg { kind, who, a, b, w1, w2, t, x[4] }`. What each
  field means for each kind is written only in a comment, often with packed bits. For example, `I_WAIT`'s w2 is
  "1 quietly | the long task's kind × 256 | the thing's kind × 2". The referee's `struct req` packs RS_TIME into
  `x[4..7]`, `x[16..21]` and `x[18..21]`.
- **The vocabulary is large and grows by one word per feature:** I_* 33, E_* 23, IO_* 22, LT_* 60, TE_* 38,
  PO_* 64, WB_* 46, DE_* 36, SA_* 56, PA_* 16. Each feature adds an intent, an event, a few result codes, and
  wording for them.
- **Constants are kept in step across languages by hand:**
  - dq0c/hub.inc mirrors 20 names from hub.h (checked by make.js, so the build stops if they differ);
  - commands.actions mirrors actions.h (checked);
  - `KIND = 37`, `AT`, `SQ_FT`, `SND_KINDS`, and theme.h's byte offsets (`HEAD=1739`, `EXIT_AT=1594` …) are
    restated in up to three make.js files **with no check at all**;
  - dq0meaning.js recovers struct offsets by compiling expressions with cc65 at test time.
- **Who may write what** is a table in hub.h's comments, enforced by a test (hubown.js) that watches the emulator's
  memory writes.

**Stage 1, done 2026-11-09: nothing written twice** (DECISIONS.md's "The hub's numbers, made by the build").  The C
headers stay the one statement; the build reads them as the compiler sees them and writes what the other languages
need (games/dq0/hubgen.js, before the games are built: some 0.3 s).  The #defines and enums by parsing, every named
struct's fields (through nested and anonymous structs) by parsing, and all their values, offsets and sizes from one
generated C file compiled by the build's cc65 and read back from its assembly.  It writes obj/gen/dq0hub.json (the
names, the enums, the layouts and the writers' map: read by dq0's theme compiler, dq0w's, dq0d's and dq0s's make.js,
outfits.js, sim/lib/dq0meaning.js and hubown.js, tests.js's offsets and commandwords.js), one assembly include per
assembly file (obj/gen/dq0hub_*.inc: dq0c.s, cmdmean.s, line.inc, items.s, letter.s, pulse.s, at.s, lbsaid.s), and
actions.h from commands.actions (the catalogue the one list; the C finds it in obj/gen).  Gone: games/dq0c/hub.inc and
the make.js checks that held it and cmdmean.s to hub.h, hub.h's HUBO_*/CQO_* and their assert, actions.h and its check,
`KIND = 37`, `AT`, `SQ_FT`, `SND_KINDS` and the theme compiler's sixty-odd offsets restated in the make.js files,
things.c's assert on items.s's numbers, and every cc65 compile at test time (dq0meaning.js's layout, hubown.js's map,
tests.js's `evaluate`).  The writers' table is the block between hub.h's `own:` and `own.` lines (a line in it that
isn't one stops the build), its regions' bytes the build's.  The images came out byte-identical.

**What stage 2 (typed payloads) needs from it.**  The generator reads structs, fields, enums and #defines and lays
them out by cc65 already, so stage 2 adds a declaration, not a mechanism: a table of each intent's and event's payload
(its fields, their widths and bit fields, in hub.h beside the kinds or in a small file of its own, read the way the
writers' table is), from which the same pass would write a C header of accessors (obj/gen, as actions.h is now) and the
JSON's decoders for dq0meaning.js's `decode` (its switch of packed `w1` bits today), and check each payload fits
`struct msg`.  The vocabulary's numbers (I_*, E_*, ...) are already the JSON's; what's missing is only which fields each
kind means.

**What I'd do: one schema, with everything else generated from it.**

A single declarative file (or a few) states:

- each shared record and its fields, with types and owning task;
- each request and reply;
- each intent and event kind, **with a typed payload** (`I_WAIT { tenths:u16, quiet:bit, task:LT, what:kind }`);
- each enum.

From it the build generates:

- the C header (structs, enums, payload unions, accessor macros, size asserts);
- the .inc for the assembly (dq0c, at.s and the rest);
- the JS constants for make.js files and the tests' recorder (no compiling expressions to find offsets);
- the writers' table for hubown.js;
- a printable reference (what HUB_VERSION's comment list tries to be).

Bit-packing becomes declared bit fields that the generator lays out. The "build stops if they differ" checks become
unnecessary because nothing is written twice.

**The vocabulary can shrink too.** Many intents and events differ only in which action happened and what came of
it. With the command critique's model (actions, handles, outcome classes), an intent becomes
`{actor, action, obj, instr, target, count, manner}` and an event becomes `{actor, action, obj, outcome, numbers}`.
The per-feature I_/E_/PO_/D_/WB_ families collapse into the action catalogue plus a shared outcome vocabulary
(done, refused (reason), partial, interrupted, found, broke …), with a small payload for the numbers. §3 goes further.

**Step 3, stage 2, as planned (2026-11-09): typed message payloads.** Every intent and event travels in one untyped
`struct msg { kind, who, a, b, w1, w2, t, x[4] }`, and what each field means for each kind lives only in hub.h's
comments, often packed (I_WAIT's w2: "1 quietly | the long task's kind × 256 | the thing's kind × 2"). Stage 2 states
each kind's fields once and generates the rest, without changing the layout (the images stay byte-identical):

- **One declaration file** (games/dq0/msgs.def, read by stage 1's generator): a line a kind, naming its fields and
  where each sits in struct msg, packed bits as bit ranges. For example
  `I_WAIT  w1 tenths; a min:WA_MIN sleep:WA_SLEEP; w2 quiet:0 what:1-7 task:8-15`.
- **Generated for C:** an accessor a field (`I_WAIT_TASK (m)`) and a builder a kind (`I_WAIT_SET (m, tenths, quiet,
  what, task)`), plain macros over the struct, so the code costs nothing more than today's hand-packed shifts.
- **Generated for JS:** a decoder a kind, for the tests' recorder and dq0meaning.js, so a test reads `e.task`, not
  `e.w2 >> 8`.
- **Order:** the intents (I_*, 33 kinds) first: each sender and receiver switched to the accessors, the suite after
  each family; then the events (E_*, 23); then the referee's requests (struct req's RS_*, its packed x[]).
- **What it doesn't do yet:** shrink the vocabulary (stage 3, the critique's actions and outcomes); that's easier once
  every field is declared, as which kinds differ only by name becomes plain in the declaration file.

**Stage 2, the intents done 2026-11-09** (DECISIONS.md's "The messages' payloads, declared").  games/dq0/msgs.def, read
by hubgen.js in the same pass as the headers: a `struct msg` line, then a line a kind, `I_WAIT  a how; w1 tenths;
w2(busy) task:8-15@LT_NONE what:1-7 quiet:0`: each slot's fields, a bit range, a one-bit field's constant
(`keep:8=STEP_KEEP`, checked against the headers), an enum for the tests' names (`@LT_NONE`), a packed slot named whole
(`w2(busy)`), another form of the same kind (`I_STEP:den`, `I_PORTAL:search`, `I_KIT:dig`, `I_ROB:give` ...) and the
fields of another included (`< I_DEN`, the denizens' intents' head: doing, face).  Every intent declared: the 31 kinds
after I_NONE, in 45 forms (and I_DEN's head).  The build writes obj/gen/dq0msg.h (hub.h includes it): an accessor a field
(`I_WAIT_TASK (m)`, an lvalue for a slot whole), a packer a packed slot (`I_WAIT_W2 (task, what, quiet)`), a builder a
form (`I_PORTAL_SET (m, who, act, which, x, y, bar, strike, hand)`), checked that each field is in its slot's bits and
none overlaps another; and the payloads into dq0hub.json, read by `load ().decode` (the tests' decoder).  Every sender
and receiver of an intent switched: the UI's packed slots by their packers (`intend2 (I_WAIT, t, I_WAIT_W2 (k, n, 1))`)
and its hand-built messages by the builder or the accessors (main.c, search.c, pace.c: 61 lines), the denizens'
(dq0d.c: 177), the world's receivers (dq0w.c, wbody.c, wport.c, wtake.c, wfetch.c, wfight.c, wdig.c, wlimbs.c, outfit.c,
wgame.c, wtrace.c, things.c: 184).  The images came out byte-identical.  The tests' recorder records the intents (the
UI's ring and the denizens'), each by name (`e.tenths`, `e.task`), in dq0Turns' `intents`; dq0longwait and dq0tend
read them so.

**Stage 2, the events done 2026-11-09** (DECISIONS.md's "The events' payloads, declared").  A form chosen by a field's
value, msgs.def's selector `FIELD==CONST[,CONST]` (`E_TASK:robbed  b by+1; w1 te:0-7==TE_ROBBED@TE_STOPPED purse:8
caught:9 asleep:10`), the decoder reading an event by the form its selector matches, else by the kind's own; such a
form's accessors `KIND_FORM_FIELD` (`E_TASK_LIGHT_TASK`), but for those the kind's own form has the same
(`E_TASK_TE`); a selector of one value its packer's constant (`E_TASK_ROBBED_W1 (purse, caught, asleep)`).  And three
marks for what dq0meaning.js's hand switch worked out: `!` a signed byte, `+K` a value stored plus K (an actor's
entry plus 1), `@A,B` or `@a/b/c` names of several kinds or words.  Every event declared: the 21 kinds after E_NONE,
in 41 forms (E_TASK's ten, E_BEAT's nine, E_HARM:struck, E_DIG:notool, E_BEARING:sky); the hand switch gone, held to
the declared decoder for a run of the suite first.  The event records keep their raw slots at the top (the tests read
`e.a`, `e.w1`, `e.t` in some forty places), their fields named clear of them (a square's `sx`, `sy`; the recorder
checks).  Switched, 239 lines: the world's senders (155: `event (E_CLIMB, 0, E_CLIMB_W1 (K_BEGUN, cl.need))`, evb by
its form's packer, the hand-built E_DEN, E_FIGHT, E_NOISE, E_LIGHT ... by the accessors), the UI's receivers (60), the
denizens' (10), the sound's (13), the light layer's (1).  Left as they were: a word passed whole to its words
(task_words, beat_words ...), and the shapes cc65 codes otherwise (the low byte's `& 0xFF` in a `||` or an argument,
a middle field packed unmasked).  The images came out byte-identical.

**Stage 2, the referee's requests done 2026-11-09** (DECISIONS.md's "The referee's requests' payloads, declared").  A
`struct req` line in msgs.def, then a line a kind, the request's fields and after its `->` its reply's, written over the
same slots and declared apart (a form of its own, `RS_TIME:reply`): `RS_HARM  a cause@HC_FALL; b ground@GR_FLOOR; c how;
w1 feet  -> result caught; b band; w2 out; x effects`.  Slots spanning bytes: `x[4-7] next` (RS_TIME's clock, its
accessor `(*(unsigned long*) ((m).x + 4))`, the very cast the code wrote), `x[16-17] spent` a word, `x[0-3] resist[]` a
span's bytes (`RS_ATTACK_RESIST (*r)[k]`), `x effects` the array whole (the reply's struct rs_effect ...); a slot alone
its field of its own name (`steps`, `grade`).  Forms by purpose, cause and target (RS_TEST's `:dig`, `:appraise`,
`:read`, its swim's, bearings' and drawing's replies; RS_ATTACK's `:missile`, `:thing`, `:rise`; RS_HARM's `:strike`,
`:struck`; RS_TREAT's `:done`).  All 27 kinds declared (17 with fields, the 10 not asked yet `-`), in 60 forms (35
requests, 25 replies); no builder for a request (filled slot by slot, as the code computes it).  Every sender and
receiver switched, the generic ones' bodies left (the world's ask_w, the UI's ask_b, the denizens' ask, the world's trap
harm): the world (dq0w.c, things.c, wfight.c, wblow.c, wdig.c, wlimbs.c, wtend.c, outfit.c, wrope2.c, wrope3.c,
workpart.h), the UI (main.c, read.c, tend.c, way.c, feed.c), the denizens (dq0d.c), the HarnMaster referee (dq0harn.c,
fight.c, missile.c, gear.c, heal.c, sheet.c, afflict.c, want.c, outfit.c; and rshouse.h, rswant.h, the rulesets'
shared), the null ruleset's (dq0null.c) and the referee's unit tests (tests/ram/t_rules: RS_DICE, RS_LOAD_PC).  The
images came out byte-identical, t_rules.hyx too.  The tests' recorder keeps each reply's request as it was asked
(`ask`, read as it's posted) and its reply (`reply`), by name (`rs`); dq0well, dq0swim and dq0hardthrow read them so.

**Left of stage 2:**

- **The requests' forms by a field's value:** the recorder reads each request by its kind's form and its reply by
  `:reply` (a dig's `mat` read as RS_TEST's `how`); the events' selector (`==`) could pick RS_TEST's form by its purpose,
  RS_HARM's by its cause, RS_ATTACK's by its target and flags, as it picks E_TASK's.  Not done with the requests: a
  selector renames a chosen form's own accessors (KIND_FORM_FIELD: RS_TEST_MAT would be RS_TEST_DIG_MAT, RS_HARM_IMPACT
  two names, the strike's and the struck part's), and RS_ATTACK's missile (a bit of its flags) and RS_TREAT's done (a
  1) have no constant of the headers' to be chosen by: a pass of its own.  Left as bytes, by design (an accessor would
  change the code): rswant.h's and dq0null.c's `x[16] | x[17] << 8` (the minutes, the referee's own copy, declared
  `x[16-17] spent`), dq0harn.c's swim using `result` as scratch, the refusals' `result = 1` and the defaults' `0xFF`.

---

## 3. Every feature is written in five places

**The evidence.** Following one feature, say digging, a portal's bar, or a rope, through the engine:

1. **The UI:** a command case (with its own kind tests, §COMMANDS 3.4), its `walk_says` first words, and its
   `*_words` function. There are **33 separate wording functions** (`dig_words`, `climb_words`, `fixt_words`,
   `harm_words`, `limb_words`, `throw_words` …) and 277 inline `sayf`/`msg("…")` sites.
2. **The world:** an intent case, its own time literals, its own `told()`. **`told()` is defined six times** (wdig,
   wfetch, wfight, wport, wshoot, workpart.h) plus `told68`. Messages are built by hand 30 times and `hub_event` is
   called directly 26 times.
3. **New event kinds and result codes** (E_DIG with D_*, E_PORTAL with PO_*, E_BEAT with WB_*).
4. **A referee test**, set up inline: `r->kind = RS_…` 24 times in 10 files.
5. **Sometimes the denizens**, and often a new LT_* long-task kind. 33 of LT's 60 values are reach steps, with pairs
   such as LT_LAYLIT/LT_LAYLIT2, picked by ternaries.

Rope is the extreme case: 212 mentions in dq0w alone (dq0w.c 51, wrope.c 46, wrope3.c 29, things.c 25, wrope2.c 25),
three swapped parts, and an overlay loaded on demand. The sconce has 64 mentions in 10 files, including hooks in the
world's main loop.

**What I'd do: three generic mechanisms, then features as data.**

1. **An action table with costs** (the world side of COMMANDS §4.4). Each action has data:
   - base time, and time per unit;
   - the skill it tests, and the circumstances that modify it;
   - noise, and fatigue;
   - whether it's interruptible, and the long-task bar.

   The 33 `busy(` sites and the scattered time `#define`s and literals (`POSE_T`, `FORCE_T`, `TIE_T`, `BLOW_T`, 30,
   600 …; `FORCE_T` is even defined twice) become columns. The referee test is set up once, generically, from the
   row. A feature's handler is left with only the part that is truly its own: what changes in the world.
2. **Narration as data.** The world emits outcome events: `{actor, action, obj, instr, outcome, n1, n2}`, with
   handles, not codes particular to a feature. The UI has **one renderer** and a phrase table keyed by
   (action, outcome), with templates that know articles, plurals, pronouns, the relative direction, and "you" versus
   a denizen's name:

   ```
   DIG.begun        "You set to digging %dir with %instr."
   DIG.through      "You break through into %place."
   PORTAL.opened    "%Actor open%s %obj%reveal."
   TAKE.no_hand     "You've no hand free for %obj: you put it in %dest."
   ```

   The 33 wording functions, the inline strings and most of SA_*'s 56 refusals become rows. This also:
   - makes the "first words" (D17) trivial, since the UI renders the action's *begun* row from the command's handles
     before the world answers;
   - gives the denizens' actions the same words with the actor swapped ("The dwarf opens the door.");
   - gives the sound task an event-to-sound mapping as a column of the same table, rather than dq0s's switch.

   lbsaid.s's no-sentence-twice rule fits naturally into the renderer.

   *Stage 1 built 2026-11-11 (DECISIONS.md's "Narration as data, stage 1"; §11's row 5 says what's left): the outcome
   event (msgs.def's E_OUTCOME), the renderer (said.c) and the table (said.txt, made by saidgen.js); digging, climbing,
   portals and traps moved onto them.*
3. **A step planner instead of hand-coded reach steps** (COMMANDS §4.9). A long task is a queue of step records
   `{step kind, handle, time, phrase}`, planned from the containment tree and the action's preconditions. The 33
   reach-step LT_* kinds and their variant pairs become a handful of edge kinds (take off, open, reach in, get out,
   put back, put on, set down). The "free again" continuation chain becomes "pop the next step". (That chain is
   `take_n ? take_up() : gath ? … : pfoe ? … : srch ? … : lwork ? … : trav ? …`, plus `fwant`/`batter_on`.)

With those three in place, a new feature like the rope is mostly data: kinds with affordances, compartments,
action rows, phrase rows. Its genuinely new code (what a tied rope does to climbing) stays small and in one place.

---

## 4. The same algorithm, many times over

Some copies have no memory-placement reason at all (§1 covers the ones that do).

| Algorithm | Copies |
|---|---|
| Chebyshev distance | `apart` in dq0d/apart.s, dq0p/sense.c, dq0p/sound.c, dq0w/wbody.c, wdig.c, wport.c, dq0/choose.c, dens.c, resolve.c, seen.c; `span`×3 in wshoot.c; inline in dq0w.c and things.c (~16) |
| Chunk-cache lookup | dq0d ×4, dq0p/light.c, dq0m/map.c, dq0m/changes.c, dq0w/chunk.c, dq0w.c, wkept.c (≥9) |
| Line of sight / line of fire | dq0p/eyes.c `clear_line`, dq0p/cast.s ray casting, dq0w/wshoot.c `passes` with its own portal rules (3, possibly disagreeing). *Made one, 2026-11-08: dq0w/line.inc's `line` (below)* |
| Step legality | chunk.c `step_ok`, dq0w.c `step_by` (re-implemented), ways.inc ×2, dq0d's `open_sq` and `taken`. *Made one, 2026-11-09: dq0w/walk.inc's `wk_cell` and its step (below); `step_by` gone; the minds' three copies one `legal` and `may`* |
| Path finding | dq0d `route` (BFS) and `step_for` (greedy); dq0w `ways`, `s_go_to`, `s_path_ways`, `s_ways_go`, `s_step_to`. *Made one, 2026-11-09: dq0w/walk.inc's `wk_search`, the world's `s_ways_go` and the denizens' `route` both its (below)* |
| Name matching | things.c `named`, `named4`, `a_word`, `word` ×2; resolve.c `has`; dq0d `called`; lower-casing loops ×4 in the UI |
| Dice and hash | the SDK's `rseed`/`random`, plus dq0m's own xorshift `dr` ×3 (fixtures.c, sconce.c, pass.c), three `wh` hashes (caves, plan, sky), seed constants 40503, 2654435761, 69069, 7919 |
| Overlay loading | `ovl_load` in dq0d, dq0p, dq0m, dq0w, dq0harn (5, near-identical). *Made one, 2026-11-09: the game's library's `OVL_LOAD` (modules/dq0lib), the UI's too* |
| atoi without ctype | `num` ×3 (dq0d, dq0p, dq0w), plus dq0life and dq0end. *Made one, 2026-11-09: the game's library's `NUM`, for the five tasks and the UI's `dnum` (dq0life and dq0end, RAM programs with some 20K free and no library found, keep theirs)* |
| Event emitting | `told` ×6 + `told68`, plus 30 hand-built events |
| Paged caches | §6 |

Two of these are correctness risks, not just size:

- **Three lines of sight.** If the senses' line of sight and the world's line of fire disagree, the player can see
  a goblin they can't shoot, or a denizen can sense through something the character can't. One `line(a, b, mode)`
  with modes for sight, fire, sound and scent, in the shared library, makes them agree.
  *Done 2026-11-08, as a shared source (DECISIONS.md's "One line of sight"):* they did disagree (a lowered portcullis
  blinded a denizen the character saw through its bars; closed shutters didn't; no heights for a denizen's eyes; three
  ways of stepping the squares). Now dq0w/line.inc's `line (sn, x, y, tx, ty, mode)` (LN_SIGHT, LN_FIRE, LN_AIR) and
  its square and portal rules (`ln_pass`, `ln_portal`) are assembled into the senses and the world; cast.s keeps its
  grid for speed but steps the same squares and heights and asks `ln_portal` of its doors. A line between the
  character and anyone is drawn from the character's end, so a denizen sees the character just when the character
  would see it. Not yet in the paged ROM: the line reads the chunks through each task's own `at` (the cache and the
  maker are the task's), which a ROM module can't call back into; that waits for the chunk lookup itself in ROM.
  *Looked at again 2026-11-09, with the game's library built (modules/dq0lib):* a ROM routine *can* call back into
  its caller: XCALL leaves the task's RAM and zero page as they are, so a `jsr` to an address the task gave runs the
  task's own code (its `at`, which may map banks and ask the maker), and XCALLs nest (dq0lib's OVL_LOAD calls the
  unpack library). The real obstacles are three. (a) The ROM can't link to the task's symbols (`at`, `portal_at`,
  `cached`, `cx_`, `cy_`, `at_x0`, `at_y0`, `ln_way`), so they must be handed over at run time. (b) The line keeps
  some 45 bytes over its calls of `at` (its BSS today), and a library has no RAM; the call registers won't do, since
  `at`'s long way (C, the maker's request) changes them. (c) It reads its arguments off cc65's C stack, whose
  runtime (`sp`, `pusha`) is at another address in each task. Nor can the chunk lookup go to the ROM whole: `at`'s
  quick way is some 80 cycles and a look asks it thousands of times, so an XCALL each would more than double it. Its
  long way, the search of the hub's cache table for a chunk's slot (`find`, `cached`, `made`/`made9`/`made10`/`made16`,
  map.c's and changes.c's: nine copies or more), is once a chunk and suits the ROM.
  **The plan.** (1) A zero-page block for the line, fixed, in each task that draws one: `$50-$7F` (the program's zero
  page is `$22-$7F` and no call touches it; cc65's and the tasks' own end at `$41`), its layout in one include that
  the ROM module and the tasks both assemble (games/dq0/lnzp.inc): the line's state, and vectors for the callbacks.
  (2) The task's side, beside dq0lib.inc: `_line`, `_ln_pass` and `_ln_portal` taking cc65's arguments off the C
  stack into the block and calling the library; and three short callbacks of its own, `ln_at` (sn, x, y from the
  block: `at`'s pointer, `cx_`, `cy_` into the block), `ln_door` (`portal_at` of that square) and, in the senses,
  `ln_cached`. (3) The ROM side: line.inc as it is, its BSS the block's names, its `jsr _at` a `jmp (vector)`. (4) The
  hub cache's search as the library's `CS_FIND` (the world bank, sn, cx, cy: the slot, or none), each task keeping its
  own bookkeeping (the world's stamp and `inuse`) round it. What it would win: the senses' main RAM some 770 bytes (892
  of line, less the stubs), the world's some 100 and part 65's 547; for some 150 cycles more a line (a line is
  4,400-5,000, a denizen's senses some 100,000). **Not done yet**, because it doesn't pay enough now: neither task is
  short of room (3,122 and 2,624 bytes free, the floor 2,048), the correctness is had already (one source), and the
  fixed zero-page contract is new here (the numbers library saves and puts back `$70-$7F`; a line calling back into
  C can't). The router (step 2's next) needs the same contract, callbacks into `at` from the ROM, so it's best
  designed once for both, with the router.
- **Path finding in five-plus places.** A denizen and the PC route differently through the same doors, and every
  fix to door, slope, water or rubble rules has to be made in each copy. One router in the shared library, with
  each walker's rules as a cost table, removes that.
  *Done 2026-11-09, as a shared source (DECISIONS.md's "One way-finder"):* the copies did disagree, though less than
  feared (slopes, steps, rubble and the diagonal ways' corners were the same everywhere a player can reach). The
  denizens' way (`route`) took no notice of water deeper than a walker wades or of bad air, which its steps refuse,
  so a denizen bound round a pool or a pocket of bad air was led into it and stood at its edge for good (test
  dq0route); its fields were shared by walkers with other rules (a beast following a field found for hands through a
  shut door); a diagonal past a door's corner was let through or not by the order its squares were reached. Now one
  source, dq0w/walk.inc (C: `wk_cell`, the rule for a square by the walker's rules; `wk_search`, the search over a
  grid of cells, the step's rise, steps and diagonal corners), is compiled into the world's part 21 (`s_ways_go`: the
  character's ways across a room or about a passage) and the denizens' part 18 (`route`); each walker's differences
  are its rules (WK_HANDS: a shut door a way; WK_NODOOR: none) and what its reader leaves out (the character: a
  denizen's square, a trap it knows of; a denizen: deep water, bad air, a chunk not made), and the world's times its
  pace's (WK_REACHED). The rule a step at a time stays chunk.c's `step_ok` and slant.s's `slant` (the same rule);
  the minds' three copies of the step test are one (`legal`, `may`). Left: the character's straight walk across a
  room (`across`) takes diagonals over open floor, and between two corners, by its own rule (a walk, not a way
  found); the placing's `open_sq` is a stricter rule on purpose (where a denizen is put, not where it may step). Not
  yet in the paged ROM, for the line's reason: it reads the chunks through each task's own `at`.

---

## 5. Generation: a fixed pipeline, and many dice streams

**The evidence.**

- `make_chunk` is a fixed call sequence: edges → style → caves or built → fixtures → sky → wet → air → changes →
  stock → sconces → sky → seen.
- Each fixture type has its own code path *and its own randomness*:
  - doors with their own hash;
  - windows, portcullises, fortification, door bodies;
  - traps from a local hard-coded array with pit, plate and wire rules in code;
  - sconces with a copied `dr`;
  - wells inside stock.c ("a kitchen's well").
- Style proportions are `styles[4]` in uses.c, about 45 fields each, whose own comment calls it "a stand-in for the
  theme's data".

**What I'd do.**

1. **One deterministic randomness API**: `rng(seed, stratum, chunk, purpose)`, a keyed hash with a purpose id per
   placer. Determinism (the turns-pace rule, saves, regenerating a chunk) then depends on keys rather than on call
   order inside each placer's private stream, and adding a placer can't shift another's numbers.
2. **Placers as a table.** Each fixture kind is a row: where it may go (wall, floor, room centre, corridor end, by
   water), how often by style and room use, which kind or portal it makes, and its parameters. One generic placer
   walks the table. Traps, sconces, wells and windows become rows. A new fixture (a fountain, a shelf, a cabinet with
   drawers from COMMANDS §4.9) is a row plus, at most, a small custom hook.
3. **Move the style tables into the theme**, as their comment intends. Themes then vary the dungeon without code.

*Steps 1 and 2 done 2026-11-11 (DECISIONS.md's "The placers' table and the keyed dice"): games/dq0m/place.c's
`dkey (purpose, where)` and `dr (n)`, each purpose's key the one its placer had (the doors', the fixtures', the
sconces'), and `rstream` the growers' and the stock's stream seeds; the windows, portcullises, room traps, door traps,
the rooms' marks, the kitchens' wells and the sconces rows of one table walked by one placer, each with a small hook;
every seed's dungeon byte for byte as it was. Step 3 not begun.*

---

## 6. Five persistence caches, each written separately

The item store (ipage.c, `itSXXYY.dat`), the change records (store.c/wfile.c, `rgSXXYY.dat`), the strata (sfile.c,
`stSS.dat`), the traces (wtrace.c, `trSXXYY.dat`) and the chunk cache (chunk.c) each page fixed-size records by
region with their own least-recently-wanted eviction. They share only vfile.inc.

**What I'd do: one region-paged record store.** It's a small library:

- `rp_open(kind, recsize, slots)`, `rp_get(kind, region, n)`, `rp_dirty`, `rp_flush`, `rp_save`;
- one eviction policy and one file-naming scheme;
- one save and restore path.

Each store then declares its record kind and keeps only its own logic. Save and restore (and `-same` determinism)
get one implementation to test instead of five. It belongs in the shared library (§1.4).

---

## 7. The world's control flow

- **The main loop polls everything** in a fixed order (dq0w.c:6831-6996): the UI's first letter, stocks, chart_late,
  traces, denizen intents, intents, wreq, the clock, stepping, the seen checks, burns and sconces, turns, arrivals,
  the free-again chain, ireq.
- **"What the PC is doing" is spread over about eleven flags** besides `pc_task.kind`: `move.on`, `pc_cut`,
  `burn_ask`, `fwant`, `lwork` (with sentinel values 0xFD/0xFE/0xFF), `pfoe`, `trav`, `gath`, `chart_late`, `srch`,
  `take_n`.
- **About 384 mutable file-scope variables and 152 function statics** in dq0w; 1,515 `HUB->` references to 116 fields.

**What I'd do.**

- **One activity stack per actor**, the step planner from §3.3: what it's doing now, and what's queued after. This
  replaces the flag set and the continuation chain, and the denizens get the same structure (their minds already
  post intents that the world carries out).
- **Periodic work as a scheduler table**: `{job, period or due time, budget, give-way rule}` for the lights' minute,
  the traces' journal, stocking, sconces and charting. That's in place of hand-placed hooks in the main loop. The
  give-way-to-the-UI rule (D17) becomes a property of the scheduler, not something each hook checks.
- **Group globals into per-subsystem state structs** (`struct pc_activity`, `struct burning` …). That documents
  ownership, makes save/restore a matter of listing structs, and makes the hidden-parameter globals visible as
  fields.

---

## 8. The minds, the senses and the referee

**The minds (dq0d)** are in good shape. `decide()` (dq0d.c:2024, 206 lines) is a priority ladder: hidden → fleeing
→ strike → shoot → keep off → ambush → allied → senses → scent → drawn on → hurt → alert → daily → move → wander.
Then come filters (take, door and dig way), then a wait. Subsumption suits this. Two improvements:

- **Return a decision struct, not a byte with sentinels** (0xFA shot, 0xFB scent, 0xFC blow, 0xFD dash, 0xFE still,
  0xFF none: 37 uses). `{intent, heading or target handle, wait}` removes the second if-chain that maps the byte to
  an intent.
- **Make the ladder a table**: `{condition fn, behaviour fn, min mind level, kind flags required}`. Kinds and mind
  levels then choose which rungs apply by data. That's in place of the 14 hard-coded mind-level tests, and is the
  same direction kinds.txt already took the minds.

**The senses (dq0p)** rebuild `struct sight` from scratch at each full look, scanning the room's walls, doors, mouths
and openings twice (once for what's seen, once for what's remembered). The view grid next to it is already
incremental (`mark_rows`).

- **Build the scope table** (COMMANDS §4.1) as the senses' output: one list of entries, each with its class, the
  owner's handle, where it is and its state. The descriptive arrays the look's words use can be derived from it, so
  the separate door, open, item, den and trap arrays (and their parallel copies in resolve.c) go away.
- **Update it incrementally** from the view grid's changed rows and the world's change events. The memset-and-rebuild
  is a large part of a look's cost.

**The referee interface** (rs.h, RULESETS.md):

- **27 RS_* requests, 10 of them never used** (DROP_ACTOR, TEST_MANY, REST, INITIATIVE, EXPERIENCE, POWER, EXPORT,
  IMPORT, RESTORE, QUIT); `struct rs_neutral` is never used.
- **HarnMaster leaks into the "generic" interface:** OT_* is HarnMaster's cultures, BP_* its strike locations,
  D_COUNTER, C_STUNNED as "HarnMaster's shock", grades one for one with its levels. dq0acct and dq0schol include
  dq0harn's own headers, and dq0life runs dq0acct only if the referee is dq0harn.
- **The two referees repeat the same boilerplate**: the main loop (near-verbatim), `tools[]`/`equip_all`,
  `put`/`putn`, `bseed`, the beasts.dat save and load. The "house rules" are headers of *static code* switched on by
  `#define`s (rshouse.h, rsair.h, rswant.h).
- **`P_` names three unrelated enums** (purposes in rs.h, stratum purposes in theme.h, town places).

**A choice to make:**

- **(a) Swappable rules are a real goal.** Then put a referee skeleton in a library (main loop, slots, dice handshake,
  tools, saves, the house rules as linked functions) so a ruleset is only its answer table. Trim rs.h to the
  requests used. Move HarnMaster's lists (cultures, strike locations, skill names) behind the referee, so the engine
  asks for names and counts rather than including them. Have the meta programs talk to "the ruleset" through a
  defined interface, not dq0harn's headers.
- **(b) HarnMaster is the house system** and dq0null is a test double. Then say so, delete the unused requests, and
  stop paying for the indirection.

Either is fine. The current halfway state costs both ways: the indirection, *and* HarnMaster everywhere.

---

## 9. Between dungeons, the build, the tests

**The meta programs** pass state through long argv lists and a dozen files in the save folder:

- dq0town takes `-t -i -p -k -h -r -n`; dq0schol gets town.dat's money over argv.
- They copy their helpers: `say`/`sayn`/`amount` ×3 (dq0acct, dq0town, dq0schol; town's and scholar's differ in 6
  of 68 lines), a program launcher ×4 (dq0life `run`, dq0acct `arg`, dq0town `run`/`arg`, dq0/hub.c `hub_run`),
  `words_of` ×2, `value_of` ×2, `num` ×2, `in` (path join) ×3.

The fix is a small libdq0meta (output and money formatting, launch, save-folder records) and **one save-folder state
record**, described in the §2 schema, in place of per-program argv.

**The theme compiler is written twice**: dq0/make.js `compile` in JS and themec/comp.c in C, and tests hold them to
the same bytes. Keep one. Either the C one is the only compiler (the build runs it in the emulator, or as a host
build of the same source), or the JS one is, and the game only reads compiled themes.

**The build's JS:**

- The "key | value, indented continuation" reader is written three times (dq0/make.js `lines`, dq0d/make.js
  `blocks`, themegen/make.js `read`), and a `lines` one-liner twice more.
- dq0harn's, dq0m's and dq0p's make.js are the same three lines.

A small `tools/dq0build.js` library (the readers, the schema generator from §2, the placement generator from §1, the
ovlpack call) makes each task's make.js a short list of declarations.

**The tests:**

- tests.js is **one 15,886-line array**; the dq0 section is about 11,950 lines, roughly 47 per test.
- The prelude repeats: `'bind -a /sd/0/games /bin'` 247 times, the same `hmchar -q -s 5 -u feudal -c hunter …` line
  216 times, `init: 't_rc'` 344 times.
- The dq0 tests take **150 of the suite's 175 CPU minutes**, mostly booting, making a character and growing a
  dungeon before reaching the moment under test.

What would change that:

- **Scenes.** A scene file (or a scene constructor in a test) declares the room, the things, the denizens and the
  character's kit. `dq0Poke` (85 uses) and `-T`/`-k` already go part of the way. A saved game folder made once per
  scene and copied, or a `-S scene` flag that skips generation, would turn many 47-line tests that boot the world
  into 5-line checks against a ready state. It would also make the action × class sweep (COMMANDS §4.6) cheap.
- **Split tests.js by area** (the areas table exists), with a dq0 test helper that supplies the prelude.
- **Generate the recorder's offsets from the schema** (§2), not by compiling expressions at test time.

*As built, 2026-11-11 (DECISIONS.md's "The tests: split by area, scenes, the runner"):*

- *Measured first: the dq0 tests were 90% of the suite's CPU time, and in a typical one (dq0parse) the game's start,
  its first dungeon grown, was 61% of its cycles (254M of 417M); the boot and hmchar another 14%.  Every test also
  spent about a second making its ROM image.  The runner already ran a process a test, 8 at a time, the longest
  first, and kept the cores busy to the end.*
- *tests.js split: tests/suites/*.js by area, tests/suites/dq0/*.js the game's by subject (NEAR's areas), the shared
  helpers in tests/lib (common.js, dq0.js), tests.js the index.  The selector reads the helper modules, and every
  test's areas and helpers are what they were.*
- *Scenes (sim/lib/scenes.js): not a scene file or a saved game, but the emulator's whole machine saved at the prompt
  waits just before and after a test's first dq0 line, and taken up by the next test that starts the same way.  This
  is exact, because nothing after a prompt wait reaches the machine while it waits.  It is keyed by the input to
  there, the images' and the code's sha1; a test with watchers of its own saves its own.  Held to runs without
  scenes, every dq0 test ends the same to the cycle and the byte, but dq0layers and dq0looks (scene: false).*
- *The runner: the ROM images kept (obj/imgcache), the longest first by the last run's times, CPU time recorded.*
- *The whole suite's CPU time: some 4,450 s before; 4,209 s split, with the images kept; 4,012 s with scenes cold (new
  images: the scenes made as the run goes); 2,899 s warm (the dq0 tests' a third less).  On 8 quiet cores that is about
  9 minutes before and 6 warm; the runs measured shared the machine with other agents' suites.*
- *Left: scene constructors (a room declared, not reached by commands) and the recorder's offsets from the schema;
  the tests' preludes are still written out in each test.*

---

## 10. Comments and design documents

This is a judgement about maintainability, not correctness.

- **The code's comments are mostly history and placement.** 1,048 dates in the source; "the user's" 46 times in
  things.c alone; "part N" hundreds of times; "slice 4.x's" throughout. 55% of hub.h and 58% of rs.h are comment
  lines.
- The prose style ("its X the Y's: Z, the W's") is dense to read, and it carries information that git and
  DECISIONS.md already hold. Placement notes go stale every time code moves (§1), and history notes grow without
  bound.
- **It costs every reader.** That includes AI sessions, whose context fills with history before it reaches the logic.
  It also hides the invariants that matter among notes that don't.
- **The design documents are chronicles.** 9,592 lines; HOUSE.md alone is 4,004, with decisions in date order and
  later ones amending earlier ones. That makes "what is the rule now?" hard to answer. (The docs survey for the
  command critique found the same rule stated three ways in three places.)

**What I'd suggest.**

- **Code comments state what is true now:** invariants, units, ownership, why something non-obvious is so. History
  goes to git and DECISIONS.md. Placement comments disappear when placement is generated (§1).
- **Each topic gets a reference document** (Items, Portals, Commands, Denizens, Time, Rules interface) describing
  current behaviour, **regenerated or edited in place** when a decision changes it. DECISIONS.md stays as the
  chronicle that points to them.

---

## 11. Order of work

Ranked by how much each unlocks, and arranged so the game keeps working throughout.

| # | Change | What it unlocks | Size |
|---|---|---|---|
| 1 | **Generated placement** (§1.1): parts.txt, the stubs and the config's SWn lines made by the build. *Done 2026-11-06 (games/dq0/parts.js), every task.* Then the automatic packer (§1.2: *done 2026-11-07*, the parts' banks chosen by the build, parts.txt's bank a wish; the calls routed by part; a part past the swap area has its pieces moved out by the build, games/dq0/pack.js) and part-local leaf copies (§1.3) | ends "moved for room" churn; a function moved is its pragma alone; dispatch can be organised by meaning | medium, then large |
| 2 | **Shared helpers in paged ROM** (§1.4, §4): line of sight, path finding, chunk lookup, name matching, the keyed hash and dice. *Line of sight done 2026-11-08 as a stage: one line (dq0w/line.inc: sight, fire, air), a shared source assembled into the senses and the world, not yet ROM (it calls the task's own chunk lookup); the look's grid ray the same squares, heights and portal rule (DECISIONS.md's "One line of sight"). The game's library in the paged ROM done 2026-11-09 (modules/dq0lib, each task's games/dq0/dq0lib.inc; DECISIONS.md's "The game's own library in the paged ROM"): the overlays' loading (`OVL_LOAD`, six copies) and `NUM` (atoi's: five copies and dq0m's atoi), the main RAM 88 to 687 bytes freer in every task (the UI 2,128 to 2,307); the line and the chunk lookup's move to it planned in §4 (a fixed zero-page block and callbacks into the task's `at`), to be done with the router. The router done 2026-11-09 the same way: one way-finder and one rule for a square (dq0w/walk.inc), compiled into the world (the character's ways) and the denizens (their ways found), each walker's rules its own (DECISIONS.md's "One way-finder").* Next: the line, the way-finder and the chunk lookup into the ROM library together (§4's plan) | one line of sight and one router (they disagree today); one copy to keep. Small room gains (measured: a few hundred bytes a task) | medium |
| 3 | **The hub schema and generated headers/inc/JS/ownership** (§2). *Stage 1 done 2026-11-09: nothing written twice (games/dq0/hubgen.js, DECISIONS.md's "The hub's numbers, made by the build"): the headers' names, enums, structs' layouts and the writers' table made by the build with cc65 into obj/gen (dq0hub.json for the make.js files and the tests, an include per assembly file, actions.h from the catalogue); the hand-typed copies and the checks that held them gone, nothing compiled at test time; the images byte-identical.* *Stage 2, the intents, done 2026-11-09 (DECISIONS.md's "The messages' payloads, declared"): games/dq0/msgs.def, each intent's fields where they sit in struct msg, packed bits as bit ranges; the build's obj/gen/dq0msg.h (an accessor a field, a packer a packed slot, a builder a form) and the tests' decoders (the recorder's intents by name); every sender and receiver of an intent switched (the UI, the denizens, the world); the images byte-identical.* *The events done 2026-11-09 (DECISIONS.md's "The events' payloads, declared"): forms chosen by a field's value (msgs.def's `==` selector, `KIND_FORM_FIELD` accessors), all 21 kinds in 41 forms, dq0meaning.js's hand switch gone, 239 senders' and receivers' lines switched; the images byte-identical.* *Stage 2, the referee's requests, done 2026-11-09 (DECISIONS.md's "The referee's requests' payloads, declared"): a `struct req` line, a request's fields and its reply's apart (`->`, a form KIND:reply), slots spanning bytes (`x[4-7]`, `x[0-3] resist[]`, `x` whole); all 27 kinds in 60 forms; every sender and receiver switched (the world, the UI, the denizens, both referees, t_rules); the recorder's `ask` and `reply` by name; the images byte-identical, t_rules.hyx too.* Next: §2's "Left of stage 2" (the requests' forms by their selector), then stage 3 | typed payloads; no hand-synced constants; cheaper tests | medium |
| 4 | **Handles, the scope table, the action table** (COMMANDS §4.1–4.4 and §3.1 here). *Mostly done 2026-11-07* (COMMANDS-CRITIQUE.md §5.1: every choice of a thing made once and sent as a handle, one "which?", one name match; §5.2 G–K: both sweeps clean, one path for things and one for features, one walk over, refusals as data); the action table, §5.2's step L, under way | consistent commands; one intent shape for PC and denizens | large (the command plan's steps) |
| 5 | **Narration as data** (§3.2). *Stage 1 done 2026-11-11 (DECISIONS.md's "Narration as data, stage 1"): the outcome event declared (msgs.def's E_OUTCOME, a head its kinds include: who the actor, the outcome, n1, n2, the thing done to and the thing done with as handles; E_DIG, E_CLIMB, E_PORTAL, E_TRAP its kinds, the action the kind); one renderer (games/dq0/said.c, overlay 27) and a phrase table keyed by (kind, outcome), made by the build (games/dq0/said.txt, games/dq0/saidgen.js: obj/gen/dq0said.h), its templates' fields by msgs.def's names, articles, "you" or a denizen, a heading from the facing, the theme's names, lbsaid.s's rule a row's flag; dig_words, climb_words, portal_words, fixt_words and trap_words gone, the words byte for byte the same.* Left: the other 28 wording functions (task_words, beat_words, harm_words, limb_words, throw_words, fight_words, shot_words, water_words ...) and the inline strings; the world's `told` copies (none could go yet: each sends kinds not moved, E_NOISE, E_HARM, E_DEN, E_FIGHT, E_TASK); the SA_ refusals as rows; the first words from a *begun* row; the denizens' E_DEN worded by the same rows, the actor swapped; the sound's mapping a column of the table (dq0s's world_sound) | removes the 33 wording functions, `told` ×6, most SA_ refusals; first words for free; denizen actions worded alike | large, migratable one action at a time |
| 6 | **The step planner and activity stacks** (§3.3, §7) | removes 33 reach LT_ kinds and the continuation chain; containers (COMMANDS §4.9) | medium–large |
| 7 | **One region-paged record store** (§6) | one save/restore path | medium |
| 8 | **The generation placers table and keyed randomness** (§5). *Done 2026-11-11, but for the styles (DECISIONS.md's "The placers' table and the keyed dice"): one keyed dice API (games/dq0m/place.c: `dkey (purpose, where)`, `dr (n)`, purposes DP_*, each one's key the one its placer always had, so every seed's dungeon is byte for byte as it was; doors', fixtures.c's and sconce.c's private copies gone; the growers' and the stock's stream seeds made there too, `rstream`); the placers a table (`placers[]`: stage, sites, purpose, the style's odds, what a site mustn't be, a room's use, qualifiers, where, what it makes, a hook) walked by one placer (`place (stage)`): windows and slits, portcullises, room traps, door traps, the rooms' marks, the kitchens' wells, the sconces.* Left: the styles' proportions into the theme (§5's step 3); the growers' layout streams (call order, by design) and the private hashes of caves.c, sky.c, plan.c, uses.c and make.c's mixed third; the doors' placement (the grower's) and the materials, fortification and the portals' make as steps, not rows; the old keys omit the stratum's seed (two strata's chunks at the same place share their fixtures' numbers) | fixtures as data; furniture with compartments | medium |
| 9 | **Decide on the referee** (§8 a or b), then the skeleton library or the trim | | small–medium |
| 10 | **Test scenes, tests.js split** (§9). *Done 2026-11-11 (DECISIONS.md's "The tests: split by area, scenes, the runner"): tests.js split into tests/suites by area and tests/suites/dq0 by subject, the helpers in tests/lib; scenes, the emulator's machine saved after the boot and hmchar and after the game's start and taken up by every test that starts the same way, exact to the cycle (sim/lib/scenes.js); the ROM images kept, the runner's order by the last times.  The suite's CPU time some 4,450 s before, 4,012 s cold, 2,899 s warm.* Left: scene constructors, the recorder's offsets from the schema | much faster tests; the action × class sweep | medium |
| 11 | **Shared meta library and one save record; one theme compiler** (§9) | | small–medium |
| 12 | **Comments and docs as current reference** (§10) | done gradually, file by file as each is touched | ongoing |

Steps 1–3 are infrastructure that make everything after them cheaper and are worth doing first, even though no
player sees them. Step 4 is the command plan, already sequenced around the designator work. Steps 5–6 are where the
per-feature cost drops most: after them, a new item, fixture or verb is mostly rows of data.

---

## 12. In one paragraph

The engine's architecture (tasks with single-writer queues, perception first, one intent shape for every actor,
deterministic turns) is sound. Its implementation is shaped by two things that aren't design: **hand-done memory
placement**, which turns every full part into a copied helper and every dispatch into a chain split by size, and
**the absence of a schema**, which makes every message an untyped record whose meaning lives in comments and every
shared constant something to keep in step by hand. On top of those, each feature is built end to end in five places
(command, intent, event codes, words, timings), where three generic mechanisms (an action table, narration as data,
a step planner) would make it mostly data. Fix placement and the schema first; then the command plan's handles and
the action table; then narration and the planner. Each step deletes more code than it adds.
