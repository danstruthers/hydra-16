# Dingus Quest Zero's rulesets

The game is ruleset-neutral: everything it knows of the rules it asks a referee, in rs.h's terms (a purpose, the
circumstances in steps, a grade and conditions, effects and words back), and the referee is the only place a
ruleset's rules live.  This is what a ruleset must answer, what the game still takes for granted of one, and what it
no longer assumes.  Phase 3A (2026-10-19) made it so and proved it with a second, tiny ruleset, the null one; the real
second ruleset comes in Phase 8.

## A ruleset

- **Its referee** is a program of the game's, games/dq0NAME, on the card as lib/dq0/dq0NAME: a task in the referee's
  role (dq0NAME SEG ROLE DIR), answering rs.h's requests in the hub's three referee slots (the world's, the UI's, the
  denizens' task's), the asker rung with the reply (hub.h's protocol).  It writes the hub's pcsum (the character's
  summary) and pcstatus (its words on the status line), and nothing else of the hub's but its slots' replies (hub.h's
  owners' table; sim/lib/hubown.js checks it in every dq0 test).
- **The ruleset is chosen at the start:** dq0 -r NAME (harn if none's given: HarnMaster's), kept in the save
  (game.dat's referee): a game carried on keeps its own.  The UI knows the referee's program only by that name.
- **Its character generator,** if it has one, is named by its referee's RS_HELLO (the reply's second line: hmchar);
  the UI runs it in the window when no character file is given (dq0 -c), as NAME -o FILE -s SEED, and gives the file
  it made to RS_LOAD_PC.  None (an empty line): RS_LOAD_PC's path is empty, and the referee makes its own character.
- **Its files:** the character's file is the referee's to read and write (RS_LOAD_PC, RS_SAVE: the save's pc.hm), and
  its creatures' sheets its own (dq0harn and dq0null both keep theirs in the save folder's beasts.dat).  A save is its
  ruleset's.

The two so far:

| Name | Referee | Generator | What it is |
|------|---------|-----------|------------|
| harn | games/dq0harn | hmchar | HarnMaster 3.5 (the C SDK's harn.h): the game's own ruleset |
| null | games/dq0null | none (its own, Dingus, a wanderer; any file's name, occupation, pace and language lines) | Flat odds, no skills: HOUSE.md's last section |

## What a ruleset must answer

Each request the game makes today, who makes it, and what of the reply it reads.  A referee may answer more (rs.h
has requests no one makes yet: RS_TEST_MANY, RS_CONTEST, RS_REST, RS_TREAT, RS_INITIATIVE, RS_EXPERIENCE, RS_POWER,
RS_EXPORT, RS_IMPORT, RS_RESTORE, RS_QUIT), and RS_DICE is the referee's unit tests' alone (tests/ram/t_rules: the
next dice's faces fixed, the dice rolled since said back; dq0harn answers it, a ruleset needn't); one it doesn't know
it answers with result 0xFF.

| Request | Asked by | The game reads |
|---------|----------|----------------|
| RS_HELLO | the UI, as the game starts | w1 the round (tenths: the world's clock, a climb's round, a creature held back); the text's lines: its name, its character generator ("": none) |
| RS_LOAD_PC | the UI (its slot's text the file's path; "" none) | result (0, or 1: no character); the text's lines, the name and the calling; a and b the pace and watchfulness the file kept (GA_WALK and 0 if none); pcsum and pcstatus set |
| RS_SAVE | the UI (the path of the save's pc.hm) | nothing (the referee writes its character, and its creatures' sheets into the save folder: the hub's savedir) |
| RS_EQUIP | the UI (a 2: every key to hand, each after its mark and with its quality, w1 the load in quarter pounds) | nothing (pcsum anew: load, step) |
| RS_TEST | the world, the UI, the denizens' task | grade (G_CF ... G_CS); result 1 can't (no tool, magic only), 2 a tongue it doesn't speak; P_DIG's text the tool's key ("hands": bare), w1 the feet, w2 the wear |
| RS_EXERT | the world (w1 tenths at pace a, or GA_REST) | nothing (pcsum anew) |
| RS_HARM | the world (a cause, HC_*, and its numbers) | x the effects (rs.h's struct rs_effect, X_NONE after the last); b the worst wound's band (0 none); w2 the tenths it's senseless (0); result 1 a fall stopped by a hold; the text its words; pcsum anew (cond: C_DEAD ends the game) |
| RS_ATTACK | the world (a blow: actor at other, a its aim, b AS_BEST, w2 shy and prone bits) | result O_* (0xFF: none struck), a whose fumble or stumble, c AF_* (AF_CAUTIOUS, AF_BLEEDING), grade hub.h's FF_* (senseless, dead, flees, fallen), w2 the tenths one's out for, x the effects (X_COND_ON C_PRONE: fallen), the text its four lines (below); at RS_THING (other): w1 the harm the thing takes, c, w2 and result's bit 0 the striker's own hurt (bit 1: a part of it struck) |
| RS_TIME | the world (actor 0xFF: the bleeders, w2's bits) | w1 the bits of those dead of it |
| RS_NEW_ACTOR | the denizens' task (its text the kind's key, w1 the record's number) | x a struct rs_summary (cond: C_BLEEDING) |
| RS_MORALE | the denizens' task (an actor, the steps) | result M_* (M_BROKEN, M_CAUTIOUS flee) |
| RS_DESCRIBE | the UI | b 0 the sheet (said whole); b 1 the map panel's lines (each 15 characters at most); b 2 the last harm's words; b 3 the state (status: the pace, the watchfulness, the condition, the light; a 1, the pace and watchfulness only) |

The words the game knows in a reply (a convention every referee keeps):

- **RS_ATTACK's four lines:** the attacker's weapon, the defender's, what the attacker's strike did, what the
  defender's did.  A strike's line beginning "a " is a wound (": a serious cut ..."); any other a piece of armour that
  turned it ("your ring byrnie"); empty, no harm.  The character's weapon "foot" is a kick, "head" (any word in h) a
  butt; a creature's "bite" bites and "claws" claws; a creature's empty one just strikes.
- **P_DIG's tool** is a key of the things' catalogue, or "hands"; the UI names the tool by the thing it takes.
- **The keys to hand** (RS_EQUIP) are the catalogue's (items.h's tool keys: "pick", "picks", "ltunic" ...): a referee
  maps them to its own weapons and armour.

## The house's rules, every referee's

Rules the game makes where no ruleset speaks (HOUSE.md, marked *every ruleset's*) are code each referee includes,
games/dq0/rshouse.h, so a second ruleset plays them alike: the tools that dig and their rates (RS_HOUSE_DIG, and
RS_HOUSE_DIG_TOOL the choice of the one to hand), a notice's steps by the pace and the watchfulness (RS_HOUSE_NOTICE:
a creep a step better, a jog two worse ...; a hazard underfoot by a glance or a search; listening), and the state's
words of the pace, the watchfulness and the light (RS_HOUSE_STATE).  Lockpicking wants "picks" to hand and magic's
hiding wants magic in both referees (a line each).

## What the game no longer assumes (fixed in Phase 3A)

- **The referee's program** was named in the UI (start ("dq0harn")): now the ruleset's, dq0 -r NAME's, kept in the
  save (SAVE_VER 11).
- **The character generator** was the UI's (gamedir/hmchar, run before the referee started): now the referee's to
  name (RS_HELLO's second line), run after it's started; a ruleset with none makes its own.  pcfile's 64 bytes of
  the UI's main RAM went with it.
- **The house's rules lived in the HarnMaster referee** (dq0harn.c's digger table and tool choice, sheet.c's notice
  steps and state words): now rshouse.h, which both referees include.
- **A climb's round** was the world's 100 tenths (dq0w.c's ROUND_T): now the ruleset's (the hub's round,
  RS_HELLO's); dq0.h's unused ROUND (HarnMaster's 10 seconds) is gone.
- **A creature's blow with no weapon named** read "strikes at you with its ": now "strikes at you".
- **Skill names and the book's terms outside the referee:** the code's comments named the skills a test would use
  (Lockcraft, Awareness, Animalcraft, Concealment, Timbercraft, Metalcraft, Masonry, HarnMaster's Climbing, Mining,
  melee, walking, Combat 18): now the purposes (P_PICKLOCK, P_SEARCH, P_ANIMALS, P_CONCEAL, P_APPRAISE ...), in
  wdoor.c, wtrap.c, dq0w.c, wfight.c, dq0d.c, kinds.txt, hub.h, items.h and main.c.  The player never saw a skill's
  name but in the referee's own words (the sheet, the state).
- **Grade words:** the game uses rs.h's four grades (G_CF, G_MF, G_MS, G_CS) to judge what came of a test (a climb's
  feet, a hiding's difficulty, a sway's steps, an appraisal's sureness): they're rs.h's, not HarnMaster's (rs.h maps
  a d20's onto them), and no grade's word ("critical", "marginal") reaches the player but in a referee's words.
- **Tables read by other tasks:** none; no task but the referees includes harn.h, and none reads the character's
  file but the referee (the UI only names its path).

## What it still takes for granted (known, not fixed)

- **The stand-in bestiary is in the HarnMaster referee** (fight.c's, by kinds.txt's keys: rat, spider, ghoul, skel,
  bear, kobold): the creatures are the game's, their numbers HarnMaster's.  Phase 3D's bestiary as the game's data
  (the pop agent's) is to give the referee a kind's numbers; a ruleset with no bestiary (null) treats every creature
  alike.
- **The fight words key on the referee's weapon words** ("foot", "head", "bite", "claws": above).  A ruleset must use
  them for the UI's verbs; better, RS_ATTACK's reply naming the kind of blow (a kick, a butt, a bite, a claw) as a
  number.
- **The file names:** hmchar's output is /ram/dq0.hm and the save's character pc.hm (HarnMaster's extension) whatever
  the ruleset; harmless, and the tests read them (dq0harm, dq0pace, dq0watch).
- **The state's words are the referee's** (RS_DESCRIBE b 3): the pace's and watchfulness's words are shared
  (rshouse.h), but the UI asks the referee for them.  The game's own words could be the UI's, the referee giving only
  the condition and the feet a round; the UI's room (pace.c, part 4) has it.
- **"You are in shock."** (intro.c's words for C_STUNNED): the condition's name in rs.h is stunned, HarnMaster's
  shock; the words read plainly for any ruleset, but a ruleset with a stun that isn't shock would want "stunned".
- **The grammar's comments** (commands.ebnf) cite HarnMaster's books as the source of its verbs (Rhetoric, Unarmed,
  Combat 6-9, Religion, Magic): the grammar's own words are the game's; left for the command interpreter's work (3C).
- **HOUSE.md mixes the two:** the rules every ruleset plays (marked) and HarnMaster's use of the book (the referee's
  own: untrained skills at their opening level, shock four worse, the bestiary's variation ...).

## The meaning checks with the null ruleset

RULES=null node sim/test.js runs the dq0 tests with the null referee (dq0 -r null added to each dq0 line typed); with
no test named, those marked meaning: true in tests.js (their checks are about what happened, not the ruleset's words
or numbers).  The soak too: RULES=null node sim/tools/soak.js.

On 2026-10-19 (Phase 3A) the 57 dq0 tests ran with it, 33 passing.  Then the tests were cut to meaning (each
system's tests check what happened, read from the hub: tests.js's dq0Turns, sim/lib/dq0meaning.js; the game's
sentences held to in three tests about wording alone, dq0words, dq0wordsfix and dq0wordsthings), 47 dq0 tests and
the referee's unit tests, dq0rules.  These 29 pass with it, and are marked: dq0, dq0deep, dq0dig, dq0talk, dq0items,
dq0equip, dq0save, dq0vast, dq0soak, dq0names, dq0denizens, dq0prompt, dq0answer, dq0sound, dq0soundx, dq0portc,
dq0choose, dq0veraplain, dq0pure, dq0stairtrap, dq0rebar, dq0pace, dq0steps, dq0theme, dq0pieces, dq0plan, dq0gate,
dq0start0, dq0start1.

The other 18 depend on HarnMaster on purpose, and aren't meaning checks for a second ruleset:

- **HarnMaster's dice at a seed, each outcome in its turn (what happened, but the referee's to decide):** dq0fight,
  dq0armour, dq0flee (its melee: blows dodged, a rat knocked senseless at the fourth, the armour that turns a claw,
  the kin fleeing), dq0batter (blows at doors: the striker's hurt by its armour), dq0break (the bars broken and bent,
  the sill climbed at the first try), dq0traps (a door holding, then bursting; the mechanism made out, then
  disarmed), dq0sway (Animalcraft 90's coaxing, a step each time), dq0watch (Awareness 95's search as it goes;
  fatigue's universal penalty, the status line's "UP n"), dq0glow (Eyesight 4's reach, the eyes' adapting by it),
  dq0load (encumbrance's bands by the load), dq0harm (pc.hm's inj and pace lines: HarnMaster's sheet).
- **HarnMaster's words:** dq0words and dq0wordsthings (wording alone: the status line's feet a round, a door afar by
  Eyesight 4; dq0wordsfix passes too, but is about wording, not marked), dq0vera (its status line must read "UP 0"),
  dq0veraserial (hmchar's questions in the game: HarnMaster's character generator).
- **A reference made with HarnMaster's referee:** dq0looks.
- **dq0heard** passes with it, but holds the UI's words of a sound ("You hear ... to your right"): not marked.
