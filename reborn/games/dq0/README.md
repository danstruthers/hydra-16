# DQ0: Dingus Quest Zero

DQ0 is a text adventure for the Hydra-16, on HydraOS. It is told in prose on an 80 by 24 serial terminal, in a
dungeon made at random from a seed, in three dimensions, with a theme that says why you came and what you're after.
At the real-time paces the clock doesn't wait: the denizens move while the player types. A turns pace (`dq0 -p t`)
moves the world only as the character acts, and the tests use it. The rules are HarnMaster 3.5's, asked of a referee
that another ruleset can replace.

The user's brief (DECISIONS.md's "The game", 2026-10-14): "a text adventure game, something like Zork or Adventure,
but with more depth and completely random dungeons, similar to NetHack ... somewhat realtime". The name is "Dingus
Quest Zero, which is in preparation for the bigger game, Dingus Quest", so the program is `dq0`.

This file is a newcomer's map:
- the tasks, and how they talk;
- the data files;
- how to build, run and test;
- a glossary;
- how to add a verb, a sentence, a kind of thing, a fixture and a denizen kind.

Paths are relative to reborn/games, and a bare file name is games/dq0's own; build.js, sim/, tests/, tools/ and
modules/ are relative to reborn/.
A file:line is the line at the time of writing (2026-11-17, commit a46c5c38); lines move, so search for the name
given beside it.

**The other documents, in this folder:**
- **DUNGEON.md**: the dungeon as it stands: strata, chunks, the maker, the world, the save.
- **DENIZENS.md**: the theme and the denizens: minds, factions, morale, the day.
- **SOUND.md**: what's heard, and how the chips play it.
- **HISTORY.md**: where a dungeon comes from: a region's history, and the theme's format.
- **HOUSE.md**: the house rules: every rule the game makes where HarnMaster is silent, with its numbers and where the
  code applies it. Its last two sections are what's decided but not built, and what's open.
- **RULESETS.md**: the referee's interface (rs.h) and the two rulesets.
- **DECISIONS.md**: the user's decisions, dated. Before 2026-11-17 it also took the "as built, for review" reports;
  new ones go in built/, one file a branch.
- **ENGINE-CRITIQUE.md, COMMANDS-CRITIQUE.md, GAME-CRITIQUE.md**: critiques and plans. GAME-CRITIQUE.md's section 10
  is the current order of work.
- **OPEN.md**: every open question, in one place.
- **AGENTS.md**: the working rules for anyone, person or agent, who changes the game.
- **../../tests/TESTING.md**: the test tiers and how to pick the tests a change can affect.

---

## 1. The tasks

A game runs as eight tasks at once, each with its own main RAM and private banks, sharing the hub (section 2).
Their roles are hub.h's `enum { T_UI, T_WORLD, T_DENIZENS, T_REFEREE, T_SOUND, T_MAKER, T_SENSES, T_COMMANDS }`
(hub.h:317). There are nine programs, since the referee is one of two.

- **dq0, the UI** (dq0/main.c; C with some assembly; 30 overlays, 29 swapped parts).
  - The screen, the keys and the words.
  - A typed line goes to dq0c and comes back as a struct cmd; the UI resolves what it names and sends an intent to
    the world, or answers itself.
  - It tells the world's events as the story, in said.txt's words.
  - It makes the hub and starts every other task.
  - It never reads the dungeon, only what the senses report (ENGINE-CRITIQUE.md:38).
- **dq0c, the grammar** (dq0c/dq0c.s; 65C02 assembly, no C runtime; no overlays or parts).
  - cmdmatch.s says whether the line is in the grammar: yes or no.
  - cmdmean.s scans the words for their meaning and builds the struct cmd.
  - Its tables are dq0c.dat, built from the commands.* files. It answers the hub's cq request.
- **dq0w, the world** (dq0w/dq0w.c; C with some assembly; 32 overlay banks and 4 data banks; 109 swapped parts).
  - The game's clock.
  - The intents carried out, and the results told as events.
  - The dungeon's change records; doors, portals and traps; the item store; the actors' bodies; every blow.
- **dq0d, the minds** (dq0d/dq0d.c; C with some assembly; 7 overlays, 24 parts).
  - The denizens: each stratum's population, placed; the nearby actors' decisions, posted as intents; attitudes.
  - Not to be confused with dq0/minds.c, which is the UI's part 28 (say, knock, sway).
- **dq0p, the senses** (dq0p/dq0p.c, with view.c, sight.c, eyes.c, light.c, sense.c and sound.c; 4 overlays, 6 parts).
  - The look for the character, and each denizen's senses.
  - The light and sound layers, and the memory of what's been seen.
- **dq0m, the maker** (dq0m/dq0m.c, with plan.c, make.c, built.c, pass.c and caves.c; 9 overlays, 3 parts).
  - Generation only. It grows a stratum's plan and its chunks on request: the same seed always makes the same chunk.
  - It lists a new chunk's things (its stock) and draws the map.
- **dq0s, the sound** (dq0s/dq0s.c; C; no overlays or parts).
  - It puts the sounds' table in the hub, then plays what the character hears on the YM2151 and the Vera's PSG.
  - With no sound driver it exits once the table is in, and the game goes on silent.
- **dq0harn, the HarnMaster referee** (dq0harn/dq0harn.c; C, on the C SDK's harn.h; 9 overlays, 4 parts).
  - It answers rs.h's requests by HarnMaster 3.5, and holds the sheets. The default ruleset.
- **dq0null, the null referee** (dq0null/dq0null.c; C; everything in main RAM).
  - Flat odds (60%, and 5 more a step) and no skills. It proves that the game asks a ruleset nothing beyond rs.h.
  - `dq0 -r null` runs it.

**How they start** (main.c's start_world, main.c:653-744):
1. The UI makes the hub (hub_create).
2. It starts dq0c, then dq0s, then the referee (`dq0` and the `-r` name, default harn), and asks the referee's
   RS_HELLO: its round, its character generator, its words.
3. hmchar makes the character, unless `-c FILE` gave one.
4. It starts dq0m, dq0p and dq0w, and sends the world WQ_START.
5. It starts dq0d last, then asks the world's IO_BEGIN.

Each task is started with the hub's segment, its role and the lib folder (hub.c:315-334).

**The other programs** in reborn/games:
- **dq0life** keeps a life's record (life.dat). `dq0 -d DIR` runs it when no game is going in the folder; it runs
  themegen's rumours, the account and dq0end in turn.
- **dq0end** tells the life's account at a death.
- **themegen** makes a region's history and theme from a seed, with the bank programs in reborn/bank (histgen,
  goalgen, linegen, namegen, stratagen). It writes theme.txt, then runs themec. dq0 runs it for a seed of 65,536 or
  more, or with `-t FILE`.
- **themec** is the one theme compiler and checker: every goal reachable, every faction placed. themegen runs it, and
  so do the build (tools/themec.js) and the tests.
- **hmchar** is the HarnMaster character generator, on its own or run by dq0.
- **On hold** (the between-dungeons work, GAME-CRITIQUE.md:31): dq0acct and dq0nacct (the account of the time
  between, HarnMaster's and the null ruleset's), dq0town and dq0ntown (the town), dq0schol and dq0nschol (the
  scholar's house).

## 2. How the tasks talk

### The hub

- **What it is.** A shared segment of 26 banks of 8K (HUB_BANKS, hub.h:77). The UI makes it; every other task
  attaches to it and checks its magic "DQ" and its HUB_VERSION (hub.h:28-29).
- **Bank 0** holds struct hub:
  - a doorbell for each task (a counting semaphore it waits on);
  - the clock;
  - the request slots;
  - the actors' records;
  - three rings: iq (the UI's intents for the world, 16 entries), eq (the world's events, 128, each reader with its
    own place) and di (the denizens' intents, 8) (hub.h:1262-1264).
- **The header's size.** It must leave 2K of bank 0 free: `hub_fits` (hub.h:2096-2097) fails the compile if it
  doesn't. It is 5,874 bytes (GAME-CRITIQUE.md, section 6.5).
- **The other banks** (hub.h:77-117, 268-313): the plans; the chunks' cache; the kinds (KIND_BANK); the items;
  DEN_BANK (the denizens' events, the sounds, the fights); LOOK; THEME; SIGHT; UI; WORLD; the change pages; CMD; CAT
  (the catalogue's code and words, the world's alone).
- **One writer for each region.** hub.h's "Who writes what" (from hub.h:1836) is a table of lines
  `own REGION OWNER [+TASK ...]`.
  - hubgen.js turns each region into byte ranges in obj/gen/dq0hub.json.
  - sim/lib/hubown.js checks every write to the hub in every dq0 test. `HUBOWN=1` lists every violation;
    `HUBOWN=0` turns the check off.
  - Another task changes a region by asking its owner (an intent, a request), never by writing it.
- **Rings.** Each ring has one writer and one reader, so nothing needs a lock: an entry is written whole, then its
  index (hub.h:9-12).
- **Requests** (hub.h:933-944):
  1. The asker writes the owner's slot and posts it (REQ_POSTED; the owner's doorbell rung).
  2. The owner replies (REQ_DONE; the asker's doorbell rung).
  3. The asker takes the reply and frees the slot (REQ_FREE).
- **The slots:**
  - the referee's three: SLOT_WORLD, SLOT_UI and the denizens' dslot;
  - the maker's mreq;
  - the things' ireq (IO_*);
  - the look (LK_*);
  - the world's wreq (WQ_*);
  - the parser's cq (CQ_*);
  - sway, dsave and mapq.
- **The calls.** `hub_map (b)` puts hub bank b in the task's 8K window at $8000. `hub_call (q, t)` posts a request
  and waits for its reply. Every task compiles dq0/hub.c through a small hub.c of its own.

### msgs.def: the messages

- msgs.def gives the payload of every message kind: where its fields sit in struct msg's and struct req's slots.
- hubgen.js makes obj/gen/dq0msg.h from it (accessors and packers), and the decoders in dq0hub.json that the tests
  use.
- **Four groups:**
  - **intents**, I_*: what an actor means to do. The UI sends the character's; the minds send the denizens'.
  - **events**, E_*: what happened, from the world. The outcome events share E_OUTCOME's head.
  - **the UI's own kinds**, UK_*: never in the hub, but worded by said.txt as the events are.
  - **the referee's requests**, RS_*, written `request -> reply`.
- **An example** (msgs.def:54): `I_WAIT a how; w1 tenths; w2(busy) task:8-15@LT_NONE what:1-7 quiet:0`. It makes
  `I_WAIT_TASK (m)` to read the long task's kind, and `I_WAIT_W2 (task, what, quiet)` to pack w2.
- Some enums are kept by hand in the headers, not in msgs.def: the things' requests IO_* (items.h), the door and
  portal actions PA_* (hub.h:860), WQ_*, CQ_*, LK_*. hubgen.js reads them all from the headers.

### records.def, and the record store

- **records.def** declares the save records the between-dungeons programs share (records.def:28, 100, 122):
  - between.dat (the account, the town, the scholar's house);
  - life.dat (the life's record);
  - clues.dat.

  hubgen.js makes obj/gen/dq0rec.h, and the layouts in dq0hub.json.
- **The record store** is a different thing. It is one paging scheme for every store kept by region:
  - its calls are rp.h's RP_*, into the paged ROM library;
  - its stores (rp.h:4-8) are the change records (dq0w/store.c, wfile.c), the strata (dq0w/sfile.c), the traces
    (dq0w/wtrace.c), the things (dq0w/ipage.c), the denizens (dq0d/dq0d.c), the seen maps and plans (dq0m/changes.c),
    and the hub's chunks' cache;
  - a stored page's file is named by its store and region (preSXXYY.dat), and each write is a new version.

### The paged ROM library (dq0lib)

- **What it is.** modules/dq0lib: assembly in the paged ROM. It runs in the calling task, by an XCALL (about 120
  cycles), and has no RAM of its own. Its jump table is at $A030 (modules/dq0lib/dq0lib.s:33).
- **What's in it:**
  - the overlay loader (OVL_LOAD: a task's .ovl read and unpacked into its banks), and NUM;
  - the record store (RP_*);
  - the chunk lookup (AT_FIND, CK_MADE, CK_AT);
  - the one line of sight (LN_LINE, LN_PASS, LN_PORTAL);
  - the one way-finder (WK_ROW, WK_SEED, WK_SEARCH).
- **How a task calls it.** Its bank.s includes dq0/dq0lib.inc; C code uses dq0/lnzp.h's macros, and rp_x for the
  store.
- **The zero page.** A program's zero page is $22-$7F. $44-$70 is the library's fixed block (lnzp.inc:15, 50), which
  keeps its state across calls.
  - The tasks that read the chunks (the world, the minds and the senses) end their own zero page at $43
    (dq0w/dq0w.cfg:18, dq0d/dq0d.cfg:15, dq0p/dq0p.cfg:20). One more zero-page variable of theirs fails their link:
    that is the check (DECISIONS.md:5778-5780).
  - The UI, the maker and the HarnMaster referee keep $22-$7F.

### The referee

- **rs.h** is the ruleset-neutral contract: a purpose (P_*), the circumstances in steps of 5%, a grade (G_BADLY to
  G_WELL), and effects and words back. RULESETS.md tables each request: who asks it and what's read. An unknown
  request gets 0xFF.
- **The skeleton** is dq0/rf/, with referee.h its interface:
  - rf.c, the main loop: it attaches the hub, seeds the dice from the game's seed (a stream for each asker), and
    answers the world's, the UI's and the denizens' slots;
  - sheets.c reads the creatures' sheets (beasts.dat);
  - the house rules every ruleset shares (rshouse.h): dig.c (digging), notice.c (noticing), state.c (the state's
    words), bdice.c (the bearings' dice), want.c (hunger and thirst) and air.c (bad air).
- **dq0harn** puts the skeleton in its main RAM and the house rules in its parts and overlays (RULESETS.md:84-86).
  **dq0null** puts them all in main RAM.
- **A ruleset supplies:**
  - rf_start, rf_begin, rf_answer and rf_sheets (referee.h:53-60);
  - want_say and air_say (rshouse.h);
  - an answer to every RS_* request;
  - for the time between dungeons, an entry in rsmeta.h's RULESETS, and its acct_* and meta_* hooks;
  - optionally, a character generator named in its RS_HELLO (HarnMaster's is hmchar), and an outfits.txt.
- `dq0 -r NAME` starts the referee dq0NAME (main.c:2931-2933).

### The paces

- **relaxed, normal, menacing** (`-p r`, `n`, `m`; PACE_* in dq0.h:88-91). A round takes 5, 3 or 2 real seconds,
  whatever the ruleset's round (dq0w/wgame.c:340). Without `-p`, the title asks for one of these three.
- **turns** (`-p t`). The world moves only as the character acts, a slice of TURN_SLICE (100 tenths) at a time. In
  each slice the denizens take their turn and answer I_DONE, so it's the same game whatever the tasks' timing
  (hub.h:1088-1100). In practice some turns-pace tests still differ between builds (GAME-CRITIQUE.md, section 6.4).
- Not to be confused with the character's walking pace: creep ... sprint.

## 3. The data files

**games/dq0:**
- **said.txt**: the phrase table, the words for every outcome, refusal and thing-sentence.
  - A row is `KIND.outcome[COND] "text"`, with {slots} and @pieces; the format is its own header (said.txt:7-49).
  - saidgen.js packs it at build time into obj/gen/dq0said.h. said.c (overlay 27) renders it, from overlays 28-30.
- **help.txt**: what "help" prints, a paragraph a line. It is copied to the card and read at run time by help.c.
- **The command files**:
  - commands.ebnf, the grammar;
  - commands.verbs, the verbs ([verbs]: a verb and its action by form; [words]: a phrase and its verb);
  - commands.words, the word classes (`door = OB_DOOR`);
  - commands.aliases, the default one-word aliases;
  - commands.actions, the abstract actions (VB_*) with their roles, from which hubgen.js makes obj/gen/actions.h;
  - commands.corpus, 669 test lines: the input, the action expected, its roles, the context and a note.

  dq0c/make.js builds dq0c.dat from them, with sim/tools/commandtables.js and commandwords.js. The corpus is for the
  checks (sim/tools/commandcorpus.js, commandcompile.js).
- **msgs.def, records.def**: section 2.
- **outfits.txt**: the denizens' gear, in [role KEY], [lair KEY], [purse KEY] and [standing N] sections. Its header
  defines the format every outfits file uses. dq0w/outfits.js compiles it into dq0o.dat, after each referee's own.
- **styles.txt**: the maker's style proportions, in named sets (house and open). tools/dq0build.js makes
  styles.dat; a theme may name a set.
- **parts.txt**: the UI's swapped parts, a line each: `N OV what` (section 5: swapped parts). Every task with parts
  has one.
- **themes/1crypt.txt, 2temple.txt, 3warren.txt**: the hand-written stand-in themes. 1crypt.txt documents the theme's
  format. The build compiles them with themec into dq0t.dat. A seed under 65,536 picks one.
- **town/town.txt, scholar.txt**: the town's and the scholar's words (on hold).
- **The shared headers**, which hubgen.js reads (hubgen.js:37): hub.h, rs.h, level.h, items.h (the catalogue's
  types and numbers), theme.h, dq0.h, life.h and dq0w/world.h.

**games/dq0d:**
- **kinds.txt**: the bestiary, 29 kinds, a line each of 36 fields (documented at its top).
- **factions.txt**: the stand-in factions (11).
- **inhabit.txt**: a theme's inhabitants, as faction lines; copied to the card for a game folder's theme.txt.
- **peoples.txt**: the peoples (7): their walls, doors, light, marks, rooms and uses. It makes dq0f.dat, which
  nothing reads at run time yet (GAME-CRITIQUE.md, section 5.4).
- **deities.txt**: the deities (11). Their names go into the catalogue's last two records, for the parser.

dq0d/make.js makes dq0d.dat, dq0tr.dat, dq0f.dat and dq0g.dat from them.

**Elsewhere:**
- **dq0w/kinds.c**: the catalogue, in C. kinds[] holds a row for each kind of thing, comps[] the compartments, and
  throws[] each kind's throwing reach. They are linked into dq0w.cat, which the world reads into the hub's KIND_BANK
  and CAT_BANK at start.
- **dq0s/sounds.txt**: two sounds a denizen kind (a step and a noise), then the world's sounds; it makes dq0s.dat.
- **dq0harn/outfits.txt**: HarnMaster's starting kit, by occupation, skill and bent.
- **themegen/goals.txt** (the six goal kinds) and **history.txt** (the history's words): they make themegen.dat and
  bestiary.dat.
- **hmchar/occupations.txt**: the book's occupations, for hmchar.
- **tests/dq0/looks.ref**: dq0looks' hashes of the senses' replies. `DQ0LOOKS=write` makes it again.

**Generated, never edited by hand** (obj/ isn't in git; the build makes it all):
- obj/gen/actions.h, dq0hub.json, dq0msg.h, dq0rec.h and dq0hub_*.inc (hubgen.js);
- obj/gen/dq0said.h (saidgen.js);
- obj/games/dq0c.dat and dq0cover.txt (the coverage report);
- each task's parts.inc, link config and placed.txt (parts.js, pack.js);
- the .dat files above.

bin/prom*.bin and bin/sdcard.img are build outputs that are kept in git: rebuild and commit them with a change to
what's in them (reborn/.gitignore:1-2).

## 4. Build, run and test

**Build.** `node build.js` in reborn/ builds everything: the BIOS, the modules (dq0lib among them), the games, the SD
card's image and the paged ROM's images. It takes ten minutes or more, so run it in the background.
- Before any game, tools/dq0build.js runs hubgen.js, then saidgen.js, then the styles file (tools/dq0build.js:86-88).
- Then each game folder is compiled, and its make.js run.
- A folder with a parts.txt gets its stubs and link config made by parts.js (build.js:318-325).
- **The floors** (build.js:259): every dq0 task must keep 2,048 bytes of main RAM free (HEADROOM), or the build
  fails; themec and themegen 512. An overlay or a part under 512 bytes free only warns. The UI's own floor of 3,000 is
  a rule the build doesn't check yet (AGENTS.md).
- A failed build leaves obj/build.failed, and the tests won't run on it.

**Run.**
1. `node sim/run.js -i` opens the Hydra's serial console in the emulator (Ctrl-A x quits). The SD card,
   bin/sdcard.img, is in SD device 0.
2. At the shell, `bind -a /sd/0/games /bin`, then `dq0`. Or `/sd/0/games/dq0`, without the bind.
3. The tests start a game this way (dq0answer's, tests/lib/dq0.js:860-861): `hmchar -q -s 5 -u feudal -c hunter -o
   /ram/c.hm`, then `dq0 -p t -s 3 -c /ram/c.hm -T`.

**dq0's switches** (main.c's s_args, main.c:2831-2897):
- **For play:**
  - `-s N`, the seed;
  - `-p r|n|m|t`, the pace;
  - `-c FILE`, a character;
  - `-d DIR`, a save folder (a game kept there goes on);
  - `-H`, hard mode;
  - `-t FILE`, a hand-written theme;
  - `-r NAME`, the ruleset.
- **For tests:**
  - `-T`, start at a way down (more `-T`s go further);
  - `-k THINGS`, extra things to start with;
  - `-a`, soft questions answered yes;
  - `-N`, bearings never tested;
  - `-J`, the reach in one lump;
  - `-E`, a dusk start;
  - `-F`, an outfit not fitted to strength;
  - `-I`, no initiative, groping or facing tests;
  - `-R N`, a dice offset;
  - `-C`, small caches;
  - `-m`, minutes on the status line.
- **The maker's flags** (GE_FLAGS "KWMUAVPD", hub.h:1633-1648): `-K` the old kit, `-W` no water, `-M` mixed strata,
  `-U` no rubble, `-A` diagonal passages, `-V` great caverns, `-P` ramps, `-D` rooms at 45 degrees.
- Case matters: `-p`/`-P`, `-d`/`-D` and `-m`/`-M` are different switches.

**Test.**
- `node sim/test.js` runs all 460 tests: ten minutes or more alone, so run it in the background.
- `node sim/test.js NAME ...` runs some; `-v` shows a test's output.
- `--near AREA`, `--changed REF` and `--tier report|lead` choose the tests a change can affect: tests/TESTING.md.
- The game's tests are tests/suites/dq0/*.js, by subject (map, parser, gear, fight, denizens, senses, body, time,
  save, theme, screen). Their helpers are tests/lib/dq0.js.
- `RULES=null` runs the meaning tests (`meaning: true`: what happened in the hub, not the words) with the null
  referee.
- **The test switches.** The runner adds `-a -I -J -N -K -F -E` to every dq0 line unless a test opts out
  (sim/test.js:176-249). So the tests play a slightly different game from the player's (GAME-CRITIQUE.md, section
  7.1).
- **Transcripts.** A behaviour change is checked sentence by sentence against the transcripts of the build before:
  AGENTS.md.

## 5. Glossary

- **Actions, the dispatch table** (actions.c, overlay 11): a row for each VB_ action, made with ROW, ROWR or ROWP. A
  row gives the handler (H_*), the kinds of thing it takes, its refusals and its request to the world. main.c's
  s_command calls the handler's group by its H_ number.
- **The activity, the step planner** (dq0w/activity.h's struct pc_activity): the character's long task now
  (pc_task), and a stack of what follows it (AK_*), pushed by act_plan and popped by dq0w/dq0w.c's act_next.
- **Bank programs** (.bp): small programs loaded into a RAM bank and called like functions: popgen, histgen and the
  others (sdk/c/include/bankprog.h).
- **The bank window**: $8000-$9FFF in every task. The bank register ($00) chooses what's there: a hub bank, an
  overlay, or one of the task's own banks. A call that moves it must put the hub back for its caller. Nothing checks
  this yet; wrong-bank bugs are the commonest class (GAME-CRITIQUE.md, section 6.1).
- **The book, the house, the user's**: whose a rule is (HOUSE.md:13-16). The book is HarnMaster 3.5; the house is
  the game's own rule where the book is silent; the user's is the user's own decision, whose words are DECISIONS.md's.
- **The catalogue**: the kinds of thing (KI_*, items.h; their rows in dq0w/kinds.c). Kinds 1-119 are the game's own,
  120-127 the theme's own eight, and 128 on (KI_HIGH) the game's again.
- **Change records**: the world's record of every change to the dungeon (a door opened, rock dug), kept by region and
  paged to the save folder (DUNGEON.md).
- **Chunks**: 32 by 32 squares of 5 feet, made on demand from the seed and the place. **The chunks' cache** is 24
  slots in the hub (dq0w/world.h:44-46).
- **Compartments**: every place in, on or hung from a holder: a lid's top, a drawer, a side pocket, a false bottom,
  a secret niche. A row of comps[] each (dq0w/kinds.c; struct comp, items.h:113-154), at most four a kind.
- **The coverage report** (obj/games/dq0cover.txt, made by dq0/make.js): for each verb, a cell for a word and for
  each kind of room part: done by the handler, refused by the row, or left to the handler. It fails nothing.
- **D17**: the user's targets for the answers' speed at 115,200 baud (rethought 2026-11-17, built/2026-11-17-d17.md):
  every command's first letter within 720,000 cycles (a fifth of a second), and a reading's words (a look, the map,
  what's carried, a thing examined) with no pause longer than 1.79M (half a second) (tests/lib/trends.js's D17).
- **Designators**: letters that name things. Lower case for things carried; capitals for things and denizens seen
  (A-Z, then AA-AZ). The player types them in place of a name ("drop d"); "Which one?" offers them (letters.c,
  caps.c, which.c).
- **Events** (E_*): what happened, from the world, onto the UI's ring and the denizens'.
- **Features**: room parts named by kind (OB_*: a door, a window, a portcullis, stairs, a ladder, a shaft ...). One
  path finds them, walks to them and faces them (main.c's s_feature); their handlers are hplaces.c's.
- **First words**: the words said as a command begins, before the world answers (said.txt's FIRST rows): "You reach
  for the sack."
- **Fixtures**: three meanings. In DUNGEON.md, the portals and traps built into a chunk (dq0m/fixtures.c). In the
  catalogue, a thing that stays put (KF_FIXED: a well, a sconce, shelves). Loosely, any room part.
- **Furniture**: desks, cabinets, cases and shelves, with their compartments, placed by the maker by a wall
  (dq0m/place.c's o_put_furn). `dq0furn=0` in the environment leaves them out, for test scenes.
- **The hold-back**: while a line is being typed, the world, the minds and the senses hold back so the player isn't
  overrun (HOUSE.md's "Holding back while a line is typed").
- **The hub**: section 2.
- **Intents** (I_*): what an actor means to do, sent to the world. The character's come from the UI (on iq), the
  denizens' from the minds (on di).
- **Kinds**: an overloaded word. A kind of thing (KI_*); a denizen kind (a key in dq0d/kinds.txt); a kind of room
  part (OB_*); a message kind (I_*, E_*, UK_*, RS_*; said.txt's KIND); a room's kind (chamber, hall ...).
- **lb**: the UI's 400-byte buffer of words for an answer (dq0.h:462).
- **Long tasks** (LT_*): actions that take time and can be interrupted; "stop" ends one.
- **The maker, the minds, the senses, the world, the UI**: the tasks (section 1).
- **Overlays**: 8K banks in a task's .ovl file, read into the task's own banks at start by the library's loader.
  `ovbank[n-1]` is overlay n's bank.
  - A code overlay is linked at $8000. A call goes through an OVFN stub that keeps the window, puts the overlay in,
    calls it and puts the window back. Its code can't read the hub while it runs: the hub's bank isn't in the window.
  - Some hold only data: the UI's phrase rows are overlays 28, 29 and 30 (linked at $8000, $A000 and $C000 so one
    address names a row in any of them), read a byte at a time by bank.s's said_b.
  - An overlay with another after it in the same file needs `fill = yes, fillval = $FF` in the .cfg (AGENTS.md).
- **The paged ROM library**: dq0lib (section 2).
- **Paces**: section 2.
- **Portals**: a door, window, portcullis or arrow slit between two areas: a struct portal in its chunk, with a kind
  (PK_*), a state (PS_*), a lock and a material. The character and the denizens work them by one path (dq0w/wport.c,
  wdoor.c).
- **Reach**: the house rule for getting at a thing by where it is (in hand, on the body, in a container, in a pack
  worn), told and timed a step at a time (HOUSE.md's "Reach, handy tools and things used up").
- **The record store**: section 2.
- **The referee, rulesets**: section 2. A ruleset is a referee program, games/dq0NAME.
- **Refusals as rows**: the reasons U_VERB, U_YET, U_CANT, U_NOTE and U_WHAT, then the A_* refusals (dq0.h:304-311),
  each worded once by said.txt's NO rows, through refuse.c.
- **Regions**: the 256-square stretches by which the world's records are kept and paged (DUNGEON.md).
- **said.txt's rows, SA codes and pieces**: a row is `KIND.outcome`; SA is the kind for the world's sentences about
  things. The world writes an SA_* code and the names into the UI bank's things_text, and the UI words it by the SA
  rows (items.h:554-562). A piece (`@NAME "text"`) is text shared between rows.
- **Scenes**: the whole machine saved at a dq0 test's first prompt, and taken up by the next test with the same start.
  It skips the start's 235M cycles or so (sim/lib/scenes.js; tests/TESTING.md).
- **The soak** (sim/tools/soak.js): many seeds played by a random player at the turns pace, looking for crashes,
  hangs, a clock standing still, owner violations and sentences told twice. `-fight`, `-life` and `-same` vary it.
- **Strata**: a stratum is one level of the dungeon, numbered from 0 at the top, with its own bounds, seed, style
  (built, natural, mine or mixed), purpose and plan (dq0w/world.h; DUNGEON.md).
- **Swapped parts**: main-RAM code run one part at a time in a swap area under the C stack (the UI's is $1100
  bytes: dq0.cfg's __SWAPSIZE__). A part's image is kept in an overlay bank and copied in when a function of it is
  called. A function is put in part n by `#pragma code-name (push, "SWnCODE")`, and called through an SWFN stub and
  sw_call. parts.txt says where each part's image is kept; parts.js makes the stubs, and pack.js moves code between
  parts that are too full. Unlike an overlay's code, a part's code can read the hub.
- **Sway**: words that would change a denizen's attitude (talk, persuade, deceive, threaten, calm, befriend), and
  requests to an ally. The UI asks the minds; the referee judges the contest.
- **Themes**: what a dungeon is about, as data: its name, story, strata, room words, bestiary, eight kinds of its
  own, the puzzle's pieces and rules, mission, endings, history and inhabitants (theme.h). A game's own theme comes
  from themegen; the stand-ins from dq0/themes/.
- **Tiers**: the test runs for while working, before reporting and for the lead (tests/TESTING.md).
- **The turns pace**: section 2.
- **The zero-page block**: section 2.

## 6. How to add things

Each recipe follows a real commit. Every step names the file and a line where a recent example sits. After any of
them, build and run the tests the change can affect, and compare the transcripts (AGENTS.md).

### A verb

The worked example is rip (62b56369, 2026-11-10): "rip the cloak", "wash the bandage". It is the only action added
since the verbs plan, so it shows the whole path. The verbs of the plan's batches already had their actions; B0
(75cb16ce) gave its 50 unbuilt verbs their rows and stub handlers, and B1-B7 replaced the stubs, so those commits
show the handler half only.

1. **commands.actions**: the action, at the end of [actions]. Its place is its number. Example (:149):
   `VB_RIP_OBJECT object instrument`. hubgen.js makes obj/gen/actions.h from it.
2. **commands.ebnf**: the rule, added to `<phrase>` (:37) and defined (:86):
   `<rip> ::= <RIP> <object> [ "up" ] [ <USING> <object> ] | "wash" <object>`.
3. **commands.verbs**: a [verbs] row (:187, `rip thing=VB_RIP_OBJECT unsaid=last aux=IO_RIP`; the fields are
   described at :9-24), and its [words] (:643-648, `rip = rip` ... `wash = rip`).
4. **commands.corpus**: test cases (:588, `rip the cloak<TAB>VB_RIP_OBJECT<TAB>object=cloak<TAB>near=cloak<TAB>...`).
5. **sim/tools/commandcompile.js**: phraseSem's case for the rule (:873, `case 'rip': return
   withTool('VB_RIP_OBJECT');`).
6. **actions.c**: the dispatch row, in actions.h's order (:203, `ROWP (VB_RIP_OBJECT, H_HANDLE, 0, 0, U_CANT,
   M_NONE, IO_RIP)`). The build fails if the rows and actions.h differ (dq0/make.js:32).
7. **dq0.h** (only for a new handler): an H_* inside its group's range (dq0.h:214-243; main.c:2405-2423 routes by
   range). rip reused H_HANDLE.
8. **The handler**: a case in its group's switch: hthings.c, hplaces.c, hmove.c, hdens.c or hself.c (for example
   hself.c:114, `case H_PRAY:`). A missing case falls silently to the switch's default (hself.c has none: nothing
   happens).
9. **cmds.h** (only for a new group entry or a function shared between files).
10. **Room**: if the handler's part is full, a new part (parts.txt and `#pragma code-name (push, "SWnCODE")`). A new
    overlay takes three hand edits that must agree: dq0.cfg's OVn line, bank.s's OVMAX, and dq0.h's ovbank[] (which
    already says 27 against OVMAX's 30).
11. **The world**, if it must act: an IO_* request (items.h, e.g. IO_RIP), its case in dq0w/things.c's s_serve, the
    world's code (dq0w/wcloth.c) and its line in dq0w/parts.txt; or an intent (hub.h's I_*, its payload in msgs.def,
    a case in dq0w/dq0w.c's switch). Add the minds and both referees when the rules are theirs (9b13e40f, grappling).
12. **Words**: refusals through say_no and said.txt's NO rows; the world's sentences as SA codes (next recipe). A help
    line in help.txt if wanted.
13. **Tests**: an entry in tests/suites/dq0/*.js (dq0rip is in gear.js), its name in tests/areas.js's NEAR lists, and
    a verb that takes a thing in the sweep's lists (tests/suites/dq0/parser.js:21-22, 38-39).

**The catch: the meaning records are full.** dq0c.dat holds at most 255 meaning records, and all 255 are used
(GAME-CRITIQUE.md, section 2.4). A record is one combination of a word's meanings (sim/tools/commandwords.js:356-361),
so one more one-word verb fails the build with "dq0c.dat: too many of the game's words". Today a new verb is often a
[words] synonym of an old one that its handler tells apart by reading the raw line (`wash = rip`, then hthings.c's
`strstr (cl, "wash")`), or a phrase of several words, which takes no record (commands.verbs:95-99, 187-190). The
user has chosen to have the grammar record roles (DECISIONS.md, 2026-11-17, phase B), which will change this recipe.

**The checks:**
- **The build:** commandwords.js (an action missing, a verb unknown, too many records), dq0c/make.js (the tables'
  room), dq0/make.js (actions.c's rows), the floors.
- **The parser's checks** (sim/tiers.js; `--tier report` runs them when a grammar file changed):
  - commandcorpus.js: an action with no case fails;
  - commandmatch-test.js: the 65C02 matcher against the host's grammar;
  - commandmatch-test.js --mean: the 65C02 meaning against commandwords.js.
- **By hand:** `node sim/tools/commandcompile.js` holds the corpus to phraseSem (not all pass).
- **Only a game test catches** a verb missing from commands.ebnf ("I don't understand that."), a word in the grammar
  but not in commands.verbs ("You can't ... in this game yet."), or a missing handler case.

### A sentence

The narration is data: every sentence the player reads should be a row of said.txt.

1. **Write the row** (format: said.txt:7-49): `KIND.outcome[FIELD=V,...] [once] [+]"text"`.
   - KIND is an event's kind less its E_ (DIG for E_DIG), or a UI kind (NO, SA, SELF, FIRST ...).
   - outcome is the kind's enum member in lower case, less its prefix (SA.hold for SA_HOLD), a member named whole
     (NO.A_OUT), an `own` name, or `any`.
   - `+` goes on after what's said; without it the sentence starts a new line. `once` isn't said twice in an answer.
   - Slots: {text}, {obj} and {instr} are the words the UI gives; {F} is the event's field F, named as in msgs.def;
     {F?a|b} chooses by value; {arg} is the UI's number; {a}, {kind F}, {name F}, {rel F} and {@piece} as the header
     says.
   - Examples: `NO.A_OUT +"\nYou lie senseless."` (said.txt:663); `SA.hold +"You take the {text} in {obj}."`
     (said.txt:684).
2. **Add the outcome's enum member by hand** (SA_* in items.h:576-604; A_* and U_* in dq0.h:304-311; the events'
   enums in hub.h). saidgen.js matches each row to a member and fails the build if there's none: "SA.x? (no SA_X, nor
   an own one)". It generates only the `own` outcomes' names (SAID_KIND_NAME) and the packed tables.
3. **Say it from C:**
   - **A refusal:** `say_no (A_X, name)` (refuse.c).
   - **A world event:** the UI sets said_s[] or said_n and words the event (main.c).
   - **A UI kind:** a struct msg with kind UK_* and `w1 = outcome << 8 | value`, then outcome_words (task.c:471-479).
   - **The world's sentence about things:** the world writes SAY_AT, the SA_* code | 0x80, the names and SAY_SEP into
     things_text (items.h:554-562; dq0w/wcloth.c's told). There are 158 SA codes, so a new one is past 127 and must
     be written with SAY_AT2, as dq0w/wdrag.c does.
4. **Mind the room.** The rows live in overlays 28, 29 and 30 (1, 6 and 3,223 bytes free, DECISIONS.md:6456), and a
   kind's rows must sit whole in one bank, the first with room (saidgen.js:391-399). So a row added to a kind in 28
   or 29 moves the whole kind to a later bank, which said_b reads more slowly: that once pushed dq0answer over its
   time budget.
5. **Traps:** a code with no row says nothing, with no error (said.c:403-405); the text must be plain ASCII; a
   duplicate row fails the build.

The worked example is 943f386b (2026-11-13): the row NO.A_OUT in said.txt, A_OUT in dq0.h, `say_no (A_OUT, 0)` in
main.c, and the test dq0outcold (tests/suites/dq0/fight.js). For the world's SA path, 3f8ab7c8 (2026-11-15) moved
the world's own sentences about cloth, water and things into SA rows (items.h, said.txt, dq0w/wcloth.c, wwater.c,
things.c), and put overlay 30 in for them (saidgen.js, bank.s, dq0.cfg).

### A kind of thing

The worked example is the whetstone (9ef6bf42, 2026-11-02); for a weapon, the mankar and the mang (1a0d12cb).

1. **items.h**: the KI_ name, after KI_SHELVES and before KI_COUNT (items.h:226-228). Kinds are numbered by position
   and the numbers reach the saves and the generated files, so add at the end, never between.
2. **dq0w/kinds.c**: the catalogue row, in KI_ order (kinds.c:241 is the last). Example (kinds.c:204):
   `{ "whetstone", 0, 1, 1, 0, 0, 0, 0, 2, "whet", MT_STONE, AP_METAL, 24 }`. The fields (kinds.c:18-24,
   items.h:49-111): the name (13 letters at most), flags (K_*), weight in quarter pounds, bulk in pints, a holder's
   room and load, slots or family, light reach, seen at (squares), the tool key (the referee's, 7 letters at most),
   material (MT_*), craft (AP_*), value in farthings, what it keeps (KF_*), its form (KF_*).
3. **kinds.c's throws[]**: its throwing reach, 0 for none, in the same order. A compile check holds the count
   (kinds.c:357).
4. **A holder** (K_CONTAINER, K_BELT or KF_ON) also needs compartment rows (next recipe). dq0w/make.js fails the build
   for a holder without rows, or rows without a holder.
5. **Words**: nothing to add; the kind's name is its noun (the match: dq0w/things.c:397-423; plurals: said.c:186-192).
   **The catch:** a name word the parser already knows is copied into the kinds' bank, and that bank is full: two
   more words ("side" and "bottom") were 10 bytes over (DECISIONS.md:6613-6614). A new name word the parser knows
   fails the build.
6. **The referee (HarnMaster)**: a melee weapon is a row in dq0harn/weapons.h keyed by its tool key, above the torch,
   stick, club, staff and shield rows (gear.c finds those by position), and its Weapon Quality in gear.c's wqs[] at
   the same index (nothing checks it). A missile weapon goes in missile.c's mws[]; armour in gear.c's pieces[]; a
   digging tool in dq0/rf/dig.c (and NDIGGERS in rshouse.h). The null referee needs nothing.
7. **Where it appears**: a room use's leftovers or marks (dq0m/uses.c's leavings[], signs[]); a style's kinds
   (styles.txt); the character's kit (dq0harn/outfits.txt, e.g. `free 25: whetstone@pack`); the denizens' gear
   (outfits.txt). outfits.js fails on an unknown name.
8. **said.txt**: nothing per kind; rows name the thing by {kind F} or {text}.
9. **Test**: give the character the thing with `-k` (gear.js: `dq0 -p t -s 3 -c /ram/c.hm -T -k mang,whetstone`).

**Limits:** 166 kinds are used (KI_COUNT); dq0w/kinds.c:25 allows 204, and the account's bank check
(dq0/acct/acct.c:44) allows 183. The catalogue bank's code has 242 bytes free (GAME-CRITIQUE.md, section 6.2).

### A fixture or a piece of furniture

The worked examples are d1bf52b3 (the furniture's kinds and compartments) and 4590305d (their placing).

1. **The kind**, as in the recipe above. Example (kinds.c:235): `{ "desk", 0, 200, 100, 30, 400, 0, 0, 7, "",
   MT_WOOD, AP_WOOD, 240, KF_ON }`. A thing that stays put has KF_FIXED in what it keeps (the well, the sconce,
   shelves): taking it is refused, and it's never dragged.
2. **Its compartments**: rows in kinds.c's comps[] (kinds.c:253-289), a kind's rows together and in KI_ order, at
   most NCOMP (4). Row 0 must match the kind's own record (dq0w/make.js checks it). Example (kinds.c:277-279):
   `{ KI_DESK, CP_ON, 0, 30, 400, 0, 0 }`, then the drawer `{ KI_DESK, CP_IN, SS | KF_LID, 6, 40, 0, CN_DRAWER |
   CC_LOCK }`, then the secret niche `{ ..., CN_SECRET | CC_HIDDEN }`. The fields (items.h:122-143): the holder, the
   preposition (CP_IN, CP_ON, CP_HUNG), what it keeps and what closes it, room in pints, load in quarter pounds,
   family, and its own name (CN_*) with CC_INNER, CC_LOCK or CC_HIDDEN. A new compartment name is a CN_* and a cnouns
   entry (kinds.c:293).
3. **How it's stored**: no record changes. A thing inside has the holder as its place and the compartment in its y
   (items.h:119-121, 155-157); the closed bits are in the holder's shut, and the locks and finds in its fuel (CS_*,
   items.h:144-153). The user has decided the item record will gain a byte for this (DECISIONS.md, 2026-11-17).
4. **Its placing**: a row in dq0m/place.c's placers[] (place.c:195, `{ PG_STOCK, SI_ROOM, DP_FURNITURE, ...,
   WH_BYWALL, 0, o_put_furn }`), whose hook chooses by the room's use (place.c:137-172). A new dice purpose goes after
   DP_SEEDED (dq0m/grow.h), so no other placer's rolls move. Placing anything new changes every seed's dungeon: the
   furniture changed 98 transcripts.
5. **What's in it**: nothing yet; the maker puts nothing in furniture. The user wants containers stocked (2026-11-17,
   phase D).
6. **Tests**: dq0furniture, dq0compdesk, dq0compcase, dq0comppocket and dq0lidslide (tests/suites/dq0/gear.js).

Windows, portcullises and traps are the map's fixtures, not things: dq0m/place.c's rows call dq0m/fixtures.c's
hooks.

### A denizen kind

The worked example is 1a0d12cb (2026-10-29), which added twelve.

1. **dq0d/kinds.txt**: a row at the end (a kind's number is its line, and the number is used by the sounds, the
   outfits, the deities and the themes). The 36 fields are documented at kinds.txt:9-104. Example (kinds.txt:132):
   `dwarf miner | khuzmin | lfm | mx | 1-7 | 2-4 | 2 | 150 | 50-10 | ...`.
2. **A faction**: an existing one in dq0d/factions.txt, or a new one (15 at most).
3. **The referee (HarnMaster)**: an entry at the end of dq0harn/beasts.c, and NBEASTS raised (dq0harn/dq0harn.h:260).
   Example (beasts.c:96): `{ "khuzmin", { 15, 15, 12, 11, 13 }, 37, 4, 50, 45, { 2, 2, 1, 1 }, BF_BIPED | BF_FOLK, { {
   "pick", 50, 5, HARN_POINT, 0xFF }, { "" } } }`. Nothing ties kinds.txt to beasts.c at build time: a missing key
   falls back to the giant rat's sheet (dq0harn/fight.c:500-505). A venomous kind also goes in dq0harn/course.h's
   venoms[]. The null referee needs nothing.
4. **Its sounds**: two rows in dq0s/sounds.txt, a step and a noise (sounds.txt:68-69, khuzmin's). dq0s/make.js checks
   the keys against kinds.txt.
5. **Its gear** (folk only; beasts carry nothing): [role KEY], [lair KEY] and [purse KEY] sections in outfits.txt.
6. **Words**: nothing to add; a denizen is named by its kind's name, and its plural made by said.c.
7. **Into a dungeon**: a stand-in theme names its kinds in a `denizens` line or its `faction` lines
   (dq0/themes/1crypt.txt:126); a game folder's theme.txt in inhabit.txt's format; or, for a generated history, the
   kind's strata and styles (themegen/make.js copies them into themegen.dat). themec rejects an unknown key.
8. **Tests**: dq0peoples (tests/suites/dq0/denizens.js) and dq0peoplegen (theme.js).

**Limits:** 31 kinds at most (SND_KINDS, 32: hub.h:116), and 29 are used. The referee's overlay 5, where beasts.c
lives, was full at the last build.
