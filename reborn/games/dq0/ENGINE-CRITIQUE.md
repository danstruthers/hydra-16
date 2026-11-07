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
5. **Typed stub calls.** A generated stub can pass a small argument block (or use the C stack, as the copies already
   do), which removes the hidden-parameter globals.

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
| Line of sight / line of fire | dq0p/eyes.c `clear_line`, dq0p/cast.s ray casting, dq0w/wshoot.c `passes` with its own portal rules (3, possibly disagreeing) |
| Step legality | chunk.c `step_ok`, dq0w.c `step_by` (re-implemented), ways.inc ×2, dq0d's `open_sq` and `taken` |
| Path finding | dq0d `route` (BFS) and `step_for` (greedy); dq0w `ways`, `s_go_to`, `s_path_ways`, `s_ways_go`, `s_step_to` |
| Name matching | things.c `named`, `named4`, `a_word`, `word` ×2; resolve.c `has`; dq0d `called`; lower-casing loops ×4 in the UI |
| Dice and hash | the SDK's `rseed`/`random`, plus dq0m's own xorshift `dr` ×3 (fixtures.c, sconce.c, pass.c), three `wh` hashes (caves, plan, sky), seed constants 40503, 2654435761, 69069, 7919 |
| Overlay loading | `ovl_load` in dq0d, dq0p, dq0m, dq0w, dq0harn (5, near-identical) |
| atoi without ctype | `num` ×3 (dq0d, dq0p, dq0w), plus dq0life and dq0end |
| Event emitting | `told` ×6 + `told68`, plus 30 hand-built events |
| Paged caches | §6 |

Two of these are correctness risks, not just size:

- **Three lines of sight.** If the senses' line of sight and the world's line of fire disagree, the player can see
  a goblin they can't shoot, or a denizen can sense through something the character can't. One `line(a, b, mode)`
  with modes for sight, fire, sound and scent, in the shared library, makes them agree.
- **Path finding in five-plus places.** A denizen and the PC route differently through the same doors, and every
  fix to door, slope, water or rubble rules has to be made in each copy. One router in the shared library, with
  each walker's rules as a cost table, removes that.

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
| 1 | **Generated placement** (§1.1): parts.txt, the stubs and the config's SWn lines made by the build. *Done 2026-11-06 (games/dq0/parts.js), every task.* Then the automatic packer (§1.2; its first half done 2026-11-07: the parts' banks chosen by the build, parts.txt's bank a wish) and part-local leaf copies (§1.3) | ends "moved for room" churn; a function moved is its pragma alone; dispatch can be organised by meaning | medium, then large |
| 2 | **Shared helpers in paged ROM** (§1.4, §4): line of sight, path finding, chunk lookup, name matching, the keyed hash and dice | one line of sight and one router (they disagree today); one copy to keep. Small room gains (measured: a few hundred bytes a task) | medium |
| 3 | **The hub schema and generated headers/inc/JS/ownership** (§2) | typed payloads; no hand-synced constants; cheaper tests | medium |
| 4 | **Handles, the scope table, the action table** (COMMANDS §4.1–4.4 and §3.1 here) | consistent commands; one intent shape for PC and denizens | large (the command plan's steps) |
| 5 | **Narration as data** (§3.2) | removes the 33 wording functions, `told` ×6, most SA_ refusals; first words for free; denizen actions worded alike | large, migratable one action at a time |
| 6 | **The step planner and activity stacks** (§3.3, §7) | removes 33 reach LT_ kinds and the continuation chain; containers (COMMANDS §4.9) | medium–large |
| 7 | **One region-paged record store** (§6) | one save/restore path | medium |
| 8 | **The generation placers table and keyed randomness** (§5) | fixtures as data; furniture with compartments | medium |
| 9 | **Decide on the referee** (§8 a or b), then the skeleton library or the trim | | small–medium |
| 10 | **Test scenes, tests.js split** (§9) | much faster tests; the action × class sweep | medium |
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
