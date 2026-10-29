# Dingus Quest Zero's theme and denizens

The design of what a dungeon is about and who lives in it, as it stands: what it's for, how the engine does it, and,
at the end, what the plans bring later.  When and why each part was decided, in the user's words, is DECISIONS.md's;
the rules the game makes where the book is silent are HOUSE.md's.  The dungeon's shape is DUNGEON.md's; the rules
questions go to the referee in rs.h's terms.

## The aims

- **A theme.**  Each dungeon has a theme, chosen at random as the game starts, aligned with the puzzle the
  character must solve (the pieces to find, the mission: to escape, or to become strong enough to rule the place).
  The theme helps decide how the dungeon grows, what kinds of denizens it has, and their attitudes.
- **Not every denizen is hostile.**  Some can become allies: still run by the denizens' task, but working, perhaps,
  for the player.
- **Influence.**  Rhetoric and the other skills of persuasion and presence can change the attitude of a denizen
  that can be swayed that way; Animalcraft (HarnMaster's skill of handling animals) can do the same with animals.
- **Encounters that aren't contrived.**  The denizens are placed as a stratum's plan is grown, by the theme and the
  style, and go on living there whether they're met or not.

## How

### The theme

A theme is data (a file of the game's, one of several, picked by the game's seed), not code.  It says:

- **the puzzle:** its pieces (what each is, which strata it may lie in, hidden or in plain view), what bringing them
  together does, and the mission's endings (the way out opened, the dungeon ruled);
- **the dungeon's growth:** its strata, from the top down: each one's name, the styles its plan may be grown in
  (one picked by its seed) and its purpose, fortified or open (DUNGEON.md, built to a plan); the words for rooms (by a
  room's kind, and the built rooms' uses) and its rooms' light;
- **its denizens:** the bestiary's kinds that live in it (dq0d/kinds.txt's keys: the denizens' task populates only
  those), or its inhabitants, a faction a line (its theme file's, theme.txt in the save folder: who lives there now,
  as the history left them; popgen places them, below);
- **its own kinds of things:** eight at most (the catalogue's last eight, KI_THEME on, written into the hub's
  catalogue as the game starts), one of them perhaps fixed where it stands (an altar, a throne: never taken up);
- **its history** (HISTORY.md): the region, its peoples and eras, each stratum's layers, the inhabitants now (the
  factions: their kinds, strata, numbers, leaders, lairs, attitudes and how they stand with each other), the goal
  (an artifact, an evil, a prisoner, a far exit: its stratum and its guardians), each piece's era and guardians.

A game's own theme is themegen's (games/themegen, HISTORY.md): its region's history and its theme made from the
game's seed as it begins, a new region each game, its inhabitants among its lines (theme.txt in the save folder).
There are three stand-in themes for the engine and the tests (a seed under 65,536 picks one; the real ones are the
user's to write): a crypt of a dead king (the first: the styles, rooms and light the dungeon was first grown by, the
six kinds the bestiary had then), a drowned temple, a kobold warren; each with a line or two of history and its
factions written down, no layers (nothing of theirs in the save folder: their denizens the stand-in's, below).

- **The data:** a text file a theme (games/dq0/themes, in their files' order; the first has the format), made at build
  time the game's lib/dq0/dq0t.dat (make.js: theme.h's struct theme, 4K a theme with its words).  It says its name and
  its story (told as the game starts); its mission (escape, rule the place, or either); its strata; its rooms' and uses'
  words, and its light; its kinds; its own things; the puzzle's pieces, each a kind of its own, the strata it may lie
  in, hidden or in plain view; the rules of what's done with them, each with its words and its effect; the way out's
  lock in words, shut and open; and the endings' words (escaped, ruled, died).
- **Picked:** a game's own, made by themegen into the save's folder (theme.txt, and its record theme.dat: themec's),
  or for a seed under 65,536 a stand-in by the seed's second byte, counted round them (the tests' small seeds pick
  the first); `dq0 -t FILE` a theme written by hand, filled out by themegen.  Kept in the save (game.dat's Q.theme:
  a stand-in's number, or the folder's own; the puzzle as it stands the world's, its world.dat).  The UI reads it
  into the hub's theme bank (THEME_BANK: every task's to read) and its overlay 5's bank (theme.c's copy: the
  theme's words, its puzzle).
- **The pieces placed with the plan** (DUNGEON.md): each one's stratum is picked from the game's seed among those it
  may lie in; when that stratum's plan is grown its place is decided, a chunk of a region of halls, from the stratum's
  seed (the maker's, dq0m/plan.c: the same each time it's grown), kept in the theme bank; and it's put down when that
  chunk is first made, on a square of a room's floor, by the maker's stock (stock.c), concealed or not, as the theme
  says.  So it was there all along, and found as anything else is: seen, or searched for.
- **The rules:** read X (its words: a clue); combine X with Y (both to hand: they become Z, X's record kept, Y's
  gone); put X on (or in) Y (an offering on an altar, a crown on a throne); use X on the way out (open the way out
  with X).  The world judges a command of the puzzle's by the rules (the UI's request, IO_PUZZLE: the things found to
  hand by the item store, IO_FIND, the rule for their kinds, done: IO_COMBINE, IO_PUT), keeps it done (a bit each in
  its world.dat), and does its effect: none more, the way out opened (the hub's way_open: the world lets the
  character up it, and it's gone: E_LEFT), or the place ruled (the UI's to end the game); the UI tells its words
  (DUNGEON.md, who keeps what: the game's own rules).
- **The endings:** escaped (up the way out, opened), ruled (a rule's), or dead (a fatal wound); the theme's words, the
  game over, and the save marked done (dq0 -d on it again begins a new game).

The things lying about come from the maker's styles (dq0m/uses.c's lists, by the style, the rooms' uses, the
chunk's layer of history and the stratum's age), not the theme; the factions are a theme's inhabitants, or the
denizens' own stand-in's (dq0d/factions.txt, below).

### Populations

Nothing is made up as the character turns a corner.  When a stratum's plan is grown (DUNGEON.md), the denizens'
task (games/dq0d) decides its populations at once, from the stratum's seed.  If the theme has inhabitants (its theme
file's faction lines: dq0d/pop.h has their format), they're placed as the history left them by popgen (bank/popgen,
a bank program, bankprog.h's): each faction's number spread over its strata, the most on its lair's
and fewer the further from it, shared among its kinds (the first twice each other's share), in lairs of its kinds'
group sizes, its leader in its home (HOUSE.md, who lives where).  Else the stand-in's: for each of the theme's kinds
that lives in the stratum's style at its depth, its lairs and how many in each.  Each lair lies where its kind lairs
(by the stratum's ways in and out, for those that lair there; else a chunk of a region of halls), each denizen a
record from then on (its kind, its faction, its attitude, where it is, what it's doing, its state: the referee's
sheet once it has one).  The kinds are data, the game's bestiary (dq0d/kinds.txt: seventeen, modelled on the HarnWorld
Bestiary's creatures that suit a dungeon, and folk of the game's own; a name, the referee's key, its ways, the styles
and depths it lives in, its numbers, its range, its senses, its pace, its stand-in faction, its mind, its sway, its
eyes, and for the history its habitat, nature, size and arms, and what it leaves dead), made a data file the task
reads into a bank of its own (lib/dq0/dq0d.dat, with the stand-in's factions).  The world's peoples and deities are
data beside them (dq0d/peoples.txt, deities.txt: lore.h), the history's to draw on.  No lair is by the dungeon's way
in (the top stratum's first: where the character begins).

- **Placing:** a lair's denizens are put on squares of their own as the character comes into its chunk or one beside
  it (round the way, or in a room of the chunk), by the lair's own numbers, so it's the same whenever it happens: none
  on the way or beside it, nor beside the character (it comes out there, and isn't boxed in).  Each one's scent is
  left at the lair's home as it's placed (the user's, 2026-10-22: "repeated passage or occupancy should enhance prior
  smells"): a passing and the most stays there are (I_TROD to the world's traces: DUNGEON.md's a stay counts as
  passings), so a lair smells strongly of its keepers from the first ("A musky smell hangs heavy here: giant rats, by
  the smell of it."), and their comings and goings about it keep it fresh.
- **Away:** the denizens placed but not actors move coarsely, every half minute to a minute or so, to a square of
  their range about their lairs (in a chunk that's made: none is made for them), so the dungeon goes on where the
  character isn't; a patrol met at a crossroads was walking its round, the rats in the cellar were there all along,
  and a lair cleared stays cleared.
- **Their lights** (slice 4.1; DUNGEON.md, light kept): a kind that keeps lights (kinds.txt's f: kobolds and their
  lord, bandits and theirs, cultists and their priest) keeps lit the rooms about its lair, as far as its range takes
  it (its range either way from its lair's chunk's middle).  As a stratum's populations are decided, each such lair
  is written to the hub's places kept (KEPT_AT: its stratum, its chunk, its range, its keepers living) and the
  stratum marked done there: the world waits for that before its first chunk is made, and the maker lights the rooms
  by it as it first stocks them (their sconces' torches lit and tended).  As one of its keepers dies the keeping's one
  fewer; the last, and it's gone (the world tends its torches no longer: they burn down).  The dead, the beasts and
  the vermin keep none.  (Sweeping its floors, kinds.txt's t, is the traces'.)
- **Kept:** the records are a cache by region in a bank of the task's own (240 denizens, 64 lairs: those of the
  regions round the character's always), the rest in the save folder by region (DUNGEON.md, no fixed size: each
  denizen and each lair a number of its own); the bank as it is in dens.dat, with its head (the factions' attitudes
  now, the allies' names, which strata are populated, which regions are in the cache), the places kept lit after it.

### Attitudes

Every denizen has an **attitude** to the character, on one scale whatever the ruleset (hub.h's AT_*): **hostile**
(attacks on sight), **unfriendly** (threatens, keeps its distance, attacks if pressed), **wary** (watches, won't
help), **indifferent**, **friendly** (doesn't shy from it), **allied** (comes with the character, takes its
requests).  Each denizen's is in its record, saved with them, and an actor's in the hub's (struct actor's attitude
and faction: the fights read them, a hostile one fights; its foe, an actor of a faction hostile to its own within
four squares).

**Factions** (a theme's inhabitants; else the stand-in's, dq0d/factions.txt, part of the denizens' data file): each
one's tongue (a theme's faction speaks its first kind's stand-in faction's), its attitude to the character as it first
meets it, and how it stands with each other one (hostile, wary, indifferent, friendly: the worse of the two ways it's
said).  Each denizen's record has its faction (a kind may be in two); each kind (kinds.txt) its stand-in faction, its
mind (mindless, a beast, low, average, cunning) and its own steps for or against swaying it.  The denizens' head
keeps each faction's attitude now (its newcomers': a stratum populated later starts from it).

An attitude moves by what happens: a lair intruded on (a kind that keeps to its lair, the character beside its home:
a step down, once a visit), the character seen with an ally of a faction hostile to its own (a step down, once), harm
done (the one harmed hostile, its lair's kin two steps down, the rest of its faction one, the faction's own attitude
one; those of factions hostile to it that sense it a step up, to friendly at most), and an **influence** (below).  And
each acts by its attitude: a hostile one comes at the character (to its side); an unfriendly one keeps three squares
off, a bold one coming that near; a wary or indifferent one by its kind's ways (a shy one keeps its distance); a
friendly one doesn't shy from it.  Denizens of factions hostile to one another keep off one another (a step away
within three squares).

### Influence

The character's words (talk to, persuade, deceive, threaten; calm and befriend an animal) are the UI's request to the
denizens' task (the hub's sway, SW_*): it finds the one meant (a word of its name or its kind's, the nearest seen, or
within four squares), and asks the referee in the UI's slot, lent it while the UI waits: a test as a **contest**
where the denizen resists, of Rhetoric (P_PERSUADE, P_INTIMIDATE), Intrigue (P_DECEIVE) or, for a beast, Animalcraft
(P_ANIMALS), at steps by its attitude (hostile -3 ... friendly +1), its kind's own and its mind (the low easily fooled
and hard to reason with, the cunning hard to fool).  Words want a tongue in common: its faction's, said to the
referee, which knows the character's (common, its own, and its file's "language" lines: HOUSE.md's tests); a threat
doesn't.  Nothing sways the mindless; words don't reach a beast (Animalcraft within eight squares); a critical success
moves the attitude two steps, a success one, a critical failure a step back (a lie found out two, a threat laughed at
hostile), as far as that way goes (talk and deceit to friendly, a threat to wary, calming to indifferent, persuasion
and befriending to an ally).  A round's work (a threat half that).  The character can also look at how one seems.

### Allies

An ally is still the **denizens' task's**: it moves, fights and acts on its own turn by its own nature, but its goal
is the character's.  One won over follows the character by its own senses (to where it last sensed it, till it's
beside it), comes along with it up and down a way (beside it as it goes: put on a square beside the character on the
stratum it comes to), waits where it's asked, goes its own way when it's dismissed (friendly then).  Its requests are
the parser's: follow me, stay here (every ally in earshot), tell Grub to wait, ask the rat to follow me, dismiss Grub;
a name given it (name the rat Grub: the head's names, kept) is its own from then on (DUNGEON.md, names).  Left
waiting out of the character's reach an hour, it goes its own way; harmed, it's hostile.  The party, designed for
from the start, is the character and its allies.

### In the tasks

- **The world** owns every body, the character's and the denizens' near it (the actors) alike: it carries out what
  each means to do by the same rules, strikes every blow (the referee asked), keeps the character out of the
  denizens' squares, and tells what happened as events (the character's and the dungeon's, its own queue; the
  denizens' bodies', the denizens' queue).
- **The senses** answer for every actor (DUNGEON.md: perception as layers): which bodies the character sees, added to
  what it perceives (the world reports the bodies, the senses decide); and, as each denizen's mind asks, whether it
  senses the character.
- **The denizens' task** is their minds: it decides what each denizen means to do, by its kind, its attitude, its
  faction and what it senses (the senses tell it, by the same layers as the character's: its kind's sight,
  darkvision, hearing and eyes); allies by the character's requests; and says so, its intents to the world, as the UI
  says the character's (minds and bodies, below).  The populations, the records and the denizens away from the
  character are its own.  At the real paces its minds wait while the UI has a command in hand (the hub's ui_cmd, a
  second at most), as the senses' look does, so a command's answer doesn't wait for them.
- **The referee** answers the tests and contests (influence, morale, combat), and keeps each creature's sheet.
- **The UI** words it all, and passes the player's influence and requests to the denizens ("persuade the goblin",
  "tell Grub to wait").

### Minds and bodies

Every body is the world's, the character's and the denizens' alike, and every blow (DUNGEON.md, who keeps what).  The
denizens' task is their minds.

- **The minds:** the denizens' task decides for each actor from what it perceives (the senses'), its attitude, its
  faction and its plans (an ally's request, a noise heard, fleeing), and says what it means to do, as the UI says the
  character's: its intents, a ring of their own in the hub (di) that the world reads.  I_STEP a square (H_*;
  STEP_COARSE, a coarse move's: no time, nothing heard), I_GOTO a dash (past the character, two squares), I_CLIMB
  along with the character up or down (to a square beside it), I_WAIT, I_ATTACK a blow (the one struck at, the aim),
  I_ROUSE its wait cut short (a noise, a word, harm), and I_ENTER and I_LEAVE as it becomes an actor (its body handed
  the world from its record: its place, its step, its time, whether it bleeds) and stops being one; each with what
  it's doing (the world keeps it, and tells as one notices the character or loses it) and the way it faces.  The
  populations, the records (their places while they aren't actors: the coarse moves away from the character are the
  task's own), the attitudes, the memories, the sway, the names stay the task's.
- **The bodies:** an actor's record in the hub is two halves, its body the world's (a body there, its stratum,
  square, facing, when it's free, its step, its position, what it's doing) and its mind the denizens' (its name, its
  record, attitude, faction, foe, kind, its remains, timid), each written by its own.  The world carries out the
  intents in turn by the character's rules (dq0w/wbody.c): a square it can walk (no wall, no shut door), no one on it,
  nor the character, nor where the character's move ends (the hub's pc_tx, pc_ty); its time its step's, more uphill,
  steeply down, on steps, as the character's.  It tells what the bodies do (E_DEN, the denizens' events), and bumps
  den_seq for the senses to look at them again (those the character sees in what it perceives).  A denizen doesn't
  set off a trap underfoot, nor open a door it can't walk through.
- **The blows** (dq0w/wfight.c): the character's (I_ATTACK, the hub's pc_foe: struck at once at the real paces; at
  the turns pace as the clock moves on, before it, so the denizens see it first in the slice) and a denizen's (its
  I_ATTACK) alike, the referee asked RS_ATTACK in the world's own slot.  The world keeps what the blow left the body
  in (senseless, the time it's out; fleeing; shaken, a round held back; bleeding; dead: its remains, a body or a
  skeleton's bones, put down at once, and looked at again), puts the fights' words in their ring (FIGHT_AT, in the
  denizens' events' bank: the actors' names, the referee's lines) and tells it (E_FIGHT).  The referee answers and
  writes nothing of the hub's (its words its reply's).  The denizens take each blow to heart as they read it
  (fought): one broken flees a while, unfriendly; its kin, as one falls, test their courage (RS_MORALE, in their own
  slot); one the character struck at turns on it with its own (harmed); one dead, its record so.  The bleeding lose
  blood a minute at a time (RS_TIME, the world's), seen to as the denizens' intents come in (and their turns' ends,
  every five seconds at the real paces); a body dead or gone from the world's keeping isn't reckoned.  The referee
  sees the requests in one order: the character's blow, the morale tests it brings, the bleeding, the denizens'
  blows.
- **The turns pace's handshake:** the world gives the denizens their turn (the hub's den_go, bumped, their doorbell
  rung) at each slice of the clock, and at the same clock after work of its own (an event told since their last: the
  character arrived somewhere, its work done, free again), and as the character gets somewhere, after the look and
  before the denizens it sees are told (so those placed there, as the look's chunks are made, are in it).  They act
  only in their turn, and say when they've done (I_DONE, the turn's number); the world carries out their intents as
  they come, asleep on its doorbell between, and goes on once the I_DONE is in (one slow to, five seconds, is left to
  catch up as it can).  So the game is the same whatever the tasks' timing.  The intents that change what another's
  decision reads (a step, a dash, a blow, a rouse, an entry) are waited for; a wait isn't, but goes with the next, the
  ring's order the world's.

### Acting, and time

- **Actors:** those within 24 squares of the character are actors (the hub's, eight at most): each, as it's free,
  senses the character (the senses asked, the hub's dsense: seen lit, by its own light or the light layer's where it
  stands, within the kind's sight, or unlit within its darkvision, the less as its eyes are still used to more light
  than there is, along a line clear of rock, walls and shut doors; heard working, nearer; a far one answered coarsely,
  how far it may go with the answer standing, no line cast), and wanders about its lair (a few squares, or its range),
  keeps to it, watches the character, keeps its distance (a shy one: till it's a few squares further; cornered close
  by, a step no nearer, a way round; cornered beside the character, it darts past it), or comes towards it (a bold
  one), by its attitude (above); and it's moved a square at a time by the character's rules, by the world, as its
  mind says.  No two denizens share a square, and none goes onto the character's, nor where the character's move ends.
- **Time:** the denizens decide for their own, on their own time, at every pace: they read the world's clock (the
  hub's now) as they go, and the world never asks them anything.  At the turns pace the world moves the clock on
  through what the character does a slice at a time (ten seconds: TURN_SLICE), the denizens given their turn at each
  (the handshake, above), so they act through all of it: a ten-minute spell of digging gives them ten minutes, and
  they may come and go while it lasts.  In its turn the task decides for its actors cheaply: those out of the
  character's sight and not sensing it a few squares at a time (coarse), a watcher left a while unless the character
  changes (moves, its light, busy or not).  At the real paces they act as the clock runs, the world never waiting for
  them (one fallen behind makes up a few seconds at most).

### Events: the denizens in the background

What the denizens' bodies do is **published as events**: a queue of their own in the hub (the denizens' events, in a
bank of their own, DEN_BANK: one writer, the world, each reader its own place, as with the world's events), each an
E_DEN: who (the actor), what (DE_*: a step, a noise of its own, the character sensed, lost), how loud (0, silent, to
9), where (its stratum, its square), when, its kind, whether the character sees that square now (the senses' marks: a
hint for its perceiver), and with each loud one what lies between it and the character (x[3]: the squares of wall
or rock, the shut doors).  An event doesn't say who perceives it.  Nobody waits for one, nor is rung: whoever reads
them reads them as it goes, late or not at all, and holds nothing up.  The protocol stays neutral: the world's events
are the character's and the dungeon's, the denizens' events the denizens'; a perceiver (the UI, the senses, the sound
task, an ally's mind one day) reads either and applies its own senses, and the hub only carries them.

### Seen and heard

- **Seen:** the senses add the actors on squares the character sees to what it perceives, after each look; and as
  they change (the world bumps the hub's den_seq as it moves one, or what one's doing changes) the world asks them to
  look at them again (LK_DENS), and the UI says what stirs ("Something stirs.") only when one has come into sight,
  gone from it ("The giant rat to your left is gone from sight."), moved, or is doing something else (watching, coming
  towards it, keeping its distance, going about its business): not at each step, and nothing if it's as it was told
  (dq0/dens.c keeps what was told).  Those alike are told together ("Two giant rats are ahead to your right, 10 feet
  away.").  Not while the character's on its way somewhere (it looks about as it gets there); while it digs, yes.
- **Heard:** the denizens' events go out as the world moves their bodies (a step quietly, a noise of their own every
  eighth, loud as a squeak; a dash past the character louder; the character sensed, or lost, silently).  The UI tells
  those the character hears and doesn't see (one it sees is told as seen), in each kind's sound's words (SOUND.md):
  heard within loudness times (hearing + 3) / 2 squares (the character's hearing, the referee's, 0-9), each square of
  wall or rock between four squares further, each shut door two, by the way it lies ("You hear a scurrying behind you
  to your left."), the same way not again for half a minute, nor any for five seconds; and the sound task plays them
  on the chips (SOUND.md).  A denizen hears the others' events and the world's (the character's noises: E_NOISE,
  digging), within its kind's hearing times the loudness over two: it turns to face the noise, stands a while, and
  looks for the character again at once (and so may see it).
- **Smelt** (slice 4.1): a kind with a nose (kinds.txt's nose, 0-9) smells the character in still air within a third
  of it (squares: a rat's 2, a bear's 2, a man's none), along a line the air passes (no rock, wall nor shut door): the
  senses answer it with what's seen and heard (the hub's dsense: its nose's reach asked, SN_SMELT told), and it's
  sensed as if seen.  The character's scent the stronger as it stays where it is (hub.h's TR_MORE, of a passing each
  four minutes there: the minds' own count, pcstay), the nose's reach a third of the nose and those points more (a
  rat's 3 squares once the character's been on its square some three quarters of an hour, a man's 1).  Downwind
  further, upwind less, with the air layer (slice 4.5).
- **A stay** (the user's, 2026-10-22): an actor standing, resting or fighting where it is leaves its scent the
  stronger there, a passing each four minutes on its square (dq0d.c's stayed, as each goes by: I_TROD's TS_STAY, the
  world's traces).
- **A scent followed** (slice 4.1; the user's, 2026-10-21: keen noses follow a scent far longer than an hour): a kind
  with a keen nose (6 or more, K_NOSE: giant rats, ghouls, cave bears, cave adders, lindworms), hostile, that doesn't
  sense the character follows its trail, coming after it: the world's journal of the character's last 64 squares,
  the freshest beside it that its nose can still smell (hub.h's SCENT_LIFE: the character's sweat's smell, by its
  nose: a rat's most of a day, a bear's a day and a half), and on to the next; past them, the world's store's older
  trail (its intent STEP_SCENT: the world finds the way the character went from its square, or the freshest square
  beside it, each square's scent lasting the longer for the passings and the character's stays there: hub.h's
  TR_MORE; none, it stands a moment and goes about its business, and sniffs again ten seconds on).  So a rat that
  lost the character round a corner comes on after it, and a trail where it waited lasts the longer.
- **In the way:** the character can't walk onto a denizen's square: a walk along a passage, across a room or up to
  a square stops short of one (the way across a room found round them), and one in the very next square is told ("A
  giant rat is in the way.": E_NOWAY's N_DEN, the actor's).

### Fights

- **Sheets:** each actor has the referee's sheet (RS_NEW_ACTOR, by its kind's key, its record's number:
  dq0harn/fight.c's bestiary: the first six kinds' the game's own numbers, the rest the HarnWorld Bestiary's for the
  like creature; each attribute varied by a die of its own so no die of the referee's is spent on it: HOUSE.md's
  fights), made the first time and kept after (its wounds, its state) in a
  bank of the referee's, saved with the game (beasts.dat).
- **Blows** (the referee's HarnMaster melee, behind rs.h's RS_ATTACK): a hostile actor sensing the character beside it
  strikes at it, a round at a time (the referee's round: at the turns pace a slice; at the real paces as the clock
  runs); an unfriendly one only when the character is beside it; a hostile one comes towards it.  The character
  strikes at one beside it ("attack the rat", "hit", "fight": a round of its time, the world's I_ATTACK, the blow
  posted, pc_foe) with the best weapon it holds (else its fists; or a fist, a foot, its head as it says: punch, kick,
  head butt the rat, by Unarmed, the book's hand, foot and head), defending as the referee chooses each blow (a
  round's defences, HOUSE.md's: a block or a dodge, each once a round; its stance, "defend dodge", "block", "counter",
  "none", "best", puts one first, a counterstrike only so); its armour is what it wears (the things tell the referee
  what's held and worn, and how well made: RS_EQUIP's marks).  Each blow is told (E_FIGHT in the denizens' queue, its
  words in a ring in their bank: the referee's for the wounds and the armour, the UI's for the rest), heard by the
  others (they turn to it) and played (the sound task's *hit, *swish).
- **After:** a denizen wounded tests its morale (RS_MORALE: HOUSE.md's fights): broken (or a shy one cautious) it
  flees a minute; cautious, it holds back a round.  One knocked senseless falls and lies there till it comes round,
  doing nothing meanwhile (its mind's intents and rouses not carried out, words not heard: HOUSE.md's hands and
  limbs, 2026-10-29), and is finished if it's struck meanwhile; come round, its first turn is getting up; one bleeding loses blood a minute at a time (RS_TIME) and may die of it; one dead
  leaves its body (an item: a body, a skeleton's bones: the world's, put down as it dies) and never comes back.  A
  blow is harm done (attitudes, above); one that flees is unfriendly after.  The character knocked senseless waits it
  out (it can't defend meanwhile), fallen, up again as it comes round; dead, the game ends.
- **The character helpless** (senseless, or in shock: the referee's conditions in the hub's summary, C_UNCONSCIOUS
  while it lies out, C_STUNNED): what a foe does to it is its mind's, by its kind's nature (the user's, 2026-10-21;
  kinds.txt's ways k and r, the kind's record's K_HELP; HOUSE.md's fights).  A killer strikes on (the referee finishes
  one senseless where it lies); a robber comes up to it and robs it (I_ROB: the world takes what it holds and its
  pennies off to the robber's lair's home square), then flees a while, unfriendly; one that only defended itself
  flees a while, unfriendly, without another blow.  No foe engages it meanwhile (the world's engage initiative test
  waits for it to be itself again), and none cornered beside it strikes.  To come: a captor leading it away, an ally
  standing over it.
- **Missiles** (slice 4.7, 2026-10-29: HOUSE.md's missiles, for review): a hostile actor with a launcher of its own (a
  sling, a bow, a crossbow among its things: the world's agear AG_SHOOTS, as it tells the referee its things) and its
  missiles, sensing the character further off than beside it, shoots at it a round at a time (its mind's: I_ATTACK's
  AIM_SHOT, the world's to say if it can, dq0w/wshoot.c): its launcher taken up from its back or its belt first, its
  round's ("The bandit unslings its shortbow."), not if its hands hold something else; no shot to be had from where it
  stands (out of range, no clear line: AG_NOSHOT, till it moves), it comes on; its missiles spent, it shoots no more.
  The character beside it, it lets its launcher fall ("The bandit drops its shortbow.": no time) and draws its weapon.
  Its shots told as its blows are ("The bandit shoots at you with its shortbow: a serious puncture to your thigh"), its
  missiles lying where they fall.

### Their things

*(2026-10-27: the outfits' gear, the plan's phase 1; HOUSE.md's the denizens' gear, for review.)*

- **Made as a lair's placed:** for each of its folk and its dead the denizens' task posts I_KIT (its kind, its
  number, its lair's home square); the world draws its kind's role (games/dq0/outfits.txt's [role KEY], the index's
  entry by the kind's number: outfits.js's), every line given, on a die of its own, by the character's outfit's code
  (dq0w/outfit.c, outdraw.c, outmake.c: parts 57-59), and makes each thing its own at once.  They're the item store's
  (items.h's IW_DEN: its lair's home square, its number in the record's owner and hide), filed with that chunk (the
  paging's, ipage.c: in its region's file while the chunk's out), not seen lying there.
- **An actor's:** the hub's actor its denizen's number and its lair's home (did, hx, hy, hsn: the denizens'); the
  world reads its lair's chunk into the store as it comes near (I_ENTER) and keeps it there while it's an actor
  (ipage.c's kept_for), and tells the referee what it holds and wears (RS_EQUIP for the actor, the character's marks:
  dq0w/wlimbs.c's part 60), again as they change (drawn, let fall, taken up).  The referee keeps each actor's weapon,
  shield and pieces (gear.c's dwr, dsh, dpc) till it's made an actor again (RS_NEW_ACTOR): its blow by the weapon it
  holds, its armour by its pieces (fight.c's beast_side and guard; folk none but them, BF_FOLK).  Never told (a
  beast, dq0 -K), the bestiary's as before; the null referee ignores them.
- **Drawn as it strikes:** its hands empty of a weapon, its first blow draws the best on its belt instead, its round
  (WB_DRAWS); its weapon in one hand and a shield slung on its back, the next unslings the shield into its other hand
  (WB_READIES, 2026-10-29).  A weapon let fall is its own (the same record, on the floor and back in its hand).
- **Its shield** *(2026-10-29; HOUSE.md's the denizens' gear, for review)*: held, it blocks with it when that's the
  best of its defences, once a round (the referee's: fight.c's defence, gear.c's den_side; the null referee's two
  steps), told "it fends off the blow with its buckler" (rs.h's AF_SHIELD, the fight ring's FW_SHIELD).
- **Dead:** what it held where it fell, the rest on its body (the remains a surface: KF_ON), anyone's to take.
- **Where it sleeps** *(2026-10-27: the plan's phases 2 and 3; HOUSE.md's the denizens' belongings, for review)*: its
  lair's home is the room of its chunk its kind likes best (kinds.txt's home), its bed a square of that room's floor
  (its role's bed and stash things: a blanket, a kobold's straw and stash), its lair's own things with its first
  denizen's I_KIT (KT_LAIR: outfits.txt's [lair KEY], the larder, the water, the spare arms, the candles, the hoard);
  each lying there (IW_SQUARE, owner its kind's faction, a hoard's OW_HOARD | it), paged with that chunk as any.
- **First met:** as it's first made an actor (s_enlist; its record's D_MET, kept) its personal things (I_KIT KT_PURSE:
  [purse KEY], else [standing N] by kinds.txt's standing): its purse in its pouch, its valuables worn, a box at its bed
  locked with its key (a key made locks the last lidded thing made before it, and fits it alone), a thing worn beneath
  its clothes hidden (found by searching its body).  Since 2026-10-28 by the book's Personal Possessions (Treasure 2-3:
  each kind its category, its silver, gold and jewels by the table's chance and dice; DECISIONS.md's money, HOUSE.md's):
  a bandit a Brigand/Outlaw, its lord a Warrior/Noble too, the high priest a Cleric, the kobolds the Gargun's
  Hunters/Tribesmen; its things' condition the Treasure condition table's Person column (most good); its coins today's
  mints', four in five the region's own.
- **A robber's takings** (I_ROB's x[2], b): to its faction's hoard, in the chest at the home of a placed lair of its
  faction's leader (kinds.txt's keeps H) on its stratum (RB_HOARD); else hidden at its home if its kind hides its
  things (RB_HIDE); else lying there.
- **Seen:** the world keeps each actor's look (the hub's agear, AG_ARMED, AG_ARMOUR: as it tells the referee), the
  senses copy it with its body (struct sden's gear), the look words it ("a bandit, armed and armoured"); examined or
  assessed (IO_GEAR, the world's: dq0w/things.c's part 62) what it holds, wears and has at its belt, as far as the
  eyes make out (HOUSE.md's what's seen of an actor).
- **The beasts' dens and the dead's lairs** *(2026-10-29; HOUSE.md's the denizens' belongings, for review)*: a beast's
  lair's things too with its first's I_KIT (KT_LAIR alone: dq0d.c's place), outfits.txt's [lair KEY] for the giant
  rats, the cave spiders, the bear, the basilisk, the lindworm, the ghouls, the skeletons, the wights and the shade:
  bedding, its prey's bones, a past victim's remains with its things, the dead's grave goods, found (Treasure 1's Found
  column) and of old (their stratum's age; their coins its oldest era's people's).
- **Not yet:** the robbers' own stashes (no kind hides its things yet but the kobolds, whose takings are their lord's).

## Still to come

What the plans bring later, by the slice or phase that brings it:

- **History and themes** (Phase 3D): a region's history generated first (themegen), its eras, peoples and goal;
  factions that fit the region and the history, from the bestiary (built: the denizens placed as a theme file's
  inhabitants say, by popgen, with their relations, numbers, leaders and lairs); four goal kinds (an artifact found,
  evil cleared, a prisoner rescued, a way through); the theme checker.
  The theme format completed (Phase 7): factions and tongues, the bestiary's kinds and numbers, the pieces' keepers
  (a guardian, a faction's hoard), the strata's ages and sizes, the things lying about by theme.
- **The command interpreter** (Phase 3C): a denizen's mind making the same command structure as the player's, the
  world applying it the same way (opening and breaking doors, taking things, throwing, attacking).
- **Daily life** (slice 4.5): schedules by kind and faction, patrols that keep a pattern; the sound layer (the
  denizens hearing through walls as the character does, the character's steps heard by its pace).
- **Body position and moving quietly** (slice 4.6): every actor's position and gait, Stealth tests against the
  denizens' Awareness.
- **Fights complete** (slice 4.7): one combat path, missiles (a bow through an arrow slit), allies fighting beside the
  character, factions fighting each other, morale from the layers (leaders seen or heard, darkness, din).
- **Fetching and influence** (slice 4.8): an ally fetching things (tell Grub to fetch the key), gifts, bribes,
  reputation by deeds; an ally moving aside for the character (D10).
- **Tunnellers and builders** (slice 4.11): denizens that dig their own ways, by the same rules as the character's.
- **Psionic and magical denizens** (slices 5.4, 5.7): kinds with talents, shielded minds, casters, the undead.

Designed, but in no slice of the plans yet (DECISIONS.md has the user's words for most):

- **The denizens exploring**: a denizen that goes where nobody has been has the chunks made for it as the character
  would, and what it disturbs there (another faction's lair, a sleeping beast, a guarded door) is an event like any
  other, the disturbed acting against it and its own faction answering in turn; a lair cleared moved into by
  something else, as the theme's rules say.
- **Breaking** a wall or bringing a ceiling down, on purpose or by mishap, the denizens' as the character's.
- **More of the attitudes**: a **devoted** one past allied (following the character into anything); **entertain**
  and **bargain** beside persuading, deceiving and threatening; an attitude moved by the character's strength as the
  denizen sees it (rs.h's power, 0-100: the strong feared, the weak despised or pitied), by a rescue, or by a piece of
  the puzzle shown or taken; limits a guard's duty or a hunter's hunger sets on words.
