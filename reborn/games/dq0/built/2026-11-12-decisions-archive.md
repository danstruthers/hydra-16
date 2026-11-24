# DQ0: the decisions archive, 2026-11-12

The agents' accounts of what they built, moved out of DECISIONS.md on 2026-11-24 (branch docs2) with their text
unchanged, as AGENTS.md's Records rule asks: DECISIONS.md holds the user's words only. Each section left a one-line
pointer there; where a section held the user's words too, those stay in DECISIONS.md, quoted, and the whole section
is here. Each section is marked with its lines in DECISIONS.md as at d35194df.

<!-- DECISIONS.md:4191-4286 at d35194df -->
## The life's record and the clues declared, the generators driven from one place, one rumours' reader (2026-11-12, as built, for review: ENGINE-CRITIQUE.md's §9 and step 11's "Left")

What step 11 left: life.dat into records.def, the other files the meta programs share where it's clean, and the
generators into tools/dq0build.js; and the rumours' reader, written twice (dq0acct's quest.c, dq0town).  Every
program's words and every save-folder file the same; the images' bytes as below.

**1. The records.**  records.def (games/dq0/records.def, read by hubgen.js into obj/gen/dq0rec.h and dq0hub.json's
records) now declares, beside struct between:

- **struct life** (life.dat): `struct life_head` (the 32-byte head: magic, version, flags, seed, dseed, dungeon,
  date, born, reputation, clues, thread, fast, room[5]) and `struct life_dungeon` (seed, days, outcome, flags) as
  structs of no file of their own, and `record life "life.dat" 0x464C 1` their record (h, d[LIFE_DUNGEONS]); with
  LIFE_DUNGEONS, the head's flags LF_BETWEEN, LF_OVER, LF_ABROAD, LF_HARD and the dungeon's LD_DIED.  lives.dat (the
  lives over, this record one after another) noted with it.
- **struct clue** (clues.dat): `record clue "clues.dat"`, a file of records one after another with no head; with
  CLUE_SIZE, CLUE_WORDS (its words' 342 bytes: was `CLUE_SIZE - 106` in the struct), CL_MORE and the asked bits CA_*.
  CLUES_FILE is CLUE_FILE now, the record's name's (read.c, dq0life.c, dq0schol.c: three files, seven uses).

records.def's grammar grew three things for them: `struct NAME` (a struct with no file, a field's type for a record
declared after it), a field's type `struct NAME`, and `record NAME "FILE"` with no magic or version (a headless file
of records); a record's head may be its first field's struct (life's h).  hubgen's decoder decodes a struct field as its
own object (an array of them its objects), and ok is the head's magic and version (a headless record's always true).

life.h keeps what isn't a record's layout: the folder's files told, struct save and struct quest (game.dat's head,
the UI's), the outcomes' enum LO_* (game.dat's too), the thing's fuel for a clue (CLUE_HERE, CLUE_COPY), CLUE_NOCOPY,
the grades' enum CL_*, the clue's size check (clue_fits), LIVES_FILE, FAST_STEP and the calendar; it includes dq0rec.h.
No C program had a copy of struct life of its own: they all included life.h, so the hand-written copies were the tests'
and the soak's, reading life.dat by offset (`a[38]`, `readUInt16LE(12)`, `a[26]`, `0x464C`, `& 2`, `544`).  Now:

- tests/lib/dq0.js's `dq0Life (bytes)`: the decoder's record ({ h, d, ok }, null if the bytes are short of it), its
  `size ()` and `C (NAME)` (LF_*, LO_*): dq0carry, dq0account, dq0hardon, dq0lifeend, dq0fastkept, dq0nosave
  and dq0why (the reputation) read their fields by name.  Each check holds the same
  thing it did.
- sim/tools/soak.js's lifeRecord: the same decoder; its flags and outcomes by name (LF_OVER, LO_SOLVED, LO_NONE,
  LIFE_MAGIC).

Left as they are, not clean:

- **kit.dat, kit0.dat, townkit.dat** (dq0w/outfit.h's struct kitrec): the world's.  Its layout is struct item's own
  fields in their order (KIT_ITEM, KIT_STATE: memcpy'd onto a thing as it's made) and its ofit's `in` holds the worn
  places' bits (WP_*: the referee2 work's); declared in records.def it would be a copy of struct item's order, held
  to it by hand.  The tests read kit.dat by offset in one place (dq0account's sword, 23 bytes a record).
- **heard.txt, rumours.txt, quest.txt**: text, the theme format's (themegen's), not records.
- **game.dat's struct save**: the UI's save's head, its versions main.c's (SAVE_VER); not a file between the programs
  but the dungeon's.

**2. The generators driven from tools/dq0build.js.**  What was copied or open-coded across build.js and the make.js
files, now each one function there:

| | before | now |
|---|---|---|
| the headers' numbers (hubgen's load) | dq0, dq0d, dq0s, dq0w make.js and dq0w/outfits.js, each `require('../dq0/hubgen.js').load ()` | `hub ()` |
| hubgen's generate, then saidgen's | build.js's hubNumbers, the two calls | `generate ({ say, cc65 })` (build.js passes cc65 as the C target has it) |
| pack.js's and parts.js's driver | build.js's swappedParts (some 45 lines: compile, route, measure, plan, apply, make) | `swappedParts (dir, objdir, extra, tools)`, build.js's cc65, od65, compileC, csources and flags passed in |
| the overlays' packing (ovlpack.js) | dq0, dq0d, dq0w make.js, packOnly | `pack (objdir, name)` |
| themec in the emulator, its failures thrown | dq0/make.js's after and themegen/make.js, each its own compile and check | `themes (list, opt, size)` |

Each generator's own work stays in its file (hubgen.js, saidgen.js, parts.js, pack.js, ovlpack.js, tools/themec.js):
dq0build.js is where they're called from, not their code merged into one file.  Why not moved: saidgen.js is the
narration work's (narr4's, being edited); hubgen.js's load is the tests' and the sim's (dq0meaning, hubown,
commandwords, the suites: some twenty requires by path); pack.js has its own command line (node games/dq0/pack.js TASK);
and every one's comments name its file (parts.txt, bank.s, the cfgs).  A move is a rename the day those settle.

**3. One rumours' reader.**  meta.c's META_HEARD: `heard (name)` (a rumours' file in the save's folder read into rbuf
after what's there, each block begun by a rumour line noted in rat[], rto[], RUMOURS at most) and `rumours_read ()`
(heard.txt's, then rumours.txt's, afresh).  The two copies' differences are parameters: META_RB_N (dq0town's RB_N, its
words' bank; sizeof rbuf the default), META_MADE (dq0town counts rumours.txt's, nmade: themegen -r -a goes on from
them), META_TEXT_ON/OFF round rbuf's filling (dq0town's WORDS () and KINDS (), as it had).  dq0acct's quest.c joined
its path itself into path (in_'s same bytes) and used its file's fd: the library's in_ and a local fd.

**Images:** dq0acct's (its main RAM's and overlay 4's code: quest.c's reader now the library's, in_
and a local fd) and dq0town's (the same length, 1,322 bytes moved: its reader's code where the library's section puts it,
rumours_read's clearing written apart) changed, and with them the SD card's image and the browser page's; every other
image byte for byte the same: the UI (dq0), dq0life, dq0schol, dq0end, dq0harn, the world, the denizens and the rest
(the records' header made from records.def lays out struct life and struct clue as life.h did), the paged ROM's chips
too.  So the start is as it was (dq0's image the same).

**Room** (bytes free, before → after): dq0acct's main RAM 2,157, 2,157; its overlay 4 250, 279; dq0town's main RAM 3,379, 3,379
and overlay 1 6,443, 6,443; the others the same, their images being so.

**Held to:** every dq0 test's transcript before and after (294), sentence by sentence (the scratch's sent.js,
a hook in sim/test.js not kept): none differs.  Byte for byte, 279 the same; 15 (the life's and the town's, whose card
changed: dq0account, dq0why, dq0life ...) differ only in the progress dots ("The dungeon is being made ready ...", "Nearly
ready") and the status line's clock as a long wait goes: the SD card's reading times, its image changed.  The save folder's files: the programs that write them, but dq0acct, are
byte for byte the same images, and dq0acct's change is in its reading of the rumours' files alone (no write); the
tests' xd of life.dat, kit.dat and the rest in those transcripts the same.  The suite: all 444 tests pass, before and
after.

**For review:**

- **CLUES_FILE renamed CLUE_FILE** (the record's name, as BETWEEN_FILE and LIFE_FILE are): the UI's read.c touched for
  it, two lines.
- **The outcomes' enum (LO_*) and the grades' (CL_*) stay in life.h**, as enums: records.def has no enums, and LO_* is
  game.dat's struct save's too.
- **The generators' files not moved** (above).

<!-- DECISIONS.md:4287-4357 at d35194df -->
## HarnMaster's remnants behind the referee (2026-11-12, as built, for review: ENGINE-CRITIQUE.md's §8 and step 9's "Left:")

The user's decision of 2026-11-11 (§8's option (a)) wants a ruleset to be its answers alone, behind the referee, and
no HarnMaster in the engine.  "The referee's skeleton" left four pieces outside; three are moved, the fourth is a
question.  dq0null, the second referee, is the check: it includes no HarnMaster header and now neither does the world.

- **1. The places on the body a thing's worn at are the engine's.**  items.h's `PB_*` (`PB_HEAD`, `PB_FACE`, `PB_NECK`,
  `PB_SHOULDER`, `PB_UPPERARM`, `PB_ELBOW`, `PB_WRIST`, `PB_HAND`, `PB_BREAST`, `PB_WAIST`, `PB_GROIN`, `PB_HIP`,
  `PB_THIGH`, `PB_KNEE`, `PB_CALF`, `PB_FOOT`, `NPB`) and `PB_SIDED` (a limb's, a left and a right) in place of harn.h's
  `HARN_SKULL ... HARN_FOOT`, `HARN_NLOCATIONS` and wworn.c's `sided`; named for what the game says of them (the
  words were already "head", "wrist", "breast", "waist").  **The numbers are the same**, so an item's `sn` (WP_AT | its
  place | WP_PAIR | WP_RIGHT) and kit.dat's worn entries (WP_KIT's) read as they did: no save file changes.
  kinds.c's `wplaces` and `wroom`, wworn.c and the UI's worn.c (its letters A ... P the places) name them; the world's
  kinds.c and wworn.c include no harn.h (the world now includes no HarnMaster header at all; nor does the UI, the
  denizens, the senses, the maker).  **The mapping to a ruleset's locations is the referee's:** dq0harn.c's
  `PB_HARN` (HarnMaster's strike locations in the same order: the head its skull, the wrist its forearm, the breast its
  thorax, the waist its abdomen; WP_RIGHT its HARN_RIGHT), checked at compile time.  No referee reads a place worn yet
  (the jewelry worn at places covers nothing; armour is by its kind, the referee's own tables); one that does maps it
  there.
- **2. The ruleset's words for its stunned state and its own stance.**  RS_HELLO's text, after its name and its
  character generator, gives three more lines (rs.h): the words for the state C_STUNNED is (HarnMaster's "in shock"),
  the word for its own stance, D_OWN ("counter": "" none), and that stance told ("You will strike back first.").  The
  UI keeps them (main.c's `rs_stun`, `rs_own`, `rs_owns`, the main RAM's 54 bytes) and uses them where it had its own:
  tend.c's refusals while stunned ("You're " and the words, ": you can do no more than rest, or walk slowly away."),
  said.txt's HARM.state (" You are {text}." and "still", the words its text: main.c's state_said), blows.c's stance
  (the word's first letter after dodge's and block's, as 'c' was; o_fight_note's D_OWN sentence the ruleset's), and
  the help (help.txt's "@own " the word and a comma, help.c's).  dq0harn says "in shock", "counter", "You will strike
  back first." (part 4's s_hello_words, after part 1's name and generator); dq0null "stunned" (never set) and no stance of its own.  **dq0null's one change:**
  "defend counter" under it was D_OWN, which it took as D_AUTO, told "You will strike back first."; it's now D_AUTO
  told as such ("You will defend yourself as best you can."), and its help lists no counter.  No test runs it.
- **3. The character's outfits are the ruleset's.**  outfits.txt was HarnMaster's through and through where it
  outfits the character: hmchar's occupations, harn.h's skill names, the book's cultures and classes as tags.  Split:
  games/dq0harn/outfits.txt (the universal kit, the skills' weapons and tools, the bents, the occupations, the
  soldiers, the clerics: the character's, by its tags) and games/dq0/outfits.txt (the denizens' roles, lairs, purses,
  standings, and the format).  **The tags are the ruleset's file's own `tags` line** (its words in RS_OUTFIT's bits'
  order, dq0harn.h's OT_*), not outfits.js's `TAGS` list: the build names no HarnMaster word.  make.js reads each
  referee's outfits.txt in rsmeta.h's RULESETS order (those that have one: dq0null has none, its RS_OUTFIT names no
  occupation and no tags), then the game's, into the one lib/dq0/dq0o.dat: **byte for byte the same file** (the same
  sections in the same order).
- **4. dq0acct: not moved (a question, below).**

**The words.**  Every dq0 test's screen saved before and after (sim/test.js's TRANSCRIPTS hook, not committed) and
compared sentence by sentence (sent.js): 7 of 294 differ, all timing noise: dq0antidote's real-time fight; dq0answer's, dq0jobs' and dq0prompt's denizens met at other moments (a rat 10 or 15 feet off, a squeak ahead or not, "Something stirs."); dq0sway's; dq0reachstop's accepted race (the stop landing a step later); dq0starttime's progress dot landing after "(B) a sconce." (the start's timing).  No sentence of the stunned state, the stance, the help or a place worn changed: "in shock" in the same 10 tests, "You will strike back first." in dq0parse, the places' words as they were.  The save files: nothing written changes (an item's sn and kit.dat's places the same numbers, dq0o.dat the same bytes, RS_HELLO's words not saved).

**Room** (main RAM free, before → after): the UI (dq0) 3,415 → 3,359 (its overlays 1 1,112 → 989, 8 236 → 93, 22
508 → 536, 28 0 → 7, 29 6,577 → 6,595: the packer's placing; parts 1 406 → 263, 11 2,346 → 2,336, 12 2,214 → 2,193);
dq0harn 2,209 → 2,189 (its part 4 4,509 → 4,456, put in bank 5 by the packer; part 1 as it was, 108: its bank, shared
with part 3, was full, so the words are part 4's, asked by rf_answer); dq0null 4,605 → 4,595; dq0w's main RAM as it
was (2,236; part 107 1,481 → 1,471, overlay 3 247 → 237).  **The start** at seed 1: 231.5M → 234.1M cycles (245M
the budget); at 123456789, 308.2M → 308.8M (325M).  **The suite:** 444 of 444, before and after.

**For review:**
- **dq0acct (the question).**  It's the ruleset's own program already, the table's (rsmeta.h's RULESETS: dq0harn's
  "/../acct/dq0acct"), run by dq0life for dq0harn's games alone; dq0null's games have no time between.  What in it is
  HarnMaster's: the healing told day by day (body.c: course.h, watch.h, the library's harnheal.c), the month's
  practice and the wages (skill.c), the sheet read and written (sheet.c, hfile.c: hmchar's format), the physician's
  words (town.c: harn.h's locations), the living by the sheet's social class and the age by its birthday (dq0acct.c).
  What isn't: what solving brings, the standing and the rumours (quest.c), the money's matters (living, selling,
  mending, restocking), the town (dq0town).  Putting more of it "behind the table" means splitting it into a generic
  account every ruleset's games run and the ruleset's pieces (its healing, its practice, its sheet) through rsmeta.h's
  functions: a large change to a program whose words and saves must stay the same, and one that only pays when a
  second ruleset wants a time between dungeons.  **For the user: split dq0acct now (the generic account, rsmeta.h's
  hooks for the healing, the practice and the sheet), or leave it the ruleset's whole program until a second ruleset
  wants the time between?**  Not built.
- One outfits file for the game: a second ruleset with its own `[universal]` and its own tags would clash with
  dq0harn's in the one dq0o.dat (a section twice, the tags twice: the build fails, loudly).  Then the file wants to be
  each referee's (lib/dq0/REFEREE's dq0o.dat, the world opening the save's referee's).
- The bestiary (the creatures' sheets, RULESETS.md) and dq0schol's d100 test are as they were.
- A ruleset's own stance word is matched by its first letter after dodge's d and block's b/p (and before none's n), as
  'c' was: a ruleset's word beginning with d, b or p would never be reached.

<!-- DECISIONS.md:4358-4453 at d35194df -->
## Narration as data, stage 4 (2026-11-12, as built, for review: ENGINE-CRITIQUE.md's §3.2 and step 5, its fourth stage)

Stage 3 left fight_words, sway_words, sky_words and throw_words, the UI's refusals and a command's first words.  This
stage gives the UI kinds of its own, puts those words on the phrase table, and deletes the four functions, and with them
grip_words, time.c's sky[] and refuse.c's said[].  The words the player sees are the same, letter for letter.

- **The UI's own kinds (dq0.h's `UK_*`, declared in msgs.def).**  This is the critique's "kind of the UI's": an outcome
  event's shape (`< E_OUTCOME`, the outcome in w1's high byte) for words that no world's event carries: a reply, a
  refusal, a command's first words.  They are numbered from 0x80, past the hub's events.  They are never sent: the UI
  fills one on the C stack and gives it to outcome_words, so the tests, the denizens and the sound task never see them.
  Each is declared in msgs.def like any kind, its fields named so that said.txt reads them by name, and in dq0.h's
  enum (hubgen.js reads dq0.h).  There are six:
  - `UK_THROW` (THROW rows: ir, how, hard): a throw told.  No mark seen and none carried stay the refusals'.
  - `UK_SKY` (SKY rows: phase, shows): the sky seen, with or without what shows it.  There is one row a phase, and
    `{shows?...}` chooses between the sky's own words and a window's, a skylight's or a shaft's.  Also a shaft's
    refusals and a rope's (own outcomes tore and far).
  - `UK_SWAY` (SWAY rows: res, how, now, order; arg, how the attitude moved): every answer of sway_words'.  The packer
    (sway.c's sway_said) keeps what is the sway's own: the one it's about, as it's said ("the giant rat", "Grub"), and
    how the attitude moved (won over, warmed, swayed, not swayed, badly, a gift spurned).  A row can't work that out,
    since it compares two fields.
  - `UK_FIGHT` (FIGHT rows: out, the record's which, the flags it left them in, the weapon's word, the strikes' lines
    told apart, who isn't seen; arg, the attacker's body): every blow, stray, rise, grapple and dispatch that
    fight_words and grip_words said.  blows.c's fight_said keeps the rest: reading the record, working out who's seen
    ("?": something), cutting the four lines apart, and setting the black-out's once-an-answer flag.  FO_GONE, FO_BLED
    and FO_BOUND are 0xFF, 0xFE and 0xFD, and 0xFE is the table's "any".  So they, and a stray, are own outcomes that
    the packer maps to.  Getting up is one piece, `@rises`, shared by BEAT.risen and FIGHT.rise ("The cave adder coils
    itself up again.").
  - `UK_NO` (NO rows, named by the U_* and A_* enum's own names, as in `NO.A_SEE_NO`): refuse.c's one table of
    refusals.  say_no still says them (it adds the article, the capital and the nameless form), but their words are
    the rows.  `A_LB` (into lb, not said) is now said_q's, with the row's leading new line dropped.
  - `UK_FIRST` (FIRST rows: form FS_*, the verb FV_* as arg): a command's first words (D17), such as "You reach for the
    spent torch.", "You head for the door." and "You look them over."  sway.c's first_word, walk_says and walk_ahead
    still choose (which verb, which thing, its word), but their sayf lines are gone.  **For review:** this is the
    critique's "first words from a *begun* row", as a row keyed by the verb.  It is not yet the world's own begun row
    for the action.  That would need each action's begun row declared once, with the UI rendering it from the
    command's handles; when it comes, the rows are there to share by piece.
- **The table's field index is wider.**  The rows used 31 of 32 fields, and FIGHT alone needs some twenty more (each
  flag is a field).  A field past the 31st is written as its code + 31 followed by a byte (SC_NUM, SC_CHOOSE,
  SC_CHANGED).  A row's condition byte gives 7 bits to the field, so 126 fields at most.  The rows already built are
  the same bytes.  There are 49 fields now.
- **Three more of the UI's words** (said_s[3-5], `{w4}` `{w5}` `{w6}`: SC_STR2 and a byte).  A blow names two actors,
  a weapon and three lines.  This costs 6 bytes of the main RAM.  As before, a word must be in the main RAM (lb, cmd
  or a main RAM buffer).  The renderer runs in overlay 27, so the packers first copy a bank's strings (such as "it", a
  kind's name, the door's word) into lb.
- **What wasn't moved, and why:**
  - **The world's things' words (dq0w's SA_*, kinds.c's said[]).**  These are the world's, not the UI's.  The world
    builds the sentence with the thing's names and puts it in the UI bank's things_text, next to the inventory's words
    and a count's.  The UI reads that text in a dozen places: tt (), the list, the referee's slot, and a strstr for
    "already".  Moving them would change the protocol in three ways:
    - the world would write a code and the names, not the sentence;
    - the UI would expand the code wherever the text is read;
    - the renderer would write into a buffer that isn't lb.

    It would touch dq0w's things.c, things.h and kinds.c, which other work has open (the step planner, the referee's
    worn places).  **Open, for the user:** whether to make things_text coded (the next stage), or to keep the world's
    words the world's.
  - **The sound's mapping (dq0s's world_sound).**  It is not a clean column.  Only E_PORTAL's and E_TRAP's cases come
    from an outcome.  E_NOISE has no rows (it has no words).  dq0s's own listen loop chooses the denizens' and the
    fights' sounds by loudness, square and ranges of outcomes.  A column would hold half of the mapping and leave the
    rest in the code: two places instead of one.  dq0s would also need a generated table of its own, since it can't
    read the UI's banks.  Left as it is.
  - **The look's wording** (seen_words, den_words, stir_words, focus_words, heard_words, felt_words' two, pose_words,
    way_words, facing_words, the time told), fight_note's stances and body_words.  These describe what's perceived or
    a state, not an action's outcome.
- **Room** (the build's table):

  | | before | after |
  |---|---|---|
  | dq0's main RAM | 3,415 | 3,412 (said_s 6 bytes more; sky_words', fight_words' and grip_words' stubs gone) |
  | overlay 27 (the renderer, the words most said) | 695 | 99 (the codes past 31, SC_STR2, more words most said) |
  | overlay 28 (the rows' first bank) | 0 | 3 |
  | overlay 29 (their second bank) | 6,577 | 2,418 (the rows 13,963 bytes: 351 rows of 14 kinds, 59 pieces) |
  | overlay 1 (sky_words gone) | 1,112 | 2,295 |
  | overlay 6 (fight_words gone) | 67 | 2,294 |
  | overlay 10 (throw_words) | 2,761 | 3,065 |
  | overlay 15 (sway_words) | 308 | 736 |
  | overlay 17 (first_word, walk_says; the packer's) | 329 | 2,547 |
  | overlay 20 (walk_ahead) | 2,898 | 3,039 |
  | overlay 24 (refuse.c's table gone; the packer's) | 132 | 4,136 |
  | overlays 16, 19 (the packer placed parts 17 and 3 there) | 1,672, 1,295 | 161, 234 |
  | dq0w, dq0d | 2,236, 2,379 | the same (untouched) |

  The rows' banks have 2,421 bytes left.  When they fill, a third bank can be added the way overlay 29 was.
- **The start** (dq0starttime): seed 1 231.5M → 231.9M cycles (budget 245M), seed 123456789 308.2M → 308.4M.
- **Checked:** every dq0 test's screen was kept before and after (a temporary hook in sim/test.js, not committed), and
  their sentences compared.  8 of 294 tests differ, all of it noise:
  - the real-time fight in dq0antidote (its new sentences all use the rows' right words: "You strike at the cave
    adder with your torch, but stumble.");
  - denizens met at other moments (dq0answer, dq0jobs, dq0prompt, dq0skykin);
  - dq0reachstop's accepted race ("You reach for the tinderbox." before its stop);
  - dq0sway, a key typed ahead echoed a moment later;
  - dq0starttime, a progress dot after the first look's last sentence ("a sconce..").

  No moved sentence is said differently in any test.
- **Tested:** all 444 tests pass.

<!-- DECISIONS.md:4454-4545 at d35194df -->
## The plans' pages on the record store, the private hashes keyed; the styles not moved (2026-11-12, as built, for review: ENGINE-CRITIQUE.md's §5, §6, steps 7 and 8)

Three things were left by "One record store" and "The placers' table and the keyed dice": the maker's plans' pages
off the record store, the private hashes of caves.c, sky.c, plan.c, uses.c and make.c's mixed third, and the styles'
proportions in uses.c rather than the theme.  The first two are built; the third is not, and why is below.  Nothing
the player sees changes.

- **The plans' pages (changes.c's `plt`) on the record store.**  The pages of a plan too big for its bank (plan.c's,
  more regions than PLAN_MAX, with a save folder: plSS0000.dat, 42 regions a page, 32 pages in the maker's bank, 16
  with dq0 -C) had their own lookup and their own least-lately-wanted loop.  Now a page is found by `RP_FIND` (its key
  plt's first three bytes, the stratum and the page; its stamp the clock's next, `plstamp`) and one is made way for by
  `RP_OLDEST`.  The old loop's choice wasn't quite the others': it took each free entry as it met it and went on,
  comparing the entries after it with the free one's stale stamp, so it chose the last free entry, unless a used one
  after it was older than that stale stamp.  `RP_OLDEST`'s `RTF_FREE` takes the *first* free entry, so it would have
  moved the pages about in the bank (and, once the stamps wrap, which page is written back when).  A flag of the
  library's, `RTF_EACH` (rp.h, dq0lib.s), takes a free entry as it's met and holds those after it to its stamp: the
  old loop exactly, in the ROM.  o_pl_put's second search for the page is gone (pl_at's `plk`).  The file stays the
  plan's own, written in place and not versioned (the plan's grown whole each time its stratum is: there's no older
  version to keep).
- **The private hashes, keyed (place.c's `dhash (p, s, a, b, c)`, grow.h's DH_*).**  Thirteen decisions made by
  hashes of their own are purposes now, each row its old formula: a * m1 + b * m2 + c * m3, plus the stratum's seed's
  part (none, its low half, its halves xor'd, or one of its bytes) xor'd with a constant, the sum xor'd with another,
  then mixed (`h ^= h >> 7; h *= 0x2F1B`, its high byte) or left as the sum.

  | purpose | was | decides |
  |---|---|---|
  | DH_CAVERN | caves.c's `ch` (its seed `V.seed`, the halves xor'd ^ 0x3C5D) | a region's great cavern, its size, wander, pillars; a cave's sloping tunnel |
  | DH_AIR | caves.c's air pocket | firedamp, blackdamp and the room it's in |
  | DH_WET | caves.c's `wh` | a room's water's depth, the rubble, a stream's deep stretch |
  | DH_SKY | sky.c's `wh` (c the top stratum's seed's low byte plus k, a byte) | skylights and windows on the surface |
  | DH_PWATER | plan.c's `wh` | where the plan's streams rise |
  | DH_WELLS, DH_SLOPE | plan.c's water under a region, a stairway a long slope | |
  | DH_RING, DH_VAULT, DH_ROT, DH_ROTU | uses.c's ring, a great hall's vault, a room's leavings and an older layer's mark gone to dust | |
  | DH_MIXED | make.c's mixed third | a mixed stratum's chunk left natural |
  | DH_DIAMOND | caves.c's | a stratum's passages on the diagonal (its seed's top byte) |

  **Not a key of `dkey`'s:** `dkey`'s keys are a 32-bit xorshift seeded from the chunk's place; none of these
  formulas can be written as one without changing every number, so each keeps its own derivation as a row (the
  "own derivation" the earlier entry foresaw).  `dhash` is in the main RAM beside `dr`, since its callers are in six
  overlays and the main RAM.  The files keep their little wrappers (caves.c's `ch` and `wh`, sky.c's and plan.c's
  `wh`) where a parameter's type truncates its argument, so the truncation stays where it was.  sky.c's `wh` no longer
  writes `Y.h`, the shafts' stream's state, which it used to clobber; the shafts' function reseeds `Y.h` before
  every roll, so nothing read it.  **Left as they were:** the streams (sky.c's shafts, plan.c's pieces' and details',
  the growers' and the stock's: call order by design) and the hashes outside these five files (fixtures.c's,
  pass.c's).
- **The styles' proportions: not moved (open question).**  themec can carry them only by taking room from the
  theme's words.  struct theme is 1,739 bytes and the record is THEME_SIZE, 4,096: the UI copies it into its overlay
  5's bank at $9000 (4K exactly), and the theme bank's pieces' places sit at $9000 after it.  The words fill the rest
  (2,357 bytes).  The four styles are 4 × 52 = 208 bytes.  Carried in the head, they cut the words' room to 2,149,
  and themegen's linegen (bank/linegen) budgets each clue by `THEME_HEAD + words + its words + 64 > THEME_SIZE`.
  With the head 208 bytes longer, a generated theme within 208 bytes of the room loses a clue it had, and every
  random () after it shifts: another theme, another dungeon.  Keeping linegen's sum at the old head instead would
  leave such a theme too big to compile.  The stand-ins use 1,224 to 1,612 bytes; the generated themes weren't
  measured over enough seeds to say none comes that close.  Ways the user could choose:
  1. the cost taken (the head grows; the seeds whose themes come within 208 bytes of the room change);
  2. a theme's own styles only when its text gives them (a `style` line; a mission bit, TM_STYLES, bits 2-5 being
     free; the table at the record's end, the words' room cut only then).  The house's styles stay the maker's
     default and no generated theme changes;
  3. the styles outside the record (a styles file the build makes, the theme naming a set).
- **Checked.**
  - The plans' pages: every save-folder file's writes (fhash, as "One record store" did: each OPEN, CREATE, WRITE
    hashed, SEEK, by file, in order per task) over 48 games: 8 seeds (3, 5, 7, 42, 99, 1000, 31337, 70000), each with
    -C -T -T -T, -C -T, -T -T, -C, and -T -T -T -T with and without -C.  The last two are the vast stratum's: its plan
    paged, some 100 to 4,000 plan-page operations a game, -C's making way.  plSS0000.dat's writes are byte for byte
    the baseline's in all 16, and every file's in 46 games.  Two games (s5C3, s70000C1) were another of their
    timing's outcomes at the first power-up: the rats met at other moments, a walk in the dark stopped elsewhere.
    Neither game has a big plan.  This build at the second and third power-ups gave the baseline's bytes; the
    baseline at seven power-ups never gave this variant.  life.dat is left out (its date the host's clock).
  - The hashes: a scratch script ran each row against its old formula in 16-bit C arithmetic, 200,000 random inputs
    each; none differed.  Then the maker's every write to the hub's shared banks (each task's runs of bytes, hashed,
    a sorted list per game) over 80 games: 8 seeds (1, 3, 7, 42, 259, 1000, 70000, 123456789), each as it is, -T,
    -T -T -T, -M, -A, -D, -V, -M -T, -A -T -T and -V -T -T -T, a random player's 50 commands and two ways down.  The
    maker's writes were the same in all 80, and the sentences of all 80 the same.  The other tasks' writes move with
    the timing; the baseline is the same as itself at one power-up.
- **Room** (bytes free):

  | | before | after |
  |---|---|---|
  | dq0m main RAM | 4,635 | 4,193 (the pages' block 33, dhash and its rows 409) |
  | dq0m overlays 2, 3, 4, 5 | 947, 527, 51, 845 | 1,175, 596, 62, 915 |
  | dq0m overlays 6, 7, 8, 9 | 508, 165, 223, 477 | 572, 224, 296, 554 |
  | dq0lib (ROM) | 2,507 bytes | 2,529 bytes |

  No other task touched.
- **The start** (dq0starttime): seed 1 231.5M cycles before, 231.7M after; seed 123456789 308.2M before, 309.4M after (budgets 245M and 325M): dhash's generic rows a little slower than the inline formulas, at the maker's first chunks.
- **The suite:** 444 of 444 (440 run from a copy of the tree, whose four machine tests (hwtest, hwtool, fs, mml) want the repository's own folders and fail there for the baseline as well, run again in the worktree and passed).  Every dq0 test's sentences the same as the baseline's but the known timing noise: dq0antidote's real-time fight, dq0answer's and dq0jobs' rats heard at other moments, dq0robrest's rest ending at another moment (a skill improved, hungry and thirsty, not reached).
- **Open questions.**
  - The styles: the three choices above.
  - The chunks' cache's plain stamp comparison (RTF_PLAIN) made round: a behaviour change, not taken.
  - The old keys' omission of the stratum's seed (dkey's purposes before DP_SEEDED, and these rows too: DH_AIR,
    DH_WET, DH_SKY, DH_VAULT and the rots have none): not taken.  It would change the dungeons.

<!-- DECISIONS.md:4546-4652 at d35194df -->
## The scheduler table, the continuations' stack, the after-work's queue (2026-11-12, as built, for review: ENGINE-CRITIQUE.md's §3.3, §7 and step 6, stages (d) to (f))

The step planner's stages after "The step planner and the activity stack"'s (a) to (c), each its own commit, the suite
whole at each.

**(d) The scheduler table** (§7).  The world's own work as it falls due was a chain of hand-written checks in main ():
the chunks stocked, the map making's late work, the traces' journal, the denizens' intents before the intents are
taken; a look again as a body falls, the denizens' bodies seen, the lights' minute after the clock.  Now each is a row
of one table, dq0w.c's three columns:

| row | trigger (jobs_due) | gives way (jobs_held) | handler (jobs_run) |
|---|---|---|---|
| the chunks stocked | none: every pass, its own look | never | stocks |
| the map making's late work | chart_late (a thing handled) | ui_long | chart_job: a sconce's light, the map's things |
| the traces' journal | TR_FLUSH passings journalled | ui_long | trace_job: tr_flush (TF_SOME) |
| the denizens' intents | some posted (DEN_INTENTS) | den_held | den_intents |
| a look again | den_saw (a body fallen), not on the move | ui_long | look_job: see (0) |
| the denizens' bodies | the hub's den_seq changed | uf_held (the UI's command in hand this pass) | dseq_job: dens_seen, V_STIR |
| the lights' minute | the game begun, and burn_ask or burn_due come | ui_long | burn_job: the sconces' pass, the time believed alone, or burns_now |

jobs.s walks rows first to end: each row's trigger asked, then, if it's due, its give-way rule, then, if it doesn't
wait, its handler; main () walks the first four before the intents are taken and the last three after the clock and
the long task's step, where the chain had them.  The order and the short-circuits are the chain's (ui_long and den_held
keep their own clocks from their first asking: a rule asked only as its row's due, as the chain asked it).  The
give-way rule is the row's now, not each hook's test; a hook's own looks as it goes (tr_flush's, sconce_lights',
chart_now's) are as they were.  In assembly for the main RAM's room: cc65's walk of a struct table was 103 bytes and
the rows 54; the three arrays 42 and jobs.s 66.  Rows a period of their own (burn_due) keep it as their trigger: no
row's period moved.

**(e) The continuations a stack.**  activity.h's struct pc_activity keeps each continuation's own field (what it
carries: the thing walked to, the gathering's family, the close-in's target, the search's kind, the timed work's end,
the journey's leg) and now q[6], nq: their kinds (AK_TAKE ... AK_TRAV) in the order they were planned, the last on top.
act_plan (act.s) pushes one as its field's set, out of its place first if it was on the stack: at every setter
(wgame.c's take, wshoot.c's gathering, the close-in in part 37's intent (part 24 had 2 bytes free), a search and a
room's, the timed work's ends (part 12's work, the trace, hiding, the reach's steps and its after-work in things.c,
work_done), a journey's leg and the dig's tool after it).  act_next (act.s) pops those on top whose field's been
cleared since (a stop clears take, foe and gath; a handler clears its own) and hands the top to its handler; none,
0.  **Held to the chain:** the stack is the chain only if nothing's ever planned out of the chain's order.  A check
build ran both (the chain's choice used, the stack's compared, a count in the world's RAM read after each test): no
difference in any of the 286 dq0 tests that run the world, nor in the 32 save-folder games.

**(f) The after-work's planned steps one queue.**  What a redip or a new light owes after its work was two bits and a
stack, read in a fixed order by rs_after: RR_RELIGHT (the torch to light again, the UI's), RR_STOW (the old one put
away), then `aside` (what was set down, taken up, the last first).  Now one queue, things.c's aq: each step queued as
it's planned (rs_queue: AQ_RELIGHT and AQ_STOW once, an AQ_UP as each thing's set down) in its place, above the kinds
done after it, so the queue is in the order they're done whichever is planned first (the old torch's stowing is planned
before a thing's set down to free a hand for the spare, and done before it's taken up); rs_after pops it.  RR_STOW and
RR_RELIGHT gone; the take-ups' step records stay aside's (the queue says when; aside, which thing and its words).  The
step records themselves on the queue (a struct step a place, shifted as one's put under) cost part 86 296 bytes of its
212: the bytes' queue costs 195.

**What wasn't done, for review:**

- **`rs_after_on` and `rs_using` stay.**  The tool put back and the pack put on, the after-work's first and last steps,
  are read from the reach's state, not queued: `rs_using` is the tool's own state as well (out, caught a spark, in hand
  before it so nothing to put away), and `rs_pack` which pack's off; both are cleared in eight places as a handling
  begins or is cut, and a queued step for them would have to be cleared with them or kept stale and guarded by them,
  the same flags read twice.  The pack can also come off part way through the after-work (the torch lit again in the
  dark wants the tinderbox, and the pack off for it), so its step would go under steps already queued.  `rs_after_on`
  is the after-work's phase (none, under way, waiting on the UI's relight or a timed work's end), not a step.
- **The reach isn't planned whole ahead.**  Each ask walks the tree again (s_reach_step): the world changes between
  steps (a denizen takes the pack, a grope fails, a thing's used up, the dark comes as the torch is blown out), and the
  handling's own checks come first.  A plan made whole would have to be checked against the world at every step and
  made again where it's wrong, which is what the walk does now.  The after-work is the part that's planned ahead.
- **The denizens.**  struct pc_activity and struct step don't name the character, but nothing of a denizen's is on
  them.  What it would take: the world carries out a denizen's intents at once (den_intents: a step, a door, a blow)
  and keeps no continuation for one; its walk's next step, its door work's end and its wait are the mind's (dq0d), asked
  again each turn.  Moving them would mean an activity record per actor in the world's bank (8 actors, some 16 bytes
  each, not the main RAM), act_plan and act_next taking the actor, and the denizens' long work (a door forced, a lock
  picked, a pack searched) made timed work with an end the world pops, in place of the mind's asking again: a change in
  when a denizen's work ends, for the user's word first.

**The words.**  Every dq0 test's screen saved before and after each stage (a hook in sim/test.js, not committed) and
compared sentence by sentence (the scratch's sent.js).  Stage (d): 7 of 294 differ, all noise (dq0antidote's real-time
fight; denizens met at other moments in dq0answer, dq0jobs, dq0prompt, dq0stopwords; dq0reachstop's game 2 race, the
side the user accepted; dq0restbusy's news after its sleep cut off by the quit typed, as dq0robrest's has been).  Stage
(e): 5 (dq0antidote; dq0answer, dq0jobs, dq0prompt, dq0skykin's denizens).  Stage (f): 4 (dq0antidote; dq0answer, dq0jobs, dq0prompt's denizens).  No reach step's
words changed: each "You take up the other torch.", "You put the old torch in your pack." and relight question said
as often as before in every test that has them (dq0newlight, dq0twoalike, dq0torchwords, dq0packsteps, dq0light ...).

**The save folder, byte for byte** (fhash, as "One record store"'s: every save-folder file each task wrote over 32 games,
8 seeds at four cache sizes, the soak's player): the baseline at four power-ups, each stage's games against all four.
The baseline's own games vary with the power-up (5 of 32 between its first two), its four together covering each other.
Stage (d), power-ups 1 and 2: 30 and 31 games one of the baseline's; s1000C1 (both), s5C3 (the first) not.  Stage (e),
power-up 1: 31, s70000C1 not (s1000C1 and s5C3 the baseline's again).  Stage (f), power-ups 1 and 2: all 32 games, both times, one of the baseline's.  Each
game that differs is the timing the baseline varies by: its first difference a walk in the dark interrupted by a sound
a step sooner or later ("You hear a squeak ahead. Keep walking?" where the baseline's "The passage turns left."), or a
rat seen as a search ends where it wasn't, the start's dots before it one fewer or more; the files after it follow
from the walk.  Not a job moved: no row's trigger or order changed; the walk's own cycles (some hundreds a pass of the main
loop more than the inline checks) move where the denizens' task lands against the world's, as the record store's
lookup did.  For review: whether that timing is acceptable as the record store's was, or wants a pace that doesn't
hang on cycles.

**Room** (bytes free; before; (d); (e); (f)):

| | before | (d) | (e) | (f) |
|---|---|---|---|---|
| dq0w main RAM | 2,236 | 2,118 | 2,093 | 2,088 |
| dq0w part 86 (the reach's after-work) | 212 | 212 | 206 | 11 |
| dq0w parts 12, 25, 37, 82 | 1,121, 53, 1,843, 40 | the same | 1,101, 48, 1,833, 35 | the same |
| dq0w overlays 16, 26 | 139, 85 | the same | 134, 79 | 8, 10 |

The other tasks as they were.  The start at seed 1: 231.5M → 231.8M (d) → 232.0M (e) → 232.2M cycles (245M the budget);
at 123456789 308.2M → 308.7M → 308.9M → 307.8M.

**The suite:** all 444 pass at each stage.

<!-- DECISIONS.md:4653-4746 at d35194df -->
## dq0acct split: the account and the ruleset's hooks (the user's, 2026-11-12; as built, for review)

**The user's decision, 2026-11-12:** split dq0acct now.  The time between dungeons is the engine's (the quests and
rumours, the money's matters, the days passing, between.dat, the hand-off to dq0town); what's the ruleset's rules (its
healing, its practice and wages, its sheet, its words for a wound's place, its living by station and its birthday) are its
hooks.  HarnMaster's hooks are HarnMaster's code; dq0null gets minimal ones, and a dq0null game a working time between.

**The mechanism: one source, linked once per ruleset** (the referee skeleton's way, games/dq0/rf/).  Weighed:

- *A program per ruleset run by the account* (a hooks program spawned for its healing, its practice ...): the hooks are
  called in the middle of the account's own work (a day at rest for each day of the road, the lull, the town and the work;
  the tools before the money's matters), on state both share (the wounds, the funds and the fee, the days passed); every
  call would be a spawn and a file each way.  No.
- *A table of overlays loaded at run time* (lib/acct/dq0harn.ovl, a jump table at $8000): an overlay calls back into the
  main RAM (say, ask, amount, the C runtime's helpers), so it must be linked against one fixed main; but the main's library
  modules are whatever all its objects pull in, so the null's overlays and HarnMaster's would want different mains.  Making
  that hold needs a fixed vector of every runtime helper: fragile, for nothing the linker doesn't already give.
- *Linked functions per ruleset* (chosen): the account (games/dq0/acct: acct.c its main RAM, quest.c and town.c its two
  overlays, acct.h, goods.h, town.h) declares its hooks in rsmeta.h (`acct_*`), and each ruleset's account is a link of
  that source with its own hooks, a program of its own in lib/acct: **dq0acct** (games/dq0acct: HarnMaster's, its name
  kept, the tests and the card's) and **dq0nacct** (games/dq0nacct: the null ruleset's).  The link's folder is a few
  one-line files that #include the shared sources, and its .cfg.  rsmeta.h's RULESETS names each ruleset's program
  ({ "dq0null", "/../acct/dq0nacct" }); dq0life is unchanged but its comment.  It costs a program image per ruleset on the
  card (dq0nacct's is 19K and two overlays), nothing at run time.

**The hooks** (rsmeta.h, called in the account's order): `acct_sheet_read` / `acct_sheet_write` (the sheet, META_SHEET),
`acct_wounds` (what's done at once as it comes out: HarnMaster's notes, binds and pays a physician from the account's
funds into its fee), `acct_mended`, `acct_day` (a day at rest; the account counts the day: `passed` is the account's now,
not body.c's), `acct_death` (its cause, in words), `acct_rested`, `acct_calling`, `acct_wage`, `acct_habit` (a day's living
by its station), `acct_birthday` (for LIFE_AGE), `acct_tool` (the things carried, a thing at a time in ans as before: what
it can practise with, or what its calling needs), `acct_first` (the words of its wounds), `acct_worked` (what the work
taught, after the account's sentence of the work), `acct_practice`, `acct_town` (its knacks for the market and the inn:
between.dat's rhet and intr) and `acct_learnt` (what a master opened, on its sheet).  Banks (acct.h): the kinds' bank, then
the account's two overlays (ACCT_OV_QUEST, ACCT_OV_TOWN), then the ruleset's (ACCT_OV_RS on), one file NAME.ovl (the old
dq0acct.ov5 is gone: an overlay followed by another is filled).  A hook called from the account's main RAM may put in its
own overlay and gives the kinds' bank back; the three called from the town overlay (acct_habit, acct_town, acct_learnt) are
the ruleset's main RAM's and put in none.  The account's segments are named by role (QUESTCODE, TOWNCODE; HarnMaster's
BODYCODE, SKILLCODE, SHEETCODE, WORDSCODE): each link's .cfg puts them in its OVn.

**What moved where:**

- *The engine's (games/dq0/acct):* dq0acct.c's main, the money's matters, the kit, the goods' and the days' words
  (acct.c); quest.c as it was; town.c without the wound words, plus the console's width (from skill.c), the names given,
  the record written, the lull's dice and the work's sentence (from the main RAM, for its room); `dead` for o_lodging (was
  pc.state); goods.h and town.h (dq0town and dq0schol include them there now).
- *HarnMaster's (games/dq0harn/acct):* hooks.c (the sheet's names, pc and the rest, in the main RAM; the hooks; the living
  by social class, the habits table, once now: town.c had its own copy; the EMLs for the town; a tongue learnt), body.c,
  skill.c (o_work now the lesson alone), sheet.c, hfile.c (as they were, their paths), and words.c (town.c's physician's and
  wounds' words: harn.h's locations), hacct.h.
- *The null's (games/dq0null/acct/hooks.c, the main RAM's):* nothing heals (its wounds, the sheet's count, as they were: it
  heals at rest in the dungeon), no one dies of the days, no physician, no practice, no lesson in the work; its living the
  middle station's (2d a day), its wage the unguilded's (42d a month: HarnMaster's tables' own defaults for no class), its
  birthday the year's first day, its knacks in town its tests' flat 60 (dq0null.c's BASE); its sheet read for its calling and
  its tongues, written back only when a master opened a tongue or a script ("script delvic", "language sindarin", no mastery,
  eight at most: dq0null.c's load_pc).

**Checked: HarnMaster's games behave as before.**

- The account's words and the save folder's files: 48 lives (16 seeds, stand-ins' and histories', three variants each: dq0auto's
  straight on, and the player asked, its town, market, inn and scholar's house answered by a script; seven occupations,
  wealth, skills, kits, the crypt solved) played through three dungeons each, the way out opened by a poke, random wounds put
  on the sheet before each time between, dq0life run by hand, and before and after it the record (life.dat), kit.dat,
  kit0.dat, pc.hm and quest.txt dumped.  Every time between that began on the same files in both builds (74 of 96: the rest
  follow a new life's clock-seeded dungeon, or the game's own timing noise, so begin on other files) said the same words and
  left the same files, byte for byte: among them 32 deaths of the days, 48 physicians, 27 months' practice (asked and
  named), 29 spells of work, 18 sales, 15 restockings, 15 rumours followed, 12 towns with the scholar's house.  The suite's
  dq0account, dq0acctgone, dq0town, dq0scholar, dq0tongues, dq0life, dq0dressbetween and dq0lifeend as before.
- Every dq0 test's transcript before and after (sent.js): 11 of 294 differ, none in a time between: dq0antidote's real-time
  fight; dq0answer's, dq0jobs', dq0prompt's, dq0skykin's, dq0stopwords' and dq0dug's denizens met at other moments; dq0mine's
  scraping heard once more; dq0restbusy's rest ending at another moment (an Initiative improvement, hunger and thirst told);
  dq0battering's blows' words in another proportion (its blow at a time as the clock goes); dq0starttime's progress dot.  No
  game's image changed; the card did (dq0acct.ov5 gone, dq0nacct added), so its files are read at other moments.
- The suite: all 445 pass (444 and dq0nullbetween).  The start: dq0starttime 606.0M cycles to 605.2M (the account isn't in it).

**A test:** dq0nullbetween (tests/suites/dq0/save.js): a null game (seed 70000, its artifact to hand) walked out solved,
three wounds put on its sheet, carried on: the days told (the road and the lull alone), the reward and the standing, the
middle station's lodging, the money, the rumours and the first followed; the record on to dungeon 2, its date on, the next
dungeon begun under the null referee, its sheet the null's with its wounds as they were.

**Room** (bytes free): dq0acct's main RAM 2,157 to 2,297 (its words of the work, the names, the record's writing and the
lull moved into the town overlay for it: the hooks' calls cost some 400 bytes); its overlays, before body 319, skill 962, sheet
9, quest 279, town 2,008; after quest 279, town 2,582, body 327, skill 1,543, sheet 65, words 6,783.  dq0nacct 7,349 (its
overlays 279, 2,582).  dq0life 18,095 to 18,077 (the null's program named).  dq0town, dq0schol and the game's tasks as they were.

**For review / left:**

- dq0town's tests (its haggling, its listening: the book's d100 against an EML) and dq0schol's (its own test, its scribes'
  and sages' figures; the sheet read through HarnMaster's rsmeta meta.c) are still HarnMaster's: a null game's town runs
  them with its flat 60 and the null sheet read as HarnMaster's lines.  The town's own ruleset hooks would be the next step.
- The calendar's month names (o_date's, the library's harn_month_name's names) are the setting's, not the rules': left in
  the account.
- dq0acct keeps its name for HarnMaster's link (the tests and the card name it); a rename to dq0hacct would be symmetric
  but touches every test that runs it.

<!-- DECISIONS.md:4747-4842 at d35194df -->
## Narration as data, stage 5: the world's sentences (the user's, 2026-11-12; as built, for review: ENGINE-CRITIQUE.md's §3.2 and step 5, its fifth stage)

Stage 4 left the world's things' words (dq0w's SA_*, kinds.c's said[]) open.  The user's decision (2026-11-12): move
them into the phrase table.  The world no longer writes these sentences into things_text: it writes a code and the
names the sentence needs, and the UI words the code by said.txt's rows, with the renderer every other row uses,
wherever it reads the text.  kinds.c's said[] and things.c's sentence building are gone.  The words the player sees
are the same, letter for letter.

- **The coded form (items.h's SAY_AT, SAY_SEP, SA_*).**  A sentence in things_text is `SAY_AT` (0x01), its SA_* |
  0x80, the thing's name, `SAY_SEP` (0x02), the other's name, `SAY_SEP`.  Both names always, as things.c's say had them
  (xn and yn: the words asked, then the things' names, "spent torch", "4 rations"; the pack's, the belt's, a
  container's).  Plain text before and after it stays as it is, so a sentence can follow the world's own words in the
  same text ("There are only 2. " then a code; a count's "You take 4 rations." after the stowing's code ...).  The
  enum moved from dq0w's things.h to items.h, the world's and the UI's shared header, so hubgen reads it and said.txt
  names its rows by it (`SA.stow` is SA_STOW).  The codes are 0x80 and up so no code byte is ever 0, SAY_AT or SAY_SEP.
- **The world (dq0w).**  things.c's say builds the code in tbuf and puts it (tlist.s's put) where it put the words;
  last_sa as before (the doff's SA_HANDS test, the count's sentence again).  kinds.c's said[] (55 strings, in the
  catalogue's bank) gone.
- **The hold's words coded too (SA_HOLD, SA_SHIFT, SA_HELD).**  part 48's s_hold_in wrote "You take the dagger in
  your right hand.", "You shift the ... to ..." and "You hold the ... in ... already." as free text; SA_HOLD, SA_HOLD2
  and SA_HELD were said[]'s, never used ("in your hand", "in both hands", "already" without the hand).  Now SA_HOLD,
  SA_SHIFT (SA_HOLD2's place) and SA_HELD are those three sentences, the hand's words ("your left hand", "both hands",
  hand_words') the other name: part 48's hold_said puts the code, the name, then hand_words' words between the
  separators.  This is what makes the "already" test a code check (below).
- **The UI's readers.**
  - **tt () (look.c), and through it every reader of the text as words**: the hthings.c, main.c, way.c and search.c
    msg (tt ()) lines, `*tt ()` tests, theme.c's othings, and the referee's slot copy (to_hand's RS_EQUIP: the tools'
    keys).  tt copies the text into lb as before, then calls tt_words (refuse.c, overlay 24): each code in lb is worded
    where it stood, the text after it read from the hub again after the words.  The renderer needn't write over what
    it hasn't read: the rest of the text is still the hub's.
  - **The inventory list (words.c's o_list_say) reads the text raw**, past lb's size in pieces, as before.  IO_LIST's
    words are the list's own put()s, never a code, so it needs no wording.  The same for main.c's dig (what it carries:
    the tools' names found in the text, a code's names among them as they were) and caps.c's which (the 'W' form, the
    choices' list).
  - **The "already" test (main.c's s_hold_first) is a code check.**  It was `strstr (tt (), "already")`: a blow's
    weapon already in the hand said to take it in.  Now tt_words keeps the first sentence's code if the text begins
    with one (tt_sa) and the test is `tt_sa != (SA_HELD | 0x80)`.  The hold's other refusals with "already" in them
    were none: o_handle's IO_HOLD path refuses with SA_WHAT, SA_NO_SUCH, SA_REACH, SA_SHUT, SA_FIXED, hold_in's held
    already, why_not's hands and the reach's steps.
- **The renderer writes into lb, but not only at its start (said.c's said_to).**  A row's words went to lb from its
  first byte.  said_to (the main RAM's, lb unless a caller says otherwise) is where they go: tt_words points it at the
  code's place in lb with said_q set (the words left there, not said), then puts it back.  flush and the row's start
  use it; the rest (the "a"'s "an", the theme's names added, a look's lb_said) is lb's as before, and no SA row uses
  them.  No other buffer: lb holds the text, and the two names wait at its end (the last 40 bytes) while the sentence
  is worded.  A code past lb's 250th byte cuts the text there (a sentence's 100 bytes at most, then the names): none of
  the tests comes near; the text past 399 bytes was cut before as well.
- **The rows (said.txt's SA, the UI's kind UK_SA: msgs.def's, `w1 sa:8-15@SA_HOLD`).**  55 rows, each said on after
  what's before it (`+`, no new line), the thing's name `{text}`, the other's `{obj}`.
- **One wording per action: two pieces shared.**
  - `@seeno` "You see no {text} here": the refusal's (NO.A_SEE_NO, say_no's) and the world's SA.no_other ("You see no
    sack here.").  SA_NO_OTHER said the other's name (yn); other () puts the same words (iq.into) in xn just before,
    and yn holds them too (o_handle's copy, untouched on the way), so the row takes the thing's name like NO's.
  - `@light` "light the {text}": SA.light ("You light the torch.") and the reach's step (TASK.step's @steps: "You
    light the spare torch from the burning one.").
  - The rest have no wording of the same action elsewhere in said.txt: the reach's steps say "take off your pack",
    "put your pack back on", "put the torch in your pack", "blow out the torch", where the world's say "You take off the
    cloak.", "You put on the cloak.", "You stow the torch in your backpack.", "You put out the torch."; making them one
    would change what the player reads.  **Open, for the user:** whether those pairs should be one wording (and which).
- **Not moved:** the world's other words in things_text (why_not's, the look at a thing's, the doors' and locks',
  wcloth's, wdrag's, the rope's, the water's, doff_feet's, a count's "You take 4 rations." ...).  They're free text
  built by the code that knows the case, not SA_* rows; each would be a row of its own, a later stage's.
- **Room** (the build's table, bytes free):

  | | before | after |
  |---|---|---|
  | dq0w's catalogue's bank (CAT: said[] and say's loop gone) | 1,847 | 3,573 |
  | dq0w main RAM | 2,088 | 2,090 |
  | dq0w part 48 (s_hold_in, hold_said) | 1,098 | 1,167 |
  | dq0w overlay 15 (part 48's image) | 299 | 368 |
  | dq0w overlays 16, 26; part 86 | 8, 10; 11 | the same |
  | dq0's main RAM | 3,356 | 3,343 (tt_sa, said_to, tt_words' stub, tt's call) |
  | dq0 overlay 24 (tt_words; the packer's part 6 now there) | 4,136 | 342 |
  | dq0 overlay 27 (the renderer; the packer's part 3 now there) | 83 | 231 |
  | dq0 overlay 15 (part 3 no longer there) | 726 | 3,806 |
  | dq0 overlay 28, 29 (the rows: 55 SA rows, 2 pieces) | 0, 2,506 | 3, 1,431 |

  The packer (pack.js) places parts 3 and 6 by the room it finds, so overlays 15, 24 and 27 moved together: the
  renderer's code grew 4 bytes (said_to) and its words most said 113 (said_dw chosen again over the new
  rows), and tt_words is 455 (OV24CODE 3,648 -> 4,103).  The rows' banks have 1,434 bytes left.
- **The start** (dq0starttime, both builds run at once on one machine): seed 1 232.7M → 231.9M cycles (budget 245M),
  seed 123456789 308.4M → 308.2M (325M).
- **Checked:** every dq0 test's screen kept before (the commit before, built in a copy of the tree) and after (a hook
  in sim/test.js, not committed), their sentences compared (sent.js): 7 of 294 differ, all the known noise: the
  real-time fight in dq0antidote; denizens met at other moments in dq0answer, dq0mine, dq0prompt, dq0stopwords;
  dq0reachstop's accepted race (the stop one step later); dq0sway's typed-ahead key echoed a moment later.  No SA
  sentence is said differently or a different number of times anywhere ("You stow the" 26 and 26, "You put on the"
  126 and 126, "You light the" 62 and 62, "already" 198 and 198, "You see no" 28 and 28, "Your hands are full: stow"
  11 and 11 ...; "You take the ... in your" 74 and 75, dq0reachstop's race).  The save folder isn't touched (things_text
  is the hub's, never saved).
- **Tested:** all 444 tests pass.
- **Open, for the user:**
  - the world's and the reach's wordings of one action that differ (above): one wording each, and which;
  - the world's other free text in things_text (why_not's "Your hands are full: stow something, or put it down." is
    SA.hands's words written out a second time; the look at a thing, the locks', the cloth's ...): rows of their own,
    a later stage, if wanted.

<!-- DECISIONS.md:4843-4922 at d35194df -->
## The styles' proportions in a styles file, the theme naming a set (the user's, 2026-11-12; as built, for review: ENGINE-CRITIQUE.md's §5, step 8)

The user's choice of the three ways "The plans' pages on the record store ..." set out: the third, the styles outside
the theme's record.  The record stays as it was (THEME_SIZE, 4,096 bytes, struct theme's 1,739-byte head, the words'
room the same), so themegen's linegen budgets every clue as before and no generated theme changes.

- **The source: games/dq0/styles.txt.**  Sets of the four styles (built, natural, mine, mixed: level.h's ST_* order),
  in the theme files' key | value format (tools/dq0build.js's keyLines).  A set's line (`set | name | the set it
  begins as a copy of`), then its rows: a field of struct style and its four columns (a field of more than one value,
  ceil, kinds, hide_kinds, its values in a column with spaces between; a thing's kind by items.h's name, KI_ left
  off, in lower case).  A set begun from another gives only the rows it changes; one begun from none gives every field
  (but slant, the chunk's own, 0).  The first set is **house**, uses.c's old table field for field, so it's the
  default.  A second, **open**, begun from house, changes one row: `doors` 0 in every style, so every way out is a
  doorway and there's no door to trap or make a portcullis of.  It serves a theme of open, unguarded places, and it's
  the test's.  A set's number is its place in the file, so a set is added after the others and never put between
  them (a theme.dat compiled before still names the same set).  16 sets at most (TM_STYLES's four bits).
- **struct style in theme.h** (moved from dq0m/grow.h, unchanged), so the build has its layout from the C (hubgen.js:
  dq0hub.json's S.style).  It's 53 bytes, not 52 (slant's byte): the four styles 212 bytes, not the 208 the last
  entry said.  STYLE_SETS 16, STYLE_NAME 12 (a set's name's bytes in the file, its 0 among them).  grow.h includes
  hub.h for it (built.c, pass.c and fixtures.c had only world.h).
- **The file: lib/dq0/styles.dat**, made by tools/dq0build.js's `styles (H)`, run in `generate` (build.js's
  hubNumbers, before the games: themegen's make.js compiles its goal kinds' themes with themec, which wants the file
  on its card).  Its head four bytes (the sets, sizeof struct style, 4, STYLE_NAME), each set's name (12 bytes), then
  each set's four structs, laid out by H's offsets.  The generator checks each row's field, its columns and values,
  and that a set begun from none gives every field.  sdcard.txt installs it as games/lib/dq0/styles.dat.
- **The theme names its set: one line, `styles | NAME`.**  themec (games/themec/comp.c) looks the name up in its lib
  folder's styles.dat (themec.c's `styles_of`, read only when a theme has the line) and puts the set's number in the
  record's mission byte, bits 2 to 5 (theme.h's TM_STYLES, TM_STYLE_SET; they were free: TM_ESCAPE, TM_RULE, TM_OPEN,
  TM_FELL the rest).  No line, 0: the house's.  So no record's byte changes for a theme that names none: dq0t.dat is
  byte for byte as it was.  An unknown name is the theme's fault ("styles: no such set").  tools/themec.js puts
  styles.dat on its card beside bestiary.dat and keys its cache by it.  A hand-written theme (dq0 -t) gives the line
  with its strata: themegen's groups put `styles` in the strata's group (stratum, layer, styles), whose lines it
  copies through as given (linegen's emit; histgen's take ignores it).  A hand-written theme that gives a styles line
  must give its strata too, as with a layer line.  The theme format's note (themes/1crypt.txt) has the key.
- **The maker reads the set** (dq0m/uses.c): its table gone, a buffer of four styles in the main RAM, filled at the
  first US_STYLE of the game (the first chunk made, the theme in the hub by then: the world begins the dungeon after
  the UI's IO_BEGIN, after theme_load).  It reads the theme's mission byte from the hub, opens libpath's styles.dat,
  checks its head (the struct's size, four styles), seeks past the names to the set (a set past the file's: the first)
  and reads 212 bytes.  US_STYLE copies the chunk's style from the buffer as before.  No file: every chance nil (a
  broken install; not hidden behind a copy of the table in the code).
- **Checked.**
  - The default set: styles.dat's house set compared with uses.c's old table (its initialisers read from the old
    source, KI_ names by dq0hub.json): the same 212 bytes.  open differs in three bytes, the built, mine and mixed
    styles' doors.
  - The dungeons: the maker's every write to the hub's shared banks (dgen, as the earlier entry did) over 80 games, 8
    seeds (1, 3, 7, 42, 259, 1000, 70000, 123456789), each as it is, -T, -T -T -T, -M, -A, -D, -V, -M -T, -A -T -T and
    -V -T -T -T: the same as the baseline's in all 80.  The transcripts of the 80, by sentence, the same in 79; s1MT a
    timing variant (a walk stopped by a rat a few squares elsewhere, at every power-up of this build, never the
    baseline's: the maker's file read at its first chunk moves the start some 0.5M cycles, and the soak's player's
    next words land on another square).
  - The suite's transcripts, by sentence: every dq0 test's the same as the baseline's but ten.  Six are the known
    timing noise (dq0antidote's real-time fight; dq0answer's, dq0jobs', dq0mine's, dq0skykin's and dq0stopwords' rats,
    adders and sounds met at other moments).  dq0dug's rat is met at another moment too, and the baseline itself gives
    that at another power-up (--seed 2).  Three move with the start's half a million cycles: the same at every power-up
    of this build (--seed 2, 3), never the baseline's.  dq0starttime: a progress dot after "(B) a sconce.".
    dq0battering: the same blows and the same door broken down, but its words of progress ("It's starting to give.")
    after another blow, as the clock goes.  dq0engage: its real-time fights (the ghoul's blows and the character's)
    rolled in another order. 
  - The test **dq0styles** (tests/suites/dq0/theme.js; tests/areas.js's theme): the same hand-written theme (its name
    and one built stratum of halls) at seed 5, without and with `styles | open`.  The styles line is kept in
    theme.txt and compiles to the set's number (1) in the record's mission.  The top stratum's chunks in the cache as
    each game quits: 4 and 4 built chunks, 51 doors and portcullises in the first and none in the second (a doorway's
    square is T_DOOR without a portal, so the test counts portals).  765M cycles.
- **Room** (bytes free):

  | | before | after |
  |---|---|---|
  | dq0m main RAM | 4,193 | 3,932 (the styles' buffer 212, its path 48) |
  | dq0m overlay 7 (uses.c) | 224 | 190 (the table out, the reader in) |
  | themec main RAM | 902 | 629 (styles_of, the line) |
  | themegen main RAM | 712 | 705 (the group's word) |

  themec and themegen were under the 2,048 before (the build's warnings); this takes 273 and 7 more.  No other task
  touched.
- **The start** (dq0starttime): seed 1 232.4M cycles before, 232.9M after; seed 123456789 308.5M before, 309.0M after
  (budgets 245M and 325M): the maker's open and read of styles.dat at its first chunk.
- **The suite:** 445 of 445 (dq0styles the new one).  Run from a copy of the tree, four machine tests (hwtest, hwtool,
  fs, mml) want the repository's own folders and fail there for the baseline as well; they pass in the worktree.
  tiers first failed on the new test's variable `db`, a word of asm's area (areas.js's WORDS): renamed.

<!-- DECISIONS.md:4923-4957 at d35194df -->
## The chunks' cache made way for round, as the other stores (the user's, 2026-11-12; as built, for review: ENGINE-CRITIQUE.md's §6, step 7)

"One record store" left one store comparing its stamps plainly: the chunks' cache (make.c's `pick`, RP_OLDEST with
RTF_PLAIN), kept as it was because changing it would change which chunk is made way for once the clock wraps.  The
user accepted that change.  `pick`'s block (`cpk`) drops RTF_PLAIN, so RP_OLDEST compares the cache's stamps round,
as it does for every other store: a stamp is older than the best so far if it less the best is past $8000.  No other
code compares the cache's stamps: the readers (chunk.c's `find`, RP_FIND) only set a found slot's stamp to the clock's
next (`wrec.stamp`, the world bank), and make.c sets a new chunk's the same way.  RTF_PLAIN stays in the library (rp.h,
dq0lib.s), no store's now.

- **What it changes.**  Only which slot is made way for when the stamps being compared span the wrap, and so the seen
  maps written as a chunk goes out (smSXXYY.dat) and which chunks are made again.  While every stamp is below $8000,
  plain and round give the same choice: for s and t both in 0 to $7FFF, s - t (mod $10000) is past $8000 exactly when
  s < t.  The clock starts at 0 with each game's hub and goes up one at each chunk made and each reader's long lookup.
- **Measured: no game reaches the wrap.**  The clock's every write was watched (a scratch hook in sim/test.js, the
  world bank's `wrec.stamp`, not committed):
  - the suite's 286 tests that make a hub: the highest 3,620 (dq0peoples), the median 131, nine in ten under 365;
  - the 48 save-folder games below (-C's twelve slots, strata down to the fourth): the highest 2,220 (s70000W4).

  Nothing comes within $8000 (32,768), so every choice is the same as before.
- **Checked.**
  - Every save-folder file's writes (fhash, as "One record store" did) over the same 48 games (8 seeds, 3, 5, 7, 42,
    99, 1000, 31337, 70000; each with -C -T -T -T, -C -T, -T -T, -C, and -T -T -T -T with and without -C): the
    maker's files (the seen maps and the plans' pages, 28,884 operations) the same as the previous commit's in all 48.
    Three games' other files moved with the timing (the traces' tr file of s1000C1 written 79 times, not 57, and one
    task with no file at all in s1000W4 and s31337C1 listed or not): RP_OLDEST's round branch takes a few cycles more
    than the plain one.
  - The suite's transcripts, by sentence: every test's the same as the previous commit's.
  - A wrap forced on purpose (the clock and every slot's stamp poked $FF00 on in 8 games with -C) gave the same maker
    files plainly and round; the poke didn't plainly hold (the clock's writes after it weren't consistent with it), so
    this proves nothing either way and isn't counted.
- **Tests.**  No test's expected result changed: none reaches a wrap.  None loosened.
- **Room and the start:** unchanged (a flag's bit in a table).  The suite: 445 of 445 (the four machine tests, failing
  in a copy of the tree, pass in the worktree).

<!-- DECISIONS.md:4958-5002 at d35194df -->
## A command's first letter the sooner: four costs on its way cut (2026-11-12; as built, for review: D17)

dq0torchwords failed at 25d2b499: "put the tinderbox in the backpack" had its first letter ("You look for the
tinderbox.") 725,269 cycles after its Enter, past the user's 720,000 (D17); the styles file (088ca38f) had only
moved the tasks' timing.  The line was 719,230 the commit before: every "look for" line sat near the limit.

- **Where the cycles went.**  A run of the test profiled (the emulator's own profile, opt.profile, keyed by task, the
  RAM bank at $8000 and the part in the swap area; a scratch hook, not committed), the 725K from Enter: the UI 292K,
  the parser (dq0c) 168K, the world 105K (its periodic passes, 8K each some 47 a second, then its answer's part
  copies after the first words were out), the console 80K (the line echoed and the answer), the denizens 27K, the
  sound 20K, idle 28K.  In order: the UI woken some 20K in; the line split into parts (parts.c) and the echo, 135K
  before the parser had it; the parser's 200K and more, the UI's names_in and o_pacing between; resolve, walk_ahead's
  copies and choice, the first words worded (said.c) 470K-640K; then eleven letters at the console's pace (4,368
  cycles each) before the "Y": the cursor sent again to where it already was (ESC [ 23 ; 36 H) and the new line.
  Nothing of the world's answer was waited on: the first words are already said before the world is asked
  (sway.c's walk_ahead, D17).
- **What was cut, the words the same:**
  - term.c: a line entered leaves the cursor at the story's place (say's away 2), and the answer's first words go
    there without sending it there again (enter() no longer sets away 1; t_line and t_prompt set it only if the
    cursor was on the line typed).  The line typed is still drawn again at t_done (away not 0).  Eight letters fewer
    before every answer, some 35,000 cycles at the console's pace.
  - pace.c's o_pacing (a line's first look for the pace's words): each word was compared with the list's 66 words,
    some 35,000 cycles a command; now a table of each first letter's word lengths (made from pace_words at the
    first line, in overlay 16's bank) passes over a word none of the list can be.
  - parts.c's parts(): "then" and "and" looked for only after a space before a t or an a (among's scans at every
    space before, some 10,000 cycles).
  - said.c's over(): a plain letter passed with one read of the rows' bank, not two (step's), when an {arg?...}'s
    alternatives are skipped: the first words' row has eleven.
- **Measured** (dq0torchwords' lines, cycles from Enter to the first letter, before -> after): put the tinderbox in
  the backpack 725,269 -> 610,825; put the rope in the backpack 681,280 -> 567,171; take the spent torch from the
  floor 656,741 -> 582,476; take the spent torch from the ground 649,831 -> 568,552; take spent torch from backpack
  610,420 -> 541,350; the inventory 275,201 -> 221,900.  Every line 50,000-115,000 sooner.
- **Not done, for review:** the parser (dq0c's matcher) is the largest single cost, 160,000-170,000 cycles for a "put
  X in Y" line (commandmatch-test.js: its slowest lines are all of that shape; the average 54,000); a larger rules'
  cache (256) changed nothing, so it's the grammar's alternatives, not the cache.  The world's, the denizens' and the
  sound's periodic passes take some 18% of the time while a command is in hand (the world's at the foreground's
  rank, FS_CMD).  walk_ahead's copies of the carried things (from_ui: through the C stack, twice a byte) and its
  name match of each kind, some 30,000.  The console's newline is "\r\n", which the console makes "\r\r\n": a letter
  more a line at its pace.
- **Checked.**  The suite's transcripts, by sentence, against 25d2b499's: seven differ, all the known timing noise
  (denizens met at other moments: dq0jobs, dq0mine, dq0skykin, dq0stopwords, dq0sway, dq0robrest; dq0reachstop's
  race).  No check loosened, no expected string changed.
- **Room:** dq0's main RAM 3,343 -> 3,338; overlay 6 2,293 -> 2,253, overlay 16 2,271 -> 2,005, overlay 27 231 ->
  206.  The start: seed 1 232.0M -> 232.4M, 123456789 310.0M -> 309.7M (timing).  The suite: all 446 pass.
