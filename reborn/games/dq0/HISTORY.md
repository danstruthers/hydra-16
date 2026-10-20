# Dingus Quest Zero's history and themes

The design of where a dungeon comes from: a region's history first, the dungeon what's left of it (Phase 3D).  The
theme's format is games/dq0/themes/1crypt.txt's; the dungeon's growing is DUNGEON.md's, its denizens DENIZENS.md's;
when and why each piece was decided, in the user's words, DECISIONS.md's.

## The aims

- **History first** (the user, DesignIdeas.md): "dungeon themes should be built around a history of an area in a
  fantasy world.  That needs to come first; determine what the history of the geographic area is, then you can build
  a believable dungeon.  After the history has been built, for each significant era, there could have been something
  built that is left over as a part of the dungeon, as layers below whatever is there now."  And: "there needs to be
  a program who's sole purpose is to generate the history and theme for DQ0 to use to generate its dungeon."
- **Not formulaic** (D12): "A temple could sit over the ruin of an older keep, say.  I just want the things on the
  same strata to make sense."  A stratum has a history of its own, one to three layers.
- **A goal** (G5): "Each dungeon needs a goal, and it may be to find a specific artifact, clean out the evil
  denizens, rescue a prisoner, or just find a way through to a distant exit"; all four, maybe more, so goal kinds
  are data.
- **A new region each game** (G2), in the game's own world (G3: its own peoples and names, HarnWorld as inspiration
  only); no fixed largest dungeon.

## How

### The programs

- **themegen** (games/themegen): `themegen -s SEED [-t THEME] [-o FOLDER] [-q]` makes a region's history and the
  dungeon's theme from a seed, the same each time, and says it (or writes it, theme.txt, into FOLDER).  Its work is
  bank programs (the SDK's: each a child called as a function from a RAM bank of themegen's own, on the history in
  themegen's main RAM): **histgen** (the region, the eras, the strata, the inhabitants), **goalgen** (the goal),
  **linegen** (the theme's lines), **namegen** (names) and **stratagen** (a stratum's layers planned: the theme
  checker's, below, and the maker's).  Its words are data: history.txt (the peoples, the lands, the works, how eras
  end, the strata's names, the clues, the rooms' words), goals.txt (the goal kinds), made lib/dq0/themegen.dat at
  build time by its make.js, which holds every goal kind to a theme's (its lines compiled and checked, their slots
  filled).
- **themec** (games/themec): `themec [-o RECORD] THEME` compiles a theme into its record (theme.h's struct theme:
  make.js's compile, in C, held to the same bytes by the tests) and checks it.  themegen runs it in its own place
  (exec) when it writes a folder: theme.txt compiled into theme.dat.
- **The game**: a seed of 65,536 or more (a game's: the clock's) has its own theme, themegen run by the UI before the
  world starts ("The place's history is being made ..."), into the save's folder or /ram/dq0; a seed under 65,536
  (the tests') picks a stand-in (games/dq0/themes, made at build time into lib/dq0/dq0t.dat).  `dq0 -t FILE` is a
  theme written by hand, themegen filling in what it leaves out.  The save keeps which (Q.theme 0xFF: the folder's
  theme.dat), so a game carried on reads it back and makes nothing again.

### The history

themegen's dice are the game's seed's; names are namegen's, by a people's tongue (six: the dwarves' hard, the men's
round, the elves' soft, the hill-men's rough, the kobolds' yapping, the sun-priests' hissing).

- **The region:** a land (mountains, hills, forest, marsh, coast, plain), its rock, its water, its climate; its name
  the oldest people's (the Kharum Hills).
- **The eras**, oldest first, two or three: a people of the land (not the one before it; kobolds only last), what
  they made (keep, temple, mine, tomb, town, hold: their people's works), how it ended (abandoned, conquered by the
  people after, collapsed, flooded, cursed; the last may still hold it), how long ago (the oldest 18 to 40 centuries,
  each after a third to two thirds of the one before); a great one (a lord, a king, a high priest) and a place
  named in their tongue; its words, the story's and the clues'.
- **The strata**, from the bottom up, then turned over: one or two of natural caves under all, then each era's works
  over the older ones' (a keep a fortified keep over its cellars; a mine two of works; a hold its halls, an inner
  keep and its delvings ...), or, one era in three after the first, an older work made its own (the top stratum: a
  layer reused).  Each era's end is a layer ruined on what it held; a collapse breaks the rock in (its styles built
  and mixed), a flood lets the caves in (built and natural).  Eight at most; their names a work's, by their place in
  it, one that fits and isn't another's.
- **A stratum's layers** (one to three, oldest first): built (by an era, to a purpose), ruined (by an ending), reused
  (by a later era, to its own purpose).  Its purpose for the uses' plan is its last built or reused layer's; its age
  its oldest layer's era's.
- **The inhabitants now:** the bestiary's factions (dq0d/kinds.txt's kinds, factions.txt's attitudes and others)
  whose kinds can live in the strata (their styles and depths), as the history calls for them: the dead where an era
  ended cursed or conquered, or laid its dead (tombs, temples); beasts in wild land (mountains, hills, forest); rats,
  spiders and kobolds wherever they can live (kobolds where there are mines).  Each faction's strata the ones its
  kinds live in; its lair one of them (the dead's a stratum of the cursed era's, the kobolds' their deepest mine); its
  number its kinds' lairs' groups; the kobolds' leader one of their own.
- **The goal** (goals.txt): one of the goal kinds, picked first so the history fits it (an evil's needs an era that
  ended cursed; a prisoner's, kobolds holding mines, in mountains or hills, else an artifact's).  Where its stratum is
  is its kind's: built (a stratum an era that's ended built or made its own), cursed (the cursed era's), lair (the
  kobolds' lair), deep (the deepest).  Its era is its stratum's; its guardians the strongest faction there (the dead,
  the kobolds, the beasts, the spiders, the rats; a prisoner's its captors); its things one of each role its kind
  has; its person (a seeker, a prisoner, an old priest) a name; its title.
- **The clues:** an era's works leave a thing to read (history.txt's clue by its work: a keep's worn plaque, a
  temple's carved slab, a mine's tally stone, a tomb's grave slab, a town's ledger, a hold's rune pillar), on a
  stratum of that era's, among its works, its words the era's own: exploring tells the story, the goal's clue (a
  journal, a scroll, a map) where it is.  As many as the theme's room for kinds, rules and words holds.
- **The story** told as the game begins: the region's first words, each era's words with how long ago, who has it now
  (history.txt's now, by the strongest faction), and the goal's words.

### The goal kinds (data)

goals.txt has a block a kind, its slots filled as the history's made (%A the goal's name, %N the era's great one, %S
its stratum's name, %g its guardians in words ...): its things (role a, b: a kind's line and a name), a far exit's
names, its title, its name (called), and the theme lines it adds (its kinds, its pieces with their era and
guardians, its rules with their words, its story's words, the way out's lock and opened words, its endings).  More
kinds are more blocks.

| kind     | mission | where  | its things                     | done by                                                    |
|----------|---------|--------|--------------------------------|------------------------------------------------------------|
| artifact | escape  | built  | the artifact; a seeker's journal | the artifact used on the way out (its wards answer to it) |
| evil     | rule    | cursed | a relic; the curse's altar or idol; an old scroll | the relic laid on the altar: the curse broken, the game won where it's done |
| prisoner | escape  | lair   | the shackles' key; the prisoner, chained | the key used on the chains: the prisoner shows a way out |
| through  | escape  | deep   | an old map                     | a far exit on the deepest stratum, open from the start, the way in fallen in |

The way out's words, shut and open, and the endings' (escaped, ruled, died) are the kind's.  A far exit is a way out
(O_WAY_OUT) the maker puts on the goal's stratum as its plan's grown, as far from its way in as it finds; the world
lets the character up any way out below the top stratum as it comes to it.

### The theme checker

make.js's check and themec's (the same rules): every goal reachable (each rule's things to be had: a piece put down
somewhere, or a thing a rule makes from things to be had, carried if it's to be carried; the mission's ending a rule
can bring about, or a far exit), every faction placed (each one's kinds able to live somewhere in its strata, its
lair among them, its leader in its lair's stratum; a piece's and the goal's guardians where they lie).  themegen
checks its layers too: each stratum of more than one planned by stratagen as a stratum of 8 by 6 chunks, every layer
built or reused some chunks of it.  A theme that fails isn't the game's: themegen's status says so, and the game
takes a stand-in.

### The maker from the history

- **stratagen** (bank/stratagen, a bank program the maker loads the first time a stratum of more than one layer is
  planned): which of a stratum's layers a chunk is now.  The oldest built layer is the whole stratum's; each later
  one reused takes a part of it round the way in (those who came after came in that way), a third to two thirds of
  the way to its far side, a later one a part of an earlier's; a ruin after a chunk's work breaks it by its kind's
  odds (collapsed one chunk in two, conquered, flooded or cursed one in three, abandoned one in four), but what a
  later people made its own they put right.
- **The uses' plan** (uses.c) by the chunk's layer's purpose: a temple's rooms where the temple's people took the old
  keep, the keep's further off.
- **The stock** (stock.c): in a chunk with an older layer under it, each room's use under the older purpose leaves
  its mark (an old chapel's candle stubs under the barracks), rotted by that layer's age; in a ruined chunk, debris
  (bones where it was taken or cursed, potsherds else).
- **The pieces** (plan.c): a piece of an era's (a clue, the goal's things) lies in a chunk of that era's works, or
  with them under it.
- **Ages:** a stratum's age (centuries: the rot of what rots, the earth floors) is its history's, its oldest layer's
  era's, as its plan's grown.

### What popgen reads (the inhabitants, the history)

The theme file's lines, popgen's to place the denizens by (the record has them too, theme.h's):
`faction | NAME | KINDS | STRATA | NUMBER | LEADER | LAIR | ATTITUDE | OTHERS` (NUMBER about how many on each stratum
it lives in); `era | PEOPLE | WORK | ENDING | AGO | LEFT | WORDS` (LEFT what was left: stores, bones, nothing, the
dead, its people); `layer | STRATUM | ERA | DID | HOW`; `goal | KIND | NAME | STRATUM | GUARDIANS`; and a piece's
guardians (its sixth value).  The file: a stand-in's lib/dq0/themes/NAME (the record's file), a game's own theme.txt
in the save's folder (the hub's savedir; /ram/dq0 without one).

### The start's cost

themegen and themec take some 15 seconds of the Hydra's time at a new game, most of it the card: a READ from it
costs some 24,000 cycles whatever its size, and a byte some 340; themegen's program, its data and its five bank
programs, and themec, are some 90K.  The bank programs' loading matches each import against every export of the
caller's (some 5M cycles for themegen's four).  A game carried on reads its theme.dat and makes nothing.

## Still to come

- popgen (the denizens placed from the factions and the history), the Bestiary's kinds and habitats, peoples and
  their cultures and building styles, the game's own deities (the pop agent's: history.txt's peoples are a stand-in
  for them).
- Each goal kind played through with denizens: the evil's dead stilled as its curse breaks, the prisoner a denizen
  who follows (an ally), the guardians' lair at the goal's piece.
- A world of many regions (G2's later), the history's format already a region's.
