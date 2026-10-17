# Dingus Quest Zero's sound

What the character hears, and how the game plays it on the Hydra's chips: what the user asked for (2026-10-15), and
how the engine does it.  What makes a noise and who hears it are DENIZENS.md's events; the dungeon's portals and what
passes them are DUNGEON.md's.

## What was asked

The user (2026-10-15): "Hearing sounds should be per creature, but I am not really sure what that means yet.  Sound
the player hears should be a mixture of the whole environment, but only so many channels exist, so probably need to
prioritize sounds to those sources closest/loudest and divide them up on different channels based on direction so if
I am facing N and the sound is coming from the E, then I hear it in my right ear (or, at least it is louder in that
ear).  That alone would help with locating the source."

And earlier (2026-10-14, the plan): "I want there to be sound and music as part of the game as well (environmental
sounds especially)"; the YM2151 always, the Vera X's PSG if it's there; sparse and atmospheric, ambience and short
motifs at moments.

## How

### Per creature

"Per creature" is two things, and both are the kind's:

- **What a creature makes.**  Each kind has its own sounds, one for each thing it does that can be heard: a giant
  rat's scurrying and its squeak, a cave spider's skittering and its hiss, a ghoul's shuffling tread and its moan, a
  skeleton's dry clatter and the rattle of its bones, a cave bear's heavy tread and its growl, a kobold's light
  footsteps and its yapping.  A sound is data, the theme's like the kinds (a table of its own: dq0s/sounds.txt, by
  the kind's key, made lib/dq0/dq0s.dat): its **words** (what the character hears it as: "a scurrying", never the
  creature's name: a sound doesn't say what made it) and how each chip plays it (the YM2151: a patch or a drum and a
  note; the PSG: a waveform, its width and a note), in **hits** (a scurry is three quick ticks, a tread two slow
  thuds, a growl one long one), at a **level** (how loud it is at its loudest).  For now the acts are the denizens'
  events' two loud ones: a step (DE_MOVED: a dash past the character is a louder step) and a noise of its own
  (DE_NOISE); and a fight's blows (E_FIGHT: *hit, a blow landing; *swish, one that doesn't), the world's own sounds
  played where the attacker is; a death's and a voice's come later.
- **What a creature hears.**  Each hears by its own kind's hearing (dq0d/kinds.txt's): the denizens' task's, as
  DENIZENS.md says; the character by its own (the referee's, 0-9), as the UI and the sound task say below.  Both by
  the same rule: a noise of loudness L carries L times the hearer's hearing (the character's: (hearing + 3) / 2)
  squares.

### The environment, mixed

Everything the character could hear is a **source**: a denizen's events (each actor's steps and noises, on the
character's stratum), the world's events (the character's own: its digging, E_NOISE; a door it opens, shuts or
batters, glass it breaks, a portcullis's chains, E_PORTAL; a trap sprung, E_TRAP), and, later, the dungeon's own
ambience (water in a cistern or a stream, wind in a shaft: a source that sounds as long as the character's near it,
from the chunk's features).  The **sound task** (dq0s, the hub's T_SOUND) reads both queues as they come, by its own
place in each (nobody waits for it, nor it for anyone), and for each source it hears works out:

- **How far it carries**, by the character's hearing: heard within L times (hearing + 3) / 2 squares (as the UI
  tells it: DENIZENS.md's events), counted as the sound goes: each square of wall or rock on the line between it and
  the character as four squares further, each shut door as two (DUNGEON.md's: "a door, closed ... sound dulled").
  What lies between is the event's teller's to say (the world's since move 2.4, as it moves the denizens' bodies: it
  reads the squares, the event's x[3], walls and rock in the low four bits, shut doors in the high), since it has the
  dungeon's squares at hand and the sound task needn't.
- **How loud it is**: the sound's level, falling off as it goes: (reach - distance + 1) / (reach + 1) of it, never
  below a whisper; a quarter less from behind (ahead or behind: the chips can't tell the two apart by the speakers,
  so behind is softer); muffled (anything between) a few semitones lower too, duller.
- **Which way it lies**: its bearing from the character, against the way it faces (the UI's eight: ahead, ahead to
  the right, to the right, behind to the right, behind ...): to the right, ahead or behind to the right, the right
  speaker; to the left, the left; straight ahead or behind, both.  So facing north, a rat to the east is in the right
  ear.  (The chips' speakers are each on or off, not a balance: "at least louder in that ear" would take two channels
  a source, one each side at its own level; later, if the channels can be spared.)

### The channels

The chips give the game a few channels for the environment, and leave the rest to the music and the moments' motifs
(later) and the console's bell:

| Chip | Its channels | The environment's | Left |
| :--- | :----------- | :---------------- | :--- |
| the YM2151 (always) | 0-7 | 4, 5, 6 | 0-3 the music; 7 the bell (and the chip's noise) |
| the Vera X's PSG | 8-23 (its voices 0-15) | 20-23 (voices 12-15) | 8-19 the music |

So three sources at once on the board alone, seven with a Vera X.  The sound task claims its channels as it starts
(snd_claim: nobody else's writes reach them while it runs), and they're given back as it ends.

### The priority

As each pass of the sound task reads what's new, it keeps the latest of each source (an actor that's taken three steps
since is heard at the last), and plays them loudest first: each on the channel already playing that source (its own,
again), else a free one (a noisy sound the PSG's first, with a Vera X: its noise suits scurries and clatters; a
tonal one the YM2151's), else the channel playing the quietest sound, if this one is louder (that one's cut short);
else it isn't played.  So with more sources than channels it's the loudest, and so mostly the nearest, that are
heard, and a channel kept for a source as long as it keeps sounding (its hits played out, then free).  A channel
plays its sound's hits in real time (the system's tick: a hit held so many ticks, the next so many after), whatever
the pace; at the turns pace, where the game's time runs ahead in slices, the sources are heard as the slices are read,
the latest of each.  Paused, it's all silent.

### Without a sound chip, or a Vera X

The words always say it, whatever is played: the UI tells a source the character hears and doesn't see by the same
reach and the same way it lies, in the sound's words ("You hear a scurrying to your right.", "You hear a muffled
squeak behind you."), so it's the same game on the serial console, with no speakers, or in the tests.  Without a Vera
X it's the YM2151's three channels; without the sound driver (or its file, lib/dq0/dq0s.dat) the sound task puts the
words in the hub if it can and ends, and the game goes on silent; without the sound task, the UI's words are its own
("You hear something moving to your right.").

### In the tasks

- **The world** publishes the denizens' steps and noises as it moves their bodies (DENIZENS.md: the denizens' task
  says what each means to do, a noise of its own with it, since move 2.4), and with each loud one on the character's
  stratum what lies between it and the character (x[3]).
- **The sound task** (games/dq0s: dq0s SEG ROLE LIBDIR, started by the UI first of its tasks: the denizens and the
  world wait till its table's in the hub, a second at most) puts the sounds' table in the hub (the denizens' events'
  bank, after their ring: SND_AT, so the UI has the words), reads the world's events and the denizens', and plays
  what the character hears on its channels, panned by the way each lies.  With it the game is seven tasks (the UI,
  the world, the denizens, the referee, the maker, the senses, the sound): at the login shell's prompt HydraOS has six
  standing (the kernel, the shell, snd, kdev, storage, cons; vid too with a Vera X), so with the game running 13 of
  the 16 are taken (14 with a Vera X): room for a second window's shell and a program in it.
- **The UI** tells what the character hears and doesn't see, in the sound's words (main.c's heard; words.c's
  heard_words), by the same reach, walls and doors as what's played: the same way not again for half a minute, nor any
  for five seconds, so the words are fewer than the sounds.

### So far (the first step)

- **The table:** dq0s/sounds.txt, a line a sound: each kind's step and noise, by its key (kinds.txt's), and the
  world's (dig, open, shut, blow, smash, chains, trap), made lib/dq0/dq0s.dat (32 bytes a sound: kinds.txt's order,
  two a kind, then the world's), read into the hub's SND_AT as the sound task starts.
- **The sound task** (dq0s): the queues read every few ticks, the sources' latest kept, heard by the character's
  hearing (the referee's), and played as above on the YM2151's 4-6 and, with a Vera X, the PSG's 20-23.  dq0s -t
  plays a scene of its own (a rat to the east of a character facing north, then more sources than channels) and says
  where each went, for the tests.
- **Later:** ambience (the dungeon's water and wind, from the chunk's features), the denizens hearing through walls
  as the character does (the line from each hearer, not only the character's), two channels for a source to its side
  ahead or behind (louder in that ear), fights' and voices' sounds, and the music and motifs on the channels left.
