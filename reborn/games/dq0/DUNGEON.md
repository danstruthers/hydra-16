# Dingus Quest Zero's dungeon, in three dimensions

The design of the dungeon the game makes and the character moves through, as it stands: what it's for, how the
engine does it on a 6502, and, at the end, what the plans bring later.  When and why each part was decided, in the
user's words, is DECISIONS.md's; the rules the game makes where the book is silent are HOUSE.md's; what the dungeon
is about and who lives in it, DENIZENS.md's; what's heard, SOUND.md's.

## The aims

- **3D.**  Up, down and slopes count.  Each **area** has an elevation; its **exits** join other areas' exits by
  **transitions**: slopes, vertical walls, level hallways, shafts, each taking the difference of elevation into
  account.  A room with three zones of elevation is three areas with slopes between.
- **Sloped floors.**  An area whose floor slopes evenly: each exit (connection point) has its own elevation, and the
  slope between them is uniform.
- **Exits of every kind.**  Windows are exits, with an elevation above the floor; shafts in a ceiling or a floor are
  exits too; a ladder down is one.
- **Ceilings.**  Each area has a ceiling height and a kind (flat, cathedral, domed ...): a dome, its height at the
  middle and at each wall, for example.
- **Movement.**  Within an area, by the character's movement rate; a transition between areas of different
  elevation, by its distance, its slope and the character's effective movement rate.
- **Strata.**  Stacked layers: within a stratum areas don't overlap in plan, though their elevations differ freely;
  shafts, stairways, ladders and long ramps join strata.  Depth in feet is the truth; a stratum has a name, the
  theme's, not a number.  The map is a plan, its heights marked.
- **Strata of any size.**  One 200 feet square; the next a storm cellar, 10 by 10, with a ladder down to catacombs
  that go on for miles in each direction.
- **No fixed size.**  The dungeon is as big as it needs to be, limited only by the room on the card.
- **Style and age.**  Each stratum has a style (natural, purpose-built ...) and an age, and the dungeon *grows* by
  rules that follow from them.
- **Water.**  Underground rivers follow downhill rules, with waterfalls, and fill lakes, by growing rules like the
  rest.
- **Theme.**  Each dungeon's theme, aligned with its puzzle, helps decide how it grows (DENIZENS.md: the theme says
  its strata's names and styles, and where the puzzle's pieces may lie).
- **The character's senses.**  It's all seen and heard through the character: its facing (the compass's eight), its
  eyes and ears as they are now, the light it carries and the light of the place; the map shows what it has seen,
  with its state (where it is, its facing, its attributes as they are now) beside it.

## How

### Scale: chunks

A square is 5 feet.  A stratum has bounds of its own, from one square to tens of thousands each way (a stratum's
coordinates are 16 bits: 65,535 squares is 62 miles).  Nothing holds a whole large stratum: it is made a **chunk**
at a time, 32 by 32 squares (160 feet), as the character comes near, from the stratum's seed and the chunk's place,
so the same game always makes the same chunk.  A chunk is 2,720 bytes (each square's kind and floor elevation, its
rooms and their zones, its openings, portals and traps, its materials), three to an 8K bank.

The chunks are kept in a **cache in the hub's shared banks** (dq0w/world.h: 24 slots, three to a bank), those near
the character, so the world, the denizens and the senses read them where they are.  A chunk that goes out of the
cache is dropped; what changed there (doors opened, traps set off, rock dug) is kept as change records, laid over
the chunk when it's made again, and what the character's seen of it as a map of bits.  Every store that grows as the
dungeon's explored is a cache in the banks of what's near, the rest in files on the card (no fixed size, below).

### Who keeps what

Every piece of the hub's state has one task that writes it (hub.h's "Who writes what" lines; the tests' writer
check, sim/lib/hubown.js, fails a test on any write those lines don't allow).  The others read it, or ask its owner
by a request or an intent.

- **The maker** (dq0m) is generation only: the same seed always makes the same chunk.  It grows a stratum's plan and
  makes a chunk when it's asked (M_PLAN, M_CHUNK, M_SLOT), lays the change records over a chunk it makes again
  (reading them, never writing them), and lists a chunk's things the first time it's made (its stock: a list beside
  the chunk's slot in the world bank, items.h's struct stock, the slot marked CS_FRESH, for the world to put them
  down and mark it stocked).  It keeps nothing of its own but a copy of the senses' memory, the squares seen and the
  rooms come into, as the cache makes way for a chunk (seen.dat, and the seen maps by region: laid over the chunk as
  it's made again, so the map it draws for the UI from the cache has them), and the pages of a plan too big for the
  hub (no fixed size).  It draws the map (dq0m/map.c) as the UI asks.
- **The world** (dq0w) owns the clock, the portals and the traps and their working (opened, forced, battered,
  raised, climbed through; set off, found, disarmed; a search: dq0w/wdoor.c, wblow.c, wwin.c, wtrap.c, swapped parts,
  a command's work all in one of them), asks the referee for its judgements itself (its own slot), and writes the
  **change records** (dq0w/store.c), the only one to: each a record (a stratum, a square, what it became: its kind,
  its floor, the link it is; or a portal's state, its harm, a trap's state, a room's light, its sconce's; a chunk
  stocked), a square's record of a
  kind replacing the one before it, kept by **region** (256 squares each way, the plan's) in the 64 pages of the
  hub's two change banks, under the hub's change_lock (the maker reads them meanwhile), and paged to the save folder
  (no fixed size).
- **The item store** is the world's too, its only writer, so it has no lock: the catalogue (read into the hub's
  catalogue bank as the world starts, from its own file, dq0w.cat; the theme's kinds added after it as the UI asks,
  IO_THEME) and every item's place and state (dq0w/things.c, its code in the world's swapped parts and, for a thing
  handled, in the catalogue's bank).  The others ask, or say what they mean: the **UI** asks the world for what's
  carried and for every command about things (the hub's ireq: take, drop, put, light, wear, the kit, a search ...),
  the world answering in its loop (and, while it waits for a look, what wants none: what's carried, the tools, a
  tool's wear); the **maker**'s stock is a list the world puts down, in the order the chunks were stocked, before
  anyone looks for them (the world as it takes its intents, answers the UI or works a door; the **senses**, as a look
  comes to a chunk stocked that isn't put down yet, ask it first: the hub's stock); a **denizen**'s remains the world
  puts down itself as it dies; the senses read the records for what the character perceives, and the world makes a
  thing its own (its number, its make: no two alike) as a look first sees it, after the look, in the order seen.  The
  character's light (pc_light, pc_lamp) is the world's, worked out as what it carries changes.  The records (744, in
  two of the hub's banks) are a cache: what the character carries, and the things of the chunks round it, a chunk's
  all or none, the rest in the save folder by region; kept with the game (items.dat).
- **The bodies and the blows** are the world's: every body in the dungeon, the character's and the denizens' near it
  (the hub's actors, their body halves: place, facing, when free, step, position, what each is doing), and every
  blow struck.  The **denizens**' task (dq0d) is their minds: it decides what each means to do, by what the senses
  tell it each perceives, its attitude, its faction and its plans, and says so, as the UI says the character's: its
  intents, a ring of their own to the world (the hub's di: I_STEP, I_GOTO, I_CLIMB, I_WAIT, I_ATTACK, I_ROUSE,
  I_ENTER, I_LEAVE, I_DONE; DENIZENS.md, minds and bodies).  The **world** carries them out by the character's rules
  (dq0w/wbody.c: a square it can walk, no one on it, its time by its step and the slopes), tells what the bodies do
  in the denizens' events (E_DEN, E_FIGHT), and strikes every blow (dq0w/wfight.c), the character's (I_ATTACK,
  pc_foe) and a denizen's alike: the **referee** asked in the world's slot, what the blow did kept by the world
  (senseless, fleeing, bleeding, dead: the remains an item), its words put by the world in the fights' ring.  The
  referee writes nothing of the hub's but its own replies and records (the sheets, pcstatus, pcsum).  The denizens
  take each blow to heart as they read it (their courage, RS_MORALE in their own slot; their attitudes), and keep
  the places kept lit (the hub's KEPT_AT: light kept, below), as they decide a stratum's populations.  At the
  **turns pace** the world gives the denizens their turn (den_go) at each slice of the clock and after its own work
  at the same clock, and waits for their I_DONE, carrying out their intents meanwhile: the same game whatever the
  tasks' timing, the referee's one stream of dice seeing the same requests in the same order.
- **The senses** (dq0p) answer for every actor, and own every actor's memory of what it has perceived: what the
  character sees (the look, as the world asks: the hub's look) and what each denizen senses (the denizens ask, for
  their minds, as one decides: the hub's dsense), by the layers below (perception as layers); the squares the
  character has seen and sees now (the chunks' tiles' T_SEEN and T_VIS: the senses' alone to write, the squares a
  walk went felt and remembered as the senses go them again, the world passing the walk in the look), the rooms it's
  come into (R_VISITED: the world asks with LF_ARRIVE and takes the first time from the reply), and which of the
  world's bodies it sees (the look's end, and LK_DENS as the bodies change: the world reports, the senses decide).
  The worked-out layers (the light layer's grids) and each actor's eyes (adapted to the dark) are the senses' cache,
  in their own bank and main RAM, rebuilt as wanted, never another task's and never saved (a game carried on begins
  with eyes used to the light where the character stands).  The world's own changes to a chunk's squares (a square
  dug, a pit opened: store.c's change) keep the seen mark; the maker keeps a copy of the memory for the chunks out of
  the cache (above).  The look's own work is in the senses' swapped parts: its squares seen (dq0p/view.c, cast.s), its
  walk and its lamps in part 1, what it perceives (sight.c) in part 2; the light layer's grids, the eyes and the
  lights far off (light.c, eyes.c), the things seen and what the senses answer the denizens (sense.c) are the main
  RAM's.  The look's grid (the hub's LOOK_BANK: the squares round the character, copied in a chunk's rows at a time, its
  lines cast in assembly) is kept from one look to the next while nothing in it has changed, and moved as the
  character steps.  At the real paces the look gives way to a command the UI has in hand (the hub's ui_cmd, half a
  second a look at most; not to one waiting for an answer of the world's that waits for the look), so the world's
  answers and the UI's words don't wait for it.
- **The game's own rules** are the world's too: the **theme's puzzle** (theme.h) is judged by the world
  (dq0w/wgame.c): the UI takes a command of the puzzle's (read the tablet, combine the sunstone with the moonstone,
  put the crown on the throne, use the seal on the way out) and asks the things' request of it (IO_PUZZLE: the verb,
  TR_*, and the things named), and the world finds them to hand (the item store's IO_FIND), looks for the theme's
  rule for their kinds, and does it (the things made one, IO_COMBINE's; put, IO_PUT's; the way out opened, the hub's
  way_open, which E_LEFT goes by), keeping the rules done; its reply is how it went (PZ_DONE and the rule, PZ_NONE,
  PZ_NO with the things' words, PZ_FAR), which theme.c only words (the rule's words, the endings).  **A thing walked
  over to** is the world's walk and its taking: a thing out of reach (IO_GET's IR_NOT_HERE, its square and its item)
  is the UI's I_TAKE, and the world walks there (again, as long as the walk gets it nearer: a denizen in the way, a
  trap that stopped it), takes it up as IO_GET would, and tells it (E_TAKEN: taken, the UI then asking its time and
  its light as for any thing handled; its hands full; a portal's part, or a thing fixed where it stands, the UI's to
  lift out or to word as the character's free).  A thing fixed where it stands (a theme's altar) the item store
  refuses itself, with its words (IO_GET's IR_NO).
- **The character's state** is the world's: its pace and watchfulness (the hub's gait and watch: the UI's request as
  the player says, the hub's wreq, WQ_SET's WS_PACE; the referee words them, RS_DESCRIBE b 3, and reads a character's
  file's for a new game, RS_LOAD_PC's reply), its stance (WS_STANCE), what it strikes with (sent with the blow,
  I_ATTACK's and I_PORTAL's x[1]: the hub's pc_strike), and its **body's position**: standing or prone (crouched,
  kneeling and sitting are named, hub.h's POS_*, but nothing takes them yet), the hub's pc_pos for the character and
  each actor's pos (and its gait) for the denizens' bodies, the world's records.  The referee reads the positions as
  it judges (the world puts them in the request, RS_ATTACK's RS_PRONE_A and RS_PRONE_D: a prone defender the easier
  to strike, a prone attacker only getting up, its turn spent) and reports a fall (a stumble, a leg giving way: an
  X_COND_ON C_PRONE effect) and a getting up (O_RISE), which the world applies; it keeps no position of its own.  The
  game's start, pause and end are the world's to set as the UI asks: begun (WQ_START, before the UI's things: the
  ruleset's round, the pace and its real ticks a round, a new game's pace and watchfulness), paused and going on
  (WS_PAUSE), ended (I_QUIT: the hub's quit, the world first, the others after it).
- **The command** is the parser's (dq0c, a task of its own, in assembly: games/dq0c): the UI posts each line to it
  as it's taken (the hub's cq) and takes its verdict where the command is carried out: whether the grammar has it
  (commands.ebnf), and the command it makes (hub.h's struct cmd: an action, actions.h's VB_*, the catalogue's,
  commands.actions, and its roles: the thing, the other, the way, how, the manner ...; made by dq0c from the verbs as
  data, commands.verbs, the game's words, commands.words, and the catalogue's and the game's things' names the UI
  sends it as the game starts).  A line the grammar doesn't have is "I don't understand that."  The UI resolves the
  command's things among those the character perceives (resolve.c: "it", the thing a bare verb takes, which of
  several, the target, a question only where it must be one) and carries it out by its action; the world throws a
  thing for it (the item store's IO_THROW: the catalogue's reach, the referee's Throwing).

### The save

The save is on the SD card the player chooses: a folder of the game's (asked for as a new game starts, or given: dq0
-d /sd/1/dq0), holding the game's state (the clock, the character, the referee's sheets), each stratum's record and
populations, and the dungeon's changes, written as they change (so a game quit is kept, and a game whose machine is
switched off loses little).  It's kept as the game ends (quitting keeps it, death too), on "save", and as the
character comes into a stratum; the plans aren't kept, but grown again from their seeds, the same.  The files: the
UI's game.dat (the seed, the pace chosen, the character, the theme and the puzzle's ending, and the names given),
the referee's pc.hm (the character's sheet, RS_SAVE) and beasts.dat (the creatures' sheets), the denizens' dens.dat,
the maker's seen.dat, the world's world.dat (hub.h's struct wsave: the clock, the character's place, its pace and
watchfulness, its body's position, the puzzle's way out and rules done, then the strata's records), items.dat and
changes.dat, and the region files (no fixed size).  The UI asks each in turn (WQ_SAVE the world's): the referee's,
the maker's and the denizens' first, then the world's (its change records, its things, its own, then the versions
written made the save's: its last step), then game.dat.  A game carried on (dq0 -d on a folder with a save) is read
back by each (WQ_START, the hub's resume, the world's).  Only a save of the game's own version (main.c's SAVE_VER) is
carried on: an older one isn't read, and a new game begins in its folder.

Death is permanent: a game ended (dead, escaped, ruled) is marked done in its save, and dq0 -d on its folder begins
a new game there.  (A death in a fight doesn't mark it yet: main.c's blow ends the game without the theme's ending,
so the save is carried on.)

### No fixed size

Every store that grows as the dungeon's explored is a cache in the banks of what's near, and the rest is in the save
folder on the card, a file for each of its regions, read in as it's wanted and written back as the cache makes way
for another's.  Without a save folder each cache keeps what it can hold.

**The region files** (dq0/vfile.inc, every writer's and reader's: the world's, the denizens', the maker's): a store's
region (rgSXXYY.dat the change records', itSXXYY.dat the things', dnSXXYY.dat the denizens', smSXXYY.dat the seen
maps', stSS0000.dat a stratum's record, plSS0000.dat a big plan; S or SS the stratum, XX YY the region, in hex) is
written whole, as a new version, into one of its file's slots (.dat, .da1, .da2), its header (the game's generation,
a stamp) written last, so a version cut short is no version.  It's written over a slot that's neither the last
save's version nor (three slots: the things', a region's file read as its next version's written) the newest.  Read,
it's the newest valid version: one stamped up to the last save (the hub's vsaved) or in this session (past its
vbase); one a session wrote and ended without its save (a crash, stamped between them) is never read, so the folder
is never ahead of the save.  changes.dat keeps the stamps (the save's, and the most a session may give: written again
as a session begins, before anything else is), and the save's last step, the world's, makes every version written so
far the save's.  A new game in an old game's folder deletes the old one's region files first (dq0w/wfile.c's tidy),
and its generation keeps them from being read in any case.

**The change records**: by region in the hub's 64 pages, a region's all in them or none (dq0w/store.c, wfile.c).
**The things** (dq0w/ipage.c): the store's 744 records hold what the character carries (never paged out) and the
things of the chunks in its table (in the world's own bank, 64 chunks): the 25 round the character's always (as it
moves, its looks: it_near), a chunk's as its things are put down (stocked, a denizen's remains); a chunk's things all
in the store or none, a region's file holding a group for each of its chunks (its place in the region, its count, its
things, a thing in another its container's place in the group), renumbered as they're read in.  A chunk goes out
(its group written, a new version of its region's file, the other groups as they were; its records freed) as a
record's wanted and there's none, the one least lately wanted that isn't round the character.  Every reader looks for
things lying about only near the character, so the numbers the others hold (the senses' record's, a thing walked over
to) are of things that stay.  **The denizens** (dq0d): its bank of records a cache by region (the frame's regions,
256 squares each way, so no stratum's record is wanted to find one: a lair's by its chunk, a denizen's by its square,
or its lair's till it's placed), those round the character's region always (one each way), a region's all in it or
none, the least lately wanted put out (never one an actor is in) as a record's place is wanted; each denizen and
each lair has a number of its own (a name given is by it, a denizen's lair by it, a lair's placing by its dice), its
record's place the cache's.  **The strata** (world.h's STRATUM: every task's strata[] by slot): the hub's eight
records a cache, stratum n's in slot n & 7 (the stratum's number a byte: 255 of them); as a slot's wanted for
another, the world puts out the one there (its things' chunks first, it_flush; its record to stSS0000.dat:
dq0w/sfile.c) and reads one back as the character or a shaft comes to it (stratum_in), the maker growing its plan
again (M_SLOT, the chunks of a stratum no longer in the hub dropped from the cache, their seen squares kept).  **The
plans** (dq0m/plan.c, changes.c): a plan of more regions than its half of a plan bank holds (PLAN_MAX, 682) is grown
into its file in the save folder, through a bank of the maker's own (32 pages of 42 regions, the least lately wanted
written back), and read from it (the denizens' populations too); without a folder it's cut to PLAN_MAX.  **The seen
maps** (dq0m/changes.c, the maker's: the senses' memory of the chunks out of the cache): by region, its maps all in
the maker's banks or none, the least lately wanted written to its file.  They stay the maker's: it keeps a chunk's
seen squares as the cache makes way for it and lays them over it as it's made, both in the middle of making a
chunk; were they the senses', the maker would ask the senses then, as the senses wait on the maker for that chunk.

**Rooms across a room's squares** (dq0w.c's ways): the ways from a square to every square of a room are worked out
at once in the world's own bank (no fixed room size, as large as its chunk), so a room is walked across whatever its
size.  A room is a chunk's (struct room's squares, 0-31): a cavern across many chunks is still to come (below).

**dq0 -C** (the tests'): the caches small, so a short game pages them (twelve of the chunks' slots; a change page; 24
of the store's records, only the character's chunk kept round it; two of the denizens' regions, only the
character's kept; two strata's slots; two seen maps, one region; 16 of a big plan's pages); dq0 -T -T -T, three
strata grown below the top and the character on the second (its record read back into its slot); dq0 -T -T -T -T,
the stratum below the top 27 regions each way.  The tests: dq0vast and dq0soak (every cache filled several times over, kept
and carried on; eviction and write-back, a new game in an old folder, each stratum the same at every return).

**What's still a fixed size:** a stratum's number is a byte (255 strata) and the frame is 16 bits each way (65,535
squares); a region's change records must fit in the change pages (2,048), the things of the 25 chunks round the
character in the item store (744), the denizens of the regions round its own in their bank (240 records, 64 lairs).

### A stratum's plan: grown whole, then detailed as explored

When the character first comes to a stratum, the whole of it is grown in outline, its **plan**, kept in the hub's
plan banks (a slot's half a bank; a bigger one paged: no fixed size): a grid of **regions**, each 8 by 8 chunks
(1,280 feet), grown by the style's rules over the whole stratum at once:

- **each region's** kind (world.h's RG_*: solid rock, or a region the style grows its rooms or caves in) and its
  elevation (the lie of the rock: a chunk's floors start from a slope its neighbours share; a built stratum's
  level);
- **the ways between regions:** where a passage, a cave or a river crosses each region's edges, at what height, so
  the regions (and so the chunks) made apart fit together; within a region, the same for its chunks' edges, worked
  out when the region is first detailed;
- **the populations** (DENIZENS.md): the theme's kinds for the stratum's style and depth, placed then: their lairs,
  their numbers, each denizen a record from the start, so that what the character meets was there before it came,
  and goes on living there whether it's met or not;
- **the puzzle's pieces** this stratum holds, where the theme says they may lie (DENIZENS.md's theme: each piece's
  chunk decided as its stratum's plan is grown, put down as the chunk is first made).

A 5-mile stratum is about 440 regions: a few K of plan.  **Detail** comes as the dungeon is explored: a region's
chunks' crossings when the character first comes near the region, and each chunk's squares when it comes near the
chunk.  A stratum below is grown when the character first takes a way down to it, or a shaft dug starts down to it.

Features bigger than a chunk (great halls, chasms, lakes, rivers), the water traced over the plan, more kinds of
region (a gallery of tombs, a flooded hollow) and the age's marks on them (collapsed, flooded, abandoned) are still
to come (below): the water with slice 4.9, the caverns and the age with slice 4.10.  So are **water
levels**: a lake's, a flooded hollow's, a cistern's, a channel's surface; a square whose floor is below it under
water, to that depth (wading, then swimming, then under), and moving there the referee's (Swimming, its fatigue, the
breath held, drowning, a spell or an ability to breathe water); sight under water short, and light too; a level
that can change (a flood, a sluice opened, a dam broken).

### Growing a chunk: the style's rules

A chunk is grown by its stratum's style, from what the plan gives it (its slope, its crossings), filling the rest
(dq0m: built.c, caves.c, pass.c, fixtures.c, stock.c, uses.c):

- **purpose-built** (halls, crypts, catacombs, cellars): rooms and passages, laid out to a plan (built to a plan,
  below), rooms of one to three zones (each a plane, its ceiling built: flat, a barrel vault, a dome, a cathedral's
  pointed vault), passages between doors, the crossings joined;
- **natural** (caves): irregular: tunnels that wind from each way across the chunk's edges to its middle, heading a
  while and bending, now narrower and now wider, with pockets in their sides and blind forks off them; chambers
  grown by a wanderer from their middles, ragged, each in a box of its own clear of the others', a chamber's room
  record the bounds of its floor (the squares in them that are floor are its own: its stock, its traps, the walks
  across it go by them); a chamber's floor a hollow or a hump on the ground's unevenness, smoothly (dq0m/caves.c);
- **mines** and **mixed** strata (caves that sentient hands have been at): grown by the built grower, hewn straight,
  by their own styles' records (their proportions, their materials), on the rock's lie, not flat; their own rules (a
  mine's tunnels along a vein, galleries where it widened, shafts and ladders, timbering; natural breaking into
  built) are still to come.

**Every proportion is the style's**: how often a room is lit, in zones, sloping; its ceilings' kinds; how often a
way out has a door, and the doors' states, locks, bars, hiding and materials; a cave's chambers, its tunnels'
widths, its glows; its windows, slits, portcullises and traps: a record for each style (dq0m/grow.h's struct style,
make.c's styles, till the theme's data gives them), the growers reading it, never a proportion in a grower's code.
Natural caves have no doors; the mixed style has some.

**Age** wears what's built: the older, the more a door's state is worn (wood fastest, stone hardly), the more of a
built floor bare earth, the more of what rots gone to dust in what lies about.

Within a chunk, an **area** is a zone of a room, a passage, a stretch of cave: a plane (an elevation, a slope) and a
ceiling (a kind, its height at the middle and at the walls).  A square's floor elevation is kept square by square
(the planes made concrete, with the rough floors of caves), so movement and sight can take the floor as it is.

### Built to a plan: a stratum's purpose, its rooms' uses

A stratum grown by the built grower has a **purpose**, the theme's (a stratum's line in its file: a keep, a temple,
a manor's cellars, a mine's works, a crypt, or plain halls), and is **fortified** or not (a keep is, the rest not,
unless the theme says so).  The purpose is a plan of **uses** for its rooms, laid out over the stratum by how far a
chunk is from the **way in** (the stratum's first way up: the way out of the top one, the foot of the way down from
the stratum above): each chunk has a **role** by its ring (the chunk of the way in its ring 0, its neighbours ring 1,
and so on to 4 and beyond), a few chunks in four taking the next ring's for variety:

| purpose  | ring 0         | ring 1     | rings 2-3           | ring 4 on         |
|----------|----------------|------------|---------------------|-------------------|
| keep     | gate           | garrison   | domestic            | lord's            |
| temple   | shrine         | cloister   | domestic, cloister  | shrine            |
| cellars  | stores         | stores     | domestic, stores    | cells             |
| crypt    | shrine         | tombs      | tombs               | tombs             |
| works    | works          | works      | garrison, works     | stores            |
| halls    | domestic       | stores     | domestic, lord's    | cells             |

A stratum's purpose is its **history's** (HISTORY.md): its layers, oldest first, one to three (built to a purpose,
ruined, reused for another: a temple over an older keep's ruin), each stratum making sense in itself.  A stratum of
one layer (a stand-in's) is its purpose throughout; of more, each chunk is the layer stratagen says (a bank program
the maker loads: the oldest built layer the whole stratum's, a later one reused a part of it round the way in, a
ruin breaking a chunk of the works standing by its kind's odds), and its rooms' uses are that layer's purpose's.

(A fortified stratum's ring 0 is a gate whatever its purpose.)  A role is a **suite**: its main room (the chunk's
largest; a gate's, the room the way in comes out in) and the uses along the chain of rooms each side of it, as the
grower joins them (each room to the next from the west): a domestic suite's hall, then its kitchen, its pantry, its
storerooms on one side, quarters on the other; a gate's gatehouse, guardrooms and barracks each side, an armoury
beyond; the lord's solar by a chapel and a vault, far from the gate.  So the hall's next to the kitchen, the
kitchen's next to the pantry, the stores beyond; the barracks by the gate; the lord's quarters as far from it as the
stratum goes.  The rooms' uses: a chamber (its use lost), a hall, cells, a vault, a crypt, a gallery, a storeroom,
a kitchen, a pantry, barracks, an armoury, a chapel, quarters, the lord's solar, a well room, a cistern, a gatehouse,
a guardroom, a workshop: each named by the theme (its rooms' eight words, and the uses' twelve more: a kobold's
"cookpit" for a kitchen).  A room's kind is its use (struct room's kind, world.h's U_*), the theme's word for it,
for anyone who reads it (the lights', the denizens', the senses').

- **Water:** a **well** where the stratum has water (for now from the stratum's seed and the region, till the plan
  has its water): in a domestic suite's kitchen, or a well room of its own in the stores'; a cistern where the
  cellars want one.  A well is a fixed thing (its stone curb) on its room's floor.
- **Flat and straight:** a built stratum's ground is level (the plan's rock lies flat under it), so its rooms and
  passages are flat, one floor; its zones one plane each, never sloping, all at the room's floor, but a hall's
  **dais** (its last zone two feet up, a step along its front).  Passages run straight between their doors, a
  dog-leg at most.
- **Ceilings by size and use:** a great hall's high and vaulted (a barrel vault, a pointed one in a big hall), a
  chapel's high and pointed (domed if it's square), a gatehouse's high enough for its portcullis and the murder
  holes over it, a kitchen's high for its smoke; crypts, vaults, galleries and cisterns barrel-vaulted and low; the
  rest (storerooms, pantries, cells, quarters, barracks, guardrooms, a well room) flat at the stratum's own height,
  the passages' too: one height for all of them, the builders' (a chunk's passages' ceiling kept with it).  The
  bigger the room the higher, within its use.
- **Fortified or not:** a fortified stratum's way in is a **gatehouse** (the room the way in comes out in): its
  doors portcullises (raised; their winches on its side) and stout iron-bound doors, murder holes in its vault (a
  room's flag, R_MURDER: their words the senses'), arrow slits in its walls and its guardrooms' where a passage runs
  beside them, covering the way in; its doors thicker and more often iron-bound, fewer of them open, fewer loops
  among the rooms (a choke point: one way on).  An open plan's the other way: many doors, many standing open, more
  ways round.
- **What's left lying about**, by the room's former use and the stratum's age (the stock's): a kitchen's pots, its
  bones and potsherds; a pantry's sacks and baskets; an armoury's rusted blades and arrows; a chapel's candle stubs;
  a barracks' or a cell's straw and bones; a guardroom's table and chair; a vault's chest and pennies; a workshop's
  tools; the theme's word for each where it has one.  Age rots what rots: the older the stratum, the more of the
  wood, the cloth, the straw and the food gone to dust (bones and potsherds last).  A chunk with an older layer
  under it has that layer's remnants too, each room's use under the older purpose its mark (an old chapel's candle
  stubs under a garrison's barracks), rotted by the older layer's age; a ruined chunk its debris (bones where it
  was taken or cursed, potsherds else).  A stratum's age is its history's, its oldest layer's era's.

### Openings and ways between strata

An opening that isn't a plain doorway is a record (world.h's struct opening, level.h's O_*): a window (its sill's height
above the floor), a stairway or a ladder up or down, a shaft in the ceiling or the floor, the way out; where it leads
(the stratum's link: a stratum, a place there) and how far.  A long ramp between strata isn't made.  A stratum below is
made when the character first takes a way down to it: its bounds and style (a storm cellar is a stratum one chunk, 10 by
10, with a ladder down; the catacombs under it miles each way), and the chunk where the way comes out first.

### Changing the dungeon: exits made by those in it

The dungeon isn't only what the growers made: those in it change it, and the change stays.

- **Digging** (dq0w.c: I_DIG): a tunnel through rock or a wall, by the compass's four, or a shaft down; by hand and
  tool (the tool's rate on the material: HOUSE.md's digging), a spell of ten minutes at a time, earth at the tool's rate
  and anything harder a Mining test each spell (a pick's alone), the work's
  feet of progress (kept in half-feet; bare hands half a foot, and only in soft soil); slow (hours for a wall, days for a shaft), and loud (the denizens hear it: an event).  A shaft
  dug down goes through the floor (earth or soil two feet, flags or masonry one) and then the stratum's rock, and comes out
  in the stratum below (grown then, if no one has been there): a new link between the strata, its two ends' squares
  the same.  A wall is two feet, a square of rock five.  A dig stopped keeps the feet it's dug (HOUSE.md's what
  stopping costs: a change record of its own, DUG, the hole's half-feet and the way it's dug), and one begun again on
  its square, the same way, goes on from there ("You set to digging again where you left off.").
- **Breaking** (powder, a ram, magic; by accident, a ceiling an old stratum's age has weakened brought down) and
  **tunnellers** (creatures that dig by nature, the denizens' task choosing where) are still to come (below).

**Mining any way**: a dig goes level (a tunnel) or down (a shaft); up (a raise through the ceiling, to the stratum
above) and slanting (an incline, its floor rising or falling square by square) aren't made.

**Materials, and who judges them**: the maker gives everything its material as it grows it, by the stratum's style
and age (dq0m/fixtures.c): each room its floor's, its walls' and its ceiling's (struct chunk's mat), its passages
theirs (their floors, the rock they're cut in): a built room's walls masonry, its floor flagged or bare earth, trodden hard
(the more earth the older); a cave's floor soft soil or stone, its walls the rock; a mine's all stone (rs.h's MAT_EARTH
hard-packed, MAT_SOIL soft: bare hands dig only soil, and make no progress in anything harder).  Flags are a material
of their own (rs.h's MAT_FLAGS: lifted, and only with a pick).  The
world reads what's there and asks (the dig takes the floor's, or the wall's, from the chunk; a fall lands on the
floor's material: earth or soil the ground, flags and masonry hard, stone broken rock); the referee judges how what's applied
to it works on it.

**Tools**, and **tools to hand**: what an actor carries is the referee's to know (rs.h's RS_EQUIP: every tool it
carries, wherever, sent again as anything is taken up or put down), so nothing is equipped by hand for a skill.  The
world says what the work is on (digging: soil, earth, stone, masonry, flags), and a skill used takes out the tool that
suits the work best: by its rate on that material, and of two as good the one the work wears less (a blade digs earth or soil
faster than bare hands, and is dulled by it).  The reply names the tool and the wear the work did it; the item records its
wear (a blade dug with is dulled, then dull: worded in what's carried).  A tool that can't do the work isn't used for
it (a sword won't dig stone).  The tool's named as it's first taken up ("You dig with your pick").  A game can begin
with a thing in hand (dq0 -k TOOL: dq0 -k shovel digs an earth floor till the rock under it stops it; only a pick
lifts flags or cuts rock).

Each change is a **record**, written by the world (dq0w/store.c: by region, the cache's pages and the save folder's
files), laid over a chunk by the maker each time it makes it again, and in the save.  Seen squares are kept the same
way, a chunk's map of them kept by the maker when the cache makes way for it.  A change that makes a way between
strata adds a link to both strata's records.

### Portals

A portal is a fixture between two areas (or an area and a passage), a record of its own in its chunk (world.h's
struct portal): its kind (PK_*: a **door**, a **window**, an **arrow slit**, a **portcullis**; the records also name
a drawbridge, a gate, a grate and a hatch, which no grower makes yet), its **state** (PS_*: open, closed; barred, and
from which side; locked; warded; its lock or bar burst; hidden, or hidden by magic; wrecked), its **lock** (a
mechanical lock's complexity, 1-9: the referee's Lockcraft; a magical lock, warded, is the powers'), its **material**
and **toughness** (wood, iron-bound, iron, stone: what breaking it takes, worn away blow by blow, each one loud), and
what works it (a portcullis its winch, on its room's side; a door from either side).  Closed, it stops movement and
sight (but a window's glass or a portcullis's bars let sight through), and dulls sound; open, it joins its two areas.
The referee judges every way through: a key, picking, forcing (Strength), breaking (a blow's impact against its
toughness), climbing (a window's sill); magic (unwarding, a knock) is the powers'.  Its state changes are kept as any
change is.  Denizens don't work portals: a denizen at a door it can't walk through goes no further (DENIZENS.md, still
to come).

Windows, arrow slits and portcullises are grown by the style's say (dq0m/fixtures.c): windows, their parts any of
glass, shutters and bars, and arrow slits, in a room's wall where a passage runs beside it (PK_WINDOW, PK_SLIT on a
T_WINDOW square); portcullises for some doors (PK_PORTCULLIS); none in a natural cave.  Seen through as the table
below says (a window's shutters closed not; a portcullis up or down), and worked by the world (dq0w/wdoor.c, wblow.c,
wwin.c: it walks the character up to them, works them, and keeps their changes): a portcullis raised and lowered by
its winch, on its room's side, or heaved up from the far side (Strength), never walked through while it's down, but
its bars bent apart (bend the bars: Strength, harder the thicker they are) or broken through by blows (break the
portcullis: as a door's, off their body, iron's resistances), and it's a way through for good, jammed where it is
(PS_WRECKED, "a portcullis (its bars forced apart)"); a window's shutters opened, closed and forced as a door's, its
bars bent (Strength), its glass or its shutters broken by a blow (the referee's impact against theirs), an open one
climbed through (its sill a climb); a slit only seen through.  Worded as what they are: "a barred window", "an arrow
slit", "a portcullis (down)".  Their states are kept as a door's are, a window's parts with them.

A **window** is parts, any of: **glass** (sight through, not movement, till it's broken: loud, and sharp),
**shutters** (open or closed, barred, as a door is), **bars** (sight through, not movement, till they're bent or
broken, by the strength or the tools applied, as the referee judges); each part its own state, and the window's sill
its height above the floor (a climb to go through).

**What passes**: every portal, and every part of one, says what it lets through, **sight**, **missiles** (an arrow, a
thrown spear), **movement**, **sound**, **liquids** (boiling oil, water); the portal's own is what all its parts let
through, as they are now (sight and movement are worked so now; missiles and liquids come with them, below):

| Part, or opening    | Sight | Missiles | Movement | (and)                |
|---------------------|-------|----------|----------|----------------------|
| open (any)          | yes   | yes      | yes      |                      |
| murder hole         | yes   | yes      | no       | liquids poured       |
| arrow slit          | yes   | yes      | no       |                      |
| bars                | yes   | yes      | no       |                      |
| glass               | yes   | no       | no       | (till broken)        |
| shutters, closed    | no    | no       | no       |                      |
| a door, closed      | no    | no       | no       | sound dulled         |

**Hidden portals**: a hidden portal is the wall it seems to be (to movement, to sight, to the map) till it's found:
hidden mechanically (PS_HIDDEN), by a search (the referee's Awareness, at its hiding's steps); hidden by magic
(PS_MAGIC), only by magical means (a spell or a power of detection, the referee's: till the powers, never).  Found,
it stays found.

**Doors on the way**: a walk stops at a closed door ("The door is shut: open it first."), but one the character has
opened or shut before (its parts' PP_KNOWN, kept with its state), not locked or barred, it opens again as it goes.

**A portal's make**: each portal is made with its chunk, by its own dice (the layout never moves for them:
dq0m/fixtures.c), with a **size** (a built place's doors much alike, the stratum's own, a few odd; a cave's or a mixed
place's each its own), a **thickness**, a **state** (0 rotten ... 4 sound ... 9 new: as made, by the style, then worn
by the stratum's age, wood fastest, stone hardly), a door's **features** (a little window with a grille, a spy-hole, a
bar's brackets, iron studs, which side its hinges are, two leaves), and so a **body**: the net harm it takes before it
fails, by its material, size, thickness and state.  **Breaking** (break the door; a window's shutters, glass, bars)
goes by it: blows, a dozen at most a time, each an attack the thing ignores (the referee's RS_ATTACK at RS_THING) with
what the character strikes with: the best tool to hand (never a weapon, which a thing would spoil), or a fist, a foot,
its head (punch, kick, head butt the door), its impact less the material's resistance to the blow's aspect (the less,
the worse its state), off its body; the harm kept (PORTAL_HARM); a door broken down open for good (PS_WRECKED: it
won't shut).

**Bare hands, feet and heads**: each blow at a thing is the referee's unarmed attack (DENIZENS.md: fights; the
referee's dq0harn/fight.c and gear.c), the thing ignoring it:

- the attack: the striker's Unarmed tested (the book's hand, foot and head: their attack classes and impacts; a
  tool's skill and its attack class), the strike as against a defender that ignores it, the impact the dice and the
  weapon's (a gauntlet a point more on a punch, hobnailed boots on a kick: the game's numbers);
- the thing's harm: the impact less its resistance to that aspect (its armour), never below nothing, off its body;
- the striker's own: only as a blow goes wrong (HOUSE.md's portals: a marginal failure a d6, a critical failure 2d6,
  and half the thing's resistance to a blunt blow, less the part's armour: gauntlets a hand's, a helm the skull's, a
  kick the soles of its footwear, sandals little, shoes and boots more, hobnailed boots most): a blunt hit on it
  (rs.h's HC_STRUCK), the referee's injury, worded ("You hurt yourself on it: a minor bruise to your right foot
  (M1)"), on the sheet.  So skill spares the striker (an Unarmed 95 batters a rotten door down unhurt), armour and
  boots spare it, and a sturdy door hurts more than a rotten one.  The character stops when the referee says it's
  hurt enough (that part's injuries three levels or more, a serious one, or the shock of it).

A tool carried (a pick ...) is used instead (break the door), sparing the hands.  Forcing (Strength) is easier on a
rotten door, harder on a new one.  Its **parts** are items of their own (items.h's IW_PORTAL, put in it as the
chunk's stocked): a door's **bar** (in its brackets, on the square on its bar's side; some unbarred doors' set aside
by them), a window's **pane** and **bars**, each with its material, its state, its size and thickness: examined as
any thing is, lifted out (take the bar, lift the bar: only from its side; a pane worked out whole; bent bars lifted
out) and carried; a bar put back (bar the door, put the bar on the door: from its side, the door shut, a bar carried
or in reach that spans it, the widest; the maker's state barred, the world's bar in its brackets again); a pane broken
is shards at the character's feet.  **Examined** (examine the door, the window, the portcullis): its kind, size,
material, thickness and features plain to see, its state and how stout it is the appraiser's judgement (the referee's
Timbercraft, Metalcraft or Masonry, as near the truth as the grade).  A room's words are general at a glance (a small
hall, a high ceiling, a low, rough one); its figures (its size in feet, its ceiling's heights) come with a close look
(look closely, examine the room); search is search.

### Traps, pits and other nasty surprises

A trap is a hazard of its own, a record in its chunk as a portal is (world.h's struct trap), placed by the grower
where the style says: its **kind** (TK_*: a pit, open, covered, or spiked and covered; a deadfall, a scything blade,
darts or needles, gas, an alarm), its **trigger** (TT_*: a plate trodden, a wire tripped, a door opened or forced),
its **hiding** (in steps, a search's: mechanical, found by a search; magical, TS_MAGIC, only by magical means; or
none, an open pit in plain sight, its depth), and its **state** (TS_*: set, sprung, disarmed, broken; found; its
mechanism made out).  Unfound, it's the floor or the door it looks like.  Pits are made where the squares round them
are floor and as level as a walk, so a chunk's ways are never parted; a closed door's trap goes off as it's opened; a
natural cave has only pits (dq0m/fixtures.c).

The world sets them off (dq0w/wtrap.c): a walk worked out stops on the first it would set off, a plate's or a wire's,
and one the character knows of is gone round (a walk to the stairs or a ladder too); a door's goes off as it's opened
or forced, and a door with one it knows of is left shut.  Stepping on one, the character may notice it in time
(Awareness, a smaller chance than a search's; never a magical one: HOUSE.md's noticing); else a pit's cover gives way
(a fall, its depth; a spiked one's spikes a strike), and it climbs out; gas is resisted (Condition) or choked on,
senseless a while; an alarm's a noise the denizens hear; the rest are dodged (Dodge) or strike (their aspect, dice and
place, the referee's injuries: rs.h's RS_HARM).  A search finds hidden ones as it finds hidden doors.  One known of is
disarmed in two checks: its mechanism made out first (P_ASSESS, Lockcraft, a minute a try, never deadly; a critical
success makes the disarm easier), then disarmed (P_DISARM, Lockcraft, a minute a try; a fumble sets it off, and
fumbles can be deadly); magic only by magical means; a pit not at all.  Their states (sprung, disarmed, found) are
kept in the world's change records (TRAP_STATE), a pit's square a pit once it's given way; the ones the character knows
of in what it perceives.  A trap sprung is an event, heard.

### Items: the dungeon's layer of things

Items are a layer of their own over the strata (items.h, the world's item store): each item a record (what it is:
its kind, the catalogue's, with the referee's figures for its rules: weight, bulk, a weapon's or an armour's; its
state; where it is), and where it is one of (IW_*): on a **square** (an area's inventory: a stratum, a square, in
plain view or hidden there), an **actor**'s (the character's: carried, worn, in a hand), **inside or on another item**
(a chest, a sack, a table), or a **portal's** part (a door's bar, a window's pane or bars).  The puzzle's
pieces are placed when a stratum's plan is grown (DENIZENS.md, the theme); the ordinary things (what the style, the
rooms' uses and the stratum's age leave lying: tools, bones, rubbish, lost things) when a chunk is first made, from
its seed (the maker's stock, dq0m/stock.c: once, as it's first made, a STOCKED record kept), so they're the same each
time; what's moved since is the item store's.

**Containers**: a container item has a capacity, its space (bulk) and the weight it bears, and holds items up to both;
what it holds is carried with it, its weight counting toward its carrier's load.  A **belt** holds in its own way:
things hang from it (a pouch, a sheath, a quiver, a blade), each in a slot of its own, by how many it has room for,
not by space inside it.  The things (dq0w/things.c): held in a hand (hold, wield, ready: two hands at most, a
two-handed thing or a big one both), stowed in the pack, put in a container (by its room and the weight it bears; a
container in another, by its own bulk or what it holds, the more; never into itself or what's in it), taken from it
(into the pack, else a hand), hung from the belt (by its slots; only what hangs), worn and taken off (one of a kind
worn); the inventory says which hand (both), what's worn, what's in or on what, and the load.  Taking up fills the
pack first, then a hand.  **The load** (all the character carries, worn or not, its armour too) is the referee's to
judge: the things give it with the tools to hand (RS_EQUIP), and the referee makes it HarnMaster's encumbrance (the
load over the endurance, its physical penalty: the body's tests harder, its move less, so the world's step slower),
worded on the sheet (the load, the encumbrance) and in the inventory ("You're heavily burdened.").  Every container
holds what fits in it as yet: a scabbard isn't kept to its one blade, nor a quiver to its arrows.

**Surfaces, and what a container keeps in**: things can lie **on** an item as well as in it (a table, a chair, a
shelf, an altar: a surface, items.h's KF_ON; what's on it seen with it, not hidden inside: "On the table lies a
candle", taken from it, taken up from it); and a container says what it keeps in by form (its keeps, KF_*): solids,
small things (a sack keeps coins, a basket doesn't keep sand), liquids (a skin, a bottle, a bowl, not a basket or a
sack), gases (a bladder, a balloon, not a cloth sack); each kind has its own form.  A thing put where it won't stay is
refused (a basket and a heap of sand; a waterskin keeps only liquids); a lit thing isn't put in a container (put it
out first).  Liquids and gases aren't things of their own yet (a skin's water: slice 4.9); a light's fuel is a
measure, the thing's own (a flask's oil: light, below).

**Light** (the character's): from the lit things it carries, the brightest's reach (a torch 20 feet, a candle 10, a
lantern 30: the kinds'), in a hand, worn or hung from the belt; lit (light X, or the first unlit) with a tinderbox
carried or another flame (one carried, or lit within reach), put out (douse X); a thing lit in the pack taken into a
hand first.  Put down lit, it lights the squares round it it has a clear line to (the senses' light layer: perception
as layers, below), seen from where the character stands, and its own glow is seen much further off in the dark; the
room's words name what gives its light ("by the light of your candle").  The world keeps it in the hub (pc_light,
pc_lamp), its item store's; it takes the time and looks again (I_LIGHT).

**Fuel** (slice 4.1; the user's "lights burning down at their own rates", D5; HOUSE.md's light and traces): every
light lit burns down by the clock wherever it is (carried, put down, set in a sconce), but one its keepers tend (a
kept room's sconce's: items.h's I_TENDED, the keepers' to set, gone as the thing's moved) and one that never burns
down (a glow, the theme's).  Its fuel is the thing's own (struct item's fuel: minutes of burning left; a fuel's, a
flask of oil's, what's in it, in the same minutes), decided as it's made its own (its kind's whole life, the
catalogue's burns: a torch an hour, a lantern's fill of oil six hours, a candle by its make, its thickness and its
wax; a found one part used, by its stratum's age), and taken off a game minute at a time by the world (dq0w.c's part
27: one pass of the store's lit records a game minute, off every hot path; a chunk's things paged out caught up as
they're read in).  In its last ten minutes the character's told it's burning low; at none it goes out, told (the
character's own, or one it sees lying there: E_TASK's TE_LOW, TE_OUT, the UI's words), the light layer about it made
again (E_LIGHT) and the look with it.  A candle burns away; a lantern's dry and a torch spent till they're renewed: a
lantern filled from a flask of oil, a torch dipped in oil or pitch (IO_FILL: "fill the lantern", "dip the torch in
the oil").  Left in the dark by it, the character stops its long task (a climb part way, a fight, go on).  The kit
has a spare torch and a flask of oil (D5); the status line shows the minutes left of the light carried (the hub's
pc_fuel, its words pc_left).

**What closes, and looking in** (slice 4.1, the user's playtest, 2026-10-21: "look in the sack", "open the chest"): a
kind says what closes it (its keeps: KF_LID a chest's, a pot's, a backpack's flap; KF_TIE a sack's, a pouch's;
KF_STOPPER a waterskin's, a flask's; a basket, a quiver, none), and a thing's own state says if it's closed or locked
(struct item's shut, IS_*: decided as it's made its own, a found one closed, a chest locked one time in three; the
character's kit open, but what's stoppered).  Closed, nothing's put in it or taken out of it, nor out of anything in it
("The sack is closed."), and nothing in it is seen or listed (but it's weighed).  Opened and closed, a lock picked or
forced (IO_SHUT: the world's, the referee's P_PICKLOCK, P_FORCE; a lock burst never locks again); looked into, onto,
what hangs from it (IO_INSIDE: "In the sack lie a candle and two chisels.", "On the table lie two chisels.", "The chest
is closed."; a well's shaft); "look at" a thing an examine of it, with what's in it.  Out of reach, each is walked
over to first.

**Of several, and how many** (the user's: "if I have 3 sacks, and I say put the sack in the sack"; "drop two sacks",
"take 4 rations"): the world chooses the thing a request names (things.c's pick, part 4's): of those the name fits (a
word of the kind's, or its plural; the name the character gave it), the one most to hand (held, then worn or hanging
from the belt, then in what it carries, the shallower first, then lying within reach, the nearer first, then further
off), the request's selector picking among them (struct ireq's sel and osel, the command's: the nth, "the other", or
by state: lit, unlit, open, closed, empty); the other thing never the first nor one in it.  A count (struct ireq's
count: a number, all, half) moves several in turn, each its own record; a lot counted (KF_COUNT: rations, pennies,
arrows; its qty how many, its weight each one's times it) is split for part of it, and a lot moved joins one alike
where it goes; fewer there than asked, as many as there are, told.  The inventory, the look, a look in, tell things
alike together ("two sacks"), and those on one square in one sentence.

An item is **found** through the character's senses, as everything else is: only on a square it sees now, lit;
noticed by the referee's test of its sight (rs.h: P_NOTICE_SIGHT, or P_SEARCH when it searches), the steps from the
distance, the light, the item's size, and how it's **hidden**: by nothing, or concealed (its hide, in steps: found only by
searching, at the hiding's difficulty; HOUSE.md's traps, searching and hiding).  An item found stays known (the
character's knowledge, the item store's); the perception record lists the items it knows of that it sees now, and the
UI words them.

**Concealment**: a thing concealed on purpose carries its difficulty in steps (the item record's hide), set as it's
hidden: by the place (a floor's dust and corners) and the concealer's test of the hiding skill.  The skill is
**Concealment**: one skill for hiding a thing and for hiding oneself, rs.h's P_CONCEAL, which a ruleset's referee
maps to its own (HarnMaster has no skill of that name: its Stealth).  The character hides a thing where it stands
(hide, conceal, stash X), and knows where it hid it; a search finds a thing by a test whose steps are its
difficulty's.

**No two things alike**: each item has its own number, never another's; its quality (0 crude ... 9 a master's, most
middling); its material (a kind's as a rule, but a blade of steel if it's well made, of bronze if crudely), whose
resistances to each kind of harm (blows, edges, points, fire, cold, acid, 0-9) are its; and its durability, the harm
it can take, and what's left of it, its most by its material's toughness, its weight and its quality (a thing found
in the dungeon the worse for its years).  Each is made its own as the world first comes to it (as a look first sees
it, a search, a close look).

**Appraising**: what anyone sees of a thing is its kind and its plain state (battered, nearly broken); its make, its
material's worth and its value are an appraiser's judgement (examine X): the referee tests the craft that knows that
kind of thing (rs.h's P_APPRAISE, its AP_*: HOUSE.md's tests), and the judgement is as near the truth as the grade: a
critical success right, a marginal success near (its make a step off, its worth a fifth), a marginal failure well
off, a critical failure wild (said as surely).  The judgement isn't remembered with the thing; the theme's things
have no values of their own yet.

### Light kept: somebody's

The user (2026-10-16): "If I am in an old dungeon, why would all of the torches in the wall sconces be lit if no one
is there to light them?  If the denizens of the dungeon do not need the lights, would they be on?  Would they be
wasting torches if the rooms were never in use?"

**A room is lit only if someone keeps it lit** (slice 4.1).  A population whose kind keeps lights (kinds.txt's f: a
mind that uses fire and needs light; DENIZENS.md, their lights) keeps lit the rooms about its lair, as far as its
range takes it; the dead, the beasts, the vermin and the things that see in the dark keep none, and an old place
nobody tends is dark.  A light that's nobody's (a cave's glow, magic) stays as the style has it.

- **Sconces.**  The style's old odds of a room lit by its own light are now the odds of a room built with sconces
  (R_SCONCE, the maker's: one fewer in 8 each stratum down, as before), and every room its keepers keep lit has one
  too (their own brackets), but a glowing one.  A room's sconce (one item, standing for its sconces) is a fixture on a
  square of its floor beside a wall (the catalogue's: iron, fixed, never taken up; a surface a torch goes in, not a
  container), placed as the chunk's first stocked, each room's own dice (dq0m/sconce.c, the maker's part 3, after
  stock.c's: nothing else moves for them).  A kept room's holds a torch lit and tended (I_TENDED: its keepers renew
  it, so it never burns down while they live); another's a cold one, part burnt (the lights' found lights), or none
  (one in four: burnt away, or taken, long ago).
- **A sconce's light is its room's own** (R_LIT, the light layer's: its floor and walls lit throughout), whoever lit
  it: its keepers, or the character lighting the cold torch where it is (light the torch in the sconce), or putting a
  lit one in it.  Put out, taken away, burnt away: the room's dark again.  So a lit thing in a sconce is never a lamp
  of its own (the senses' lamps leave it out: dq0p/things.c), and the room's light is a change in the world's change
  records (ROOM_LIT, on a square of the room: R_LIT or 0), the world's to keep as the sconce's lights change (dq0w/
  wkept.c, its part 34: after a thing's handled within reach, and each game minute for the chunks round the
  character, a torch burnt out; each sconce keeping its room's light as the world last made it, so a pass changes only
  what's changed), written as its kept room's first stocked too, so a chunk made again has it.
- **Who decides.**  The denizens' task, as it decides a stratum's populations, writes the places kept in the hub
  (KEPT_AT, in DEN_BANK after the fights' words: each lair of a kind that keeps lights, its chunk, its keepers' range,
  how many of them live) and marks the stratum done there; the world, as it grows a stratum's plan (and as a game's
  carried on), waits for that before its first chunk is made (ten seconds at most, the denizens' intents carried out
  meanwhile), so the maker stocks every chunk by them from the first: a chunk's always stocked the same.  A room's
  kept if its sconce's square is within the range of a keeping whose keepers live, either way from the middle of its
  lair's chunk (the lair's home isn't known till it's placed).  The places kept are kept with the game (the denizens'
  dens.dat, after their bank).
- **Keepers gone.**  As a lair's last keeper dies its keeping's gone (kept no longer); the world, each game minute,
  finds the tended torches in sconces no live keeping reaches and tends them no longer: they burn down from then (the
  lights' burning), and as each goes out its room goes dark.  A torch in a chunk that's out of the item store is seen
  to as its chunk comes back near the character (it burns from then: no one saw it meanwhile).  None flee a stratum
  yet.
- **Seen**: a sconce is told as a thing on the wall, what's in it with it: "On the wall to your left is a sconce, a
  lit torch in it." (the way it lies, the nearest of the four; "beside you" on its square); a room lit by its sconce
  says the theme's light ("lit by smoky tallow lamps": the people's), a dark one by the character's own light ("by the
  light of your torch"), its sconce's torch cold ("a torch in it").

### Hidden things, and seeing in the dark

- **Searching** is one ability for every hidden thing: an item (above), an exit (a secret door: a square that looks
  like wall till it's found), a trap.  Each hidden thing has its hiding (how well, and how: mechanical, magical), and
  a search (rs.h: P_SEARCH) of the squares round the character it can see (within three), a spell of time, tests each
  one (HOUSE.md's traps, searching and hiding); a passing glance (P_NOTICE_SIGHT) has a smaller chance, as the
  character moves.  In the dark a search is by touch, of the squares within reach, seen or not (what's lying there
  felt plainly, what's hidden by the test, a little harder); and a search has its forms, light or dark: nearby, a wall,
  the floor, for a thing (till it's found), on or in a thing (what's on it, its trap) (HOUSE.md's walking and searching
  in the dark).  What's found is known (the change records keep a found exit or trap as what it is).  A denizen
  that hides is still to come.
- **Seeing in the dark** goes by the referee's summary, whatever the ruleset calls it: **night vision** (the faintest
  light carries further: a torch's reach, or a glow's, multiplied), and **ultravision** (sight with no light at all,
  to a range, without colour or the finest detail).  Both are the character's senses as they are now, so a species, a
  spell or a wound changes them; a denizen's darkvision is its kind's (DENIZENS.md, and perception as layers, below).

### Moving

A square's time is the referee's (its summary's step: the pace's, below), more uphill, a little more steeply
downhill, more on steps.  A step takes a rise or a drop of 2 feet, steps or stairs 4 (world.h's STEP_MAX, STAIR_MAX).
The character takes the shortest way it can walk to where it's going (around a ledge, by the steps).  A rise or a drop
too steep to walk, a window's sill, a shaft or a ladder: a climb, the referee's test (Climbing: feet of progress each
test, a test a round; a fall's impact by its height, the landing's floor: HOUSE.md's climbing and falling); a ledge
or a shaft can be jumped down (jump down: a fall of its height).

- **Long tasks** (HOUSE.md's): whatever the character does that takes more than a round (a walk, a dig, a climb, a
  wait) is one long task, the world's (the hub's pc_task: its kind, its length and its part done in its own units).
  A walk's way is worked out as it starts and walked a square at a time as the clock goes, the character on each
  square in its turn (the denizens see it there, the senses look every third square and at each beat); a dig's
  spells, a climb's rounds and a wait the same task in place.  Its progress is a bar on the status line, ten cells,
  and words as it goes: a walk's beats (the passage turns, a doorway, it rises or falls, a room crossed as it leaves
  it for a door or a passage, "You cross the room.", and on a quiet stretch "You go on down the passage" every 30
  feet), a dig's spells, a climb's rounds.  A wide passage is one passage (the user's, 2026-10-21): a square beside the
  way that's only its breadth (it and the square walked going on side by side, nothing out of it the square walked
  hasn't, nothing past it: two or three squares across, chunk.c's breadth) is no way on, for the walk and the senses alike; the openings into one room beside a
  passage one way into it; and a walk set out from a fork crosses it, three squares the way said, before a fork stops
  it again (a wide one's every square no fork of its own).  It's stopped hard by any wound or blow,
  or by a hostile denizen coming into sight within 30 feet ("You stop walking"); anything else new (a denizen seen
  for the first time in the task, the first sound heard) asks, soft: "Keep waiting?", the clock still at the turns
  pace till it's answered, yes going on, no stopping it, and any other command stopping it and then carried out.
  "stop" stops it, and "stop digging" (walking, waiting ...) names it, a task that isn't the one refused ("You aren't
  walking: you're digging"); a climb can't be stopped part way, and struck or hurt it's a test to keep the grip, a
  fall if it fails.  (dq0 -a, the tests': every question answered yes, unasked.)  A search is one too, a minute,
  "Searching about..." as it begins and what it finds told as it ends (cut short, only the squares nearest as far as
  it went, each at the full chance); a walk's exertion is the referee's as it goes, and a climb's, a dig's and a
  battering's (strenuous work), and a fatigue level more asks ("You're tiring.  Keep walking?", "Keep digging?").
  The timed work (the user's, 2026-10-22: HOUSE.md's long tasks) is one too, whatever its length: a trap's mechanism
  made out, a trap disarmed, a lock picked (a door's, a chest's), a door or a chest forced, bars bent, a portcullis
  heaved up or wound up, a pane worked free, a window climbed through, a track or a smell read, a light filled, a thing
  hidden.  Its first words at once ("You study the mechanism."), its outcome as its time's done, the test made and the
  world changed then ("You make out how it works: now to disarm it."); stopped first, nothing ("You stop studying the
  mechanism.").  The world's portals' and traps' work begun by its part with no test made (work.h's plan) and done
  as the character's free again (dq0w.c's work_done); the things' (a chest's lock, a light, a thing hidden) begun and
  done by the UI's asking twice (items.h's IO_DONE), a long task of the UI's between (I_WAIT's kind).  A battering is
  told as it goes too: its first words at once, a beat as the blows tell ("Your blows begin to tell on it.", "It's
  starting to give.", "You keep at it."), and a bare fist or foot stops short of hurting itself once a door, asked
  ("...but stop short of hurting yourself.  Keep at it?": HOUSE.md's portals).
- **Engagement** (HOUSE.md's, the book's engagement zones): a hostile or unfriendly denizen on its feet and conscious
  has a zone, its square and the eight round it (not round a corner); any walk stops as it comes into one ("The
  giant rat engages you: you stop walking."), and one begun in one goes a square and stops if it's still in one.
  As two are newly engaged both test their quickness, the defender doing better gaining a tactical advantage (it
  strikes first, or backs away: HOUSE.md's engage initiative test); a thing taken up, held or put away engaged is a
  grope, a test of the hands (failed, the round's wasted: "You fumble for the sword.").  (dq0 -I, the tests': neither.)
- **Out of reach, walked to first**: a command whose thing is out of reach (take, hold, wear, put in or on, take
  from, light; the way out's puzzle) walks over to it first, a walk like any other (stopped by what it meets), and is
  carried out as the character gets there; a blow at a denizen out of reach ("attack the goblin", "attack" with a
  target) closes with it first, the walk ending beside it and the blow struck (the denizen's own blow as it comes up
  no stop), or engaged by another first, no blow; engaged already, refused.
- **Going places in a room**, by the facing: the centre, the far side (the other side, the far end, the wall ahead),
  the near, left or right side, a corner ("go to the far left corner", "walk forward to the other side"); where the
  place itself is rock (a cave's), the nearest square the way reaches.  And where you are said the same way: "You are
  in the far left corner of a large, long hall"; a thing in a corner or against a wall said so ("Against the wall to
  your left lies a pick."; a denizen keeps its bearing and distance, a fight's words).
- **The pace** is the player's, kept in the hub (its gait: creep, walk slowly, walk, jog, run, sprint; "set pace to
  jog", or a pace's word alone; "run north" sets it and goes: the world's to set, as the UI asks, WQ_SET), and kept
  with the game in the save (the world's world.dat).  Each pace's step is the referee's, in its summary (rs.h's
  gstep, tenths a square): HarnMaster's four from the effective Move (Agility less the physical penalty, so wounds,
  fatigue and the load all slow it), a walk half of it in squares a round, a jog all of it, a run twice, a sprint
  three times; and the game's two slower, a slow walk a quarter of Move (twice the walk's time) and a creep a sixth
  (three times).  What the character goes at is the pace held to what it's doing: a slow walk at most while it
  searches or maps as it goes, or in the dark (no light of its own or the room's: by feel; or blind), unless a faster
  pace is said outright there (its dangers: stumbling over what's in the way, running into a wall or a door, over an
  edge); the world takes that pace's step for every square.
- **In the dark** (the user's playtest, 2026-10-22: HOUSE.md's walking and searching in the dark) a walk goes by what
  the character knows (the squares it's seen and those it's felt walking: the senses' memory), never turned toward an
  opening it hasn't seen or felt: from a room by a door it knows of, else straight on the way said; along a passage
  straight on, round a bend only where it knows the way, a fork it knows of stopping it.  Its cautious walk feels
  ahead and stops short of what it meets, told by touch (a wall, a door, a drop, a rise, a thing in the way, known
  after); a faster one blunders into it, the world's to judge by what's really there (the stumble the referee's, the
  book's stumble roll; a blow, a fall).  A denizen whose kind has no darkvision feels its way on a dark square too, at
  half its pace.
- **Fatigue** is the referee's (the book's movement fatigue: HOUSE.md's moving and tiring): walking (or slower) a
  fatigue level a day, jogging one each five minutes, running one each two, sprinting one each minute, the time at
  each pace added up across moves; standing still (waiting, the time between moves) is rest, a level gone each ten
  minutes of it; strenuous work (a climb's rounds, a dig's spells, a battering's blows; swimming, slice 4.9) the
  book's rate for it, a level each five minutes, as a jog.  The world tells the referee the time moved at each pace,
  at strenuous work, and the rest (RS_EXERT), and the referee keeps the levels on the sheet: the book's fatigue
  penalty, part of the universal penalty every test pays and of the physical (so a tired character's Move, and every
  pace, slows); kept with the sheet (pc.hm's "pace" line: the pace, the watchfulness, the exertion).  It doesn't
  faint worn out.
- **Watchfulness** (the hub's watch, WA_*, the player's too): **searching as it goes** ("search for traps as I go",
  "watch for traps", "move carefully"; "stop searching"), a slow walk at most, and searching as a search would: a
  trap on its way (the walk stops on the first it would set off, as always) tested by the search's chance (P_SEARCH's:
  the referee's Awareness, a step a point of its hiding) not the smaller passing chance, and wherever it stops (a walk
  stops at every door, fork and end, a small move at every square or few) a search about it (the hidden doors and
  traps within three squares it sees, each the referee's test), quietly and in no more time, found ones told and
  known from then on; **listening** ("listen as I go"), a walk at most, its hearing two better (the summary's: the
  denizens heard further); **mapping** ("map as I go"; "stop mapping"), a slow walk at most (what it does for the map
  is still to come: below); **moving quietly** ("sneak", "move quietly") is the creep.  The hub's pc_going says the
  pace the character's going at; the denizens don't hear the character's steps by it yet.
- **What it notices** goes by its state: every notice test (P_NOTICE_SIGHT, SOUND, SMELL, whoever asks: a trap
  underfoot, the senses' look as it arrives) is the worse the faster it was moving (the steps for each pace, the
  watchfulness and a hazard underfoot are house rules every ruleset plays alike, rshouse.h's), and the worse for its
  condition (the referee's own: shock; HOUSE.md's noticing).  A trap underfoot is told by its
  hiding (rs.h's P_NOTICE_SIGHT b 1), the referee choosing the chance: a search's while it's searching as it goes,
  else the passing glance's.  And the look as a run or a sprint ends is worded briefly: the place and its ways out,
  not its ceiling, its zones or what lies about.
- **status** says it all in a few lines (RS_DESCRIBE b 3: the pace's, the watchfulness's and the light's words
  rshouse.h's, every ruleset's alike; the condition's the referee's own): the pace chosen and what it's held to, its
  feet a round; the watchfulness; the condition (well or hurt, its wounds in brief, bleeding, shock or
  senseless; its fatigue levels; its load's encumbrance; the penalties they make); and its light (what gives it, or
  none: by feel).
- **Small moves** ("move left one space", "step forward", "go north 20 feet", "back up two squares", "sidestep
  right"): a square at a time (5 feet: "a space", "a square" and "a step" each one, feet and yards made squares, at
  least one), by the character's facing (forward, back, left, right, between) or the compass; the distance by the
  words, else by the pace (a square at a walk or slower, or searching or mapping as it goes, two at a jog, three at a
  run, four at a sprint); the time each square's at the pace (the world's step, slopes, diagonals as across a room).
  It faces the way it goes, but backing up and stepping to the side keep its facing.  In a room it goes straight,
  stopping at the floor's edge, a ledge, a denizen in its way or a trap (as a walk across does: one it knows of
  stopped short of); in a passage, along it by the four (a corner stops it).  (I_STEP.)

**Harm** (rs.h's RS_HARM, the referee's): a fall (its height, the floor it lands on, whether it's clear or past holds
to grab), a trap's strike (its aspect, its dice, where it strikes), a burn, a striker's own hurt by its blow at a
thing; each made the ruleset's injuries on the sheet (HarnMaster's: the book's falling, an impact on a location, its
severity and levels, the shock roll, bleeding, a limb made useless), kept in the save, told back as effects and
conditions (the summary's), the referee's words of the wounds and the UI's of the rest.  A blow at a thing (a door,
bars, shutters) is an attack (RS_ATTACK at RS_THING: above, portals) the thing's resistance taken off by the referee.
Falls come from a climb's critical failure (a hold grabbed may stop it) and from jumping down a ledge or a shaft; a
death ends the game.  Armour worn (the items' armour kinds: a leather cap, an iron helm, a leather tunic, a gambeson,
a ring byrnie, a mail hauberk, gauntlets, sandals, shoes, knee boots, hobnailed boots) is taken off a trap's strike
and a burn where they strike, and off a blow in a fight (DENIZENS.md: fights), but not off a fall.  The character
doesn't bleed as time passes yet, nor is it treated (slice 4.3).

### Perception as layers

The world has **layers**, one for each kind of perception.  Everything that affects a layer contributes to it where
it is (its **emitters**), and each sense samples it like a directional antenna: by its facing, its acuity and its
threshold; what it reads becomes the circumstances (the steps) of the referee's test, so the referee stays neutral.

- **Who owns what.**  The emitters are the world's (the bodies, the things lit, the doors, the rooms' own light: its
  records); the worked-out layers are the senses' cache, always rebuildable; the senses answer every actor's sampling
  (the character's look, the denizens' asking); the referee judges.
- **Kept by how each spreads, none on a clock.**  **Light** (instant, in straight lines) is a grid, half a byte a
  square (0 dark ... 15), 512 bytes a chunk, worked out as it's first wanted and again only near a change
  (dq0p/light.c: 16 chunks' grids in a bank of the senses' own).  Light is the only layer built as a grid; the
  traces on the floor and their smells are the world's records (below: the traces on the floor), sound and the air
  still to come.
- **The light layer.**  Its emitters: a lit room's own light (R_LIT: its floor and walls, 6; a glow's, or its sconce's
  light: light kept, above) and the lit things lying about, but those in a sconce (a lamp of reach r, its kind's
  light: the squares within r each way it has a clear line to from two feet above its floor, 2 (r - d) + 1 at d
  squares off: a torch 9 by it, 1 at its reach).  A chunk's grid is worked out again only as something changes near
  it: a lamp lit, put out, put down or taken up (each look's lamps against the last's), a door opened or shut, a square
  dug or a room's light changed (the world's E_LIGHT, from its change records), the stratum left.  The look
  takes what's lit from it (the character's own light folded in as it carries it), and seeing a square is still a line
  from the eyes to a lit square (cast.s).
- **Seeing a light isn't being lit by it.**  A light far off is its source in view, whose reach is far greater than
  the light it casts: a source of brightness b (a lamp's 2 r + 1, a lit room's 6) is seen d squares off, beyond the
  look's range, if d <= 16 b / (4 + a), a the light level the eyes are used to (31 squares at most), only against a
  darker place than itself (the light where the eyes are), ahead or beside, a line clear: "A light shows far off
  ahead" (a faint glow, if it barely stands out or is near its reach).  So a torch's light is seen 31 squares off by
  eyes used to the dark, 11 by eyes holding one; a lit room's 24 and 7.
- **The eyes adapt.**  Each actor's eyes are the senses' (dq0p/eyes.c: the character's, each denizen's): at once to a
  brighter light, dazzled a while by one six levels brighter than they're used to (half a second a level: "The light
  dazzles you", no lights far off meanwhile), slowly to the dark: a 16th of a level each 1.9 s for a man (a torch's 9
  to the dark's 0 in some 4.5 minutes).  A wait's end asks the senses for the eyes alone (LK_EYES), so a wait in the
  dark lets the lights far off show.  A kind's eyes are its data (dq0d/kinds.txt's eyes: the tenths to a 16th of a
  level; a rat's 4, a bear's 12, a kobold's 6, the dead's 0, at once); its darkvision (sight's second figure) is the
  less, by 4 / (4 + e), as its eyes are still e levels used to more light than there is.  The figures where the book
  is silent: the character's rate (19), the dazzle (6 levels, 5 tenths a level), the reach (16 b / (4 + a), 31 at
  most), a room's light (6), a lamp's brightness (2 r + 1).
- **The denizens' senses** are the senses': each actor's mind asks (the hub's dsense, its kind's sight, darkvision,
  hearing and eyes with it) as it decides, and is told whether it sees the character (lit, by its own light or the
  light layer where it stands, within the kind's sight; else within its darkvision, as its eyes allow; a line clear)
  or hears it (working, within its hearing), and how far it may go with the answer standing: a far one's **coarse
  answer**, beyond all of them, no line cast, till it's gone further or the character changes.  Its eyes adapt to the
  light it stands in (the layer's, and the character's light near it).  A mind does nothing yet with a light it sees
  far off.

### The traces on the floor, smells and smoke (slice 4.1)

The user (2026-10-16): "Dust and dirt on the floor may or may not be disturbed, depending on current movements of
denizens or the player, and tracking ability can use that to understand how many creatures and how recently (and
their types, possibly) have traveled the area.  This is a whole other layer of the dungeon that would need to be
tracked, and some surfaces (rough stone) would not show tracks well, whereas dirt or smooth stone (for different
reasons) would.  Regularly cleaned hard floors would help conceal that sort of information."  And: "Traces of passage
include smells as well, which dissipate with time (more quickly than fresh dust accumulates to cover tracks)."  And
(2026-10-21): "some denizens have rather keen sense of smell, and may be able to follow a scent for much longer than
an hour ... Rain and wind would act to reduce those further.  Wind/air movement toward a potential foe could also
alert those with particularly good sense of smell long before sound or light might reach them."

- **A layer of records, the world's** (it moves every body).  Each passing is journalled as it's made (the hub's
  traces' journal, TJ_AT, in the denizens' events' bank: two rings of 64, the character's squares and the denizens'):
  the square left, whose (a kind, 0 the character), when, and the way it went from there; a flame carried, its smoke
  too.  Its writers: the character's walk (dq0w.c's step_on, part 23), the actors' bodies (wbody.c's I_STEP, a dash,
  a step back; a coarse move's too), the denizens out of the character's way (their task's roam: I_TROD, the square
  come to, no way), and a lair of sweepers placed (I_SWEPT: its chunk kept swept).  The cost of a step is the
  journal's entry (trod.inc, in the part that makes it: no main RAM), and a part's swap each 24 passings as the
  world's main loop takes the journal into the store (tr_flush, part 28).
- **A stay counts as passings** (the user's, 2026-10-22: "repeated passage or occupancy should enhance prior
  smells"): a passing for each four minutes (a TQ) spent on a square, whoever spends it.  An actor's, the minds'
  (dq0d.c's stayed, as each TQ goes by while it stands, rests or fights where it is: I_TROD's TS_STAY, a stay alone,
  its TQs in the journal's entry); a lair's keepers' where they live, the minds' as they place the lair (each keeper a
  passing at its home square, with the most stays there are: a lair smells strongly of its keepers from the first);
  the character's found from its own ring as the store takes it in (its time on a square, from the passing before,
  the one that brought it there, to its own: so a long wait is one entry in the ring, not many, and the ring's 64
  squares stay the trail a hunter follows).  The store takes the nine chunks round the character in first, then the
  journal, so a lair placed as the character comes near it is kept.
- **The store** (dq0w/wtrace.c, in two banks of the world's own): records of eight bytes, four to a bucket, a square's
  found by a hash of its chunk and place; a passing of one there already counted in it (its passings, and its scent's
  count: the passings and the stays, each 255 at most; a stay alone keeps the way the last went), a new one over the
  bucket's oldest (so a busy place treads old traces out, and the store never fills).  Its chunks in a directory (32;
  dq0 -C's 3): the nine round the character always, a passing's chunk read in as it's wanted (not a coarse move's: kept
  only where it's held); the one least lately wanted put out to its region's file in the save folder (trSXXYY.dat: a
  version each time, vfile.inc's, each chunk a group of its own, its flags with it: swept), read back as it's wanted,
  every chunk changed written by the save (tr_flush's keep, WQ_SAVE's).  A game carried on reads them as they're
  wanted.  The kinds' numbers (tread, smell, nose, size) are read in as it begins (dq0tr.dat: dq0d/make.js's, from
  kinds.txt).
- **What shows of them** goes by the floor where each square is (the maker's material: earth, soil, rough stone,
  flags, paving), the stratum's age's dust, swept or not, how heavily the kind treads, how many passed, and their age
  against the print's life on that floor; how strongly a scent smells by its kind's smell and its reader's nose
  (hub.h's SCENT_LIFE: its strength halving each doubling of its age), and how far it's carried.  Each the stronger
  for the passings over its square, a scent for the stays on it too (hub.h's TR_MORE: one rule, wherever a trace is
  reckoned; a print by its passings alone: a stay makes no new marks), and the words by the strength (a scent strong
  enough "hangs heavy here").  HOUSE.md's light and traces has the numbers.
- **Read** (I_TRACE, the UI's: the floor looked at, the Tracking test, a smell; dq0w/wtrace.c's part 29, its words
  part 31's, wtrword.c): the floor where the character stands and the squares round it, two each way (a smell's
  carried further, through what lets the air through: the world's, as the dungeon is), each kind's traces together,
  the clearest first, the referee's test at steps by the clearest (the world's slot: P_TRACK, P_NOTICE_SMELL), and as
  much as its grade reads, in the UI bank's tread and its words (the world's, as the things' are: the UI's overlays
  have no room), said as E_VIEW V_TRACED comes (look (16), words.c's).  Looking or listening one way's sniff ("sniff
  at the grating", way.c's) is the same read, one way.
- **Smelt and followed by the denizens**: a kind's nose (kinds.txt's) smells the character in still air within a
  third of it, squares (the senses', dq0p/sense.c: dsense's SN_SMELT, its mind asking); a hostile hunter by smell
  (nose 6 or more) that doesn't sense it follows its trail: the journal's (its last 64 squares, as the nose's life of
  the scent allows: dq0d.c's scent), then the store's older trail (I_STEP's STEP_SCENT: wtrscent.c, part 32, the way
  the character went from the square it's on, or the freshest beside it, each square's life the longer for its
  passings and the character's stays there).  The character is smelt in still air the further for its stay where it
  stands (the minds' own count of it, dq0d.c's pcstay: the nose's reach the more).
- **The air and the rain to come** plug in at one place each: a draft widens a scent's reach downwind and narrows it
  upwind (wtrace.c's sniff, wtrscent.c's trail, sense.c's smelt), and carries a scent to a denizen far downwind (the
  air layer, slice 4.5, with sound); rain and running water fade scents and prints faster (wtrace.c's faded, hub.h's
  SCENT_LIFE: slice 4.9, with water).

### Seeing, and the map

The senses work out what the character sees (in front of it and to its sides, a clear line in plan and over the floors
between in height, from its eyes five feet above its floor, within its sight's range, lit: perception as layers, above)
and keep a record of it in the hub, which the UI words: the place, the openings and where they lie (as it faces) and how
far above or below, the other zones of the room, the ways on and whether they climb, the things and the denizens it
sees.  The UI says it a sentence at a time as it's made (a turn's first words, the way the character now faces, before
the look); a sentence is told once in an answer (lbsaid.s), and things or denizens alike are told together ("Two giant
rats are ahead to your right, 10 feet away."; "On the floor ahead lie two iron pots.").  The map (the map command, the
maker's dq0m/map.c) is a window on the stratum round the character (one square a character, the screen's size), drawn
into the UI's bank as the UI asks (the hub's mapq): each square the character has seen, a floor's height against its own
marked (1-9 tens of feet higher, a-i lower), the character an @, its state in a panel beside it.

The words tell nothing behind the character (the senses mark the squares beside it there seen, and the UI still
knows them: "open the door behind me" turns to it), a room's figures only once it's seen the whole of it (else "its
far side is lost in the dark"), and what's further off in less detail: a thing more than 1 + sight / 2 + its seen
squares off (the summary's sight, 0-9; the kind's seen, how far past a speck it's made out) only by its size ("something small"), a
denizen more than 3 + sight squares off only as something there; a portal or a thing more than two squares off
(NEAR_SQ) examined only as it's seen from there (a portal's kind, its size against the character's, a door's
material within 3 + sight squares), "You'd have to go closer to make out more".  Where several things or doors fit a
command and none is ahead, the UI asks which, in a numbered list (choose.c); and where several things or denizens
could be the one meant and it must be one (a bare take, a throw's mark, a target set), resolve.c asks the same way.

### Bearings underground (slice 4.2)

The user (2026-10-16): "Also, traveling underground, for many species, is disorienting.  Actual direction of travel
can be lost pretty quickly, especially if the corridors/tunnels between "rooms" are not straight.  Dwarves or other
denizens that live underground don't have this problem, but humans would, so instead of direction heading, a check
would need to be made to determine direction of travel."  And (2026-10-23): a character that starts below, come round
from a fall, could start disoriented, its north far off; everything else that disorients the same way; ways walked
often the easier.  The numbers are HOUSE.md's (bearings underground).

- **Who knows north.**  A trait of the species, the referee's (its summary's northsense): those that live below
  (HarnMaster's Khuzdul; later the denizens' kinds that dwell underground) always know which way north is; the rest
  (humans, the Sindarin, who are forest folk) keep it only by attention, and can lose it.  The denizens keep the true
  north.
- **North a belief.**  The world keeps the true facing (the hub's pc_face), and every task that works the place (the
  senses, the sound, the maker, the denizens) goes by it.  The character's north is its own: the hub's pc_skew, in
  eighths of a turn (the heading it believes it faces is the true one and the skew, & 7), and pc_lost (it knows it's
  unsure), the world's to keep, from the referee's tests and the landmarks, kept with the game (world.dat's struct
  wsave).  A new game walked into by the way in starts knowing north.
- **The check.**  The referee's P_BEARING (HarnMaster's: Awareness, or Mining if the character's is the higher), its
  dice its own (rshouse.h's), so the game's other tests come out as they did.  The world asks at a move's end (part
  52's s_bearings, from arrive), after a walk along a passage that bent (the walk's beats count its turns: beat's
  WB_TURN), or a room crossed in the dark (felt across, not seen): one bend four steps the easier, each one more two
  the harder, a room crossed in the dark as a bend, the dark four the harder.  A straight passage and a lit room ask
  nothing.  A way walked often is the easier (by the map's count of passings: the hook, part 52's familiar).
- **What comes of it.**  A success: nothing changes (a belief already wrong stays wrong: only a landmark puts it
  right).  A marginal failure: the believed north drifts an eighth one way or the other (the referee's result: the
  bearings' d8), and the character doesn't know it.  A critical failure: a quarter, and it knows it has lost its way
  (E_BEARING BRG_LOST, told; the status line's "facing NE?").
- **Landmarks.**  Coming back into a room it has been in before (the senses' R_VISITED: the room by the way in, a
  stairway's room it came down by, any it has crossed) from elsewhere (a passage, another room, another stratum) sets
  its bearings right, told if they were off (E_BEARING BRG_KNOWN).
- **What disorients.**  How the character came in, the theme's (theme.h's TM_FELL, a theme file's "start | fell"):
  come round below from a fall, its bearings are tested far the harder as the game begins, a failure leaving its
  north anywhere.  So is being knocked senseless, by whatever (a blow: the world sees the referee's summary's
  C_UNCONSCIOUS; a harm's time out, a fall's, a trap's); a fall of ten feet or more shakes it (harm_fx's: the harder the further), a failure the walk's
  drift.  Nothing moves the character while it's senseless yet.
- **What uses the belief.**  The status line, "You turn to face ...", "You face ... already.", the dark's "You feel
  your way north.", "You search the north wall...", and the parser's compass words: "north" means the north the
  character believes in (the parser's compass made as it believes it faces, the UI's cq_cmd making it the true way
  it means for the world).  The relative words (ahead, left, behind right ...) are always true, the body's own; so
  are the things the senses word as lying ahead or to the right.  An eighth off, the four ways it believes in are
  truly diagonals, which a passage won't take.
- **The map** goes by the belief too (its turning, its compass): the map's, slice 4.2's.

### Names

One table of the names the character gives (dq0/names.c), in the hub's theme bank (hub.h's `struct tname`,
`NAMES_AT`, 48 of them, at $9800 there), kept in the save (game.dat's end):

- **a thing** ("name my sword Linda", "name the sword as Linda", "call the dagger Sting"), carried or seen, by its
  item's id (never another's), its kind's name kept beside the name;
- **a room** ("call this room the Mausoleum", "name this room "The Mausoleum""), the one the character's in, by its
  stratum and its floor's northwest square (the senses put it in what the character perceives: `struct sight`'s
  `rx`, `ry`, with `rw`, `rh` its floor), so a square in it and its size;
- **an ally**: its name the denizens' task's own (DENIZENS.md: "name the rat Grub"), recorded here too, so no other
  name takes it.

A line that names (its first word name, call, alias, rename, dub, forget or unname) is the names' before it's a
command.  A name is refused if it clashes: with another name ("That name is taken by your sword."), a kind of thing's
(a word of it, or its start: "sw" would be a sword's), a kind of creature's (dq0d.dat's), a denizen's in sight, or (a
name of one word) a word the parser needs ("north", "take", "first", "one": dq0c asked).  "forget the name Linda", "unname my sword",
"unname this room" take one back (an ally keeps its name).  A thing's name works in every command that names a thing:
the line's names are made the thing's own word before it's parsed ("#" and its id, which the world's item store finds
it by), and "attack the rat with Linda" takes Linda in hand first.  The words say it: what's carried ("Linda, your
sword, in your hand": the things mark a named thing, `I_NAMED`), a thing seen lying ("At your feet lies Linda, your
sword."), and a room named, as the character's in it ("You are in the Mausoleum, a crypt ...").

**For the way back** ("go back to the mausoleum": the travel still to come): the UI's `room_named (text)` (names.c)
finds a room whose name's words are in a line (the case and its "the" aside), and gives its entry's number (0xFF:
none) and the entry in `named_room`: `sn` its stratum, `x`, `y` a square in it (its floor's northwest corner, the
dungeon's frame), `w`, `h` its floor.  Any other task can read the table where it is (the theme bank in the window:
`NAMES_AT[i].what == NM_ROOM`).

## Still to come

What the plans bring later, by the slice or phase that brings it (the detailed plan's and the game plan's):

- **The command interpreter** (Phase 3C): every command, the player's or a denizen's mind's, one structure of
  abstract actions (VB_*), verbs as data with their defaults, affordances, selectors, a target that persists,
  questions only when unclear; "alias" the command for text aliases ("alias x is dig down"), naming things and rooms
  by name and call only (the command grammar's answer 15).
- **History and themes** (Phase 3D): a region's history generated first (themegen), each stratum built from its layer
  of it (stratagen: built, ruined, reused; a temple over an older keep's ruin), the stocking and the denizens from it,
  four goal kinds.
- **Where you are, going places, and long tasks** (slice 4.0, built 2026-10-20): where in a room the character
  stands, and where things are, in words relative to its facing only, never the compass ("on the right side of a
  large room", "a rat in the far corner", "against the wall behind you"; only what's known: "somewhere in a cavern,
  its walls beyond your light"); going to named places and features, not counting squares ("go to the centre of the
  room", "walk forward to the other side", "approach the door ahead and to my left"); one system for every task
  longer than a tick, its progress a bar on the status line (redrawn at most once a real second) and words as it goes
  ("the corridor turns right", "your torch gutters"), at each change and on a quiet stretch about every 30 feet;
  interruptions hard (it stops: any wound taken, a blow, a trap sprung, a fall, the light going out) or soft (it
  asks: "A rat comes into the light. Keep digging?"), by events and by the actor's own state; "stop", "stop
  digging"; looking one way, through what lets sight through ("look north"); whatever a test finds told as found
  ("Searching about...").  Built so far (Moving, above): the long tasks, a walk a square at a time, the bar, the
  beats, stop, the hard stops and the soft questions (a denizen new in sight, a sound, tiring); where you are and
  going to places in a room; things out of reach walked to first, a blow's close-in, engagement; the search a long
  task, its finds told as it ends; a dig half done kept (its change record); battering a long task, a blow at a time;
  looking one way (3 seconds, twice a glance's reach ahead, through what lets sight through), listening one way;
  every occupation the book lists in hmchar, and the player's own.  The slice built (2026-10-20); a smell layer's
  sniffing one way the trace layer's (slice 4.1).
- **Light and traces** (slice 4.1): lights burning down and refilled (fuel), a spare torch and oil in the kit (D5);
  rooms lit only where denizens keep lights; the deposit layer: tracks in the dust, smells, smoke, fading with age;
  Tracking; a scent's age the reader's nose's (a hound follows a trail days old), the smell layer open to the air
  (slice 4.5) and to rain (slice 4.9).
- **Hands and limbs** (slice 4.1b, straight after 4.1): two hands, handedness (the medical traits': ambidextrous
  rolled), the off hand's penalty unless ambidextrous; an arm's injuries hindering that hand, the book's fumble rolls
  dropping what it holds; a leg's slowing a walk, a stumble roll failed laying the character prone; a limb made
  useless.
- **Bearings, the map and travel** (slice 4.2): the character's believed north, by Awareness or Mining; the map clear
  only where it sees, remembered by its bearings, mapping as it goes keeping it true with materials to hand; a mental
  map that fades; travel to a named place ("go back to the mausoleum").
- **Wounds over time** (slice 4.3): bleeding as time passes, first aid and bandages, healing (D6); poison, a venom's
  wounds over time (an adder's bite).
- **The sound layer and the denizens' hearing by pace** (slice 4.5): pulses over the rooms and openings, a background
  level that masks; the character's steps heard by its pace; the air layer with it: draughts through the rooms and
  openings carrying smells, the character's scent among them, downwind with the sound, so a keen nose downwind knows
  of it long before a sound or a light reaches it (why hunters keep downwind of their prey).
- **Body position and moving quietly** (slice 4.6): crouched, kneeling, sitting; low-walking, crawling; moving quietly
  by Stealth (D15); eye height by position.
- **Fights complete** (slice 4.7): missiles and ammunition as items (through slits and windows; a quiver's arrows).
- **Water** (slice 4.9): the plan's rivers, falls and lakes, water levels, swimming and drowning, a well's water from
  the plan; rain where the surface reaches (shafts, the open), washing scents and prints away; hunger and thirst,
  eating and drinking.
- **Rock and old places** (slice 4.10): the rock's own kinds and hardness across a stratum, mines and mixed strata
  grown by their own rules, age's collapse and rubble, explosives and sills; caverns across many chunks (the plan's
  features bigger than a chunk, a room followed from chunk to chunk).
- **Tunnellers and builders** (slice 4.11): denizens that dig, their new ways told when seen.
- **Powers** (Phase 5): wards and magical locks, things and doors hidden by magic found by magical means, magical
  light.
- **One path for every actor** (Phase 3C): a denizen's mind making the same commands as the player's, so the
  denizens open, shut, lock and break portals, and take things, by the same rules.
- **The house rules decided, not yet built** (HOUSE.md): Strength and weapons, free pounds, a wrecked portal appraised
  by its state, the pit climb (D13).

Designed, but in no slice of the plans yet (DECISIONS.md has the user's words for most):

- **Chunks made for the denizens** as they go where nobody has been, a stratum grown by their movements as well as the
  character's, and what they disturb there an event the disturbed act on.
- **Mining up and slanting**: a raise through the ceiling to the stratum above, an incline.
- **Things hidden by other things or on someone**: under a body, a cloak, a fallen beam (seen once it's moved, freed
  by a strength test); worn beneath clothing (found by searching the body); in a container's secret part (a false
  bottom, a hollow heel).  **Holes** as containers: a hole dug in a floor, filled in, a concealed one, dug out again
  to get at what's in it.  **Hiding oneself** by Concealment, and a hidden denizen found by a contest of its
  Concealment against the searcher's Awareness.
- **Drawbridges, gates, grates and hatches** grown (their kinds are in the records); a portcullis's winch elsewhere;
  a blow at a thing struck with the weapon in hand.
- **More traps**: a flooding chamber, a ceiling brought down, a rune or a glyph; the denizens knowing their own
  faction's traps and going round them.
- **Fainting worn out** past its endurance (strenuous work tires it now, a level each five minutes of it: HOUSE.md's
  moving and tiring).
- **An appraisal remembered** with the thing appraised.
- **A zoomed-out map** (a character for each 4 by 4 squares) for the large strata.
