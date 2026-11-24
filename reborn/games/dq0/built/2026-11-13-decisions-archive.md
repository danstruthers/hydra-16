# DQ0: the decisions archive, 2026-11-13

The agents' accounts of what they built, moved out of DECISIONS.md on 2026-11-24 (branch docs2) with their text
unchanged, as AGENTS.md's Records rule asks: DECISIONS.md holds the user's words only. Each section left a one-line
pointer there; where a section held the user's words too, those stay in DECISIONS.md, quoted, and the whole section
is here. Each section is marked with its lines in DECISIONS.md as at d35194df.

<!-- DECISIONS.md:5009-5151 at d35194df -->
## The town and the scholar's house: the ruleset's hooks (the user's, 2026-11-13; as built, for review)

**The user's decision, 2026-11-13:** give the town and the scholar's house ruleset hooks now, in the account's pattern.
Till now dq0town's haggling and listening tests and dq0schol's d100, its scribes' and sages' figures and its reading of
the sheet (HarnMaster's meta.c) were HarnMaster's whatever the ruleset: a null game's town ran the book's d100 on the null
account's flat 60, and its scholar's house read the null sheet as HarnMaster's lines (no masteries: every lore asked of a
sage, a master's figure worked from attributes its sheet hasn't got).  Now both programs are the engine's, their rules the
ruleset's hooks (rsmeta.h), HarnMaster's its own code, the null's its own.

**The mechanism: the account's, one source linked once per ruleset.**  The engine's sources moved to games/dq0/town/
(town.c, dq0town.c as it was; schol.c, dq0schol.c as it was; town.txt and scholar.txt with them, the card's paths
updated).  Each ruleset's link is a folder of one-line files that #include them, its .cfg (the old one) and its hooks:

| ruleset | its account | its town | its scholar's house | its hooks |
|---|---|---|---|---|
| HarnMaster | dq0acct | dq0town (games/dq0town) | dq0schol (games/dq0schol) | games/dq0harn/meta (test.c, meta.c) |
| null | dq0nacct | dq0ntown (games/dq0ntown) | dq0nschol (games/dq0nschol) | games/dq0null/meta (test.c, meta.c) |

Each link names itself and the next program in the chain with a define: the account's town.c ACCT_TOWN (dq0acct's
"dq0town", dq0nacct's "dq0ntown"), the town's TOWN_NAME and TOWN_SCHOL, the scholar's house's SCHOL_NAME (its overlay
file, lib/acct/NAME.ovl, its fault messages' name).  So the account a ruleset's table names runs its own town, which runs
its own scholar's house: no program asks which ruleset it's under.  HarnMaster's links keep their names (dq0town,
dq0schol: the tests, the soak and the card name them), as dq0acct did.

Why this and not another way:
- *The ruleset chosen at run time inside one town program* (both rulesets' hooks linked in, a switch on game.dat's referee):
  HarnMaster's code in the engine's program and the engine naming rulesets, what the user's 2026-11-11 decision rules out;
  each new ruleset an edit of the engine.
- *A hooks program spawned per test* (as the account's entry weighed): a haggle or a week's study would be a spawn and a file
  each way, and the scholar's house's forced rolls (dq0sg) and dice order would have to cross it.
- *An overlay of the ruleset's loaded at run time:* the account's reason holds here too (an overlay links against one fixed
  main, and the main's library modules differ by what each ruleset pulls in: HarnMaster's meta.c the harn library's
  harn_get_sb, the null's nothing).
- *Linked functions per ruleset* (chosen): the hooks are a handful of small functions called in the middle of the
  programs' own work, on their own state; the linker does the choosing.  It costs two program images on the card for the
  null ruleset (dq0ntown and dq0nschol, each with its 8K overlay), nothing at run time.

**The hooks** (rsmeta.h, a new section, "The town's and the scholar's house's rules"; the sheet's three as they were):

- `meta_test (chance)`: a test at a chance (a knack from the town's record, a mastery, a hired reader's skill, the steps
  added), its grade rs.h's generic G_* by the ruleset's dice.  The programs keep their 0-3 index (G_BADLY 0 ... G_WELL 3),
  and the scholar's house its scene's forced grade (dq0sg) before it asks.  HarnMaster's (dq0harn/meta/test.c): the book's
  d100 against the EML held to 5-95, a critical on a multiple of 5, exactly the two programs' old `test`.  The null's
  (dq0null/meta/test.c): its referee's flat odds (dq0null.c's flat_r) at the chance, a fine success or a bad failure in the
  fifth of the band nearest its end.  A file of its own so the town links it alone (the town needs nothing else).
- `meta_hired (sage)`: a hired reader's skill, a chance for meta_test: HarnMaster's a scribe's 45 + 4d10, a sage's of a
  dead tongue 50 + 4d10 (the old expressions' figures, their dice in the same place in the stream); the null's its flat 60.
- `meta_honest (set)`: a hired reader's honesty, the engine's scale (MH_LIES, MH_DEAR, MH_SELLS, MH_HONEST, each worse one
  doing the better ones' too: a wrong reading told as confidently, half again, a copy sold on).  HarnMaster's: its Morality,
  3d6 (or the scene's figure, dq0sg's first number, which stays in HarnMaster's own scale), by Character 9's bands (4, 7, 10:
  the old ones).  The null's: honest (it has no Morality; a scene's figure is still taken from dq0sg, so a scene's numbers
  line up the same under either ruleset).
- `meta_taught (mk)`: as before, but 0 now means "opened with no mastery": the scholar's house then writes the record's
  learnt without "=N" ("*ardic") and says "you can read and write it now." without "(ardic N)".  HarnMaster's never gives 0
  (70 + a skill base, or a skill base: attributes of 3 and up).  The null's gives 0: its account's acct_learnt puts
  "script ardic" on its sheet, as it already did.
- `meta_line`, `meta_ml` (as before).  The null's: every line the program's own (its sheet has no attributes or masteries;
  its "language"/"script" lines the house's tongue store reads as it did), every mastery its flat 60 (its own tongue, a script
  on its sheet, the lore, the reckoning, the stars, heraldry): so a null character reads, studies and asks the lore itself, at
  its flat chance, for a penny, as its referee tests everything at 60.

What stays the engine's (the house's): the prices (a scribe's day 3d, a sage's three times, a master's month 84d, the
library's penny a day), the days each errand takes and the scribe's days (d3, 2d4), a dead tongue (ten centuries) and the big
town, a grade's worth in a haggle (a quarter, a tenth, an offence) and the readings' words, the sheet's tongue and script lines'
form (the referee's store's, both rulesets').  The town's record keeps its rhet and intr fields (between.dat's layout
unchanged): they're the ruleset's knacks for the market and the inn, set by its acct_town (HarnMaster's Rhetoric and Intrigue
EMLs, the null's 60), now tested by the ruleset's own meta_test.

**What a null game now gets** (it ran HarnMaster's town and scholar's house before): the inn's listening and the market's
haggling at its account's flat 60, graded by its own flat odds; in the scholar's house the lore (folklore, reckoning, the
stars, heraldry) its own at 60 for a penny a day, where HarnMaster's code read no mastery off its sheet and hired a sage for
3d; its study at 60 and the condition's steps; a sage's or a scribe's reading at 60, and honest (never half again, never a
copy sold on, never a lie); a master's month opening a tongue or a script with no figure ("You study ardic for a month with
Master ... (84d): you can read and write it now."; the record's "*ardic", its sheet's "script ardic").

**Checked: HarnMaster's games behave as before** (the acct agent's method: scratch scripts towncmp.js and tcmp.js, after
its acctcmp.js and cmp3.js).

- 56 lives (two batches of 28: seeds 3, 5, 7, 42, 99, 259, 1000, 4242, 70000 (six times), 70001, 100003 (four), 123456
  (four), 200003, 314159, 555555, 1234567, 9999999; hmchar's sage, scribe, hunter, thief, miner and militia, Rhetoric,
  Intrigue, the lore and the tongues' masteries put on the sheet at random, ostari, sunglyph, sindarin, delvic and ardic lines
  too; the journal, paper and charcoal to hand, copied or not), each played through two dungeons with the account asked
  (dq0auto off) and the town answered by a script: groups of errands at random (the inn and its listening, the market's
  haggling for and over things, buying and selling, the smith, who's asking, the scholar's house through the town with its
  readings, study, masters and lore), then the scholar's house run alone over the same folder twice each time between
  (dq0schol -t -b -c -w, no dq0sg: its own dice), its readings, studies, masters and lore at random.  Before and after each:
  life.dat, kit.dat, kit0.dat, pc.hm, clues.dat dumped, and after the account heard.txt and quest.txt, after the scholar's
  house clues.dat, heard.txt and between.dat.
- Every time between began on the same files in both builds (112 of 112), and every one said the same words and left the same
  files, byte for byte; every one of the 224 scholar's houses run alone the same.  Among them (counted in the same transcripts):
  113 haggles, 67 listenings, 41 sages hired to read, 19 studies in the library, 52 masters' months, 38 lore asked of the
  character's own mastery and 59 of a sage, 30 lore that found nothing, 37 sales, 57 purchases, 4 copies sold on talked of.
  (A scribe of a living tongue never came up: no dungeon gave a writing under ten centuries old; its figure, 45 + 4d10, is
  the sage's asked of the lore, 59 times.  The dungeons played between are timing noise and aren't compared.)
- Every dq0 test's transcript before (d0e5503c, built in a copy of the tree) and after (a hook in sim/test.js, not
  committed), by sentence (sent.js): 8 of 296 differ, none in a town or a scholar's house (dq0town's, dq0scholar's,
  dq0tongues', dq0account's and dq0nullbetween's the same): dq0antidote's and dq0engage's real-time fights; dq0jobs',
  dq0skykin's and dq0stopwords' denizens met at other moments; dq0reachstop's accepted race; dq0restbusy's rest ending at
  another moment (an Initiative improvement, hunger and thirst told), as at the account's split; dq0rules' last prompt
  caught a moment later.  The card has four files more, so its files are read at other moments.
- The suite: all 447 pass (446 and dq0nulltown).  The start (dq0starttime, the town isn't in it): seed 1 232.4M cycles
  before, 231.9M after; seed 123456789 309.7M, 308.7M (budgets 245M and 325M): timing.

**A test: dq0nulltown** (tests/suites/dq0/save.js): a null game (seed 70000, its artifact and the journal to hand) walked
out solved and carried on with the account asked: its account runs dq0ntown (the inn's listening and the market's haggling
told), the scholar's house its own (the folklore asked of the journal done by the character itself, "You spend a day in the
library over it (1d).", where HarnMaster's code hired a sage for a null sheet; ardic opened with no figure, said so, its sheet
given "script ardic"); then the scholar's house run alone twice on the same scene (dq0sg '4 2': HarnMaster's Morality 4, a
success): the null's sage honest (9d a day, the journal's own place), HarnMaster's diabolical (14d a day, half again; another
place, told as confidently).

**Room** (main RAM free, the build's table):

| | before | after |
|---|---|---|
| dq0town | 3,379 (overlay 6,443) | 3,293 (overlay 6,443) |
| dq0schol | 5,143 (overlay 630) | 4,981 (overlay 665) |
| dq0ntown (new) | | 3,188 (overlay 6,443) |
| dq0nschol (new) | | 9,713 (overlay 665) |
| dq0acct, dq0nacct, dq0life, the game's tasks | | as they were (2,297; 7,349; 18,077) |

The town's 86 bytes and the scholar's house's 162: the grade's mapping (G_* to the programs' 0-3) and the hooks' calls, and in
the scholar's house the no-mastery branch.  The card: four files more (dq0ntown and dq0nschol, their overlays; 2 x 8K and the
two programs).

**For review / open questions:**

- *The prices* (answered 2026-11-13, the user's: the ruleset's base, the town's sellers' own round it; "Town prices, local"
  below) (a scribe's day 3d, a sage's three times, a master's month 84d, a sage's day for lore 3d, the library's penny a
  day) stay the house's, though their figures are the book's (Character 13): a ruleset with its own economy might want them
  its own.  Not done: they don't depend on the character's sheet, and the account's goods and the market's prices are the
  house's too.
- *The null's readers are all honest.*  The null ruleset has no Morality, so the house's dishonest scribes (half again, a copy
  sold on, a lie) never happen under it.  If the user wants them the house's under any ruleset, the null could roll its own
  (say its flat odds: a bad failure a liar, a failure dear ...).
- *dq0sg's honesty figure is in the ruleset's own scale* (HarnMaster's Morality): a test's scene for a null game sets it but
  it means nothing there.  The engine's MH_* could be the scene's scale instead; the HarnMaster tests' scenes (dq0scholar)
  would then change their numbers.
- *The town's record keeps rhet and intr* (records.def's field names, HarnMaster's skills'); they're the ruleset's knacks now.
  A rename is cosmetic (the layout the same), left.
- *Names:* HarnMaster's links keep dq0town and dq0schol (the tests, the soak and the card name them), as dq0acct kept its
  name; dq0htown/dq0hschol would be symmetric.

<!-- DECISIONS.md:5152-5262 at d35194df -->
## One wording for an action: the world's (the user's, 2026-11-13; as built, for review)

Stage 5 ("Narration as data, stage 5: the world's sentences") left open three actions worded twice: the world's sentence
(an SA row) and the reach's step (TASK.step's `@steps`).  The user's rule: one wording an action, and the world's wins.
Each such action is now one piece of said.txt, used by its SA row and by the reach's step; the actor is "You" in both
(SA's rows and TASK.step both begin "You "), the thing `{text}`, where it goes `{obj}`.  The reach's steps now read in
the world's words.

- **The pieces (said.txt):** `@doff` "take off the {text}", `@don` "put on the {text}", `@douse` "put out the {text}",
  `@stow` "stow the {text} in your {obj}", `@put_in` "put the {text} in the {obj}" (beside stage 5's `@light`).
  SA.doff, SA.don, SA.douse, SA.stow and SA.put_in are "You {@...}."; the steps use the same pieces.
- **Where it goes, the step's obj (the world, dq0w/things.c's s_room, part 90).**  The world's SA.stow names the pack
  ("your backpack": a worn container, so a bandolier would be "your bandolier"), SA.put_in the container ("the sconce",
  "the scabbard", "the sheath", "the quiver").  The step's words knew only the thing (ireq.item's kind).  The hand-freeing
  planner, as it chooses where the thing goes, now gives that place's kind in the reply's `ireq.mat` (IO_DESCRIBE's byte,
  unused by IO_STEP; items.h's note): the sconce or the hung container (`c`, SE_PLACE), or the pack (SE_STOW); 0 for
  any other step.  The UI (main.c's s_reach_said, part 19) names it from the catalogue as obj for the four verbs that
  use it (SV_SHEATHE, SV_SCONCE, SV_INPACK, SV_INPACK2).  No hub layout changed.
- **The stop's gerunds (task.c's doing[]):** a step stopped or asked about is said by its gerund ("You stop blowing out
  the torch.", "Keep blowing out the torch?").  With the steps: SV_BLOWOUT and SV_BLOWDARK "putting out" (was "blowing
  out"), SV_INPACK and SV_INPACK2 "stowing" (was "putting away").  SV_SHEATHE's "sheathing" and SV_SCONCE's "setting
  down" left as they were (see below).
- **The world's own free text, one more:** s_reach_done's "You take off your backpack for it, and put it on again."
  (a pack off and on for a thing in it, said after the handling's words) is "You take off the backpack for it, and put
  it on again.", SA.doff's "the".

**The pairs, before and after** (the step's words as the player reads them; the world's row unchanged):

| action | the reach's step, before | the world's (SA row) | now, both |
|---|---|---|---|
| a pack taken off (SV_OFF) | You take off your backpack. | You take off the cloak. (SA.doff) | You take off the backpack. (@doff) |
| ... and set down (SV_OFFDOWN) | You take off your backpack, and set it down at your feet. | (SA.doff) | You take off the backpack, and set it down at your feet. |
| a pack put on (SV_ON) | You put your backpack back on. | You put on the cloak. (SA.don) | You put on the backpack. (@don) |
| a light put out (SV_BLOWOUT) | You blow out the old torch. | You put out the torch. (SA.douse) | You put out the old torch. (@douse) |
| ... to the dark (SV_BLOWDARK) | You go to blow out the torch: that will leave you in the dark. | (SA.douse) | You go to put out the torch: that will leave you in the dark. |
| stowed in the pack (SV_INPACK, SV_INPACK2) | You put the sword in your pack (too). | You stow the sword in your backpack. (SA.stow) | You stow the sword in your backpack (too). (@stow) |
| into a hung container (SV_SHEATHE) | You sheathe the sword. | You put the sword in the scabbard. (SA.put_in) | You put the sword in the scabbard. (@put_in) |
| into a sconce (SV_SCONCE) | You set the torch in the sconce. | You put the torch in the sconce. (SA.put_in) | You put the torch in the sconce. (@put_in) |
| the pack off for a thing (world's free text) | You take off your backpack for it, and put it on again. | (SA.doff) | You take off the backpack for it, and put it on again. |

The first three rows' actions are the user's; the rest the survey's (below).  A spent or lit thing told apart keeps its
word ("You stow the spent torch in your backpack.").

**The survey: the rest of the steps, TASK and FIRST rows, against the SA rows.**  Applied above where the world's
thing does the very same change (the same edge: worn, off, unlit, in the pack, in the container).  Left, each the
user's to rule on if wanted:

- **SV_SLING "You sling the round shield."** against SA.don ("You put on ...").  The same change (the thing worn), but
  "sling" says how (a shield on the back to free a hand), and one test (dq0newlight's "the pack slung to come off
  again", `/You sling the backpack/`) tells a sling from the pack's SV_ON by the word: as "put on" both, that check
  couldn't be kept.  Left as it is.
- **SV_PUTAWAY "You put away the iron key."**  The tool back where it came from: the pack (SA.stow's words), a pouch
  or another container (SA.put_in's), or the pack's after-work (AQ_STOW).  Not one SA row; the place isn't passed.
  Left; it could take the same `ireq.mat` and choose @stow or @put_in, if the user wants.
- **SV_GETOUT "You get out the pick."** against SA.take ("You take the pick from the backpack.").  Not the same change:
  getting out is a level of the reach, the thing not yet in hand (the world's hold follows: "You take the pick in your
  right hand.").  Left.
- **SV_TAKEUP "You take up the sword."** against SA.hold ("You take the sword in your right hand.").  Close (from the
  ground into a hand), but the step's hand isn't known to its words and the row's sentence needs it.  Left.
- **SV_SETDOWN, SV_LETFALL, SV_LAYLIT (set down, let fall, laid down lit)**: no SA row (a drop's words are the UI's
  own).  **SV_HONE "set to sharpening"** is the work begun, SA.honed its end.  **SV_IN, SV_OPEN, SV_DIP, SV_KINDLE**:
  no SA row.  FIRST.into "You reach into the {text}." and SV_IN's "reach into the {text}" already the same words.
- **TASK rows:** none words an SA row's action (TASK.readied's "You take up your bow." is the missile weapon readied).
- **The gerunds of SV_SHEATHE, SV_SCONCE** ("sheathing", "setting down") stay: "putting" alone doesn't make a gerund
  the stop's sentence can use ("You stop putting the sword."), and none of the tests stops one.

- **Tests:** only expected strings this changes, in tests/suites/dq0/gear.js (dq0busyhands, dq0busyloop,
  dq0newlight, dq0keysteps, dq0feedsteps, dq0blowsteps, dq0handy, dq0packsteps, dq0darklook, dq0liststop,
  dq0packasks, and dq0reachstop's and dq0reachflee's words they type at; some in the descriptions), parser.js's one
  description and senses.js's has(): each the same check with the new words for the old ("You take off the
  backpack", "You put on the backpack", "You put out the old torch", "You go to put out the torch", "Keep putting out",
  "You stop putting out", "You put the sword in the scabbard", and a never: "You stow the torch in your backpack").
  dq0keysteps' `/You get out|take off your/` (locked already: nothing got out, no pack off) is `/You get out|take off
  the backpack/`.  None loosened.
- **Room** (the build's table, bytes free):

  | | before | after |
  |---|---|---|
  | dq0's main RAM | 3,338 | 3,338 |
  | dq0 part 19 (s_reach_said: obj named) / overlay 15 | 3,541 / 3,806 | 3,429 / 3,694 |
  | dq0 overlay 3 (task.c's gerunds) | 2,081 | 2,073 |
  | dq0 overlay 27 (the renderer, the words most said) | 206 | 175 |
  | dq0 overlay 28, 29 (the rows) | 3, 1,431 | 3, 1,540 |
  | dq0w's main RAM | 2,090 | 2,090 |
  | dq0w part 90 (s_room: where it goes) / overlay 13 | 2,153 / 79 | 2,105 / 31 |
  | dq0w part 56 | 3,029 | 3,030 |
  | dq0w overlay 17 | 14 | 15 |

- **The start** (dq0starttime): seed 1 232.4M → 232.7M cycles (budget 245M), seed 123456789 309.7M → 309.7M (325M).
- **Checked:** every dq0 test's screen kept before (this branch's base, built) and after (a hook in sim/test.js, not
  committed), their sentences compared (sent.js).  81 of 296 differ; with the rewordings above mapped (a copy of sent.js
  that rewrites the before's sentences the new way), 26.  Of those:
  - 18 only by the pack's free text, one letter shorter, wrapped at another word ("You take off the backpack for it,
    and put it on again."): dq0answer, dq0bodies, dq0disengage, dq0equip, dq0fighton, dq0firstword, dq0fog, dq0morale,
    dq0reach, dq0reading, dq0several, dq0succession, dq0sweep2, dq0throws, dq0travel, dq0usedup, dq0wordsthings,
    dq0worn (and dq0engage);
  - the known noise: dq0antidote's real-time fight; dq0engage's fights (looked at separately); dq0jobs', dq0mine's and
    dq0skykin's denizens met at other moments;
  - dq0reachstop and dq0reachflee type their lines as given words show ("You take off your backpack", now "the
    backpack"), so the first baseline, run with the tests already changed, typed them elsewhere.  Run again on the old
    build with the old tests: dq0reachflee the same; dq0reachstop its accepted race (a stop a thing later: "Taken." 5
    and 4 times, "You reach for the tinderbox." once and none);
  - dq0restbusy: its last line ("quit") is typed as the eight-hour sleep ends.  Before, the waking's news ("Your
    Initiative skill has improved (49).", hungry, thirsty) came between its "q" and "uit"; now the game quits before
    them.  The same at a second run.  Timing (the start 0.3M cycles later at seed 1): nothing it says is reworded.
  Each step reworded is said as often before and after: "take off your"/"the" 115 and 115; the pack back on 23 and 20,
  with the 3 "You put on the backpack." said before by the world; "blow out"/"put out" 7 and 7; "in your pack" 4 and
  "stow ... in your backpack" 4; "sheathe" 1 and "in the scabbard" 1.  No sconce step is said in the suite.  The save
  folder isn't touched.
- **The suite:** all 446 tests pass.

<!-- DECISIONS.md:5263-5315 at d35194df -->
## A knockout's bearings, whatever the timing (2026-11-13; as built, for review)

Three tests' transcripts had begun to differ from build to build (dq0battering, dq0engage, dq0starttime).  Asked
whether each was only timing, the answer was two yes and one no.

- **dq0starttime: timing.**  o_dots puts a dot each 240 ticks while the start's busy; whether one falls between the
  first look's last sentence and the help's line ("(B) a sconce..") is the ticks' phase alone.  12 spaces more on the
  dq0 line (some 240K cycles) bring the dot; the world's events, the denizens' and the referee's replies the same but
  for the clock's times.  Not changed (the game's dotting is the user's to decide); the sentence comparer's copy
  (the scratch's timing/sent.js) takes a trailing ".." as ".".
- **dq0battering: timing.**  Every blow's harm the same in every run and variant (5,6,2,3,5,4,6,6,6,0,6,0,9,19), the
  door's giving (WB_GIVES) its 8th blow's, harm 31 to 37 crossing half its body.  "stop battering" was typed a fixed
  10M cycles after "break the door": at d0e5503c the 8th blow's count was written 25K cycles before the stop's Enter,
  so the stop came after it ("It's starting to give." then "It's giving, but it holds yet."); a little slower, before
  it ("It's marked, but it holds.", the resumed run's first blow its "begin to tell", WB_TELLS before WB_GIVES).
  Now the test types the stop as its blows are told: the lines after "break the door" held till "You keep at it."
  (its 4th blow's words), then typed as they were (its start(m); the runner's flags still on its dq0 lines).  The
  stop lands after the 5th blow at every delay tried (0, 2M, 4M cycles, three spaces), the transcript the same.
  Every check as it was.
- **dq0engage: a real fault, timing its trigger.**  The world saw the character knocked senseless only by polling the
  summary's C_UNCONSCIOUS once a main-loop pass (dq0w.c's pc_down).  The referee makes that bit afresh at every
  request (pc_out > now); a ghoul's blow out for 100 tenths, the next request after the clock had passed its end
  (the minute's RS_TIME, as the passes fell) cleared it before main looked.  Then nothing of being knocked out
  happened: no bearings_off (no bearings test, at -N none anyway; the character not put prone by it), no black-out
  for the time it believes (burn_ask's TS_ASK; tsense marks T.gone only if it sees the bit itself).  The game's
  dice one stream, the missing test shifted every roll after it: dq0engage's last game a long fight or a short one by
  the timing alone (2M cycles before its "d", or 4M).
  - **Built:** wfight.c's blow() latches the character knocked down by a blow's reply (FF_D_DOWN at it, FF_A_DOWN
    struck by its own), `pc_ko`, where it already made it busy till it comes round; main's check takes the latch with
    the level (`cond & C_UNCONSCIOUS | pc_ko`), clears it each pass, and stays bearings_off's only caller for it.
    bearings_off marks the black-out's time gone itself (tb.gone the minute it's seen out, tb.how TG_OUT, if not
    gone already), as tsense would had it seen the bit: so the time it believes is tested as it comes round
    (wtime.c's woke) however soon.  The world's own harms (a trap's, a fall's: harm_fx's) go through it too.
  - **Checked:** dq0engage's last game at 0, 2M, 4M, 6M cycles before its "d" and a space: the black-out's time test
    in every one (none before at 0, 2M and a space), the requests the same through it.
  - **Not settled, for review:** the fight after it still splits by the timing: the character's first blow after it
    comes round at t503 (2M, 4M cycles: the short fight, a gash to the ghoul's knee) or t603 (0, 6M, a space: the
    long one), the referee's replies the same till then.  The world takes a denizen's blow (its intent's time t) at
    its own clock as it is when it takes it (wfight.c's pc_ready = now + w2), so the character's free moment moves
    with the passes.  The turns pace's, the scheduler's (sched's area): not built here.  dq0engage passes either way,
    its expected strings unchanged; at the suite's timing its words are as they were.
- **What the player sees differently:** nothing in the suite's words.  The black-out's time test now runs after
  every knockout: dq0looks' seed 7 trap (darts, "Everything goes black") makes it now and didn't before; its look
  requests' bytes in game 4 differ (looks.ref regenerated: 29 lines, the request's own bytes; what's seen, the sight
  bank and the chunks, the same in every line).
- **Transcripts** (the suite's, by sentence, the copy of sent.js), against d0e5503c's: 8 differ.  dq0battering's
  intended (the stop after the 5th blow: "It's marked, but it holds.", one "You keep at it." more).  dq0lookfar's
  rats met at other moments (the game's clock a tenth earlier at its first look: the parts' banks moved by the
  packer); dq0answer, dq0jobs, dq0mine, dq0skykin, dq0stopwords, dq0reachstop: the known noise.  dq0engage the same.
- **Room:** dq0w's main RAM 2,090 -> 2,083 (the latch, main's check); part 15 315 -> 310, part 52 2,664 -> 2,592;
  the packer moved parts' images: overlay 10 1,712 -> 893, 14 530 -> 515, 16 8 -> 765.  The start: seed 1 232.4M ->
  232.6M, 123456789 309.7M -> 308.5M (timing).  The suite: all 446 pass.

<!-- DECISIONS.md:5331-5354 at d35194df -->
## The start's dot stops at the first look (the user's, 2026-11-13; as built, for review)

The user's answer ("Four answers" above): no dot after the first look's words.  o_dots put a dot each 240 ticks while
the start was busy, through to the character's first ready; whether one fell between the first look's last sentence
and the help's line ("(B) a sconce..") was the ticks' phase alone (dq0starttime's transcript, "A knockout's bearings,
whatever the timing" above).

- **Built:** the first look's words begun is the start's `starting` made 2 (words.c's o_words, overlay 2, where a
  look's words are made: the first one made the first look's); o_dots (overlay 3) puts no dot while it's 2.  The rest
  as it was: "Nearly ready ..." and a dot each second or so till the look (no silence over about 10 s: the longest
  still 24.8M cycles, before the screen's made), the help's line with the first prompt (o_dots's k 1 once the
  character's ready, `starting` 0 again).  The dots' line ended before the look's words, as before (the look begins
  a new line where the story's column isn't 0: "Nearly ready ... . . .", then "You are at ...").
- **Checked:** dq0starttime with its first dq0 line put later (the runner's waits, 2M cycles each, 1 to 8 of them;
  12 spaces): before, a dot after "(B) a sconce." at 7 waits (14M cycles), none at the others; now none in any, nor
  as the suite runs it.  Seed 123456789's game had none either way.  The start: seed 1 232.6M -> 232.1M-233.5M over
  the variants, 123456789 308.0M -> 308.1M-309.9M (timing; budgets 245M and 325M).
- **What the player sees differently:** no "." after the first look's last sentence; the dots before it as they were.
- **Room:** dq0's main RAM 3,338 -> 3,338 free (both changes in overlays); overlay 2 42 -> 32, overlay 3 2,073 -> 2,066.
- **Transcripts** (the suite's, by sentence, sent.js), against 24e9bf99's: 7 differ, all the known noise (dq0answer,
  dq0jobs, dq0passes, dq0prompt, dq0skykin: denizens met at other moments, "Something stirs."; dq0antidote's real-time
  fight; dq0engage's last fight, the open question).  dq0starttime the same; its dot's timing gone, it's off the noise
  list.  The suite: all 447 pass.

<!-- DECISIONS.md:5355-5449 at d35194df -->
## Town prices, local (the user's, 2026-11-13; as built, for review)

**The user's words:** "Town prices should be determined by local availability, loosely based on what the ruleset dictates (so
if there is lots of local competition, maybe the range of prices is larger than you would get in a smaller market where the
range is limited)."  ("Four answers" above.)

**The base, the ruleset's** (rsmeta.h's new `meta_price[NMP]`, in each ruleset's test.c, the one file both the town and the
scholar's house link): MP_SCRIBE a scribe's working day 3d, MP_DEAD a dead tongue's sage so many times that (3), MP_MASTER a
master's month 84d, MP_SAGE a sage's day asked one of the other skills 3d, MP_LIBRARY the library's day 1d.  HarnMaster's the
book's figures (Character 13), as they were hard-coded in schol.c; the null's the same (it has no economy of its own).  The
market's goods keep their base in the catalogue (the world's worth, dq0w.cat) and the smith's mending and whetting the account's
(goods.h): neither is the ruleset's yet, and the account itself (straight on: its selling, restocking, mending) prices everything
at the base as before.  (A ruleset scale for the goods, a percentage in meta_price, would be a few bytes; not built: nothing
asks for it yet.)

**The local price** (games/dq0/town/price.h, compiled into both programs):

- **How many sellers:** a big town (town.txt's kind, as before: the sage for a dead tongue) has 4 to 6 of each trade (traders,
  smiths, scribes, sages, masters), any other 1 to 3; the figure by the life's seed and the dungeon's number.
- **Each one's price:** its hundredths of the base, drawn round 100 by the same, five either way for each seller of its kind in
  the town: one 95-105, two 90-110, three 85-115, six 70-130.  So a big market's prices go wide (shopping round pays), a small
  one's stay near the base.
- **Which one:** one of each is at hand a day (by the town's days so far): another day, maybe another.  Shopping round is coming
  back another day, each visit a day and its keep, as ever.  No new command and no new question.
- **Its own numbers:** a 16-bit mix (high bits folded into low and multiplied, twice) of the trade, the seller or the day, the
  life's seed and the dungeon's number: never the dice, so every roll of the town's and the scholar's house's (haggles,
  listening, readings, names, honesty) is where it was.
- **The market and the smith:** "Today you deal with the second of Velmor's four traders." (or "... smiths.") as the place is
  entered, only where there's more than one (town.txt's new keys nth and count hold the words).  A thing's price is its worth (one
  of it, to the farthing) at the day's trader's hundredths, then times how many, haggled as before, rounded to the penny as before;
  the smith's mending and whetting the account's price at the day's smith's.  **Never under 85 hundredths** (the town's alone):
  what the market gives for a thing is still the account's half its worth by its condition, at most 63 hundredths of the worth
  haggled, so a trader's price haggled (75 of its own) stays over it at 85 and nothing's made buying of one and selling to
  another.  A big town's buying range is so 85-130, not 70-130; the smiths are held to it too (one rule for the town).
- **The scholar's house:** a scribe's day, a dead tongue's sage's, a master's month and a sage's day for lore are the base at the
  day's scribe's, sage's or master's hundredths, to the penny, a penny at least; dear ones half again of that, as before.  The
  library's day its base alone (one library); its words keep "a penny a day" when the base is a penny.
- **Every figure the player sees from the base:** the market's list, "The price of ...", the smith's list, what's paid; a scribe's
  or sage's "asks ...", "A master asks ...", "(83d)" after a master's month, a lore sage's "(3d)".

**What a player sees differently:** the figures (a lantern 13d where it was 12d; in a big town anywhere from 11d to 16d, by the
day's trader), a torch or a sack priced in farthings when the day's trader's under 100 hundredths ("a torch 3f"), and the one new
sentence above.  And what follows from the money: an errand that couldn't be paid for before can be now, or the other way round
("A master asks 97d for a month's teaching: you haven't the money."), and what comes after it (the days it would have taken, so
the next visit's words; what's left for the account's living, work and practice; "You have no money" or not).

**Checked:**

- **No roll moved** (the flat build: price.h with every seller at 100 and one of each): towncmp's 28 lives (56 times between,
  112 scholar's houses run alone) byte for byte the same as before; and the 13 tests of the time between (dq0town,
  dq0scholar, dq0tongues, dq0reading, dq0account, dq0acctgone, dq0nullbetween, dq0nulltown, dq0life, dq0lifeend,
  dq0dressbetween, dq0nosave, dq0hardon) the same sentence for sentence.
- **As built:** the same 28 lives, prices local: of the 56 times between (53 begun on the same files, 3 on files that
  differed in the money alone), 12 the same byte for byte and 36 the same but for the figures (amounts masked, the new sentence cut, the files' dumps aside); the 8 others, and 7 of the
  112 scholar's houses, differ only after a price crossed what the money held (a master's month or a sage's reading paid for in
  one and not the other, a lore sage asked or not) or in the money's own words ("You have no money." / "You have 3 pennies.";
  "1 penny" / "2 pennies"), and in what that changed next (the days in town, so the scholar's house's greeting by its dice).  The
  JS replica of price.h (scratch spread.js) gives the same sellers and days as the transcripts (seed 3's Leond, two smiths, the
  first at 99: its shortsword 89d where it was 90d; seed 70000's second dungeon's small town in dq0town and dq0nulltown, one of each: the
  lantern 13d, the master 83d).
- **The tests:** dq0scholar and dq0nulltown named the base's figures; now this town's (seed 70000, its dungeon's): a big town's
  sage 10d a day (was 9d), a dear one 15d (was 14d), the null's sage 10d a day (60 farthings a day dear, was 56), a master's
  month 83d (was 84d); their descriptions say so.  dq0town's market checks (three torches at the listed price, the whetstone sold
  at half its worth) pass unchanged.  The suite: all 447 pass.
- **Example towns** (spread.js: seed, dungeon; each seller's hundredths; a lantern's base 12d):
  - seed 70000, dungeon 2, small, one of each: trader 105 (a lantern 13d); smith 99; scribe 3d; sage 9d (lore 3d); master 83d.
  - seed 3, dungeon 2, small, two: traders 98, 104 (12d, 13d); smiths 99, 110; scribes 3d, 3d; sages 9d, 9d; masters 80d, 89d.
  - seed 3, dungeon 1, small, three: traders 110, 85, 114 (14d, 11d, 14d); smiths 89, 86, 90; scribes 3d each; sages 10d, 9d, 9d;
    masters 92d, 97d, 85d.
  - seed 123456, dungeon 2, big, four: traders 103, 83 (held to 85), 119, 110 (13d, 11d, 15d, 14d); smiths 106, 118, 93, 119;
    scribes 3d, 4d, 3d, 3d; sages 8d, 8d, 10d, 8d; masters 91d, 91d, 81d, 77d.
  - seed 70000, dungeon 1, big, six: traders 101, 99, 130, 103, 75 (85), 90 (12d, 12d, 16d, 13d, 11d, 11d); smiths 70 (85), 83
    (85), 115, 87, 103, 107; scribes 3d, 4d, 3d, 4d, 4d, 2d; sages 11d, 10d, 7d, 12d, 11d, 9d (lore 4d, 3d, 2d, 4d, 4d, 3d);
    masters 101d, 104d, 109d, 79d, 105d, 100d.
- **Room:** dq0town's main RAM 3,223 -> 2,883 (its overlay 6,443 -> 6,172: price.h's code there), dq0ntown's 3,118 -> 2,778;
  dq0schol's 4,981 -> 4,618 (its overlay 665 -> 489), dq0nschol's 9,713 -> 9,350.  No other task's.

**For review / open questions:**

- *What the market gives* for a thing is still the account's (half its worth by its condition) at every trader: varying it by
  trader too would make buying of one and selling to another pay unless the spread is held to about a tenth.  Built the
  conservative way (buying varies, selling doesn't, buying never under 85).  The user may want selling local too (with the spread
  narrower, or a trader buying and selling at one margin), or the floor gone.
- *The account's own prices* (the character that goes straight on, or what the player left: selling, restocking, mending) are
  the base, not the town's: the account runs before the town is made (its kind, its size) and has no room to spare.  The same
  town so prices the same thing two ways depending on whether the player went to the market.
- *More competition, wider prices* is the user's figure; it isn't cheaper on average (each seller's centred on the base).  A big
  town's *cheapest* is lower, its dearest higher.
- *One seller a day, not a choice:* the player can't pick a seller by name; shopping round is visiting again on another day.  A
  named choice ("buy from Hethin") would want each seller's name (the name maker's, by its own numbers) and words to choose by.
- *How many of each trade* is one figure for the town (six smiths in a big town as well as six traders); the town's kind gives
  only big or not.  A size per kind (town.txt's kind lines given a figure) would be the next step.
- *The base for goods and the smith* stays the world's and the account's; a ruleset with its own economy would want meta_price
  to scale them too (one more entry each).

<!-- DECISIONS.md:5450-5512 at d35194df -->
## A denizen's blow at its time (the user's, 2026-11-13; as built, for review)

The user's answer ("Four answers" above): the world takes a denizen's blow at its intent's time, not at its own clock
as it gets round to it, so a fight doesn't depend on the passes' timing; some fights play out differently.

- **What was stamped with the clock.**  A denizen decides as its ready comes (dq0d: `q->ready <= now`), and the world
  carries its intent out when it gets to it, at the turns pace up to a slice (TURN_SLICE, ten seconds) later.  Its
  blow's moment is its ready as it's taken (wbody.c's I_ATTACK, before its round is put on it); the clock was used
  instead in four places: the referee's (dq0harn's tnow: the character's black-out, pc_out, a creature's, bs.out; a
  defence once a round, dbt[]; the round's, gear.c's rnd_at), the character's free moment as it's knocked senseless
  (wfight.c's `pc_ready = now + w2`), a denizen's as it is (`bd.ready = now + w2`), and a denizen's shot not taken, its
  round given back (wshoot.c's den_shot, `bd.ready = now`).
- **Built.**  A blow's moment (wfight.c's `blow_t`, the main RAM's): s_den_blow takes it as part 14 calls it, body
  bd's ready less the round it put on it; the engage test's blow first (FW_FIRST) the clock's; a missile's (AIM_FIRE)
  the shot's, as it was shot.  blow() tells the referee: msgs.def's RS_ATTACK `when` (x[4-7], in every form; wblow.c's
  blows at a thing 0), and dq0harn's tnow is it for RS_ATTACK, else the clock.  Out till its moment and the tenths
  out: the character's pc_ready (raised only, as before) and a denizen's ready; den_shot's round given back to its
  moment.
  - **The clamp.**  Never after the clock: an intent's ready is never past the clock the minds decided at, and the
    clock never goes back; if it were, the clock's.  Before it: allowed.  A black-out the world got to after it was
    over leaves the character free at once (pc_ready never set below what it was), its bearings still tested (pc_ko,
    "A knockout's bearings" above).  The character's own blows, and a blow at a thing, the clock's.
  - **Left the clock's.**  The blow's events' times (E_FIGHT's t: the rings in order, the UI's and the minds' readings of
    them as they were); a blow stopping the character's long task (stop's `pc_ready = now`: the clock can't go back for
    the work done); a rouse (I_ROUSE, `now + tenths`: the minds' "from now" at their own clock, and the intent carries no
    time of its own, im.t only the last I_DONE's); the engage test's blow first; the null referee (no black-out clock of
    its own; its shield once a round by the clock).
- **Checked: dq0engage's last game**, at 0, 2M, 4M, 6M cycles before its "d" and a space (the knockout work's
  variants).  Before: 0, 2M and 4M the short fight (its rise t503), 6M and the space the long (t603).  After: the
  knockout the same in all five (the ghoul's blow meant at t337, taken at t403: out till t437 by the world and the
  referee alike), but the fight still splits, by another cause: the "wait" typed ahead.  Typed while it lies senseless,
  it's taken at once (2M, 4M, the space: a round's wait from t403, the black-out within it, its rise t503, the short
  fight); typed later, as it comes round (0, 6M: from t437, its rise t537, the long one).  The blow's timing-free; what's
  left is the typed line's ("Tried, not built" below: open, the user's).
- **dq0antidote and dq0denpace** (the real-time fights the noise list names), at 0, 2M, 4M, 6M cycles before a line
  and a space before (antidote's "south", denpace's first "wait 1 minute"), at 0, 6M and a space after.
  dq0antidote: the same in every variant, before and after.  dq0denpace: before, its 6M variant met no bandit at all
  (the others a fight from t400); after, every variant the same.  Both still differ from build to build in the suite:
  dq0denpace by the bandits' order of deciding at t100 (before any blow), dq0antidote by its stop a round sooner
  (t360).  They stay on the noise list.
- **Tried, not built: a command typed while senseless taken as it comes round.**  A latch set by a blow's knockout at
  the turns pace (dq0w.c's main: no intent taken while it's set; turns_run going on with one waiting; cleared as the
  character's free).  dq0engage's five variants then one game, dq0denpace's and dq0antidote's too; but the typed-ahead
  lines meant something else.  dq0helpless's waits, typed as the adders bit, began after the black-out, not in it, and
  the character bled to death; and as it died senseless the latch never cleared (no turn for the dead), so dq0leisure,
  dq0disarm, dq0denportc and dq0deadonce hung at their quit.  A change in what the player's lines do, not the blow's
  time: the user's to decide.
- **What the player sees differently: fights.**  The referee judges a blow at its moment, so its round's bookkeeping (a
  defence once a round) falls otherwise and the dice after it: dq0venom and dq0antidote (cave adders: a dodge for a
  block at a bite meant at t147, the fight after it another), dq0features (giant rats: no "Everything goes black." now,
  the fight longer), dq0dulled (giant rats, from t1522), dq0engage's last game (the ghoul's: comes round at t437, the
  long fight: no "a serious gash to its right knee (S2).", "a minor cut to its left foot (M1)." and "You strike at the
  ghoul with your sword, but stumble." instead, more dodges and openings).  No expected string changed: every check as
  it was.
- **Transcripts** (the suite's, by sentence, the copy of sent.js), against 24e9bf99's: 19 differ.  The fights above (5,
  and dq0denpace's).  Denizens met at other moments, their fights the same blow for blow (the minds' order of deciding,
  before any blow): dq0kit, dq0lair, dq0strip, dq0wornden and dq0denportal (the bandits at t151), dq0deadonce,
  dq0faced, dq0jobs, dq0prompt, dq0stopwords.  A stop's race: dq0reachstop (the known one), dq0tiring ("You stop
  digging." a pass the other side of a denizen's coming, t15120).  dq0rules: a prompt.
- **Room:** dq0w's main RAM 2,083 -> 2,079 (blow_t); part 15 310 -> 124, part 6 82 -> 55, part 67 1,525 -> 1,482; the
  packer's overlays 3 237 -> 210, 14 515 -> 329, 20 101 -> 58.  dq0harn's main RAM 2,189 -> 2,123 (its tnow).  The
  start: seed 1 232.6M -> 232.7M, 123456789 308.0M -> 308.6M (timing).  The suite: all 447 pass.

<!-- DECISIONS.md:5520-5581 at d35194df -->
## Commands while knocked out (the user's, 2026-11-13; as built, for review)

The user's decision ("Commands while knocked out: not taken" above): while the character lies senseless no line is
queued to be carried out as it comes round; lines typed meanwhile, and lines waiting as it's knocked out, go.

- **What it was.**  The UI answered a knockout told (a blow's, blows.c's o_blow) with a wait of its own, the black-out's
  tenths (`I_WAIT`, quiet); the world took every intent as it came, whatever the character's state (dq0w.c's intent:
  `busy (t)`, pc_ready from its own clock).  So the black-out ended when the UI's wait reached the world plus its
  tenths, not at the blow's moment plus them: dq0engage's last fight split by it (2M, 4M cycles: the wait taken at t403,
  the rise t503; 0, 6M, a space: taken at t437 as the turns pace had moved the clock on, the rise t537), not by the
  typed "wait" as "A denizen's blow at its time" thought.  A line typed in the black-out, or a line the UI sent in the
  moment between the world's knockout and the UI's telling of it, was taken at once (a step, a wait: pc_ready set
  afresh, the black-out cut short or stretched).  And a knockout during a long task (a walk, a wait, a search) was cut
  short by the blow's own hard stop (stop's and halt's `pc_ready = now`), the UI's wait alone keeping it down: a prompt
  could come while it lay OUT (dq0deadonce's "kick the orc" was typed at one, and carried out: "You struggle, but can't
  get up.").
- **Built: the world's.**  The hub's `pc_out` (hub.h, the header's last byte; the world's: owners' table): the
  character lies senseless.  Set as a blow knocks it out (wfight.c, with pc_ko, the black-out from the blow's moment as
  before, and pc_waited: its eyes and the time it believes looked to as it comes round, as the UI's wait's end did) and
  as a harm does (bearings_off's harm path, W_SLOT.w2: a fall's, a trap's, the roof's).  While it's set: no intent of the
  UI's is taken but I_QUIT (main's loop: dropped, the UI's E_READY still coming as it's free, was_busy); a hard stop
  (stop, halt: one helper, free_now) leaves pc_ready where the black-out put it.  Cleared as the character is free again
  (main's free check, which runs for it too, `|| pc_out`: its E_READY as it comes round, though nothing was sent).
- **Built: the UI's.**  No wait sent for a knockout: busy till the world's E_READY (blows.c, if the world has it out
  still: else it's come round already).  `out_told` (main.c), set as the knockout's told (a blow's, a harm's E_OUT): while
  the world has it out, each line entered (or waiting from before, held or behind) is taken and dropped, the parser's
  reply with it, and refused once (main.c's out_line, part 2's): said.txt's new NO row, "You lie senseless." (say_no's
  A_OUT).  So the refusal comes after "Everything goes black.", never before it.  A sound heard as it lies senseless
  asks the world nothing (the long task's I_ASK: dropped anyway).
- **Taken while senseless:** "quit" (held till it comes round, as any line typed as it's busy: so the game isn't saved
  mid black-out, which a game carried on would forget: the save has no black-out; dead first, the game's end as ever)
  and "help" (at once).  Everything else refused, the things that take no time too (what's carried, the sheet, the map,
  the time, a look about, the state, the pace): it knows nothing as it lies there.  For review.
- **Death while senseless** ends the game as any death (o_ending, quit; the world's I_QUIT taken whatever pc_out says):
  dq0leisure, dq0deadonce (now dead before its third line: see below) and dq0outcold's second game.
- **dq0engage's last game**, at 0, 2M, 4M, 6M cycles before its "d" and a space (the knockout work's variants): one game
  in all five, sentence for sentence and blow for blow (knocked out at t403, the blow meant at t337: comes round at t437,
  the rise then, its blow at t537).  Its last fight no longer splits by timing: dq0engage can leave the noise list (its
  rat game's sightings differed from the last build's once, as any denizens met: not between the variants).
- **Tests.**  New: dq0outcold (dq0senseless's rats, "drop the torch" typed at "Everything goes black.": refused, no
  intent, the torch still in hand at the quit, the waits after it taken; dq0deadonce's orcs, "kick the orc" typed at
  its black-out: refused, the character dead senseless, the end told once, the shell back).  dq0deadonce: its "kick
  the orc" gone (typed at a prompt that came as it lay OUT; none comes now till it comes round, and it dies first: the
  line went to the shell, "rc: kick: not found"); every check as it was.  No other input or expected string changed:
  no other test types a line in a black-out now (their lines typed at the prompt, which comes only as it comes round).
- **What the player sees differently.**  "You lie senseless." after a line typed as it lies senseless, and no prompt
  while it's OUT.  Fights after a knockout: the black-out ends at the blow's moment and its tenths, not as the UI's
  wait came (dq0longwait: one black-out, then "You're badly hurt. Keep fighting?", "You stop fighting." for "Everything
  goes black again." and "You're in no state to fight."; dq0deadonce: dead senseless).
- **Transcripts** (the suite's, by sentence, sent.js), against abab4e9e's: 19 differ.  Intended: dq0deadonce (dead senseless, its third line gone), dq0longwait (the fight after its black-out).  After a black-out now of the blow's length, denizens seen or heard at other moments, no blow or word of the character's otherwise: dq0battering, dq0bearing, dq0disarm, dq0hidelook, dq0hoard, dq0robbed, dq0senseless, dq0spiderbite, dq0throws; dq0features (a rat seen, not heard).  dq0engage: its rat game's "watching you" wrapped at another place (the same sentences).  The known noise: dq0antidote, dq0denpace, dq0faced, dq0jobs, dq0skykin, dq0sway.  "You lie senseless." in no test but dq0outcold: no other types a line as it lies out.
- **Room:** dq0's main RAM 3,338 -> 3,314 (out_told, the gate's call, out_line's stub); its part 2 404 -> 333 (out_line),
  11 2,350 -> 2,322, 14 1,637 -> 1,632; overlay 6 2,253 -> 2,227 (o_blow); the packer moved part images: overlays 14
  1,544 -> 1,473, 15 3,694 -> 335, 18 752 -> 747, 24 342 -> 3,673, 27 175 -> 159, 28 3 -> 0, 29 1,540 -> 1,545.  dq0w's
  main RAM 2,079 -> 2,059 (the intents' guard, the free check's); part 15 124 -> 118, 23 6 -> 18 (free_now), 52 2,592 ->
  2,589; overlays 11 183 -> 195, 14 329 -> 323, 16 765 -> 762.  The start: seed 1 232.6M -> 232.5M, 123456789 309.2M ->
  309.5M (timing).  The suite: all 448 pass.
- **Open, for review.**  (1) The refusal's words, "You lie senseless.", or silence (the user's "ignore input").  (2)
  What's taken while senseless: quit and help only (the sheet, what's carried, the map refused).  (3) Lines waiting as it's
  knocked out refused with the same words (after "Everything goes black."), not dropped unsaid.  (4) A line the UI sent
  in the moment before it learnt of the knockout is dropped by the world unanswered (its echo, then the black-out's words,
  no refusal).  (5) It still hears as it lies senseless (sounds told, a denizen stirring seen: as before).
