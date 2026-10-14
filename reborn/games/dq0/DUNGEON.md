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
is dropped; what the character changed there (what it's seen, doors opened, things moved, rock dug) is kept as a
small record and laid over the chunk when it's made again.  The records go to the SD card if RAM runs short (a file
of the game's own, beside the save).

### A stratum's plan: the coarse layer

So that chunks made apart still fit together, a stratum has a **coarse layer**, worked out (not stored square by
square) from its seed:

- **the lie of the rock:** an elevation at each chunk's corners (smooth noise from the seed, its roughness the
  style's and the age's), so a chunk's floors start from a slope that its neighbours share;
- **the crossings:** what passes each chunk edge (a passage, a cave mouth, a river), where along it and at what
  height, from the edge's own hash, so both chunks it joins agree without the other made;
- **features bigger than a chunk:** great halls, chasms, lakes, rivers: a short list kept for the stratum
  (rectangles and lines, each with its elevations), stamped into each chunk they cross before the rest is grown.

**Water** is worked out on the coarse layer when the stratum is made: sources (where the hash puts them, high
ground) traced downhill, chunk by chunk, to where the ground rises all round, and there a lake filled until it
spills over its lowest rim, and on from the spill; a waterfall where the way down falls more steeply than water
runs.  Each river is kept as a line of points (its course, its width, its depth, its elevation), each lake as a
basin and a level; a chunk carves its piece of them (the channel, the banks, the falls, the shore).

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

1. **Chunks**: the chunk store and cache in the world's banks, coarse elevations and crossings, the purpose-built
   grower within a chunk (rooms, zones, ceilings, sloping passages, steps), small strata (one chunk, bounds smaller
   than it), stairways and ladders between strata; movement by the referee's rate and the slopes; the perception
   record and the words; the map's window and heights.
2. **Climbing**: ledges, windows, shafts, ladders, falls (the referee's tests and harm).
3. **Natural caves** and the **water** (rivers, falls, lakes); **mines**; **age**.
4. Then events, the denizens and the rest of the game's plan.
