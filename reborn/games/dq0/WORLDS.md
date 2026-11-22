# Dingus Quest Zero's worlds

The game is world-neutral as it is ruleset-neutral (RULESETS.md): the setting a dungeon's history is made in, its
peoples, its lands, its tongues and names, its realms and their coin, and its deities, is a world's, a plug-in chosen as
a game begins, as the referee is.  The user's (2026-11-17, DECISIONS.md's "The critique's questions: the user's
answers"): "I would like the history, peoples, etc. be part of a world plug-in, similar to the referee/game rules.  One
such world plug-in should be Harn World.  Another could be the World of Greyhawk.  Third could be Troll Lord's
Airde/Winter Dark."  And: "The world plugin would also dictate deities."  Built before phase D (GAME-CRITIQUE.md's
section 10), the data of the time moved into the first world with the dungeons unchanged: a game in the default world
plays as it did.  This is what a world supplies, who reads it, and what the game still takes for granted of one.

## A world

- **Its files** are a folder of games/dq0/worlds, its name the world's (8 small letters and digits at most):
  - **history.txt**, the words a region's history is made of (below): themegen's make.js holds it to what themegen
    wants and makes it, with the game's goal kinds (games/themegen/goals.txt) and bestiary (games/dq0d/kinds.txt,
    factions.txt), the world's **history.dat**, themegen's data file;
  - **peoples.txt** and **deities.txt**, its lore (games/dq0d/lore.h's struct people and struct deity: dq0d's make.js
    makes them **peoples.dat** and **deities.dat**, and the deities' names, spaces between, **gods.txt**);
  - **tongues.h**, if it has one: its tongues' sounds in C, for what names things with no world's data at hand (the
    first world's alone: below).  themegen's make.js holds it to the history's tongue lines.
- **On the card:** lib/dq0/worlds/NAME/history.dat, peoples.dat, deities.dat and gods.txt (sdcard/sdcard.txt).
- **Chosen at the start:** dq0 -w NAME (the first world if none's given: games/dq0/worlds/worlds.js's DEFAULT), kept in
  the save (game.dat's head, its last field: life.h's struct save's world, "" the default; SAVE_VER 46).  A game carried
  on keeps its own; a dungeon after the first is in the last dungeon's world, unless -w says another; the rumours
  between dungeons (dq0life's themegen -r) are made in it too.
- **Who reads it:**
  - **themegen** (themegen -w NAME: lib/dq0/worlds/NAME/history.dat), as a new game's history is made (a seed of
    65,536 or more, or dq0 -t's), and its bank programs: histgen (the region, the eras, the strata's names, the
    inhabitants), goalgen (the goal's guardians, its people's names), linegen (the story's and the clues' words, the
    rooms' words, the mints) and namegen (every name, by the world's tongue's sounds).  What it makes is the theme
    (theme.txt, compiled into theme.dat: theme.h's record), and from there on the game reads the theme, never the
    world: the maker (dq0m: the strata, their rooms' words, the clues as things), the denizens (dq0d: the theme's
    factions), the world (dq0w: the clues' rules, the mints' coins), the UI (the story, the reading of a clue by its
    people's tongue and script).
  - **The UI**, for the command parser's words: the world's deities' names ("call on Wend"), CQ_WORLD's third list.
    The default world's are in the catalogue's bank (items.h's GODS_AT: dq0w's make.js, from obj/games/dq0g.txt, the
    default world's gods.txt), so a game in it opens no file of them (an open on the card some 700,000 cycles); another
    world's gods.txt is read as the game begins (main.c's s_gods, part 7's).
  - **Nothing else yet:** peoples.dat and deities.dat are made and put on the card, and nothing reads them
    (GAME-CRITIQUE.md's 5.4): the history's peoples are its history.txt's people lines.  The user's (2026-11-17): the
    two sets of peoples "are just random", neither canonical; the first world keeps both as they were.

The two so far:

| Name | What it is |
|------|------------|
| first | The world the game began with (G3's: its own peoples and names, none of HarnWorld's): history.txt's six peoples (dwarves, men, elves, hill-men, kobolds, sun-priests), six lands, six tongues, the kobolds its raiders, the dwarves', elves' and men's soldiers' holds, the gargun's colonies; peoples.txt's seven peoples and deities.txt's eleven deities, as they were (games/themegen/history.txt, games/dq0d/peoples.txt and deities.txt till 2026-11-21).  The default. |
| bare | The smallest world the game plays in, as the null referee is the smallest ruleset: two peoples (hearthfolk, delvers), two lands, two tongues, one deity (Wend); no raiders (so no prisoner's goal and no scourge's: the artifact's in their place), no holds, no colony; its folk the rats, the spiders, the dead, the beasts and the outlaws.  The proof that a world is swappable: dq0world's. |

## What a world supplies: history.txt

The theme format's lines (a key and its values between bars; a line begun with spaces goes on with the one before), in
the words a slot, a % and a letter, filled as the history's made (%R the region's name, %P an era's people, %K what they
are, %N the era's great one, %X a place's name, %O the people who came after, %C how many centuries ago, %W what the era
did).  The first world's history.txt says each key's values in full; here, what each is for and who reads it.

**The region and its eras** (histgen's):
- **land**: a land the region may be (one picked by the dice), the words for a region of it ("the Kharum Hills"), its
  rocks, its waters, its climates.  One at least.  The theme's region line says them; nothing compiles them.
- **people**: a people of its history: what it is (11 letters at most), its tongue (a tongue line's number), how it
  builds, the lands it lives in (land lines' words), what it makes (works: keep temple mine tomb town hold), its
  tongue's word (the referee's, what its folk speak, a character's "language" lines: 8 letters at most) and its
  script (what its writings are in: 8 at most).  Two at least: an era's people isn't the one before it.
- **work**: the strata a work leaves, from the top down, each a purpose (theme.h's: halls keep temple cellars crypt
  works, fortified or open) and its styles (b n m x: level.h's): one for each of the game's works and the deep caves
  (keep temple mine tomb town hold deep).
- **name**: a stratum's names, by its work and its place in the work's strata (0 its top): one for every stratum of
  every work line, 19 letters at most.
- **built**, **reused**, **noun**, **ended**, **ago**, **open**: an era's words by what it made, by what it made its own
  of an older work's, a work as the eras after it speak of it, how an era ended (each of the game's endings: abandoned
  conquered collapsed flooded cursed held), how long ago, and the story's first words.  Linegen's.

**Its clues and its rooms** (linegen's): **clue** (a work's thing to read, as a theme's kind line, and the words it's read
in: each of the six works), **rooms** (the eight words for a room, by the people that has the top stratum now; a default
line), **uses** (the twelve uses' words, by the people; none, the theme's own), **light** (a room's light, by the people;
a default line).

**Its tongues** (namegen's, through themegen's name_of): **tongue** (its number, in order from 1, eight at most: its
onsets, its vowels, its codas, a people's name's ending), **common** (the tongue strangers speak: today's realms', and
those who ask the character to go, a patron, a seeker, a prisoner; a hand-written theme's era of a people it doesn't
give).

**Its realms and coin** (linegen's mint lines, the theme's: HOUSE.md's money): **realm** (a mint of today's, in order:
the tongue it's named in and its ways, l the region's own, g gold, p taken at its worth everywhere, h hard to pass, ?h
hard to pass one game in two by the seed; the first world's: the region's own realm, two neighbours, the dwarves' hold),
**coin** (a people that struck coin in its era, g gold or -; a people with none struck none).

**Its inhabitants** (histgen's, goalgen's, linegen's): the bestiary's factions are the game's (games/dq0d/factions.txt);
a world says which live in it and which are its peoples':
- **folk**: a faction of the bestiary's that lives in this world, in words (the goal's guardians, %g).  A faction with
  no folk line is never an inhabitant of a history made in it.
- **now**: the story's words for who has it now, by the faction strongest there.
- **wild**: the lands beasts live in (the beasts faction an inhabitant only there).
- **raiders**: the people that comes to a region only last, digs its own mines and keeps a prisoner or raids from them,
  and its faction (the first world's kobolds): a prisoner's goal and a scourge's are made only where they live (their
  people line's lands), their faction's lair the deepest of their mines and its leader one of its own.  None, neither
  goal is made.
- **holds**: a faction that's a people's where an era of theirs holds its works still: its kinds there, its leader.
- **colony**: a faction that settles a people's works lost (an era of theirs that doesn't hold them, and none that
  does), in the lands given, and its leader; its **tribe** lines its kinds, the first's where it's found (a land and a
  climate), the rest by the dice.

A people's faction holding its works, a colony and the raiders come first in the story's "now" (linegen's), then the
dead.  themegen's make.js refuses a world that lacks what themegen wants: a key it doesn't know, too few values, a
tongue or a people or a faction or a kind that isn't one, a work without its strata's names, an ending without its
words, more than one common, raiders or wild line.

## What a world supplies: its lore

peoples.txt and deities.txt as the first world's say them (their keys there, lore.h's records): a people's name and key,
its folk among the bestiary's kinds, its tongue, its lands and climate (lore.h's six kinds of land and three climates,
the kinds' habitats' letters: not the world's land lines), what it builds and to which plans, its deities, its walls,
doors, light, marks, rooms' and uses' words; a deity's name (one word: the parser's), title, domains (lore.h's DO_*),
temper, followers, rite, feast, number, colours, sign, temple, burial, servants (the bestiary's kinds) and enemies.
Fifteen deities at most; their names, all told, as the catalogue's bank's room has (the default world's: two kinds'
records) or the parser's (another's: hub.h's CQ_WORDS, after the things' words).

## What is the game's, not a world's

- **The goal kinds** (goals.txt: what a dungeon's goal can be, its things, its reasons to come, its words), the
  **bestiary** (kinds.txt, factions.txt: the creatures, their factions and how they stand with each other), the
  **works** and the **endings** (theme.h's W_*, EN_*), the purposes and the plans (theme.h's SP_*, level.h's ST_*).
- **The stand-in themes** (games/dq0/themes, a seed under 65,536: the tests'): dungeons written by hand, in the first
  world's words, the same in every world (they're dungeons, not a world's history: "the dungeons unchanged").

## What the game still takes for granted (known, not fixed)

The first world's, left where they were (2026-11-21):

- **The stand-in themes** are the first world's dungeons, whatever the world (above): a small seed in the bare world
  is the crypt of the Khazrim still.
- **The bestiary's factions are named after the first world's peoples** (dwarves, elves, gargun, soldiers, kobolds),
  their kinds after them (khuzwar, sindwar, kyani ...), and speak its tongues (factions.txt's khuzdul, sindarin,
  gargun, kobold, common: the referee's words); goalgen's guardians' order names the bestiary's factions (the dead,
  the gargun, the kobolds, the beasts, the spiders, the rats); histgen's dead and beasts are the bestiary's factions by
  name.  A world names them in its folk, holds, colony and raiders lines, but can't bring creatures of its own.
- **The character's deity is the HarnMaster referee's** (RD_DEITY: harn.h's deities, HarnWorld's Agrik ... Siem), not
  the world's: a prayer and an invocation name it.  "call on" takes the world's deities' names.
- **The character generator's tongues and scripts** (hmchar's tongue2 and scripts: khuzdul, sindarin, delvic, ardic
  ...) are the first world's, by culture.
- **The calendar** is harn.h's (life.h's: twelve months of 30 days, Nuzyael ... Morgat).
- **A writing with no script said** is in ardic (read.c, the scholar's dq0schol), the scholar's tongue common.
- **Between dungeons** (on hold since 2026-11-13): the town's and the scholars' names are the first world's men's
  tongue's (tongues.h, compiled into dq0acct and dq0schol), the inns' names English.
- **The catalogue's gargun weapons** (items.h's KI_MANKAR, KI_MANG) and the money's words of the dwarves' gold
  (things.c's comments, the mints' theme lines are the world's).

## Making a world

A folder of games/dq0/worlds with its history.txt, peoples.txt and deities.txt (the bare world's are the least that
build), its four files' lines in sdcard/sdcard.txt; the build checks the rest.  Then dq0 -w NAME, and themegen -w NAME
alone to see its histories (themegen -w NAME -s SEED); dq0world runs a game in the bare world.
