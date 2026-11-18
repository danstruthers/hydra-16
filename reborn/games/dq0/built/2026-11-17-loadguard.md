# The load guard: a read of the hub's header with another bank in the window, and the six bugs it found

As built, for review.  Branch loadguard, 2026-11-17, from reborn-commands' f7468c55 (actor 0 merged).  The emulator's
guards (built/2026-11-17-guards.md) stopped a store that names the hub's header landing in another bank, but not a
load of the same kind; actor 0's branch found two such loads by chance (wall_look's `HUB->seed`, mended in 7a65b8eb,
and s_fall's square) and a heuristic scan named 25 lines to look at.  Now the guard sees loads too, in every test; the
full suite with it found six places, each a latent bug, each mended in its own commit.  Eight commits:

- 382196e0 The load guard: a read of the hub's header with another bank there
- 540c42a5 The fall's floor its room's: the square read in the hub's header
- b200a0df A step's wet and air read in the hub's header, not its chunk's bank
- fd68afe4 A denizen's timed door work timed in the hub, not a chunk's bank
- 44c341cf A firedamp burst's afterdamp kept on the character's stratum
- 8f11362b A wall's words without here (): wall_look's chunk in the window
- 5f7c3f93 "stop soaking" stops a dip: the task's kind read from the hub
- this record, as built

## What was built

**The load guard** (sim/lib/guards.js; the cores: base/sim/core65/src/lib.rs, rebuilt into base/sim/lib/core65.js,
and base/sim/lib/machine.js's JS CPU; base/sim/lib/core.js the table's room):

- **The facts, the build's outputs only.**  Each listing's absolute loads (LDA, LDX, LDY, ORA, AND, EOR, ADC, SBC,
  CMP, CPX, CPY, BIT, a JMP's vector; plain or indexed) and read-modify-writes (INC, DEC, the shifts, TSB, TRB) at an
  address in struct hub's span, from a C statement that names `HUB->` or a macro that means it (the headers' and the
  file's own, followed through), or an assembly line naming a `hub.*` offset (obj/gen/dq0hub_*.inc): the store
  guard's own rule.  2,843 loads over the game's tasks before the mending (the world 1,983, the UI 349, the senses
  142, the minds 111, the referee 84, the null referee 69, the maker 50, the sound 42, the parser 13).  Cached in
  NAME.guards.json with the stores (its form's version 4).
- **Precise: the fields a line names.**  With each load go the spans of the header's fields its line names
  (`HUB->pc_x`: pc_x's two bytes; `ROUND_T`: `HUB->round`'s; an assembly line's `HUBO_CQ`: hub.cq's), and a read is
  taken for the hub's only if the byte read is in one of them.  So a line that names the hub and also reads another
  bank's bytes on purpose (a chunk's through a fixed address, an item's `ITEM_AT (i)->`, the senses' `SIGHT->`)
  isn't taken for a read of the hub.  Today every load's address is in a field its line names (checked over all
  2,843); the rule is there for the line that mixes them.
- **The check.**  The core reads the window's byte, then (flag 8) looks the instruction's PC up in the loads' table
  (GLOAD: a task's PCs below $8000, a bit each, beside GSTORE); marked, and the window's bank not the hub's header's,
  it goes to the guard (guard_hit's kind 3), which judges it as above.  A read-modify-write is both: its read the
  load guard's, its write the store guard's, two hits.  Before any hub is made (its magic not seen), a load can't be
  judged and is passed by.
- **The swapped parts.**  A task's parts share its swap area's PCs: the world's 96 parts with loads would mark 1,305
  of its area's 3,648 PCs at once, and a part's every window read at another part's marked PC would call the guard.
  The stores' and loads' tables now hold only the part in the area: swap.inc's `swapped` is watched (a watch of a new
  kind, 3: any write; the four watches a task has are full now), and as a part is copied in its PCs go in, the last
  part's out.  The store guard's hits are the same; it's called less.
- **Code in the window.**  An overlay's line that names the hub's header reads (or writes) its own bank as it runs,
  never the header: always wrong.  Such an instruction is found in the facts (its overlay by its segment) and said as
  it's fetched: its page isn't marked as code, so each fetch from it comes to the guard.
- **The words.**  A hit names the function, the C line, the field read, the bank in the window and the calls that
  reached it (the 6502 stack's return addresses):

      guard: task 9 (dq0w): s_fall+50 ($6DF2, part 2), dq0w.c:1166, read $00 from $939F (the hub's header's offset
      5023, bodies, pc, pc_x) with shared bank 165 (the hub's bank 5) in the window, not the hub's header; called from
      rp_x+438 ($0C9A), from link_here+589 ($7554, part 2) ...

- **On in every test**, as the others: GUARDS=0 none, GUARDS=note each noted, GUARDS_FLAGS's 8 the load guard's.  A
  tree moved since its build (the worktrees moved to N:) has its listings' C files found again under the tree.
- **dq0guards** makes it fire: one of the world's loads that name the header (an LDA, LDX or LDY) run with a bank of
  the world's own in the window, its register and flags made the hub's byte's after; and the first INC or DEC of the
  main code's that names it (turn's `++HUB->den_go`) run the same way, heard as a load and as a store.  sim/test.js
  puts the guard on the machine (`m.guard`) for a test's check to read its hits.
- Documented in tests/TESTING.md's guards and games/dq0/AGENTS.md's bank window.

## What it found

The full suite with GUARDS=note before any mending (480 tests): six places, every one a read of the wrong bank, none
a false alarm.  The suite's other failures were the D17 targets' known misses alone, and its transcripts the
baseline's.

| Where | The read | The bank there | Tests | Mended |
|---|---|---|---|---|
| s_fall, dq0w.c:1166 (part 2) | `HUB->pc_x`, `pc_y` for room_of | the fall's chunk, at () | 8 | 540c42a5 |
| s_zone_step, dq0w.c:5344 (part 24) | `HUB->pc_wet`, `pc_air` | the step's chunk, at () | 75 | b200a0df |
| den_portal, wport.c:251 (part 80) | `HUB->now` (and `dig_t[bk]` written) | a chunk's, the door's work's | 3 | fd68afe4 |
| air_here, dq0w.c:5488 (part 73) | `HUB->pc_stratum` for change | the burst's chunk, at () | 2 | 44c341cf |
| here (), dq0w.c:387-392, from s_start_dig (part 25) | the character's stratum, square, step, fade, the clock | wall_look's chunk | 3 | 8f11362b |
| o_stop_kind, task.c:586 (the UI's overlay 7) | `HUB->pc_task.kind` | overlay 7's own | 6 | 5f7c3f93 |

- **s_fall**: `at (sn_, HUB->pc_x, HUB->pc_y)` put the character's chunk in the window, then `room_of (sn_,
  HUB->pc_x, HUB->pc_y)` read the square from the chunk's bytes (0, 0 in the tests: no square of a stratum), so
  room_of found no room and the fall's floor was its chunk's passages' (`C->mat[NROOMS]`) wherever it landed.  The
  header is put back before the second read (room_of puts the chunk back).  3 bytes.
- **s_zone_step**: at () asked if the square was rubble, leaving the chunk there; `HUB->pc_wet || HUB->pc_air` then
  read its bytes.  A nonzero byte (4, $C2, 2 in the tests) made a dry step wade (its water and air looked at again,
  time and no words); zero bytes kept a wet character's step from wading out of the water in a dry chunk.  The
  header put back first; its two reads made one OR, so 1 byte.
- **den_portal**: a denizen's timed door work (forcing, raising, bending, picking) wrote when its time is up into a
  cached chunk's bytes (two of them overwritten; the store goes through a pointer, the store guard's blind spot),
  with the chunk's bytes for the clock; the hub's `dig_t` unchanged, the work posted again was taken as done, cut
  short after its first moment (60 seconds at most).  The header put back first.  3 bytes.
- **air_here**: a firedamp burst's afterdamp was kept on the stratum the chunk's byte named.  The stratum read once,
  in the header, for both.  5 bytes.
- **here () from s_start_dig**: a wall looked at ends with `start_dig (0xFC)`, its chunk in the window, and
  s_start_dig began with here (): the character's stratum, square, step, its memory's fade and the clock set from the
  chunk's bytes, left so till a move's here () (a fall or a dig asks sn_, mx, my meanwhile).  For a wall's words and
  the shaft through (0xFC on), here () isn't called now: s_dig_on's, just before, read the hub.  2 bytes (the call
  moved).
- **o_stop_kind**: overlay 7's line read its own byte at pc_task.kind's offset ($10, no task's kind), so "stop
  filling", "stop soaking" or "stop dipping" stopped only a task of its word's own kind, not whichever of the three
  the character was at (the design of 2026-11-06), nor the old torch's dip.  The byte copied from the hub (hub_mem, as
  the overlay's other reads are).  23 bytes.

## The 25 candidates

actor0's heuristic scan (a `HUB->` on a later line than a call that maps another bank, no `hub_map (0)` between),
over the world's C, names 25 lines (its 24 candidates and s_fall's).  Four are bugs, and the guard caught each in the
suite; the other 21 aren't:

| Where | Function | The line | Verdict |
|---|---|---|---|
| dq0w.c:1166 | s_fall | `room_of (sn_, HUB->pc_x, HUB->pc_y)` after `at ()` | **a bug**: hit in 8 tests, mended |
| dq0w.c:4823-4830 (6 lines) | s_step_on | `ox = HUB->pc_x` ... after the first step's `elev_at` | not a bug: `pc_trod ()` comes between, and its `trod ()` ends with `hub_map (0)` |
| dq0w.c:5056-5058 (3) | s_dark_way | `HUB->pc_face = h`, `HUB->now` ... after the items' and kinds' banks | not a bug: `known ()` ends with `hub_map (0)`, and `event ()` begins with it |
| dq0w.c:5344 | s_zone_step | `HUB->pc_wet`, `pc_air` after `at ()` | **a bug**: hit in 75 tests, mended |
| dq0w.c:5488 | air_here | `HUB->pc_stratum` for change, after `at ()` | **a bug**: hit in 2 tests, mended |
| wbody.c:169 | s_den_first | `++HUB->den_seq` | not a bug: `BD_PUT ()` (`hub_map (0)`, the body copied back) just before |
| wbody.c:323, 324, 330 | s_den_intents | `HUB->now`, `HUB->round` in I_ROUSE's and I_ATTACK's cases | not a bug: the `at ()` is I_GOTO's, another case; at the switch the header's in from `BD_GET ()`, and `grip_held ()` leaves the window alone |
| wbody.c:339 | s_den_intents | `++HUB->den_seq` | not a bug: `BD_PUT ()` just before |
| wdig.c:250 | s_den_dig | `++HUB->den_seq` | not a bug: `BD_PUT ()` just before |
| wfight.c:269, 272 | blow | `HUB->actor[...].name` | not a read: the name's address, passed to `to_ring ()`, which maps the header itself to copy it |
| wfight.c:366 | s_bled | `++HUB->den_seq` | not a bug: `BD_PUT ()` just before |
| wfight.c:643 | (s_den_rob's) | `#define WSLOT (HUB->slot[SLOT_WORLD])` | not code: a macro's line |
| wport.c:151 | s_den_more | `bd.ready = HUB->now + 10` after `at ()` | not a bug: reached only when `taken ()` was called and gave 0, and it maps the header first |
| wport.c:251 | den_portal | `HUB->now` (and the store to `dig_t[bk]`) after the door's work | **a bug**: hit in 3 tests, mended |
| wtake.c:156 | rest | `++HUB->den_seq` after `news ()` | not a bug: `news ()` ends with `hub_map (0)` |

The guard also found two the scan couldn't see: here () from s_start_dig (the bank wall_look left, a function away)
and the UI's o_stop_kind (an overlay's line).

## What the player sees differently

Two fixes change what's told; the other four change no sentence in any test.

**The fall's floor** (s_fall): seed 517's kobold warren, the gatehouse's ledge jumped down (dq0bearing, dq0harm,
dq0tend, dq0healaid), lands on the gatehouse's floor (the referee's ground GR_HARD), not its chunk's passages'
(broken rock): the referee's harm and its dice fall otherwise.

- "You land hard: a minor bruise to your right shoulder (M1)." is now "You land hard: a minor bruise to your right
  calf (M1).", in all four.
- With it, every telling of the bruise: "You hold a cold compress to the minor bruise to your right calf (M1).",
  "Condition: hurt: 1 injury, a minor bruise to your right calf (M1); all tended; ...", "Injuries: right calf M1
  (bruise), tended."; and "You limp." in the status (a leg hurt, where a shoulder wasn't).
- dq0tend's five days' rest: the bruise's roll is "The minor bruise to your right calf (M1) has healed." where it was
  "... right shoulder (M1) is no better." (the referee's dice after the fall run otherwise); so its last sheet has no
  injuries and no penalty ("Endurance 12, Move 17 ...; universal penalty 0, physical 0", each skill 5 higher, "No
  injuries."), and that wake doesn't say "You're getting hungry." and "You're getting thirsty." (19 of the 20 wakes
  do, not 20): the sleeps' time falls otherwise against the test's pokes of hunger and thirst, the status line showing
  "thirsty" as the later sleeps begin.
- The tests: dq0tend and dq0healaid follow the calf; dq0tend checks the fall's ground (the RS_HARM request's,
  GR_HARD).

**"stop soaking" while dipping** (o_stop_kind): it stops the dip ("You stop dipping the torch."), where it was refused
as not the task's own word.  dq0stopwords dips its torch again and says "stop soaking" to show it (its new lines:
"> dip the torch", "You lift the lit torch.", "You set to dipping the torch.", "> stop soaking", "You stop dipping
the torch.").

**Not changed in any test**: a step's wading (s_zone_step), a denizen's timed door work (den_portal: each test's work
done in its first moment), a burst's afterdamp (air_here: the byte read was 0, the bursts on stratum 0), here ()'s
statics after a wall looked at.

**The suite's transcripts** (the final run against mA5, sim/tools/sentdiff.js): 25 tests differ.  10 are known
noise.  The 15 past it: the four above (dq0bearing, dq0harm, dq0healaid, dq0tend; dq0stopwords is marked noise, its
lines as above); nine by f1cdc9ec's typed line ("echo b115200 >/dev/serctl": dq0doff, dq0letcmd, dq0letdens,
dq0letseveral, dq0letsight, dq0letstow, dq0letters, dq0letwhich, dq0torchwords) and dq0rockwall (7a65b8eb's rock),
as actor0 found them; and `round` (the sound sample's notes, a tick's lateness and its words' interleaving), which
differs since the move to N:: the same with the guards off, with this branch's emulator changes undone, with the
committed SD card, run after run, and in the polish branch's transcripts since the move.  dq0hold, which actor0's
branch moved by a race ("Something stirs."), is mA5's again.

## Room and timing

| | before (f7468c55) | after |
|---|---|---|
| main RAM free: dq0, dq0w, dq0d, dq0p, dq0m, dq0harn, dq0s | 3,043, 2,150, 2,964, 4,422, 3,852, 2,067, 17,526 | the same |
| dq0w part 2 (s_fall), its bank (overlay 29) | 429 (284) | 426 (281) |
| dq0w part 24 (s_zone_step) | 2 | 1 |
| dq0w part 73 (air_here) | 1,766 | 1,761 |
| dq0w overlay 12 (parts 24 and 73) | 222 | 216 |
| dq0w part 25 (s_start_dig), its bank (overlay 22) | 48 (905) | 46 (903) |
| dq0w part 80 (den_portal), its bank (overlay 24) | 108 (34) | 105 (31) |
| dq0 overlay 7 (o_stop_kind) | 705 | 682 |
| the start, seed 1 / 123456789 (dq0starttime) | 237.7M / 313.5M cycles | 237.8M / 312.1M |

Part 24 (1), part 25 (46) and overlay 24 (31) were under the headroom rule's 64 bytes before; each fix in them is as
small as it could be made (part 24's a byte: its two reads made one OR).  No game byte is spent on the guard: it's the
emulator's.  Its host time: about as before (dq0rockwall, two runs each: 8.3 s and 10.0 s with the guards off, 8.8 s
and 9.7 s with them on; a busy machine's noise).

**The suite** (all 480 tests, the guards on and stopping): the D17 targets' known misses alone fail (dq0answer's look
ahead's pause, dq0lookfar's looks' and map's pauses, dq0letstow's "drop 1g", dq0letdens's "attack C" and "attack A"),
9 measures.  Past 90%: dq0answer's "step forward" first letter at both paces (656,448 and 668,852: its three
typings' spread is 612,000 to 685,000 in one run; 628,974 and 626,814 in the run before the fixes, 661,991 in mA5)
and dq0lookfar's map's first letter (683,876).  No normal-pace first letter crosses 720,000.  The trends past 5%:
11, all dq0answer's and dq0lookfar's, the measures that swing from run to run.

## Open questions

1. **The headroom rule's parts and banks.**  The fixes spent a byte of part 24's two, two of part 25's 48 and three
   of overlay 24's 34; each was under 64 already.  Room to be won back there before the next feature, by the rule.
2. **dq0tend's wake words.**  On the wake its bruise heals, "You're getting hungry." and "You're getting thirsty."
   aren't told (the test pokes the wants low as each sleep's typed, and that sleep's time steps fall otherwise once
   the character's unhurt).  A test's timing, not a fault I could find; worth a look by whoever owns the wants' words.
3. **The guards' blind spots, still.**  A load or a store through a pointer into the header (`p = &HUB->x; p->y`,
   den_portal's `dig_t` store, `hub_ask`'s replies' fields) isn't seen, nor a line that reads a hub field's offset in
   another bank on purpose and names that field (none today).  The watch for the part in the swap area takes the
   fourth and last of a task's watches.
