# DQ0: the decisions archive, 2026-11-16

The agents' accounts of what they built, moved out of DECISIONS.md on 2026-11-24 (branch docs2) with their text
unchanged, as AGENTS.md's Records rule asks: DECISIONS.md holds the user's words only. Each section left a one-line
pointer there; where a section held the user's words too, those stay in DECISIONS.md, quoted, and the whole section
is here. Each section is marked with its lines in DECISIONS.md as at d35194df.

<!-- DECISIONS.md:6347-6483 at d35194df -->
## The turn's words, the well's, the look's lists (the user's, 2026-11-16; as built, for review: ENGINE-CRITIQUE.md's §3.2 and row 5's Left)

Three of the eight answers of 2026-11-16 (the entry before): a turn always "You turn to face ...", the look's lists a
list renderer (the phrases in the table, the words the same), the well's words a reader of their own.  Built a stage a
commit, the suite after each, with one more before the lists: the renderer passes over a block's alternatives in a read
(without it the lists' words were over their budgets).  Every dq0 test's screen was kept before and after each stage (a
hook in sim/test.js, not committed) and compared a sentence at a time (sent.js; and a copy of it that joins a
paragraph's wrapped lines first, so a sentence re-wrapped by a shorter one before it reads the same).

**1. A turn always "You turn to face ..." (cde43ae0).**  The world's I_TURN puts the heading the character believes it
faces now (its pc_face and pc_skew) in its E_VIEW V_FACING's b, | 32, and task.c's turned_says words it by that
(said.txt's SELF.face).  The command queue isn't read as the words are told: it was read to word the way it turned
("You turn left, to face south.", the parser's CQ_REL), and a command typed after the turn could be found there instead
(dq0prompt's typed-ahead race: "You turn to face south." or "You turn left, to face south." by the timing).  SELF.turn
and MY_TURN gone.  **What the player sees:** every "You turn <way>, to face <heading>." (left, right, half left, half
right, around) is now "You turn to face <heading>.", the heading the same in every test (40 dq0 tests have such a turn).
The expected strings changed for it alone: dq0answer's two turns ("You turn"), dq0newlight's six, dq0bearing's two,
dq0reachstop's games 6 and 9 ("You turn to face").

**2. The renderer passes over an alternative in a read (6805c3a7).**  A choice ({F?a|b|c}), a text said if a field's
changed ({F!...}) and a choice by a word given had their alternatives read a byte at a time through said_b to find the
one wanted and to pass the rest (said.c's over and step: some 100 cycles a byte).  The lists' first build made a
denizen's doings a choice of twenty-three and every sentence of a look a row: dq0traces' step cost 1,937,871 cycles
(budget 1,920,000) and dq0answer's normal-pace listen 904,490 (budget 760,000; a stir told before it).  Now saidgen.js
puts each alternative's length first (a byte, after the dictionary's packing: its bytes to the SC_ALT or SC_END that
ends it; one past 254 stops the build) and over is a read, the alternative's start and its length.  Every row is the
quicker for it: the normal-pace listen 736,302 -> 641,930, the step 1,867,570 -> 1,860,371.  The rows 754 bytes more.

**3. A list renderer for the look's lists (df2fb3e1).**  One renderer, said.c's.  A row's {list} says the UI's list
(said_li: its frame's a entries, struct li each, its fields where a struct msg has them, so a row reads an entry as it
reads an event), each entry by its own row (msgs.def's UK_LI: li, LI_*), joined by the table's own words: ", " and
" and " before the last ({list}), "; " ({list;}), ", " ({list,}).  Its words besides: {plural} (a kind's name made more
than one, its first word's before " of ", "heaps of sand"), {people} (a people's way, the whole, "dwarves"), {A} ("A",
"An" before a vowel); an "a" made "an" as each entry is said (a list has several); a field a slot's whole word (a count
past 255: saidgen's SF_WORD).  A sentence with a list is a UI kind of its own (UK_LIST: lf, LF_*), worded into lb at its
end by list_words (overlay 27's, through its stub), its caller's add ()s streaming the sentences as before (lbsaid.s's;
the stream's \002 and a sentence's first space still the caller's).
- **seen.c's things lying about** (items: at its feet, on the floor its way, hidden, in a corner or against a wall,
  on a surface, in a sconce): LIST.lie, where, in; each thing LI.thing ("two iron pots", "two lit torches", "a lit
  torch", "some bones", "5 rations", "three small things", "something large"), its capital its entry's (w4: cap_of's,
  given in the order it was); the drawing things packed, LIST.packed.  relname, few, plural, addn and one's words gone.
- **dens.c's denizens** seen, gone from sight, and their stirs (tell_one, named, o_den_words, o_stir_words): LIST.den,
  gone, stirs, nostir, heard; each LI.den ("Two giant rats", "The two giant rats", "The giant rat", "An adder",
  "Something", "Two things", "Grub"); the doings a piece (@doing: the twenty-two, and many keeping their distance); its
  gear and its way fields.  A denizen's name is copied to the C stack first: the renderer reads the main RAM, not overlay
  9's bank (the first build said its plural from $FF bytes).  relname, count, few, doing, plural and addn gone.
- **task.c's walls felt in the dark** (touch_words): LIST.touch, its kinds a list joined "; ", each LI.touch with its
  sides ("a wall ahead, to your left and to your right"), the room known by {room d}.
- **What the player sees:** nothing new (the transcripts: every dq0 test's sentences the same but the timing noise).
- **dq0traces' step** 1,860,371 -> 1,913,687 (budget 1,920,000): a look every third square, its things and denizens
  now said through the table; the step's cost moves with the layout too (1,844,902 after the well's stage, this code
  the same; 1,871,885 after the world's things'): it's close to its budget, see below.

**4. The well's words, a reader of their own (c851734b).**  wwell.c's "Far down, the well's shaft narrows to a crack the
water seeps through: there's no climbing down it." and wwell2.c's "You let the torch fall into the well. A moment, and
you hear a splash far below." (or down the shaft; it lodges a little way down; it lands) were plain words in the UI
bank's trtext, the traces' text, said by E_VIEW's V_TRACED, which tt_words didn't read.  Now the world codes them as
the things' sentences are coded (SAY_AT2, items.h's SA_WELL_CHOKED ... SA_SHAFT_LANDS, the thing's name), and words.c's
reader of the traces' words has tt_words word a coded text: tt_words takes the text it reads again from after a
code's words (things_text, as tt () reads it; trtext, the well's).  said.txt's seven SA rows and five pieces (@fallwell,
@fallshaft, @lodges, @splash, @lands), after the rope's and the drag's.  The words the same (dq0well's).  This stage was
built before the lists first: alone it put dq0answer's normal-pace listen at 761,724 (budget 760,000; 763,549 before
the alternatives' lengths), a layout's timing, not its words (the listen 628,498 with the lists before it); so it's
after them.

**5. A thing's own words, the world's, by the same renderer (48cc76c0).**  things.c's words (part 11's: a thing in what's
carried, a search's finds, what's in a thing or on it, every list the world makes of things) put a thing's words in the
things' text: "a ", "two ", "12 pints of ", its name made more than one (many's, part 105's), " (lit)", " (battered)",
" (closed)" ...  Now it puts a code (items.h's SAY_LI: how its count is said and its noun's form, LN_*; its state; its
wear and what closes it; its count, fourteen bits; its kind's name as it is; SAY_SEP), and the UI words it by said.txt's
LI.item, the list renderer's ({single}: one of a lot's, "a ration", "a penny", as many said it): tt_words where the
things' text is read (a search's finds, a thing looked at and what's in it, a container opened), and words.c's
list_say (the inventory) a line at a time, each line's code worded (tt_words with no text to read again: the line's
rest kept in lb), not in pieces of lb's size as before.  The names stay plain in the text (the dig's tools are found by
them, main.c's strstr), so the world's and_last (the last ", " made " and ") and the alike checks work as they did.
many isn't swapped in for an inventory's alike things now.
- **What the player sees:** nothing new.
- **Its cost:** the inventory's whole answer takes longer, each thing worded through the table: dq0answer's inventory
  1,494,406 -> 2,316,825 cycles to its last letter at the turns pace, 1,796,498 -> 2,693,172 at the normal (no budget;
  its first letter, "You carry", told before the world's asked, as it was).  Its first build was slower still (some
  10M: each line had tt_words word every code within its 250 bytes, the rest thrown away; now a line's alone).
- **The world's BSS as it was:** words' digits keep their static (b[6]) though the code is put from tbuf: without it
  every BSS variable after it moved six bytes and dq0traces' step cost 1,922,509 (the world's layout, as the rest's
  first build found: "Narration as data, the rest").

**Not moved, and why.**
- **The world's lists' joins and frames**: ", " between a container's things, ", and " between a search's finds,
  and_last's " and " (three copies, parts 36, 61, 62), "In the chest you find ", " (in it: ": still the world's text.
  Each is put by the world a thing at a time in parts with little room, and the compartments' work is in those lines
  now (comp2's: "(on it:", "(in it:", a compartment's own).  A list coded whole (its frame a row, its things entries,
  the joins the renderer's) is the next step: open.
- **A thing looked at's own sentences** ("Your torch is in your left hand. It's lit, burning low, with a lock."):
  look_at's; sentences more than a list (its state's words joined ", "); SA rows and a coded state would take them.
- **caps.c's "Which one?" words** (thing_said: "(E) a torch (lit), (F) a torch (spent)"): the inventory's words but
  its own way with a count ("6 arrows" where the inventory says "six"); LI.item could word it, a form more.
- **The worn places' phrases and the rope looked at** (wworn.c's, wrope2.c's), a count's words: as before.

**Room** (bytes free, the build's table):

| | before (8d476fbd, built here) | after the five stages |
|---|---|---|
| dq0's main RAM | 3,130 | 3,106 (said_li, list_words' stub, the library's: stage 3, 20; tt ()'s argument: stage 4, 4) |
| dq0 overlay 2 (words.c: the traces' reader) | 32 | 18 |
| dq0 overlay 9 (dens.c) | 156 | 1,188 |
| dq0 overlay 12 (seen.c) | 72 | 614 |
| dq0 overlay 16 (words.c's list_say) | 2,457 | 2,416 |
| dq0 overlay 19 (task.c's turned_says) | 769 | 877 |
| dq0 overlay 24 (refuse.c's tt_words) | 264 | 231 |
| dq0 overlay 27 (said.c; the packer's: a part image out of it) | 89 | 2,062 |
| dq0 overlays 28, 29, 30 (the rows) | 3, 618, 4,847 | 1, 6, 3,223 |
| dq0 overlays 4, 15, 17, 20, parts 21, 23 (the packer's moves) | 4,086, 3,650, 2,540, 346, 4,053, 3,560 | 755, 4,097, 176, 3,039, 3,606, 4,007 |
| dq0w's main RAM | 2,511 | 2,511 |
| dq0w part 11 (words), its image overlay 5 | 101, 142 | 249, 290 |
| dq0w part 53 (I_TURN), its image overlay 3 | 2,829, 29 | 2,815, 15 |
| dq0w parts 103, 104 (the well's), part 104's image overlay 32 | 920, 2,296, 1,393 | 1,013, 2,501, 1,691 |
| dq0d, dq0p, dq0s, dq0m, dq0harn | | untouched |

**The start** (dq0starttime; budgets 245M and 325M): seed 1 233.9M at the base (8d476fbd, built here), 232.7M, 234.7M,
233.8M, 233.1M and 233.6M after the five stages; seed 123456789 307.9M, 309.1M, 308.4M, 308.9M, 309.6M and 309.6M.

**Tested:** all 452 tests pass after each stage.  dq0traces' step (budget 1,920,000): 1,868,123 at the base,
1,867,570, 1,860,371, 1,913,687, 1,844,902, 1,871,885.  dq0answer's normal-pace listen (budget 760,000): 740,576 at the
base, 736,302, 641,930, 628,498, 688,475, 735,609 (the turns pace's 539,160 at the base, 556,259 after); a look's first
letter and its whole within theirs (650,000, 1,790,000).

**Open, for the user.**
- The world's lists whole (their frames and joins rows, a list coded as one), a thing looked at's sentences, and the
  "Which one?" words by LI.item: the rest of the look's lists.
- dq0traces' step and dq0answer's normal-pace listen are near their budgets, and each moves tens of thousands of
  cycles with the code's layout, not its work: the step 6,313 under its budget after stage 3 and 1,922,509 in a build of
  stage 5 with the world's BSS six bytes shorter; the listen 761,724 in the well's stage built alone, 735,609 after
  stage 5.  The renderer reads a text's letters a byte at a time through said_b (some 40 cycles each, with run's loop
  round it): a run of plain letters copied in one call (an assembly loop in the main RAM, some 40 bytes) would make
  every row the quicker and give both some margin back.  Not built: the UI's main RAM (3,106) is its tightest room, and
  the user's to weigh.
- The inventory's answer the slower by its things' rows (above): a cost of one renderer.

<!-- DECISIONS.md:6484-6624 at d35194df -->
## Compartments: the second stage (the user's, 2026-11-16; as built, for review: COMMANDS-CRITIQUE.md's §4.9, step 6b; ENGINE-CRITIQUE.md's row 4)

The user's answers of 2026-11-16 ("Compartments and narration: eight answers", above): every reader of a thing's place
by its compartment, a lid's top by its chest's size, things on a lid sliding off as it's opened, a compartment named by
its own name, the backpack's side pocket, locks on compartments past the first, hidden compartments, furniture placed in
the dungeon.  Built in three commits (the readers; the lid, the four compartment features and the furniture's kinds;
the furniture placed, after narr6's merge), the suite after each.  What's built:

- **Every reader of a thing's place knows its compartment** (the first commit):
  - *an ally's errand* (things.c's `fetch_find`): up through each compartment the thing's in that's open (`in_shut`),
    so a thing on a chest's lid is fetched (it was never found: the chooser's rule below);
  - *the chooser* (part 4's `pick_`): a thing in another lies there to be seen and taken if its compartment's prep is
    `CP_ON` (`lying`'s rule; a thing in an old record's mark-less y, by its holder's `KF_ON` as ever): "take the
    chisel" of one on a chest's lid takes it (it was "You see no chisel here."), a desk's drawer's things not seen;
  - *"the empty chest"*, *the letters' empty* (`unlike`, `ix_make`'s `CX_EMPTY`): nothing in its first compartment (a
    chest's inside), what's on its lid not counted; "the open chest" was the first compartment's already;
  - *where to stow* (part 74's `stow_where`): each candidate's compartment a put by its id goes in (`cfor`'s: its first
    that's in it), its closed bit and fit by that one (every kind but the new ones the same: its first);
  - *the nested lists* (part 11's `contents`, the inventory and a list in a sentence): a holder of several
    compartments' things each open compartment's own, "(in it: a sack) (on it: a chisel)", by its prep; a closed
    chest's lid's things listed (the inventory under it), not its inside's.  The compartment rides in `contents`'
    depth's high bits (part 11's room: 15 bytes left), each holder's open compartments the catalogue's bank's
    (`o_comp_open`: `cfilt`, `cprep`); the main RAM's `comp_open`, `ic_of`.  narr6's list renderer is the wording's
    owner: here only the "on it"/"in it" choice and which compartment's things;
  - *the senses* (dq0p's `things_seen`): a surface's things by their compartment's prep (a desk's drawer's not seen).
- **A lid's top holds by its chest's size**: items.h's `LID_BULK (b)` a quarter of its holder's pints, `LID_WEIGHT (b)`
  ten quarter pounds a pint of that; dq0w/make.js holds each `CN_LID` row to its kind's bulk.  A chest's 20 pints, 200
  quarter pounds (as it was: no game changes); a case's 5 and 50.
- **Things on a lid slide off as it's opened** (or forced open; part 61's `s_slide`, called from part 36's
  `shut_work`, its room): onto the floor where the chest stands (a carried one's, at the character's feet), told
  after what opening shows: "You open the chest: it's empty. From the lid a chisel slides onto the floor.", "...
  From the lid a dagger and a chisel slide onto the floor." (the list the UI's list renderer's, narr6's LI rows, so
  no capital at its head: the sentence begun before it).  The look told it's changed (serve's `slid`).  "Open the lid" opens
  the chest.
- **A compartment named by its own name**: the catalogue's `cnouns` ("lid", "drawer", "side pocket", "false bottom",
  "secret niche"), each row's `noun` (`CN_*`, its high bits `CC_*`); the one name match, `named`, against them in the
  chooser (part 4's `pick_`) when a verb takes a compartment (`wcn`: what closes worked, a look in, a put's and a
  take's other, the UI's other to hand, `IO_FIND`), the compartment named in `wcomp` (`o_comp_named`, the catalogue's
  bank's).  A take from it looks in that one alone (`wcin`); a put goes in it; the words name it ("You put the chisel in
  the drawer.", "In the drawer lies a chisel.", "The side pocket is closed.", "You untie the side pocket, revealing a
  chisel.").  "take the drawer", "drop the pocket": no thing ("You see no pocket here.").
- **The backpack's side pocket** (its second row: in, tied, 4 pints, 20 quarter pounds): put in, taken from, looked
  in, tied and untied by its name; reached as its pack is (off the back for it); a kit's open, one found tied.  A
  stow, "put the coin in the backpack", go in its inside as ever.
- **Locks on compartments past the first** (`CC_LOCK`: a desk's drawer): opened, closed, locked and unlocked with its
  holder's key (`key_fits` by its holder's id), picked, forced, by `shut_work`'s one path: a compartment's state made a
  first compartment's shut byte for it and written back (its closed bit in `shut`, `IS_SHUT_C`; locked and burst in its
  holder's `fuel`, items.h's `CS_*`: a holder's fuel a light's and a fuel's alone else).  Found on the floor, a drawer
  is closed and locked one time in three (by its number, no dice, as a chest's lid); the kit's key after a desk its
  drawer's (`kit_lock`).
- **Hidden compartments** (`CC_HIDDEN`: a case's false bottom, a desk's secret niche): there one time in two (by the
  holder's number, as it's made its own: `CS_NONE`), unseen and unnamed till found (`CS_UNSEEN`: not listed, not
  examined, not named); a search of its holder finds it by the referee's search test (`P_SEARCH`, mechanical, its
  steps the worse a third of its holder's quality, two more by touch: part 47's `found_c`, as part 46's `turned_up`):
  "You find a secret niche in the desk.", known from then (`CS_FOUND`).  One reached through its holder's inside
  (`CC_INNER`: the false bottom) is closed as its holder's closed, opened only with it open ("The case is closed.").
- **The furniture's kinds** (the catalogue's): a desk (its top, a drawer that locks, a secret niche), a cabinet (its
  inside behind its doors, its top, a drawer), a case (its inside, lid and lock, its lid's top, a false bottom), shelves
  (fixed, a surface).  KI_DESK ... KI_SHELVES after the towel.
- **What the player sees differently** (no existing game's sentence changes but where a thing's on a chest's lid, the
  furniture's own, and the furniture's presence: the last commit's, below):
  - a thing on a chest's lid: taken by its name alone ("take the chisel": was "You see no chisel here."); a closed
    carried chest's lid's things in the inventory; an ally fetches it; "the empty chest" a chest with things on its lid
    alone ("look in the empty chest": "The chest is empty.");
  - opening a chest with things on its lid: "You open the chest, revealing a sack. From the lid a chisel slides onto the
    floor." (was "You open the chest, revealing a sack.", the chisel on the lid still); dq0compart's words changed so
    (its look at, look on and search after: "In it lies a sack.", "There's nothing on the chest.", "In the chest you
    find a sack.");
  - "open the lid" opens the chest (was the door's work: "It has no shutters to open or close."); "put the chisel on the
    lid": "You put the chisel on the lid."; "look on the lid": "On the chest lies a chisel.";
  - the side pocket: "You stow the chisel in your side pocket.", "In the side pocket lies a chisel.", "You tie the side
    pocket shut.", "The side pocket is closed.", "You untie the side pocket, revealing a chisel.", "In its side pocket
    lies a coin." (a backpack examined, its side pocket holding something);
  - the furniture's: "The drawer is locked.", "You unlock the drawer with its key.", "You open the drawer: it's
    empty.", "You put the chisel in the drawer.", "In the drawer lies a chisel.", "You close the drawer.", "You lock the
    drawer with its key.", "You find a secret niche in the desk.", "In the secret niche you find a dagger.", "You find a
    false bottom in the case.", "The case is closed." (the false bottom, its case shut), "The false bottom is closed.",
    "There's no room there for the rope." (a case's lid's top, 5 pints).
- **Furniture placed in the dungeon** (the last commit, after narr6's merge): a row of the maker's placers (dq0m/place.c's
  `placers[]`, its stock stage, `o_put_furn`) with a purpose of its own, `DP_FURNITURE` (a seeded one, past
  `DP_SEEDED`: its key the chunk's place, the room and the stratum's seed, no other placer's rolls moved by it): each
  room whose use has furniture gets, one time in two, one of its use's two kinds (`furn[]`: quarters a desk or a case,
  the solar a desk or a cabinet, a vault a case, a chapel a case one time in four, a store, a kitchen and an armoury
  shelves or a cabinet, a pantry shelves, a hall a cabinet or shelves, a workshop shelves or a desk; a cell, a crypt, a
  gallery, a cave, a barracks, a well, a cistern, a gatehouse, a guardroom none), on a square of its floor beside a
  wall (the sconces' way), listed with the stock if the list keeps twelve entries for what comes after it (the theme's
  pieces, the portals' parts, the sconces).  Nothing is put in its compartments (an open question).  The environment's
  `dq0furn=0` places none (a test's scene, as `dq0sky`'s: read once by the maker).
- **The tests touched by the furniture** (each by its scene, no check loosened): dq0rope (seed 5's storeroom: a
  cabinet beside the character, the rope's tie took it as its anchor, not the spike: `dq0furn=0` for its scene),
  dq0blind (seed 2's walk in the dark found a set of shelves underfoot on its way and told its way twice:
  `dq0furn=0`), dq0looks (its looks' hashes against looks.ref, the dungeons as they were: `dq0furn=0`), dq0answer (its
  normal-pace look ahead past its budget with seed 3's furniture: `dq0furn=0`, its dungeon as it was), dq0traces (its
  step's cost 1,929,179 cycles with its dungeon's furniture, the store the fuller: `dq0furn=0`, 1,906,100).  New: dq0furniture
  (seed 14's bedchamber: a case seen as the game begins; the same game with `dq0furn=0`, none).
- **What changes with the furniture's presence**: every seed's dungeon has furniture in some rooms: the looks name it
  ("On the floor ahead to your right lie a well and some shelves."), the things' capitals after it move on a letter, a
  search of a room goes over it, a rope's tie can take a heavy piece as its anchor, the things made their own after it
  numbered on (a thing's make and wear by its number's dice as ever, so a few things' words shift: "battered", a lock).
  The transcripts by sentence against the merged base (narr6's merged in, the furniture not yet placed): 98 of the dq0
  tests' differ, every one where furniture's in sight or its capitals and numbers have moved on (and dq0compart's,
  dq0lidslide's and dq0compcase's "From the lid ...", below), but the known timing noise (dq0answer, dq0antidote,
  dq0jobs, dq0lookfar); no sentence of the game's own wording changed.
- **After narr6's merge** (the list renderer: a thing's words coded, SAY_LI, worded by said.txt's LI.item): the slide's
  sentence was "A chisel slides off the lid onto the floor.", its first letter made a capital in the text; the list's
  now the UI's to word, so the sentence leads it: "From the lid a chisel slides onto the floor."  contents()' "(on it:" and
  "(in it:" choice here as built, narr6's renderer the wording's.
- **Room** (the world's main RAM, its floor 2,048): 2,511 bytes free at the start, 2,463 after the readers, 2,313 after
  the second commit (comp_open, ic_of, comp_named, name_comp, wcn, wcomp, wcin, cfilt, cprep, slid; the parts' stubs),
  the same after the furniture.  The catalogue's bank's code 5,546 of 6,400 (its own count; 5,610 by the map) to 6,158
  (o_comp_of's company: o_comp_open, o_comp_named, other's and stow's compartments, IO_FIND's).  The world's parts: 4 407
  to 158, 11 101 to 9, 13 469 to 420, 35 0, 36 1,176 to 578, 47 2,467 to 2,007, 61 1,929 to 461 (s_slide, s_own_comps,
  kit_lock's), 74 2,149 to 2,081.  The senses' main RAM 4,431 to 4,486 (things_seen's one loop).  The UI's 3,130 unchanged
  by these (3,106 after narr6's).  The maker's main RAM 3,932 to 3,908 (dq0furn's read, stock's sx0, sy0 shared),
  overlay 9 554 to 648 and 5 915 to 25 (the packer's).
- **Timings:** dq0traces' step 1,868,123 cycles at the start, 1,884,499, 1,845,061, then 1,905,482 after narr6's merge and
  1,906,100 with the furniture (its scene's off; 1,929,179 on); dq0answer's listen 539,160, 543,407, 557,482, 556,582,
  its normal-pace listen 740,576 at the start, 739,107 after the second commit (a denizens' pass: 771,772 before part
  4's naming went to the main RAM, name_comp), 628,264 at the end; the start, seed 1, 233.9M, 233.7M, 234.6M, 234.7M;
  seed 123456789 307.9M, 309.3M, 310.5M, 311.0M.
- **The suite:** all pass after each commit (454, 458, 460 tests with the merge's and dq0furniture).
- **Saves:** no record's layout changes.  A holder's compartments' locks and finds in its record's `fuel` (unused by any
  holder before); an old save's holders read every lock open and every hidden compartment there and unfound.
- **Open questions (the user's):**
  - A thing put on a chest's lid while it's open (built: it lies there till the lid's closed and opened again); refused,
    or slid off at once?
  - Other lids' tops: a pot's lid, a backpack's flap (built: the chest's and the case's alone have a top).
  - "The side pocket" typed whole: the parser takes "side" as its own word (a way: "look in the side pocket" is a look
    that way, and the game ended there in a probe, the UI's way path, worth a look of its own; "close the side pocket"
    goes to the door's work); "the pocket" names it.  The kinds' bank has no room to make "side" (or "bottom") a name of
    the world's words (10 bytes over); "put the coin in the side pocket" is understood.  The desk's hidden compartment
    is "the secret niche" for the same reason ("secret drawer" read as "the drawer"; and a name past 15 letters overruns
    the world's names, xn and yn).
  - "case" names a bolt case too (the chooser takes the one most to hand).
  - Where to stow offering a side pocket as its own choice (built: a holder's first compartment that's in it, as a put
    by its id goes).
  - The maker stocking furniture's compartments (a drawer's papers, a case's coins): nothing's put in them yet.
  - The denizens' reach and gear by compartment (a robber's, a denizen's own): their code unchanged.
  - A compartment looked at ("look at the drawer"): its holder's look, as ever.
  - A hidden compartment's difficulty (built: its holder's quality's third, the steps the worse, two more by touch).

<!-- DECISIONS.md:6625-6700 at d35194df -->
## A look in from an overlay, the side pocket named whole; the timing scenes with their furniture (2026-11-16; as built, for review: ENGINE-CRITIQUE.md's rows 1 and 4)

Two things the second stage of compartments left ("Compartments: the second stage", its open question on "the side
pocket" typed whole; and the timing scenes it gave `dq0furn=0`).  Two commits.

- **The game ended on "look in the side pocket"** (and on "look in the far side": not the compartments' own).  The
  parser takes "side" for a side of the room (`OB_SIDE`, commands.words), its word the rest ("pocket", "far"); a look
  *in* with a kind goes from the things' timed works (hthings.c's `H_LOOK_IN`, `o_things_work`: overlay 1, since §5.2's
  step L, stage 3) to `look_way`, part 17.  There a word that names nothing it sees (`looked`: the world's `IO_LOOK`)
  leaves the hub's bank in the window (`things ()`'s `hub_call`), and `look_way`'s `rts` came back to overlay 1's
  code at $8000 in the hub's bank: it ran the hub's bytes till the task ended (the trace: `o_things_work`'s return,
  then the swap area's labels in a row, then the C library's exit).  And `looked` writes the hub's header
  (`HUB->ireq.to`) with overlay 1's bank in the window, into overlay 1, the world reading a stale `to`: a look that
  didn't end the game told the world's raw text ("<torch6 food5 ...>").  The same at the merged base before
  compartments (`d2945872^`: the call unchanged since `3eade6c6`), by the code; not run there.
  - *Built*: part 17's `s_look_ov` (way.c), `look_way` asked from an overlay as `othings` asks the things: the window
    kept (`hub_here`), the hub's header put in, `look_way`, the window put back (`hub_back`).  hthings.c's `H_LOOK_IN`
    calls it.  A stub (6 bytes of the main RAM).  No other overlay calls a part's function that leaves the window
    elsewhere (`o_things_work`'s other calls: `othings`, the overlays' stubs, `intend2`, each putting it back).
  - *"The side pocket" typed whole* (the open question): the resolver (resolve.c, overlay 11), as a command's
    resolved, takes `OB_SIDE` with the word "pocket", the thing's or the other's ("take the coin from the side
    pocket": the parser puts the kind on the thing), as the thing "side pocket", the kind gone: a compartment's noun
    before a way, for every verb ("look in", "close", "open", "take from", "go to the side pocket").  No backpack:
    "You see no side pocket here." as for any thing.  "The side" alone and "the far side" are the room's, as ever.
  - The words: "look in the side pocket" "You look for the side pocket. In the side pocket lies a chisel." (was the
    game's end); "close the side pocket" "You tie the side pocket shut." (was "It has no shutters to open or close.",
    the door's work); "open", "take ... from" the same; "look in the far side" a look about (was the game's end);
    "go to the side pocket" "You haven't seen a side pocket here." (was a walk to the far side).
  - *Test*: dq0sidepocket (gear.js): at seed 3 the side pocket typed whole put into, looked in, tied, looked in
    closed, untied, the chisel taken out, "look in the far side" a look about and the game on to its quit.
- **The timing scenes with their furniture.**  `dq0furn=0` had been set in dq0answer's scene (tests/lib/dq0.js, its
  normal-pace look ahead past its 450,000) and dq0traces' (its step 1,929,179 cycles, budget 1,920,000).  Measured,
  the fuller store isn't the cost: the step's window profiled by task and function (a probe on the CPU's steps), with
  and without the furniture, every task's item work the same (the senses' `items` scan, the world's `item_at`); what
  grew was the polling tasks' share as the step took a little longer (the world's main loop: `stocks`, its syscalls;
  the idle task), the scene's timing moved (its rats elsewhere, other text printed as the answer begins: dq0answer's
  listen 628,264 to 848,289, close the door 710,191 to 722,655 by the console's and the denizens' work in the window).
  So the cure is the work every step and every answer pays, wherever it was:
  - *`__osmaperrno` (sdk/c/lib/oserror.s)*: every failed system call (a poll's `E_AGAIN`: `hy_sem_try`, a key not
    there) mapped its code by the map from its end, some 400 cycles; from its start, the poll's code fourth.  A
    twentieth of the world's cycles as a step goes (102,000 of 2.2M over dq0traces' five squares), the UI's 80,000.
  - *`hy_call` (sdk/c/lib/hycall.s)*: r0-r3 copied in and out a byte at a time unrolled, not a loop: some 130 cycles
    a call less (the world's 1,400 calls over the five squares, 295,000 cycles).  54 bytes more in each program.
  - *The world's `stocks` (dq0w/store.c)*, every pass of its main loop: the slots' bytes by their offset from a fixed
    address, not a pointer stepped (some 1,000 cycles a pass to some 450).
  - *"Listen" alone* (pace.c's `o_pacing`): no longer one of the pace's lines (`s_pace_cmd` acts on "listen" only
    with a move's word or "listening"); the pace's part and the commands' were swapped for it each time.
  - *A look one way with nothing to find* (way.c's `look_way`): `feature (0, ...)` is called only if it does
    something (a thing to take in hand first, a way to turn to); else its 1 at once, part 13 not swapped in (a
    listen, a look ahead).
  - Tried and not kept: a swapped part's caller copied back only as far as the parts loaded over it reached
    (swap.inc's `sw_back`, 'SWKEEP'): 155 bytes of each task's main RAM (the UI's under its 3,000) for some 30,000
    cycles of the world's over the five squares, none of the UI's in them (its swaps there are fresh calls).
  - *The numbers* (with the furniture): dq0traces' step 1,929,179 to 1,805,880 (budget 1,920,000; 1,906,100 was
    without it); dq0answer at waits 0, 1 and 2 all within their budgets, e.g. normal listen 848,289 to 324,090,
    270,650, 279,774 (budget 760,000), normal close the door 722,655 to 432,319, 408,481, 414,223 (720,000), normal
    look ahead 381,452 to 328,414, 311,413, 311,773 (450,000).
  - `dq0furn=0` taken out of dq0answer's and dq0answer96's scene (tests/lib/dq0.js) and dq0traces'.  Kept where the
    scene is about something the furniture changes, not its timing: dq0rope (a cabinet beside the character took the
    rope's tie from the spike), dq0blind (shelves underfoot in the dark changed its blind walk), dq0looks (its looks'
    hashes against looks.ref, the dungeons as they were), dq0furniture (its own second game, none placed).
- **The words**: every dq0 test's sentences the same but the side pocket's and the far side's above, dq0sidepocket's
  own, the timing scenes' (dq0answer's, dq0answer96's, dq0traces': their furniture in sight, the denizens met at other
  moments) and the known timing noise (dq0jobs, dq0lookfar, dq0passes, dq0restbusy, dq0sway: denizens met at other
  moments; dq0tiring's accepted race; dq0peoplegen's last prompt not yet printed as its run ended).
- **Room**: the UI's main RAM 3,106 to 3,043 (`hy_call` 54, the stub 6, `__osmaperrno` 2); dq0w 2,313 to 2,285 (`stocks` 26 bytes
  less);
  dq0d 3,044 to 2,988; dq0p 4,486 to 4,430; dq0m 3,908 to 3,852; dq0harn 2,123 to 2,067; the other C programs
  that call `hy_call` 56 bytes less (dq0acct 2,297 to 2,241, hmchar 559 to 503: under its 2,048 as before).  Overlays: 1 2,145 unchanged, 11 (the resolver) 274 to
  140, 16 (the pace's look) 2,416 to 2,334, 24 (part 17's image) 231 to 173; part 17 1,258 to 1,200.
- **Start**: seed 1 234.7M to 235.2M, seed 123456789 311.0M to 310.5M.
- **Suite**: all 460 pass (with dq0sidepocket) on the two commits' build; the first commit's alone not run whole.
- **Open questions**: none of the user's.  "Side" in another compartment's name (none now but the pocket's) would want
  the resolver's rule from the catalogue's nouns, not "pocket" alone (the kinds' bank has no room for the world's
  words to name it).
