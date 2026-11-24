# DQ0: the decisions archive, 2026-10-30

The agents' accounts of what they built, moved out of DECISIONS.md on 2026-11-24 (branch docs2) with their text
unchanged, as AGENTS.md's Records rule asks: DECISIONS.md holds the user's words only. Each section left a one-line
pointer there; where a section held the user's words too, those stay in DECISIONS.md, quoted, and the whole section
is here. Each section is marked with its lines in DECISIONS.md as at d35194df.

<!-- DECISIONS.md:1302-1316 at d35194df -->
### The start's time measured, its progress shown

The user's rule (DECISIONS.md, the parser's task: a start of up to ten seconds is fine, shown; start-up work traded
for each command's speed) against the board: the start measured at 79 s (seed 1) and 104 s (a big seed, themegen's
history: every real game's) at 3.58 MHz, two thirds of it the SD card's reading (DUNGEON.md, the game's start).  Built:
the overlays' files packed (their fill left out, each bank read straight into its bank, dq0.ovl opened once: 70 s
and 94 s), and the progress shown throughout, no silence over about 10 s, the help's line with the first prompt.  And at the
real paces a stir that comes as a line's being typed held till it's entered and answered (the start's timing moved,
a rats' stir fell on dq0answer's inventory as it was typed: 795,000 cycles; the answers' times kept, D17).
Asked of the user (the lead's, 2026-10-30), not built: the overlays compressed (about 38M cycles less, 10 s); the
storage driver's sd_read with its byte loop in line (the bits' timing as it is: about 25M less, and every card read
quicker); the game on the ROM disk (its reads some 4.5 times as quick: the start some 35-40 s).

**Design:** DUNGEON.md, the game's start.

<!-- DECISIONS.md:1337-1364 at d35194df -->
### The answers' times held (the kernel's foreground)

The user (2026-10-30): the answer-time checks (D17's, dq0answer's and dq0prompt's) are the user's and never raised;
they swung 100,000-250,000 cycles with any change in how the tasks interleaved.  Their times are to hold whatever the
background is doing.

- **The kernel's foreground** (base/kernel/sched.s, TASKFORE: a general, opt-in hint any program can use): a task in
  it is picked before the others whenever it can run, a tick at most after it can, and a driver serving its call with
  it; the foreground has eight ticks at most, then the background the rest of a tick, so nothing starves.
- **The UI** is in it for good; **the world, the command parser, the senses and the referee** while a command's in
  hand (the hub's ui_cmd, from its Enter till the character's news; held while an intent's answer is owed).  The
  denizens' minds, the maker and the sound are the background.
- **The world** leaves the denizens' intents while a command's in hand at the real paces, a quarter second at most
  (a denizen's kit and its parts' copies could hold the UI's intent back a second).  The turns pace is as it was.
- **The lib folder's files** a task reads again and again are opened once and kept open (the outfits, the traces'
  words, the help): an open on the card was some 700,000 cycles.
- **The dice by who asks**: the referee keeps a stream of dice for each requester (the world's slot, the UI's, the
  denizens'), and the minds keep their own, apart from a stratum's population's and a lair's placing; so a game's
  fights at the turns pace go by its seed and its commands, not by how the tasks interleave or how the code is laid
  out.  The tests' `dq0 -R n` (0-252) offsets the dice: the same dungeon, other rolls; the tests whose rolls changed
  were given the offset that plays their game again, or their scene set (a steady kin, AM_STEADY; a room gone into).
- **Told once**: a stir's first sentence ("Something stirs.") said whatever, the rest not twice till the next line
  typed; "Everything goes black again." for the character's second fall.
- **An inventory's list** gathered of the character's things alone (its chain followed up), its alike things read
  where they lie; a line typed while the last's answer is owed gets the denizens' quarter second of its own.

**Design:** DENIZENS.md, the tasks giving way to a command in hand; docs/03-base/kernel.md, the scheduler.
