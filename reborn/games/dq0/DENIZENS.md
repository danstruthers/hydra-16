# Dingus Quest Zero's theme and denizens

The design of what a dungeon is about and who lives in it: what the user asked for (2026-10-14), and how the
engine can do it.  The dungeon's shape is DUNGEON.md's; the rules questions go to the referee in rs.h's terms.

## What was asked

- **A theme.**  Each dungeon has a theme, chosen at random as the game starts, aligned with the puzzle the
  character must solve (the pieces to find, the mission: to escape, or to become strong enough to rule the place).
  The theme helps decide how the dungeon grows, what kinds of denizens it has, and their attitudes.
- **Not every denizen is hostile.**  Some can become allies: still run by the denizens' task, but working, perhaps,
  for the player.
- **Influence.**  Rhetoric and the other skills of persuasion and presence can change the attitude of a denizen
  that can be swayed that way; Animalcraft (HarnMaster's skill of handling animals) can do the same with animals.

## How

### The theme

A theme is data (a file of the game's, one of several, picked by the game's seed), not code.  It says:

- **the puzzle:** its pieces (what each is, where it may lie: a stratum's style or depth, a faction's keeping), what
  bringing them together does, and the mission's endings (the way out opened, the dungeon ruled);
- **the dungeon's growth:** its strata, from the top down: each one's style, age and size, as ranges to pick from
  (a drowned temple: built strata over flooded caves; a dwarf-hold: halls and mines, its deepest abandoned), the
  names, the words for rooms and their light, the features it favours (shrines, cisterns, forges, ossuaries), and
  where the pieces may lie;
- **its factions:** the peoples and creatures of the place, each with its kinds (the referee's creatures, by key), the
  strata where they live, how many, and their attitude to the character as they first meet it and to each other;
  what can sway them (persuasion, presence, a bribe, a shared enemy, a piece of the puzzle) and what can't (the mindless
  dead, a beast that's only hungry);
- **its animals:** the beasts, which Animalcraft can calm, befriend or drive off, and none of the persuasive skills
  can.

Built (2026-10-16), with three stand-in themes for the engine (the real ones are the user's to write): a crypt of a
dead king (the first, the one the dungeon was grown by before themes were data: the same styles, rooms and light, all
the bestiary), a drowned temple, a kobold warren.

- **The data:** a text file a theme (games/dq0/themes, in their files' order; the first has the format), made at build
  time the game's lib/dq0/dq0t.dat (make.js: theme.h's struct theme, 4K a theme with its words).  It says its name and
  its story (told as the game starts); its mission (escape, rule the place, or either); its strata from the top down,
  each one's name and the styles its plan may be grown in (one picked by its seed: so the stand-in's dice are as they
  were); its eight words for rooms (by a room's kind) and its rooms' light; the bestiary's kinds that live in it
  (dq0d/kinds.txt's keys: the denizens' task populates only those); its own kinds of things (eight at most: the
  catalogue's last eight, KI_THEME on, written into the hub's catalogue as the game starts), one of them perhaps
  fixed where it stands (an altar, a throne: never taken up); the puzzle's pieces, each a kind of its own, the strata
  it may lie in, hidden or in plain view; the rules of what's done with them, each with its words and its effect;
  the way out's lock in words, shut and open; and the endings' words (escaped, ruled, died).
- **Picked** by the seed's second byte, counted round the themes (a game's seed is the clock's, so any; the tests'
  small seeds pick the first), and kept in the save (game.dat, with the puzzle as it stands and the game's ending).
  The UI reads it into the hub's theme bank (THEME_BANK: every task's to read) and its overlay 5's bank (theme.c's
  copy: the theme's words, its puzzle).
- **The pieces placed with the plan** (DUNGEON.md): each one's stratum is picked from the game's seed among those it
  may lie in; when that stratum's plan is grown its place is decided, a chunk of a region of halls, from the stratum's
  seed (the maker's, dq0m/plan.c: the same each time it's grown), kept in the theme bank; and it's put down when that
  chunk is first made, on a square of a room's floor, by the maker's stock path (stock.c), concealed or not, as the
  theme says.  So it was there all along, and found as anything else is: seen, or searched for.
- **The rules:** read X (its words: a clue); combine X with Y (both to hand: they become Z, X's record kept, Y's
  gone); put X on (or in) Y (an offering on an altar, a crown on a throne); use X on the way out (open the way out
  with X).  The UI matches a command to a rule by the things' kinds (the world's item store's: what's to hand,
  IO_FIND; IO_COMBINE), tells its words, keeps it done (a bit each in the save), and does its effect: none more, the
  way out opened (the hub's way_open: the world lets the character up it, and it's gone: E_LEFT), or the place ruled.
- **The endings:** escaped (up the way out, opened), ruled (a rule's), or dead (a fatal wound); the theme's words, the
  game over, and the save marked done (dq0 -d on it again begins a new game).
- **Still to come:** the theme's say on the strata's ages and sizes, the features it favours, the things lying about
  by its style (the maker's styles' lists, make.c, as they are), its factions and its animals (the attitudes' work),
  the pieces' keepers (a guardian, a faction's hoard), and a rule's needing another done first.

### Populations

The user (2026-10-14): "Initial denizen populations and such should be determined at this time [as a stratum's plan
is grown] so encounters do not feel contrived, since the denizens of the dungeon are added based on the theme and
style."  So nothing is made up as the character turns a corner.  When a stratum's plan is grown (DUNGEON.md), the
theme's factions and animals for its style are placed in the regions that suit them: a lair, its numbers and kinds,
its territory, its ways about (patrols, hunting grounds, the water it goes to), and each denizen is a record from
then on (its kind, its faction, its attitude, where it is, what it's doing, its state: the referee's sheet once it
has one).  The denizens' task moves them all, but coarsely (a region at a time, by the hour) where the character
isn't, and square by square where it is; so a patrol met at a crossroads was walking its round, the rats in the
cellar were there all along, and a lair cleared stays cleared (or is moved into by something else, as the theme's
rules say).  Each stratum's populations go in its save with its plan.

The denizens explore too (the user: the dungeon can be made by the denizens' movements as well as the character's,
"which could cause complications for the denizens if other factions hostile to them are disturbed"): a denizen that
goes where nobody has been has the chunks made for it as the character would, and what it disturbs there (another
faction's lair, a sleeping beast, a guarded door) is an event like any other: the disturbed act against it, and its
own faction may answer them in turn.  Feuds the character never saw can spill into its way.

Some denizens dig (DUNGEON.md: changing the dungeon): a creature that tunnels by nature makes its own ways as it
goes, a faction of miners drives its galleries on, a besieged one digs a way out or a way in; and any of them can
break a wall or bring a ceiling down, on purpose or by mishap.  The changes are kept as the character's are, so a
passage that wasn't there yesterday is somebody's work.

### Attitudes

Every denizen has an **attitude** to the character, on one scale whatever the ruleset: **hostile** (attacks on
sight), **unfriendly** (threatens, keeps its distance, attacks if pressed), **wary** (watches, won't help), **neutral**,
**friendly** (talks, trades, tells what it knows), **ally** (comes with the character, fights beside it, takes its
requests), **devoted** (follows it into anything).  A faction has an attitude as a whole, which its members start
from; and attitudes between factions (a war, an alliance), so that what the character does to one changes what
others think of it.

An attitude moves by what happens: an **influence** (below), a gift, a rescue, harm done to it or to its own, a piece
of the puzzle shown or taken, the character's strength as it sees it (rs.h's power, 0-100: the strong are feared,
the weak despised or pitied, as the faction is).

### Influence

The player tries to sway a denizen it can talk to (a shared tongue: the referee knows the character's languages) or,
for a beast, one it can reach: **persuade**, **intimidate**, **deceive**, **entertain**, **bargain**; for animals, **calm**
and **befriend**.  Each is a purpose the referee tests (rs.h: P_PERSUADE, P_INTIMIDATE, P_ANIMALS ...; HarnMaster's
Rhetoric, Oratory, Intrigue, Animalcraft, Singing ...), as a **contest** where the denizen resists (its Will, its
faction's stubbornness), the circumstances as steps (its attitude now, the theme's say on what moves it, gifts,
threats seen, its fear of the character): a success moves the attitude a step its way (a critical one two), a
failure doesn't, a critical failure a step back (an insult, a bite).  Some attitudes can't move past a limit (a
guard's duty, a hunter's hunger) without more than words.

### Allies

An ally is still the **denizens' task's**: it moves, fights and acts on its own turn by its own nature, but its goal is
the character's; it follows the character, takes requests ("guard the door", "wait here", "follow me", "attack the
ghoul"), and keeps them as far as its attitude and courage go (the referee's morale).  An ally can be lost: harm it,
leave it, ask too much, and its attitude falls.  The party, designed for from the start, is the character and its
allies.

### In the tasks

- **The world** owns every body, the character's and the denizens' near it (the actors) alike: it carries out what
  each means to do by the same rules, strikes every blow (the referee asked), keeps the character out of the
  denizens' squares, adds those it sees to what it perceives, and tells what happened as events (the character's
  and the dungeon's; the denizens' bodies', their own queue's).
- **The denizens' task** is their minds: it decides what each denizen means to do, by its kind, its attitude, its
  faction's goals and what it senses (each denizen sees and hears through its own senses, as the character does);
  allies by the character's requests; and says so, its intents to the world, as the UI says the character's (move
  2.4, below).  The populations, the records and the denizens away from the character are its own.
- **The referee** answers the tests and contests (influence, morale, combat), and keeps each creature's sheet.
- **The UI** words it all, and parses the player's influence and requests ("persuade the goblin", "tell Grub to wait").

### Events: the denizens in the background

The user (2026-10-15, playing): "the UI is slow to respond in many cases. The cursor flickers constantly, and that is
a problem; probably an artifact of time sharing with the other tasks.  I was thinking that an event-driven system
might work better, where the denizens task is doing work in the backend, and it publishes changes as events, which
may or may not be perceived by the other denizens or player depending on the perceptions of each."

What was slow (measured in the emulator, the normal pace): a command the UI answers by itself took a fifth of a
second to start and most of a second to finish, and a move or a turn seven to nine seconds; with nothing happening
the CPU was busy 70% of the time, and the status line went out whole three times a second (a third of what the
serial line carries), the cursor jumping to it and back each time.  The time went: to the senses' look about after
every move and turn (some 18 million cycles: the squares round each wall looked at again from every wall, a square's
chunk found by multiplying, every thing's record copied twice), the world waiting for it, the clock standing; to the
status line on the line; to the tasks' loops (the world's clock in long arithmetic a hundred times a second; at the
turns pace the denizens' task spinning while it waited for the clock).  So the denizens weren't what held the UI up;
but the user's design is the right one for them, and it's this:

- **The denizens' task decides for its own**, at every pace, on its own time: it reads the world's clock (the hub's
  now) as it goes, and the world never asks it anything.  (It moved them itself, by the character's rules, till move
  2.4: now it says what each means to do and the world moves them, below.)  At the real paces it keeps up as it can
  (one fallen behind makes up a few seconds at most: the world's to see to).  At the turns pace it acts in its turn,
  the world's word at each slice of the clock, so the game is the same whatever the tasks' timing: the tests' pace.
- **What they do is published as events**: a queue of their own in the hub (the denizens' events: one writer, the
  world since move 2.4, each reader its own place, as with the world's; in a bank of their own, DEN_BANK), each an
  E_DEN: who (the actor), what (a step,
  a noise of its own, the character sensed, lost), how loud (0, silent, to 9), where (its stratum, its square), when,
  its kind, and whether the character sees that square now (the senses' marks: a hint for its perceiver).  An event
  doesn't say who perceives it.  Nobody waits for one, nor is rung: whoever reads them reads them as it goes, late
  or not at all, and holds nothing up.
- **Each perceives them by its own senses.**  The character: by sight, the senses' look (the squares it sees, lit,
  along a clear line: the world adds the actors on them to what it perceives as they change, the hub's den_seq, and
  the UI tells what stirs); by hearing, the UI's, from the event's loudness, its distance and the character's hearing
  (the referee's, 0-9): heard within loudness times (hearing + 3) / 2 squares, of one on a square it doesn't see (one
  it sees is told as seen, not heard), each square of wall or rock between four squares further, each shut door two
  (SOUND.md), by the way it lies, in its sound's words ("You hear a scurrying behind you to your left."), the same
  way not again for half a minute, nor any for five seconds; and played on the chips (the sound task's, SOUND.md: on
  the side it lies, the loudest at once on the channels there are).  A denizen: by hearing, the others' events and the
  world's (the character's noises: E_NOISE, digging), within its kind's hearing times the loudness over two: it turns
  to face the noise, stands a while, and looks for the character again at once (and so may see it).  Smell, later.
- **The protocol stays neutral**: the world's events are the character's and the dungeon's, the denizens' events the
  denizens'; a perceiver (the UI, the senses, the sound task, an ally's task one day) reads either and applies its
  own senses, and the hub only carries them.

After the first changes a move across a room still took five seconds and more to the prompt, and a turn nearly four
(the normal pace, seed 3's first crypt), and what the character carried three and a half: the senses' look still 1.7
seconds of their CPU (some 6 million cycles: a square's chunk found the long way when a line crossed between two,
the rays in C), an inventory two (every record of the item store gone through a dozen times, each found by
multiplying); and the UI took no command till the character was free, so what was typed during a move or a look
waited for both.  Now the look is worked out in a grid of its own (the hub's LOOK_BANK: the squares round the
character copied in a chunk's run at a time, the lines cast in assembly, dq0p/cast.s, the same squares seen as
before), 0.29 seconds; the item store is gone down in assembly (items.s, the world's now), an inventory 0.14; a
command that wants nothing of the character (what it carries, its sheet, the map, the help) is carried out at once
even while it's busy, and answered while the look goes on (the senses between its rows then; since move 2.3 the
world, the item store's, as it waits for the look): what's carried, asked for during a turn's look, told in
87,000 cycles from Enter (24 million before); the clock goes on while the world waits for a look; a turn three and
three quarter seconds to the prompt to one and a third, a walk across the room five and a half to three, with the
words; idle, the CPU 64% busy to 50% (the world's and the sound task's loops slower with nothing under way).

### So far (the first step)

- **Populations:** the denizens' task (games/dq0d) decides a stratum's as soon as its plan is grown (the hub's record
  whole), from its seed: for each kind that lives in the stratum's style at its depth, its lairs (by the stratum's
  ways in and out, for those that lair there; else a chunk of a region of halls) and how many in each, a record each
  from then on.  The kinds are a stand-in theme's (dq0d/kinds.txt: a name, the referee's key, its ways, the styles
  and depths it lives in, its numbers, its range, its senses, its pace), made a data file the task reads into a bank
  of its own.  The records (64 lairs, 240 denizens) are in another bank, kept with the game (dens.dat).
- **Placing:** a lair's denizens are put on squares of their own as the character comes into its chunk or one beside
  it (round the way, or in a room of the chunk), by the lair's own numbers, so it's the same whenever it happens:
  none on the way or beside it, nor beside the character (it comes out there, and isn't boxed in).  No lair is by
  the dungeon's way in (the top stratum's first: where the character begins).
- **Acting:** those within about 24 squares of the character are actors (the hub's, eight at most): each, as it's
  free, senses the character (seen by its light, or with none by darkvision, along a line clear of rock, walls and
  shut doors; heard moving, nearer), and wanders about its lair (a few squares, or its range), keeps to it, watches
  the character, keeps its distance (a shy one: till it's a few squares further; cornered close by, a step no
  nearer, a way round; cornered beside the character, it darts past it), or comes towards it (a bold one);
  and it's moved a square at a time by the character's rules (a step it can walk, its time by its pace and the
  slope): by the world, as the denizens' task says (move 2.4: before it, the task moved them itself).  No two
  denizens share a square, and none goes onto the character's, nor where the character's move ends (the hub's
  pc_tx, pc_ty).
- **Time:** at the turns pace the world moves the clock on through what the character does a slice at a time (ten
  seconds: TURN_SLICE), the denizens given their turn at each (since move 2.4: the hub's den_go; before it, once the
  task had caught up with the clock, den_at; a task slow to, for a while, is left to catch up as it can), so the
  denizens act through all of it: a ten-minute spell of digging gives them ten minutes, and they may come and go
  while it lasts.  In its turn the denizens' task decides for its actors, the world carrying it out as it goes,
  cheaply: those out of the character's sight and not sensing it a few squares at a time (coarse), a watcher left a
  while unless the character changes (moves, its light, busy or not).  At the real paces they act as the clock runs,
  the world never waiting for them.
- **Away:** the denizens out but not actors move coarsely, every half minute to a minute and a half, to a square of
  their range about their lairs (in a chunk that's made: none is made for them), so the dungeon goes on where the
  character isn't; their records stay in the denizens' bank.
- **Seen:** the world adds the actors on squares the character sees to what it perceives, after each look; and as
  they change (the world bumps the hub's den_seq as it moves one, or what one's doing changes) it looks at them again,
  and the UI says what stirs only when one has come into sight, gone from it, or is doing something else (watching,
  coming towards it, keeping its distance, going about its business): not at each step.  Not while the character's
  on its way somewhere (it looks about as it gets there); while it digs, yes.
- **Heard:** what the actors do goes out as the denizens' events (the world's: a step quietly, a noise of their own
  every eighth, loud as a squeak; a dash past the character louder; the character sensed, or lost, silently; what
  lies between it and the character with each loud one, x[3]); the UI tells those the character hears and doesn't
  see, in each kind's words, and the sound task plays them (SOUND.md); the actors hear each other's, and the
  character's digging, and turn to them.
- **In the way:** the character can't walk onto a denizen's square: a walk along a passage, across a room or up to
  a square stops short of one (the way across a room found round them), and one in the very next square is told
  ("A giant rat is in the way.": E_NOWAY's N_DEN, the actor's).
- **Sheets:** each actor has the referee's sheet (RS_NEW_ACTOR, by its kind's key, its record's number: dq0harn/fight.c's
  stand-in bestiary, the game's own numbers, each attribute varied by a die of its own so no die of the referee's is
  spent on it), made the first time and kept after (its wounds, its state) in a bank of the referee's, saved with
  the game (beasts.dat).
- **Fights** (the user's choice, 2026-10-16; the referee's, HarnMaster's melee, behind rs.h's RS_ATTACK): a hostile
  actor (its attitude: the second step's, below) sensing the character beside it strikes at it, a round at a time
  (the referee's round: at the turns pace a slice; at the real paces as the clock runs); an unfriendly one only when
  the character is beside it; a hostile one comes towards it.  The character strikes at one beside it ("attack the
  rat", "hit", "fight": a round of its time, the world's I_ATTACK, the blow posted, pc_foe) with the best weapon it
  holds (else its
  fists; or a fist, a foot, its head as it says: punch, kick, head butt the rat, by Unarmed, the book's hand, foot
  and head), defending by its stance ("defend dodge", "block", "counter", "none", "best"); its armour is what it wears
  (the things tell the referee what's held and worn, and how well made: RS_EQUIP's marks).  Every blow is the
  world's to strike (since move 2.4: the denizens' task's before), the referee asked in the world's slot; each is
  told (E_FIGHT in the denizens' queue, its words in a ring in their bank: the referee's for the wounds and the
  armour, the UI's for the rest, overlay 6), heard by the others (they turn to it) and played (the sound task's *hit,
  *swish).  A denizen wounded tests its morale (RS_MORALE: the book's Initiative test): broken (or a shy one
  cautious) it flees a minute; cautious, it holds back a round.  One knocked senseless lies there till it comes round,
  and is finished if it's struck meanwhile; one bleeding loses blood a minute at a time (RS_TIME) and may die of it;
  one dead leaves its body (an item: a body, a skeleton's bones: the world's, put down as it dies) and never comes
  back.  The character knocked senseless waits it out (it can't defend meanwhile); dead, the game ends.
- **Later:** allies fighting beside the character, and factions fighting each other; missiles (a bow through an
  arrow slit); the character's own bleeding and its treatment; their exploring and digging; the theme's factions
  (the theme as data is built: which of the kinds live in it).

### Attitudes and influence (the second step)

- **Factions:** a stand-in theme's (dq0d/factions.txt, made part of the denizens' data file, where a theme's will take
  their place): each one's tongue, its attitude to the character as it first meets it, and how it stands with each
  other one (hostile, wary, indifferent, friendly: the worse of the two ways it's said).  Each kind (kinds.txt) has
  its faction, its mind (mindless, a beast, low, average, cunning) and its own steps for or against swaying it.  The
  denizens' head keeps each faction's attitude now (its newcomers': a stratum populated later starts from it).
- **Attitudes:** each denizen's in its record (hub.h's AT_HOSTILE ... AT_ALLIED, saved with them), and an actor's in
  the hub's (struct actor's attitude and faction: the fights read them, a hostile one fights; its foe, an actor of a
  faction hostile to its own within four squares).  They move by what happens: a lair intruded on (a kind that keeps
  to its lair, the character beside its home: a step down, once a visit), the character seen with an ally of a
  faction hostile to its own (a step down, once), harm done (the one harmed hostile, its lair's kin two steps down,
  the rest of its faction one, the faction's own attitude one; those of factions hostile to it that sense it a step
  up, to friendly at most).  And each acts by its attitude: a hostile one comes at the character (to its side); an
  unfriendly one keeps three squares off, a bold one coming that near; a wary or indifferent one by its kind's ways (a
  shy one keeps its distance); a friendly one doesn't shy from it.  Denizens of factions hostile to one another keep
  off one another (a step away within three squares) till the fights come.
- **Influence:** the character's words (talk to, persuade, deceive, threaten; calm and befriend an animal) are the
  UI's request to the denizens' task (the hub's sway): it finds the one meant (a word of its name or its kind's, the
  nearest seen, or within four squares), and asks the referee in the UI's slot, lent it while the UI waits: a test of
  Rhetoric (P_PERSUADE, P_INTIMIDATE), Intrigue (P_DECEIVE) or, for a beast, Animalcraft (P_ANIMALS) at steps by its
  attitude (hostile -3 ... friendly +1), its kind's own and its mind (the low easily fooled and hard to reason with,
  the cunning hard to fool).  Words want a tongue in common: its faction's, said to the referee, which knows the
  character's (common, its own, and its file's "language" lines); a threat doesn't.  Nothing sways the mindless; words
  don't reach a beast (Animalcraft within eight squares); a critical success moves the attitude two steps, a success
  one, a critical failure a step back (a lie found out two, a threat laughed at hostile), as far as that way goes
  (talk and deceit to friendly, a threat to wary, calming to indifferent, persuasion and befriending to an ally).  A
  round's work (a threat half that).
- **Allies:** one won over follows the character by its own senses (to where it last sensed it, till it's beside it),
  comes along with it up and down a way (beside it as it goes: put on a square beside the character on the stratum it
  comes to), waits where it's asked, goes its own way when it's dismissed (friendly then).  Its requests are the
  parser's: follow me, stay here (every ally in earshot), tell Grub to wait, ask the rat to follow me, dismiss Grub,
  tell Grub to fetch the key (not yet: fetching things wants the things' store); a name given it (name the rat Grub:
  the head's names, kept) is its own from then on.  Left waiting out of the character's reach an hour, it goes its
  own way; harmed, it's hostile.
- **A blow** (the fights', above) is harm done, as above; one that flees is unfriendly after.

### Minds and bodies (Phase 2's move 2.4, 2026-10-16)

One authority on state (DUNGEON.md): every body is the world's, the character's and the denizens' alike, and every
blow.  The denizens' task is their minds.

- **The minds:** the denizens' task decides for each actor, as before, from what it perceives (its own checks still:
  move 2.5 gives it the senses' layers), its attitude, its faction and its plans (an ally's request, a noise heard,
  fleeing), and says what it means to do, as the UI says the character's: its intents, a ring of their own in the
  hub (di) that the world reads.  I_STEP a square (H_*; STEP_COARSE, a coarse move's: no time, nothing heard),
  I_GOTO a dash (past the character, two squares), I_CLIMB along with the character up or down (to a square beside
  it), I_WAIT, I_ATTACK a blow (the one struck at, the aim), I_ROUSE its wait cut short (a noise, a word, harm), and
  I_ENTER and I_LEAVE as it becomes an actor (its body handed the world from its record: its place, its step, its
  time, whether it bleeds) and stops being one; each with what it's doing (the world keeps it, and tells as one
  notices the character or loses it) and the way it faces.  The populations, the records (their places while they
  aren't actors: the coarse moves away from the character are the task's own), the attitudes, the memories, the
  sway, the names, the KEPT and records banks stay the task's.
- **The bodies:** an actor's record in the hub is two halves, its body the world's (state: a body there, its
  stratum, square, facing, when it's free, its step, what it's doing) and its mind the denizens' (its name, its
  record, attitude, faction, foe, kind, its remains, timid), each written by its own.  The world carries out the
  intents in turn by the character's rules (dq0w/wbody.c, a swapped part): a square it can walk (step_ok: no wall,
  no shut door), no one on it, nor the character, nor where the character's move ends; its time its step's, more
  uphill, steeply down, on steps, as the character's.  It tells what the bodies do (E_DEN, the denizens' events:
  the world's queue now), adds those the character sees to what it perceives, and bumps den_seq for its own look
  again.  (Not yet: a trap under a denizen's foot; a denizen at a door it can't walk through, opening it.)
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
  every five seconds at the real paces).  The referee sees the requests in the order it did when the denizens struck:
  the character's blow, the morale tests it brings, the bleeding, the denizens' blows.
- **The turns pace's handshake:** the world gives the denizens their turn (the hub's den_go, bumped, their doorbell
  rung) at each slice of the clock, and at the same clock after work of its own (an event told since their last: the
  character arrived somewhere, its work done, free again), and as the character gets somewhere, after the look and
  before the denizens it sees are told (so those placed there, as the look's chunks are made, are in it).  They act
  only in their turn, and say when they've done (I_DONE, the turn's number); the world carries out their intents as
  they come, asleep on its doorbell between, and goes on once the I_DONE is in (one slow to, five seconds, is left to
  catch up as it can).  So the game is the same whatever the tasks' timing: before it, the denizens took a pass as any
  event woke them, at whatever the clock was then, and a blow's or a stratum's arrival could fall either side of a
  slice.  The intents that change what another's decision reads (a step, a dash, a blow, a rouse, an entry) are waited
  for; a wait isn't, but goes with the next, the ring's order the world's (so a dig's eleven hours cost the emulator
  1.41 billion cycles, against 1.28 before: the handshakes).
- **On the way:** the world's look again after a thing's handled now adds the denizens it sees (it left them out
  since the item store's move); a body dead or gone from the world's keeping no longer has its bleeding reckoned (the
  referee told it dead again each minute).
