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
  those);
- **its own kinds of things:** eight at most (the catalogue's last eight, KI_THEME on, written into the hub's
  catalogue as the game starts), one of them perhaps fixed where it stands (an altar, a throne: never taken up);
- **its history** (HISTORY.md): the region, its peoples and eras, each stratum's layers, the inhabitants now (the
  factions: their kinds, strata, numbers, leaders, lairs, attitudes and how they stand with each other), the goal
  (an artifact, an evil, a prisoner, a far exit: its stratum and its guardians), each piece's era and guardians.

A game's own theme is themegen's (games/themegen, HISTORY.md): its region's history and its theme made from the
game's seed as it begins, a new region each game.  There are three stand-in themes, the tests' (a seed under 65,536
picks one): a crypt of a dead king (the first: the styles, rooms and light the dungeon was first grown by, all the
bestiary), a drowned temple, a kobold warren; each with a line or two of history and its factions, no layers.

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
chunk's layer of history and the stratum's age), not the theme.  The factions are the theme's (its faction lines:
factions.txt's, for the stand-ins, absorbed into them) for popgen to place; till popgen, the denizens' task reads its
own data (dq0d/factions.txt, below).

### Populations

Nothing is made up as the character turns a corner.  When a stratum's plan is grown (DUNGEON.md), the denizens'
task (games/dq0d) decides its populations at once, from the stratum's seed: for each of the theme's kinds that lives
in the stratum's style at its depth, its lairs (by the stratum's ways in and out, for those that lair there; else a
chunk of a region of halls) and how many in each, each denizen a record from then on (its kind, its faction, its
attitude, where it is, what it's doing, its state: the referee's sheet once it has one).  The kinds are data
(dq0d/kinds.txt: a name, the referee's key, its ways, the styles and depths it lives in, its numbers, its range, its
senses, its pace, its faction, its mind, its sway, its eyes), made a data file the task reads into a bank of its own
(lib/dq0/dq0d.dat, with the factions).  No lair is by the dungeon's way in (the top stratum's first: where the
character begins).

- **Placing:** a lair's denizens are put on squares of their own as the character comes into its chunk or one beside
  it (round the way, or in a room of the chunk), by the lair's own numbers, so it's the same whenever it happens: none
  on the way or beside it, nor beside the character (it comes out there, and isn't boxed in).
- **Away:** the denizens placed but not actors move coarsely, every half minute to a minute or so, to a square of
  their range about their lairs (in a chunk that's made: none is made for them), so the dungeon goes on where the
  character isn't; a patrol met at a crossroads was walking its round, the rats in the cellar were there all along,
  and a lair cleared stays cleared.
- **Kept:** the records are a cache by region in a bank of the task's own (240 denizens, 64 lairs: those of the
  regions round the character's always), the rest in the save folder by region (DUNGEON.md, no fixed size: each
  denizen and each lair a number of its own); the bank as it is in dens.dat, with its head (the factions' attitudes
  now, the allies' names, which strata are populated, which regions are in the cache).

### Attitudes

Every denizen has an **attitude** to the character, on one scale whatever the ruleset (hub.h's AT_*): **hostile**
(attacks on sight), **unfriendly** (threatens, keeps its distance, attacks if pressed), **wary** (watches, won't
help), **indifferent**, **friendly** (doesn't shy from it), **allied** (comes with the character, takes its
requests).  Each denizen's is in its record, saved with them, and an actor's in the hub's (struct actor's attitude
and faction: the fights read them, a hostile one fights; its foe, an actor of a faction hostile to its own within
four squares).

**Factions** (dq0d/factions.txt, part of the denizens' data file): each one's tongue, its attitude to the character
as it first meets it, and how it stands with each other one (hostile, wary, indifferent, friendly: the worse of the two
ways it's said).  Each kind (kinds.txt) has its faction, its mind (mindless, a beast, low, average, cunning) and its
own steps for or against swaying it.  The denizens' head keeps each faction's attitude now (its newcomers': a stratum
populated later starts from it).

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
- **In the way:** the character can't walk onto a denizen's square: a walk along a passage, across a room or up to
  a square stops short of one (the way across a room found round them), and one in the very next square is told ("A
  giant rat is in the way.": E_NOWAY's N_DEN, the actor's).

### Fights

- **Sheets:** each actor has the referee's sheet (RS_NEW_ACTOR, by its kind's key, its record's number:
  dq0harn/fight.c's stand-in bestiary, the game's own numbers, each attribute varied by a die of its own so no die of
  the referee's is spent on it: HOUSE.md's fights), made the first time and kept after (its wounds, its state) in a
  bank of the referee's, saved with the game (beasts.dat).
- **Blows** (the referee's HarnMaster melee, behind rs.h's RS_ATTACK): a hostile actor sensing the character beside it
  strikes at it, a round at a time (the referee's round: at the turns pace a slice; at the real paces as the clock
  runs); an unfriendly one only when the character is beside it; a hostile one comes towards it.  The character
  strikes at one beside it ("attack the rat", "hit", "fight": a round of its time, the world's I_ATTACK, the blow
  posted, pc_foe) with the best weapon it holds (else its fists; or a fist, a foot, its head as it says: punch, kick,
  head butt the rat, by Unarmed, the book's hand, foot and head), defending by its stance ("defend dodge", "block",
  "counter", "none", "best"); its armour is what it wears (the things tell the referee what's held and worn, and how
  well made: RS_EQUIP's marks).  Each blow is told (E_FIGHT in the denizens' queue, its words in a ring in their
  bank: the referee's for the wounds and the armour, the UI's for the rest), heard by the others (they turn to it)
  and played (the sound task's *hit, *swish).
- **After:** a denizen wounded tests its morale (RS_MORALE: HOUSE.md's fights): broken (or a shy one cautious) it
  flees a minute; cautious, it holds back a round.  One knocked senseless lies there till it comes round, and is
  finished if it's struck meanwhile; one bleeding loses blood a minute at a time (RS_TIME) and may die of it; one dead
  leaves its body (an item: a body, a skeleton's bones: the world's, put down as it dies) and never comes back.  A
  blow is harm done (attitudes, above); one that flees is unfriendly after.  The character knocked senseless waits it
  out (it can't defend meanwhile); dead, the game ends.

## Still to come

What the plans bring later, by the slice or phase that brings it:

- **History and themes** (Phase 3D): a region's history generated first (themegen), its eras, peoples and goal; the
  denizens placed as the history leaves them (popgen, a bank program), factions that fit the region and the history,
  from the HarnWorld Bestiary read as a reference (in the game's own words), with their relations, numbers, leaders
  and lairs; four goal kinds (an artifact found, evil cleared, a prisoner rescued, a way through); the theme checker.
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
