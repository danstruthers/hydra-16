# Dingus Quest Zero's dungeon, in three dimensions

The design of the dungeon the game makes and the character moves through: what the user asked for (2026-10-14),
and how the engine can do it on a 6502.  Not yet built past the flat version (one 64 by 20 plan a level); this is
the plan for what replaces it.

## What was asked

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
- **Strata.**  Stacked layers (the user's choice): within a stratum areas don't overlap in plan, though their
  elevations differ freely; shafts, stairways, ladders and long ramps join strata.  Depth in feet is the truth; a
  stratum has a name, the theme's, not a number.  The map is a plan, its heights marked.
- **Strata of any size.**  One 200 feet square; the next a storm cellar, 10 by 10, with a ladder down to catacombs
  that go on for miles in each direction.
- **Style and age.**  Each stratum has a style (natural, purpose-built ...) and an age, and the dungeon *grows* by
  rules that follow from them.
- **Water.**  Underground rivers follow downhill rules, with waterfalls, and fill lakes, by growing rules like the
  rest.
- **Theme.**  Each dungeon's theme, aligned with its puzzle, helps decide how it grows (DENIZENS.md: the theme says
  its strata's styles, ages, sizes, names and features, and where the puzzle's pieces may lie).
- **The character's senses.**  It's all seen and heard through the character: its facing (the compass's eight), its
  eyes and ears as they are now, the light it carries and the light of the place; the map shows what it has seen,
  with its state (where it is, its facing, its attributes as they are now) beside it.

## How

### Scale: chunks

A square is 5 feet.  A stratum has bounds of its own, from one square to tens of thousands each way (a stratum's
coordinates are 16 bits: 65,535 squares is 62 miles).  Nothing holds a whole large stratum: it is made a **chunk**
at a time, 32 by 32 squares (160 feet), as the character comes near, from the stratum's seed and the chunk's place,
so the same game always makes the same chunk.  A chunk is about 2.5K (each square's kind and floor elevation, its
areas, its openings), three to an 8K bank.

The chunks are kept in the **world task's own RAM banks** (its 384K on a board with three modules; the UI never
reads them, so they needn't be shared): those near the character, a few dozen.  A chunk that goes out of the cache
is dropped; what changed there (what the character's seen, doors opened, things moved, rock dug) is kept as a small
record and laid over the chunk when it's made again.

**The save** is on the SD card the player chooses (the user: "the games should write to the SD card of choice to
save the dungeon, game state, etc."): a folder of the game's (asked for as a new game starts, or given: dq0 -d
/sd/1/dq0), holding the game's state (the clock, the character, the referee's sheets), each stratum's plan and
populations, and the chunks' records, written as they change (so a game quit is saved, and a game whose machine
is switched off loses little).  Death is still permanent: the save goes with it.

### A stratum's plan: grown whole, then detailed as explored

The user (2026-10-14): "Each stratum should be mapped out/grown in a general sense at once, then detail added as
explored.  Initial denizen populations and such should be determined at this time so encounters do not feel
contrived, since the denizens of the dungeon are added based on the theme and style."

So when the character first comes to a stratum, the whole of it is grown in outline, its **plan**, kept (in the
world's banks, and in the save on the card): a grid of **regions**, each 8 by 8 chunks (1,280 feet), grown by the
style's rules over the whole stratum at once:

- **each region's** kind (a hall complex, a cave system, a gallery of tombs, a flooded hollow, solid rock), its
  elevations (the lie of the rock at its corners: a chunk's floors start from a slope its neighbours share), its
  age's marks (collapsed, flooded, abandoned);
- **the ways between regions:** where a passage, a cave or a river crosses each region's edges, at what height, so the
  regions (and so the chunks) made apart fit together; within a region, the same for its chunks' edges, worked out
  when the region is first detailed;
- **features bigger than a chunk:** great halls, chasms, lakes, rivers: a list for the stratum (rectangles and lines,
  each with its elevations), stamped into each chunk they cross before the rest is grown;
- **water**, traced over the regions as the plan is grown: sources (high ground) followed downhill to where the
  ground rises all round, there a lake filled until it spills over its lowest rim, and on from the spill; a
  waterfall where the way down falls more steeply than water runs.  Each river is a line of points (its course,
  width, depth, elevation), each lake a basin and a level; a chunk carves its piece (the channel, the banks, the
  falls, the shore);
- **the populations** (DENIZENS.md): the theme's factions and animals for the stratum's style, placed then, in the
  regions that suit them: their lairs, their numbers, their territories and their ways about (patrols, hunting
  grounds, water they go to), each denizen a record from the start, so that what the character meets was there
  before it came, and goes on living there whether it's met or not;
- **the puzzle's pieces** this stratum holds, where the theme says they may lie.

A 5-mile stratum is about 440 regions: a few K of plan.  **Detail** comes as the dungeon is explored: a region's
chunks' crossings when anyone first comes near the region, and each chunk's squares when anyone comes near the
chunk.  Anyone: the user (2026-10-14), "A dungeon strata could be generated based on movements of denizens as well
as the player, so keep that in mind, which could cause complications for the denizens if other factions hostile to
them are disturbed."  So a chunk is made for whoever needs its squares (the character, or a denizen moving square by
square: a raiding party, a hunter following a scent), the same chunk whoever it's made for (it's the seed's), and
seen only by those who see it; and a stratum's plan is grown when anyone first takes a way down to it.  What the
making turns up that someone disturbs (a lair broken into, a nest woken, a sealed door opened) is an event the
denizens' task hears, so the factions it wakes act on it, against whoever disturbed them.

### Growing a chunk: the style's rules

A chunk is grown by its stratum's style, from what the coarse layer gives it (its slope, its crossings, the
features), filling the rest:

- **purpose-built** (halls, crypts, catacombs, cellars): rooms and passages, laid out to a plan (a catacomb's grid of
  galleries and niches, a keep's halls and stairs), rooms of one to three zones (each a plane, its ceiling built:
  flat, a barrel vault, a dome, a cathedral's pointed vault), passages sloping evenly between doors, steps where too
  steep, the crossings joined;
- **natural** (caves): passages and chambers where water would have cut them (following the slope, widening where
  it pooled), floors uneven, ceilings rough and high in the great caves, ledges, chimneys and pits;
- **mines:** tunnels along a vein, galleries where it widened, shafts and ladders, timbering;
- **mixed:** built into natural, natural breaking into built.

**Age** wears it: the older, the more collapse (rubble, a passage blocked, a ceiling fallen into a heap), the more
water's made of it (flooded hollows, flowstone), the less is left of what was built.

Within a chunk, an **area** is a zone of a room, a passage, a stretch of cave: a plane (an elevation, a slope) and a
ceiling (a kind, its height at the middle and at the walls).  A square's floor elevation is kept square by square
(the planes made concrete, with the rough floors of caves), so movement and sight can take the floor as it is.

### Openings and ways between strata

An opening that isn't a plain doorway is a record: a window (its sill's height above the floor), a shaft in the
floor (a pit) or the ceiling, a ladder, a stairway, a ramp; where it leads (a stratum, a place there) and how far.
A stratum below is made when the character first takes a way down to it: its bounds and style (a storm cellar is a
stratum one chunk, 10 by 10, with a ladder down; the catacombs under it miles each way), and the chunk where the way
comes out first.

### Changing the dungeon: exits made by those in it

The user (2026-10-14): "I would like to support player- or denizen-created "exits" such as digging shafts down to
other levels, blowing holes in walls (either intentionally or on accident), or just denizens with natural abilities
for tunneling, etc."

So the dungeon isn't only what the growers made: anyone can change it, and the change stays.

- **Digging:** a tunnel through rock or a wall, a way up or down (a shaft), a pit; by hand and tool (HarnMaster's
  Mining, the tool's, the rock's hardness: the stratum's style and age, a built wall's masonry, a natural cave's
  stone), a test each spell of work, the work's feet of progress; slow (hours for a wall, days for a shaft), and
  loud (the denizens hear it: an event, how far it carries the rock's).  A shaft dug down far enough comes out in the
  stratum below (grown then, if no one has been there): a new link between the strata, its two ends' squares the
  same; dug into the rock of the stratum below, it ends in rock, a dead end until dug on.
- **Breaking:** a hole blown or broken through a wall, a floor, a ceiling (powder, a ram, magic, later); on purpose,
  or by accident (a blast that goes wrong, a test's critical failure; a ceiling an old stratum's age has weakened,
  brought down by noise or a blow): rubble, a collapse that blocks a way or opens one, a fall to the stratum below.
- **Tunnellers:** creatures that dig by nature (the theme's: a burrower, a wyrm, a colony of diggers) make their
  own ways as they move, by the same rules, the denizens' task choosing where; the dungeon a living thing, changing
  where nobody's looking.

Each change is a **record** (a stratum, a square, what it became: its kind, its floor, its openings; who made it and
when), kept by the maker: laid over a chunk each time it's made again, and in the save.  Seen squares are kept the
same way, a chunk's map of them kept when the cache makes way for it.  A change that makes a way between strata adds
a link to both strata's records.

### Items: the dungeon's layer of things

The user (2026-10-14): "Don't forget to include a dungeon "layer" where items are tracked.  Each area needs an
inventory, just like each denizen and the player need.  Each item in the area's inventory should also be
discoverable based on visibility, eyesight, distance, and any magical or mechanical obscurement."

So items are a layer of their own over the strata: each item a record (what it is: the theme's kind, the referee's
for its rules: weight, bulk, a weapon's or an armour's; its state; where it is), and where it is one of: an **area's
inventory** (a stratum, a square, and how it lies there: in plain view, under rubble, in a niche, buried), an
**actor's** (the character's, a denizen's: carried, worn, in a hand), or **inside another item** (a chest, a sack, a
body).  The important ones (the puzzle's pieces, a faction's hoard, a guardian's charge) are placed when a stratum's
plan is grown, as its populations are; the ordinary ones (what the style and age leave lying: tools, bones, rubbish,
lost things) when a chunk is made, from its seed, so they're the same each time it's made, and what's moved since is
in the change records.

**Containers** (the user: "chests, backpacks, sacks, quivers, bowls, etc. are containers (as are belts, in a
different manner) that can hold a specified amount of space and weight of items"): a container item has a capacity,
its space (bulk) and the weight it bears, and holds items up to both; what it holds is carried with it, its weight
counting toward its carrier's load.  Some hold only some things (a quiver arrows, a scabbard its blade, a bowl
what's poured or small); a **belt** holds in its own way: things hang from it (a pouch, a scabbard, a quiver, a
tool), each in a slot of its own, by how many it has room for and what each will bear, not by space inside it.

An item is **found** through the character's senses, as everything else is: only on a square it sees now, lit;
noticed by the referee's test of its sight (rs.h: P_NOTICE_SIGHT, or P_SEARCH when it searches), the steps from the
distance, the light, the item's size, and how it's **hidden**: by nothing, by where it lies (in shadow, among
rubble), mechanically (a false bottom, a hidden niche, a buried cache: found only by searching, at the hiding's
difficulty), or by magic (invisible, illusion-hidden: found only by magic or a critical success, the theme's say).
An item found stays known (the character's knowledge, kept with the change records); the perception record lists
the items it knows of that it sees now, the UI words them.  Denizens find things by the same rules, through their
own senses.

### Hidden things, and seeing in the dark

The user (2026-10-14): "using your "find hidden" ability, you could discover hidden items, hidden exits, hidden
denizens, etc.  Don't forget ultra- and night-vision as it impacts ability to see in the dark."

- **Searching** is one ability for every hidden thing: an item (above), an exit (a secret door, a hidden hatch, a
  concealed shaft: a square that looks like wall or floor till it's found), a denizen (one that hides: the referee's
  contest of its Stealth against the searcher's Awareness).  Each hidden thing has its hiding (how well, and how:
  mechanical, magical), and a search (rs.h: P_SEARCH) of the squares round the character, a spell of time, tests
  each one it sees; a passing glance (P_NOTICE_SIGHT) has a smaller chance, as the character moves.  What's found
  is known (the change records keep a found exit as what it is).
- **Seeing in the dark** goes by the referee's summary, whatever the ruleset calls it: **night vision** (the faintest
  light carries further: a torch's reach, or a glow's, multiplied), and **ultravision** (dark-vision, infravision:
  sight with no light at all, to a range, without colour or the finest detail).  Both are the character's (or a
  denizen's) senses as they are now, so a species, a spell or a wound changes them.

### Moving

A square's time is the referee's (its summary's step: HarnMaster's walking pace, half of Move, AGL less PP, in
squares a round), more uphill, a little more steeply downhill, more on steps, twice in the dark.  The character
takes the shortest way it can walk to where it's going (around a ledge, by the steps).  A rise or a drop too steep to
walk, a window's sill, a shaft or a ladder: a climb, the referee's test (HarnMaster's Climbing: feet of progress each
test, a test a round; a fall's impact by its height, the landing's surface).

### Seeing, and the map

The world works out what the character sees (in front of it and to its sides, a clear line in plan and over the
floors between in height, from its eyes five feet above its floor, within its sight's range, lit) and keeps a
record of it in the hub, which the UI words: the place, the openings and where they lie (as it faces) and how far
above or below, the other zones of the room, the ways on and whether they climb, the water it sees and hears.  The
map is a window on the stratum round the character (one square a character, the screen's size), drawn by the world
into the hub; a floor's height against the character's marked (1-9 tens of feet higher, a-i lower); a zoomed-out map
(a character for each 4 by 4 squares) for the large strata.

## Steps

1. **Plan and chunks**: a stratum's plan grown whole (its regions, the ways between them, its elevations), the
   chunk store and cache in the world's banks, the purpose-built grower within a chunk (rooms, zones, ceilings,
   sloping passages, steps), small strata (one chunk, bounds smaller than it), stairways and ladders between
   strata; the save folder on a card; movement by the referee's rate and the slopes; the perception record and the
   words; the map's window and heights.  (The populations' records with step 4's denizens, placed when the plan is
   grown.)
1b. **Changes**: the change records and the seen squares' maps, kept by the maker, laid over a chunk made again;
   digging (a tunnel, a shaft down to the stratum below: a new link) with the referee's tests and the work's time,
   and the noise of it as an event.  (Breaking and accidents with the items that make them; tunnellers with the
   denizens.)
2. **Climbing**: ledges, windows, shafts, ladders, falls (the referee's tests and harm).
2b. **Items**: the item records and inventories (areas, the character; denizens' with them), the ordinary items a
   chunk's made with, finding them (sight, distance, light, size, hiding; searching), taking, dropping, carrying
   (the referee's weight and bulk: encumbrance); containers.
3. **Natural caves** and the **water** (rivers, falls, lakes); **mines**; **age**.
4. Then events, the denizens and the rest of the game's plan.
