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

- **The world** keeps the character out of the denizens' squares, adds those it sees to what it perceives, and
  tells what happened to the character and the dungeon as events.
- **The denizens' task** decides what each denizen means to do, by its kind, its attitude, its faction's goals and
  what it senses (each denizen sees and hears through its own senses, as the character does); allies by the
  character's requests; and does it, by the character's rules, publishing what they do (events, above).
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

- **The denizens' task moves its own**, at every pace, by the character's rules (a step it can walk, its time by its
  pace and the slope; never onto the character's square, nor where its move ends, nor another's), on its own time:
  it reads the world's clock (the hub's now) as it goes, and the world never asks it anything, nor waits for it.  At
  the real paces it keeps up as it can (one fallen behind makes up a few seconds at most).  At the turns pace the
  world still moves the clock on a slice at a time once the denizens have caught up (den_at), so the game is the
  same whatever the tasks' timing: the tests' pace.
- **It publishes what they do as events**: its own queue in the hub (the denizens' events: one writer, each reader
  its own place, as with the world's; in a bank of their own, DEN_BANK), each an E_DEN: who (the actor), what (a step,
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
waited for both. Now the look is worked out in a grid of its own (the hub's LOOK_BANK: the squares round the
character copied in a chunk's run at a time, the lines cast in assembly, dq0p/cast.s, the same squares seen as
before), 0.29 seconds; the item store is gone down in assembly (dq0p/items.s), an inventory 0.14; a command that
wants nothing of the character (what it carries, its sheet, the map, the help) is carried out at once even while
it's busy, and the senses answer it between the look's rows: what's carried, asked for during a turn's look, told in
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
  and moves it a square at a time by the character's rules (a step it can walk, its time by its pace and the slope),
  itself, at every pace (the world once did, as it asked).  No two denizens share a square, and none goes onto the
  character's, nor where the character's move ends (the hub's pc_tx, pc_ty).
- **Time:** at the turns pace the world moves the clock on through what the character does a slice at a time (ten
  seconds: TURN_SLICE), each once the denizens' task has caught up with it (the hub's den_at; a task slow to, for a
  while, is left to catch up as it can), so the denizens act through all of it: a ten-minute spell of digging gives
  them ten minutes, and they may come and go while it lasts.  As it catches up the denizens' task moves its actors
  itself (the world waiting, by the same rules), cheaply: those out of the character's sight and not sensing it a
  few squares at a time (coarse), a watcher left a while unless the character changes (moves, its light, busy or
  not).  At the real paces it moves them as the clock runs, the world never waiting for it.
- **Away:** the denizens out but not actors move coarsely, every half minute to a minute and a half, to a square of
  their range about their lairs (in a chunk that's made: none is made for them), so the dungeon goes on where the
  character isn't; their records stay in the denizens' bank.
- **Seen:** the world adds the actors on squares the character sees to what it perceives, after each look; and as
  they change (the denizens' task bumps the hub's den_seq, or the world moves one it sees) it looks at them again,
  and the UI says what stirs only when one has come into sight, gone from it, or is doing something else (watching,
  coming towards it, keeping its distance, going about its business): not at each step.  Not while the character's
  on its way somewhere (it looks about as it gets there); while it digs, yes.
- **Heard:** what the actors do goes out as the denizens' events (a step quietly, a noise of their own every eighth,
  loud as a squeak; a dash past the character louder; the character sensed, or lost, silently; what lies between
  it and the character with each loud one, x[3]); the UI tells those the character hears and doesn't see, in each
  kind's words, and the sound task plays them (SOUND.md); the actors hear each other's, and the character's digging,
  and turn to them.
- **In the way:** the character can't walk onto a denizen's square: a walk along a passage, across a room or up to
  a square stops short of one (the way across a room found round them), and one in the very next square is told
  ("A giant rat is in the way.": E_NOWAY's N_DEN, the actor's).
- **Later:** attitudes, factions and influence; the referee's sheets (RS_NEW_ACTOR by the kind's key) and fights;
  their exploring and digging; the theme as data.
