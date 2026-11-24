# DQ0: the decisions archive, 2026-11-15

The agents' accounts of what they built, moved out of DECISIONS.md on 2026-11-24 (branch docs2) with their text
unchanged, as AGENTS.md's Records rule asks: DECISIONS.md holds the user's words only. Each section left a one-line
pointer there; where a section held the user's words too, those stay in DECISIONS.md, quoted, and the whole section
is here. Each section is marked with its lines in DECISIONS.md as at d35194df.

<!-- DECISIONS.md:6025-6051 at d35194df -->
## The dispatch table's rest (2026-11-15; as built, for review: COMMANDS-CRITIQUE.md's §5.2, step L; ENGINE-CRITIQUE.md's row 4)

Step L's stages 1 to 3 (2026-11-08) left two things: the handling verbs' requests in their rows ("if `handle` ever
takes its op from the row"), and §4.4's coverage report ("every cell is handled, refused by design, or missing, and a
build step can print the missing cells").  Both are built; nothing a player sees changes.

- **The requests in the rows.**  A handling verb whose request is the same whatever's said carries it in its row
  (actions.c's `ROWP`): put and hang `IO_PUT`, hold `IO_HOLD`, light `IO_LIGHT`, put out `IO_DOUSE`, rip `IO_RIP`;
  `verb_row` puts it in `cmd.aux` where the line gave none, as the door verbs' `PA_*` were (stage 3).  The parser
  gives these verbs their request already, so the rows change nothing typed; what they change is a verb carried out
  as another's (hthings.c's `as_verb`: use's light and its key's unlock), now given 0 and the row's request filled in.
  Wear (don, or worn only) and take off (doff, or stow) keep theirs in the line: two requests each.
- **The coverage report** (games/dq0/make.js's `coverage`, run with the build): every verb's row read from actions.c
  (its handler, its refusals for a word and for a kind, the parts of the place it does: `masks[]`), and a cell
  written for a thing named by its word and each part of the place (OB_DOOR ... OB_SIDE): `H` its handler's (the row
  says the verb does it), `yet`, `cant`, `note` refused by the row (U_YET, U_CANT, U_NOTE, refuse.c's words before
  the handler's asked), or `-` the handler's own (the table says nothing; the handler answers as it will).  The
  powers' rows say "refused, whatever's named", the rows seen to before the table (resolve.c's, parts.c's, the
  puzzle's) "seen to before the table".  Written to obj/games/dq0cover.txt, a line a verb; the build's log the sums:
  143 verbs, 2,074 cells, 91 its handlers', 417 refused by their rows, 1,566 the handlers' own.  Nothing fails on it.
- **Not built, for review:** §4.4's flags per action (NEEDS_REACH, NEEDS_HELD, NEEDS_LIGHT ...) and a handler per
  class: the shared paths (main.c's `things`, `feature`, `reach_go`) do those steps for every verb already, and the
  "-" cells are where a row's refusal would go if the handler's own answer is ever wanted from the table instead.
- **Room:** the UI's part 29 1,647 bytes free to 1,659 (as_verb's two requests the rows'); overlay 19 234 to 246; the
  main RAM unchanged (3,143).  The world, the senses unchanged.  The start unchanged (no code on its path).
- **The suite:** all 451 pass; the transcripts the same, but the known timing noise (nine games' denizens met at other moments, dq0hold's rat among them).

<!-- DECISIONS.md:6052-6156 at d35194df -->
## Compartments: containers and surfaces (2026-11-15; as built, for review: COMMANDS-CRITIQUE.md's §4.9, step 6b; ENGINE-CRITIQUE.md's row 4)

Till now a kind held things one way: `KF_ON` a surface, `K_BELT` slots, else a container, and a thing in another
recorded only its holder (`IW_ITEM`, x).  A chest couldn't have things in it and on its lid.  The user approved row 4
finished (2026-11-15), step 6b with it.  What's built:

- **The compartment table** (items.h's `struct comp`, dq0w/kinds.c's `comps`): a row a compartment, a kind's rows
  together in the kinds' order, compartment 0 first, eight bytes: its holder's kind, its prep (`CP_IN`, `CP_ON`,
  `CP_HUNG`), the forms it keeps and what closes it (a kind's keeps' bits: `KF_SOLID` ..., `KF_LID`, `KF_TIE`,
  `KF_STOPPER`), its room (pints), the weight it bears (quarter pounds), its family (a scabbard's swords; a belt's,
  its slots), its own noun (`CN_*`: "lid", "top", "drawer"; none used to name one yet).  In the catalogue's bank, its
  own segment first (dq0w.cfg's `CATCOMP`, at $8000 there), 21 rows and an end, 176 bytes.  Read by `comp_of (k, c)`
  (the main RAM's: the catalogue's bank put in the window, its `o_comp_of` there, the row copied to `crow`), and by the
  senses for which kinds have a top past their first (dq0p's `on_lid`: a bit a kind, made once).
- **Every kind that holds things converted, one row each:** backpack, belt, pouch, waterskin, bones, sack, chest,
  basket, table, chair, quiver, body, iron pot, sconce, scabbard, sheath, bandolier, bolt case, vial, cask.  Compartment
  0's row is its kind's record's own (prep by its flags, keeps, cap_bulk, cap_weight, slots): dq0w/make.js holds them
  the same (`compsSame`: the rows in order, NCOMP at most a kind, the first its record's, every kind that holds things
  has rows and none that doesn't), so the code not yet reading the table reads the same numbers.  A kind with no rows
  (a theme's) has its record's alone (`comp_of`'s, made from it).  The heaps (a cloak, straw, furs: things hidden
  under them, `cap_bulk`) aren't compartments.
- **The chest two:** its inside (lid, lock: as ever) and its lid's top (`CP_ON`, 20 pints, 200 quarter pounds, open
  to the hand whether the lid's shut or not).  The only kind with a second compartment: the one the critique names
  that the dungeon already places (vaults, storerooms, quarters, the solar, the armoury).
- **A thing's place** is its holder and its compartment: `IW_ITEM`, x the holder, and its record's y `IC_MARK` (0xFF00)
  | its compartment's prep << 4 | its number (1 on).  Any other y is compartment 0: the stock's (its square's y), the
  outfits', an old save's.  So no record's layout changes, and a thing put anywhere but a second compartment is
  written byte for byte as before (stow's `o_place` writes the mark; `store`, `place` and `o_place` clear it as a thing
  leaves).  `icomp (i)` reads it, checked against its holder's rows (a stale mark in a one-compartment holder reads 0).
  The prep in the mark lets the senses tell a thing on a chest's lid without the table.
- **Each compartment's state:** its closed bit in its holder's `shut` (`IS_SHUT_C (c)`: compartment 0's `IS_CLOSED`,
  its lock its own as ever; 1 to 3, 0x20, 0x40, 0x80, free bits); a lock only compartment 0's (one byte a record).
  Capacity per compartment: `o_fits` reads the compartment's row, its room and weight against what's in that
  compartment alone (its things summed there, the catalogue's code's, by `items_in`); a thing's weight still all it
  holds.
- **The verbs, by the one path for things:**
  - *put X in / on Y* (and *drop X on Y*, the UI's put_in given its prep): the holder's first compartment that's in
    (or on) as said, else its first (a sack's "on" its inside, a table's "in" its top, as they always were);
  - *take X from / out of / off Y*: from any of its compartments that's open (a thing on a shut chest's lid taken; one
    inside it, "The chest is closed."); a holder of one compartment, closed, refused before it's looked in, as ever;
  - *look in Y*: compartment 0; *look on Y* (the UI's: " on " in the line, `IO_INSIDE`'s to 4): its first that's on;
    *look at* / *examine*: compartment 0 as ever, then each other that holds something ("On it lies a chisel."), a
    shut chest's lid's top told;
  - *search Y*: its inside (closed: "you go over the outside of it"), then each other that holds something ("On the
    chest you find a chisel."); a thing's own search (on it, in it) as ever for every other kind;
  - *open Y*: what it reveals its inside's alone, not what's on its lid;
  - *look* (the senses): what's on a chest's lid seen with it, as a table's things are ("On the chest lies a chisel.");
  - *look at X* on a chest: "on the chest" (its compartment's prep);
  - the reach (part 56's `level_of`, part 85's `rs_walk`): a compartment that isn't `CP_IN` no level, its holder's lid
    shut or not; the way in a put's compartment's (`rs_cc`), a thing got out its own (`icomp`).
  - Two lots of a counted thing join only in the same compartment.
- **What a player sees change** (new behaviour, none of it in an existing game: every sentence differs only where a
  thing's on a chest):
  - "put the chisel on the chest": "You put the chisel on the chest." (was "You put the chisel in the chest.", into it);
    "drop the chisel on the chest" the same;
  - "look on the chest": "On the chest lies a chisel." / "There's nothing on the chest." (was what's in it, "In the
    chest lies ...", "The chest is empty.", or "The chest is closed.");
  - a look: "On the chest lies a chisel." (new: nothing was on a chest);
  - examined, looked at: "On it lies a chisel." after its inside's words, closed or open (new);
  - searched: "On the chest you find a chisel." after its inside's words, closed or open (new);
  - "take the chisel from the chest", the chest closed, the chisel on it: "You take the chisel from the chest."
    (new: nothing was on a closed chest);
  - opened: "You open the chest, revealing a sack." (what's on its lid not revealed: new only with something there);
  - "look at the chisel" on it: "The chisel is on the chest." (new).
- **Saves:** no record's layout changes.  An old save's things in others read compartment 0, which is where they were;
  a save made now with a thing on a chest's lid read by an older build puts it inside.
- **The test dq0compart** (tests/suites/dq0/gear.js): seed 3's kit chest, a chisel on it (its record: in the chest, y
  `IC_MARK | CP_ON << 4 | 1`), a sack in it, the chest shut and opened, each verb's words.  It fails on the old build
  ("look on the chest" says what's in it).
- **Room** (against the dispatch table's rest's build): the world's main RAM 2,818 bytes free to 2,511 (`comp_of`'s
  call, `icomp`, `in_shut`, `items_in`, `store`'s clearing; the table's walk in the catalogue's bank), the catalogue's
  bank's code 4,555 to 5,546 bytes of its 6,400; the senses' main RAM 4,891 to 4,431 (`on_lid`, its bits, `lying`'s
  mark); the UI's main RAM 3,143 unchanged, part 2 45 to 39, overlay 1 2,172 to 2,145, overlay 14 1,185 to 1,179.  The
  world's parts: 10 1,527 to 1,487, 11 172 to 101, 35 60 to 0, 36 618 to 530, 39 1,529 to 1,464, 47 2,731 to 2,467,
  61 2,239 to 1,929, 85 2,654 to 2,519, 92 1,312 to 1,306; the packer moved the map's `s_draw_test` from part 56's
  bank to part 99's (56 2,632 to 2,957, 99 2,911 to 2,513), and the parts' banks among the overlays (5 259 to 142, 7
  151 to 86, 8 274 to 300, 13 31 to 1,426, 14 498 to 136, 15 368 to 32, 16 762 to 692, 17 1,351 to 133, 22 123 to
  221, 27 421 to 23, 28 392 to 327).  The timings: dq0traces' step 1,887,857 cycles (5 squares) at the baseline, 1,853,479
  after the dispatch table's rest, 1,906,964 now (its budget 1,920,000; its spread between builds some 35,000);
  dq0answer's normal listen's first letter 709,561, 695,411, now 600,809 (a denizens' pass landing before it or not).
- **The start:** seed 1 232.8M cycles to 233.1M, seed 123456789 307.8M to 308.8M (the world's bigger image read; its
  budgets 245M and 325M).
- **The suite:** all 452 pass (dq0compart new).  The transcripts by sentence against the dispatch table's rest's: six
  differ, all the known timing noise (dq0answer, dq0jobs, dq0lookfar, dq0passes, dq0prompt, dq0skykin: denizens met at
  other moments).
- **The save folder, byte for byte** (fhash, as "One record store"'s: every save-folder file each task wrote, by its
  ops and their bytes hashed, over 32 games: 8 seeds, each -C -T -T -T, -C -T, -T -T and -C, a random player's
  commands, quit, carried on, saved): all 32 the same as the build before (30 played to their end, two stopping early
  alike in both).
- **Open questions (the user's):**
  - What's on a chest's lid as it's opened: built, it stays there (no code); it would slide off, or the lid be refused
    till it's cleared.
  - The lid's top's capacity (20 pints, 200 quarter pounds: the house's guess, a table's chair's between).
  - Furniture placed by the dungeon (desks, cabinets, cases, shelves): none is placed today, and the maker's tables
    (dq0m/uses.c) placing a new kind change every seed's dungeon; the table takes a desk as rows (an on top, drawers
    that close) when it's wanted, but a drawer named by its noun ("open the drawer", "the lid") needs the resolver to
    match a compartment's noun (the `noun` column is there, unread).
  - A lock for a compartment past 0 (one shut byte: closed bits for three more, no lock bits); hidden compartments and
    their finding (a search's test, then known).
  - The denizens: a robber's reach and a denizen's gear take from any compartment (the denizens' own code unchanged).
  - Left as they were, compartment 0's state alone: an ally's errand (fetch_find: a thing in something closed), the
    letters' "the open chest" (SL_OPEN), stow-where's candidates, what's carried nested (a carried chest's lid's
    things listed "in it").
  - The backpack's side pocket (the critique's second example): its reach, its tie, a pocket named; not built.

<!-- DECISIONS.md:6157-6328 at d35194df -->
## Narration as data, the rest (the user's, 2026-11-15; as built, for review: ENGINE-CRITIQUE.md's §3.2 and row 5's Left)

Row 5's last "Left:" note named three things: the world's other free text in things_text, the look's wording with
fight_note's stances, and the sound's mapping.  The user approved finishing them on 2026-11-15.  This entry is what was
built, one group a commit with the suite after each, and what was not, and why.  The words the player sees are the
same, letter for letter (the transcripts below).

**1. The character's own state: a UI kind of its own, UK_SELF (44f0fd01).**  Like stage 4's kinds, never in the hub:
the UI fills one on the C stack and has it worded.  msgs.def's `UK_SELF < E_OUTCOME; w1 my:8-15@MY_WANT v:0-7` (and the
head's n2, b): its three fields are shapes the table had already (said_f's rows 0, 7, 14), so overlay 27 pays nothing
for them.  dq0.h's `MY_*` names the 22 rows (said.txt's SELF section, 24 rows with the noise's and one piece, `@bg`):

| was | now (SELF rows) | where |
|---|---|---|
| facing_words' eight sayf lines and its four tables (hunger, thirst, the position, the limbs) | want, limb, limp, pos, hidden, under, swim, deep, facing (the heading's name as text, head_name's) | task.c, overlay 19 |
| turned_says' four sayf lines and its two tables | step, setoff, face, turn | task.c, overlay 19 |
| s_pose_words' sayf and its two tables | pose, posed | main.c, part 12 |
| s_bg_words' three sayf lines, its msg and its three tables | draught, still, heard, behind; the background's words a piece | main.c, part 14 |
| fight_note's eight stances (its words left in lb) | stance: said_q, the words left in lb as before; D_OWN's the ruleset's (rs_owns) as text; FN_AWAY the refusals' still (say_no's A_LB) | blows.c, overlay 22 |
| time_told's five sayf lines | time (v: how sure), lost (v: how it lost track, TG_*: the hub's tb.how) | time.c, overlay 16 |
| heard_words' strcpy and strcat lines (a denizen heard, into lb) | noise: its sound's words as they are, "a muffled" (w4 past the sound's "a "), "muffled", a noise, something moving; its way | caps.c, overlay 23 |

Each row is one of the old say ()s, so the words wrap where they did.  caps.c's locals are static in overlay 23's own
bank (`static-locals (on)`, `bss-name OV23BSS`): heard_words' event is on the C stack (`static-locals (push, off)`), the
renderer in overlay 27 reading it there.

**2. The world's parts' own sentences, coded (6da7dccb).**  Stage 5 coded the catalogue's sentences (the world's say,
SA_*); the parts that built their own still put words into things_text.  Now they put a code too (items.h's form:
SAY_AT, sa | 0x80, the thing's name, SAY_SEP, the other's, SAY_SEP), each part with its own copy of the coder
(things.c's `SAY_PUT (name)`, the world's tbuf; wcloth.c's told and wwater.c's wsaid their own lines).  **Why a copy
each:** the first build had one helper in the world's main RAM (55 bytes in things.o's code), and dq0traces failed: a
step's cost 1,943,943 cycles, its budget 1,920,000.  Everything linked after things.o moved, the C library's routines
with it; nothing the helper did ran in a step.  With
a copy in each part the world's main RAM is byte for byte as it was, and the step costs 1,882,444.  (The cause, most
likely: the library's routines are called thousands of times a step.)  61 SA_* codes more (items.h, at the enum's end),
61 SA rows and two pieces (said.txt's new section after SELF, "The world's parts' own sentences"):

- part 36 (things.c's s_shut_work: lids, ties and stoppers, a chest's lock locked, unlocked, picked, forced): its
  say3 and its two tables gone, 26 rows.  Opened ("You open the chest", "You untie the sack") and forced, the row ends
  without its full stop as the words did: what it shows (reveal's text) follows it in the text as plain words.
- part 100 (s_fill: a light filled, a torch soaked): put3 gone, 10 rows; "You fill the lantern from the flask of oil,
  and it's empty now." two rows (`@fill`, `@soak`), the code chosen by the fuel used up.
- part 48 (why_not, the hold refused): SA.hands is why_not's "Your hands are full: stow something, or put it down." (the
  same words were written twice, stage 5's note); "The ... takes both hands.", "Your left arm is useless.", " You can't
  hold the ... in one hand.", "Your right hand holds the ...".
- part 101 (s_swap): "Your right arm is useless." is SA.arm, the hold's row; "You shift the ... to your right hand."
  SA.shift, the hold's row (the hand's words as obj); both hands, empty hands, the change of hands.
- s_doff_feet (part 74's): one row.
- wcloth.c (part 108, the cloth torn up and washed): told () codes its sentence; "You have no ... to hand." is
  SA.no_such, the catalogue's row (the same words); 14 rows more, the bandages' count the other name.
- wwater.c (part 74, a waterskin filled): three rows.

The rows' text took a third bank: dq0.cfg's OV30 ("no parts"), linked at $C000 as OV29 is at $A000, bank.s's said_b
choosing it by the address (OVMAX 30), saidgen.js placing each kind's rows and each piece wholly in the first of three
banks with room.  The SA kind's rows (116 then) are one block, so the kinds moved between the banks.

**3. The look one way: a UI kind of its own, UK_AHEAD (40977004).**  way.c's focus_words ("look north", "look through
the grille") said its words in a dozen say and sayf lines, its two word tables (rel_w, noun) with them.  Each is a row
now (said.txt's AHEAD section, 15 rows and two pieces, `@ahnoun` and `@ahrel`), one row a say () as before, so the
words wrap where they did: the peer ahead, a spy-hole's cover, a closed way not seen past (its plural, "doors",
"portcullises", and its state, closed, barred, locked, they're closed, its shutters, theirs), the way through and what's
beyond it (only darkness, the sky up a light well, a passage and its feet, a room by the theme's name, its feet on, its
light, another way into it), a light far off, nothing more.  msgs.def's `UK_AHEAD < E_OUTCOME; a pl; b rel; w1
ah:8-15@AH_PEER k:0-7; w2 far:0-7 room:8-15`: one field new to the table (room, w2's high byte: 5 bytes of overlay
27); the rest the shapes it had.  way.c's statics (ahpl, ahrel, ahfar: overlay 13's bank) carry a row's fields and
`ah ()` fills the event on the C stack (overlay 13's locals are static, in its bank: `static-locals (push, off)`).

**4. The rope's and the drag's: a second code byte (fc828c2c).**  The SA code was 7 bits (SA_* | 0x80, 128 at most) and
116 were used.  A code past 127 is put after its own marker, items.h's `SAY_AT2` (0x03), its SA_* | 0x80 after it as
ever; tt_words finds either (a loop of its own: the library's strpbrk cost the UI's main RAM 50 bytes) and adds 128
for SAY_AT2's.  The rope's 27 sentences (wrope.c, wrope2.c,
wrope3.c: tied off, let down, untied, shaken loose, thrown up; wrope.h's rp_say and rp_no, a copy each part as the rest
of wrope.h) and the drag's 8 (wdrag.c's dsay: fixed, not a thing to shove, no room, the strain, dragged, shoved,
against the door) are SA rows, past 127; the tie's four by where it's let down and whether it reaches, `@tie` their
piece.  "The chest is fixed where it stands." is one piece, `@fixed`: the catalogue's SA.fixed and the drag's
SA.drag_fixed (its new line first).  The SA codes now 151 of 253.  rp_look (the rope looked at) keeps its words
(below); rp_name now names into its caller's buffer.

**The words most said (saidgen.js's WORDS_MAX 200 -> 180).**  The renderer's dictionary (said_dw, said_do: overlay 27's,
beside the renderer) grows with the rows' text: overlay 27 had 126 bytes left before this, 8 with the rope's rows.  At
most 180 words now: overlay 27 has 89 bytes left, the rows' banks paying the rest (some 100 bytes).  The words the
same; a row's bytes differ.

**The SA kind's rows last (said.txt's `last SA`).**  The rows' banks are read through said_b, a few cycles more a byte
in the second bank and more in the third.  With the SA kind grown, the first fit put SA in the second bank and pushed
the denizens' DEN rows (their doings and sounds, said as they come) into the third, and dq0answer's normal-pace listen
had its first letter at 761,557 cycles (budget 760,000; 687,649 before the rope's rows).  A said.txt line `last KIND`
places a kind's rows after every other kind's and the pieces (saidgen.js): `last SA`, the world's things' words said
once an answer, now in overlay 30, BEAT, HARM, TASK and DEN in overlay 29.  The listen's first letter then 524,082.

**Not moved, and why.**
- **The look's lists**: what's seen lying about (seen.c's items, one, plural), the denizens seen and their stirs
  (dens.c's tell_one, group, o_stir_words), touch_words' walls felt in the dark, the inventory's list, a thing looked at
  closely (things.c's look at, LM_AT ... LM_TRAPS), a search's finds.  These aren't sentences but lists made clause by
  clause: counts and their words ("two iron pots"), plurals ("heaps of sand", "ies", "es"), alike things told
  together, the designators' capitals, the stream (lbsaid.s's lb_said: none twice in an answer, `\002` a thing's
  sentence), the room left in lb.  A row a clause would be fragments, the composing still the code's: the words in two
  places, not one.  **Open, for the user:** a list renderer (a row for the list's frame, the renderer making the
  list's items and "and") is the way to move them; it's a design of its own.
- **The worn places' (wworn.c's say_at: "on your left hand", a body part's phrase put into other sentences) and the
  rope looked at (wrope2.c's rp_look: what it's tied to, how far it goes)**: phrases of a look's and a refusal's, not
  sentences; with the look's lists.
- **The well's (wwell.c, wwell2.c: "You let the torch fall into the well. A moment, and you hear a splash far
  below.")**: its words go to the traces' text (UIREC's trtext, said by E_VIEW's V_TRACED), which tt_words doesn't
  read; it wants a reader of its own first.
- **A count's** ("You take 4 rations.", "There are only 3 rations."): the put and take handlers', the containers'
  work's area now (step 6b); left to it.
- **The SA_ refusals and the first words from a begun row**: stage 5 made the refusals rows; stage 4 made the first
  words FIRST rows keyed by the verb.  The world's own begun row for each action, rendered by the UI from the command's
  handles, still wants each action's begun row declared once (stage 4's note): not built.
- **The sound's mapping (dq0s's world_sound)**: still not a clean column.  Of its five cases two come from an
  outcome (E_PORTAL's po, E_TRAP's to); E_NOISE's comes from its what (NZ_*, no rows: it has no words); the denizens'
  sounds are chosen in dq0s's listen loop by loudness and square, not by an outcome.  The sound task can't read the
  UI's banks, so a column would need a table of its own made for dq0s, holding half the mapping and leaving the rest
  in the code.  Left as it is.

**What the player sees.**  Nothing new.  Every dq0 test's screen was kept before (40c7b7a1, built here) and after each
group (a hook in sim/test.js, not committed), the sentences compared with sent.js: 301 tests each.  Against the
baseline, after the last group, 8 differ (dq0answer, dq0jobs, dq0lookfar, dq0mine, dq0prompt, dq0robrest, dq0skykin, dq0sway),
and in the four runs between the groups the same kinds and a few more, all of them the known timing noise:
- denizens met at other moments (dq0answer, dq0hold, dq0jobs, dq0lookfar, dq0mine, dq0passes, dq0skykin, dq0stopwords:
  their stirs, a rat or an adder seen or gone a moment apart, a sound heard one time more or fewer); dq0hold is new to
  the list (the hold-back's test, 10341161): its rat's "gone from sight" a moment apart, as the others';
- dq0robrest (a rest ending at another moment);
- dq0sway's typed-ahead key echoed a moment later ("behind you.&O" / "behind you.1O");
- dq0prompt's typed-ahead turn (below): "You turn to face south." or "You turn left, to face south.", from group to
  group as the timing moved (group 1 the second, group 2 the first, groups 3 and 4 the second).

No moved sentence is said differently, nor a different number of times, in any test.

**Room** (bytes free, the build's table):

| | before (40c7b7a1) | after the four groups |
|---|---|---|
| dq0's main RAM | 3,143 | 3,130 (said_b's third bank and ovbank's byte, 11; groups 1 and 3 a byte each) |
| dq0 overlay 16 (time_told) | 2,000 | 2,457 |
| dq0 overlay 18 (part 14's image: s_bg_words) | 747 | 1,140 |
| dq0 overlay 19 (facing_words, turned_says) | 234 | 769 |
| dq0 overlay 22 (fight_note) | 536 | 699 |
| dq0 overlay 23 (heard_words) | 379 | 507 |
| dq0 overlay 13 (focus_words; part 9 back in its own bank) | 3,978 | 825 |
| dq0 overlays 15, 20, 24, 26 (the packer's moves: parts 3, 6, 9 and 29) | 319, 3,039, 607, 185 | 3,650, 334, 264, 4,354 |
| dq0 parts 12 (s_pose_words), 14 (s_bg_words) | 2,188, 1,632 | 2,350, 2,025 |
| dq0 overlay 27 (the renderer, the dictionary) | 126 | 89 |
| dq0 overlays 28, 29, 30 (the rows) | 3, 1,487, none | 3, 618, 4,847 |
| dq0w's main RAM | 2,818 | 2,818 |
| dq0w parts 36, 48, 74, 100, 101, 108 (the locks', the hands', the water's, the fill's, the swap's, the cloth's) | 618, 1,167, 2,092, 1,954, 2,637, 1,436 | 1,264, 1,329, 2,149, 2,186, 2,752, 2,074 |
| dq0w parts 95, 96, 98 (the rope's), 109 (the drag's) | 42, 1,301, 1,012, 955 | 299, 1,748, 1,230, 1,309 |
| dq0w overlays 9, 13, 14, 15, 18, 19, 29, 30, 31 (the parts' images) | 298, 31, 498, 368, 390, 131, 443, 254, 474 | 530, 677, 1,317, 530, 505, 188, 797, 958, 692 |
| dq0w overlay 3 (the packer's: part 53 there now, not in 14) | 210 | 29 |
| dq0d, dq0p, dq0s, dq0m, dq0harn | | untouched |

In all: the UI's code banks and parts 3,530 bytes more room, the rows' banks 4,251 less (overlay 30 new, 8,192),
the world's parts 3,126 more; the main RAMs as they were (dq0's 13 less).

**The start** (dq0starttime; budgets 245M and 325M): seed 1 232.5M before, 232.1M, 232.4M, 233.2M, 232.5M after the
four groups; seed 123456789 308.1M before, 307.9M, 308.3M, 309.6M, 308.5M.

**Tested:** all 451 tests pass after each group.  dq0traces' step cost (budget 1,920,000): 1,882,444 after group 2,
1,857,815 after group 3, 1,908,007 after group 4 (the step's cost moves with the layout, 1,842,187 in a
build of group 4 before `last SA`).

**Open, for the user.**
- A list renderer for the look's lists (above).
- The well's words (the traces' text) a reader of their own, then rows; the worn places' phrases with the lists.
- Overlay 27's room: the dictionary capped at 180 words keeps 89 bytes; the containers' work's rows will take some of
  it.  A smaller cap (or the dictionary in a bank of its own) is the next lever.
- dq0prompt's typed-ahead turn (group 1's run): "turn left" and "inventory" typed at once, turned_says reads the
  command queue as the turn's words are said, and found the inventory's command there before ("You turn to face
  south.") and the turn's now ("You turn left, to face south."): the old race, moved by the code's timing, not a
  change of words.  The turn's own event carrying its way as said, read instead of the queue, would end it.
