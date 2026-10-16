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
is switched off loses little).  Death is still permanent: the save goes with it.  (Built: dq0 -d FOLDER, a game
there carried on; kept as the game ends, on save, and as the character comes into a stratum; the plans not kept,
but grown again from their seeds, the same.)

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
- **water levels** (the user, 2026-10-14: "Don't forget about water and water-filled spaces.  There should be a water
  level that dictates if water has filled the space or not, and then swimming skill comes into play, and fatigue
  from swimming, drowning, spells for breathing under water, etc.  This should be remembered and accounted for in
  future plans, but not worked on specifically at this time."): a lake's, a flooded hollow's, a cistern's, a
  channel's surface; a square whose floor is below it is under water, to that depth (wading, then swimming, then
  under), and moving there is the referee's: Swimming tests, swimming's fatigue, the breath held and drowning, a
  spell or an ability to breathe water; sight under water short, and light too; a level that can change (a flood,
  a sluice opened, a dam broken).  (Not now: kept for the water's step.)
- **the populations** (DENIZENS.md): the theme's factions and animals for the stratum's style, placed then, in the
  regions that suit them: their lairs, their numbers, their territories and their ways about (patrols, hunting
  grounds, water they go to), each denizen a record from the start, so that what the character meets was there
  before it came, and goes on living there whether it's met or not;
- **the puzzle's pieces** this stratum holds, where the theme says they may lie (built: DENIZENS.md's theme, each
  piece's chunk decided as its stratum's plan is grown, put down as the chunk is first made).

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

**Every proportion is the style's** (the user, 2026-10-14: "All of the percentages of each type of thing should be
part of the style (natural caves don't usually have doors, for instance, unless extended by sentient beings)."): how
often a room is lit, in zones, sloping; its ceilings' kinds; how often a way out has a door, and the doors' states,
locks, bars, hiding and materials; a cave's chambers, its tunnels' widths, its glows: a record for each style (the
maker's, dq0m/make.c's styles, till the theme's data gives them), the growers reading it.  Natural caves have no
doors; the mixed style, caves that sentient hands have been at, has some.

**Age** wears it: the older, the more collapse (rubble, a passage blocked, a ceiling fallen into a heap), the more
water's made of it (flooded hollows, flowstone), the less is left of what was built.

Within a chunk, an **area** is a zone of a room, a passage, a stretch of cave: a plane (an elevation, a slope) and a
ceiling (a kind, its height at the middle and at the walls).  A square's floor elevation is kept square by square
(the planes made concrete, with the rough floors of caves), so movement and sight can take the floor as it is.

### Built to a plan: a stratum's purpose, its rooms' uses

The user (2026-10-16): "Also remember that purpose-built spaces are normally built to a plan, with a well for water
somewhere if available.  straight, flat rooms and passages (on the same strata at least), and rooms that make sense
(feast halls near kitchens near pantries and storage).  Fortified versus not are built differently, etc.  Larger
rooms tend to have higher ceilings, and corridors and smaller rooms tend to have uniform ceiling heights, depending
on original use, of course.  These are the kinds of things that make a dungeon feel real.  Prior usage can also
inform the builder/populater what kinds of detritus or other items may be in the room."  And: "If an area is
inhabited by denizens, their current use of the space would dictate what kinds of things are to be found there as
well" (that's the denizens' lairs' stocking, over this one: the lights and tracks' work).

So a stratum grown by the built grower has a **purpose**, the theme's (a stratum's line in its file: a keep, a
temple, a manor's cellars, a mine's works, a crypt, or plain halls), and is **fortified** or not (a keep is, the
rest not, unless the theme says so).  The purpose is a plan of **uses** for its rooms, laid out over the stratum by
how far a chunk is from the **way in** (the stratum's first way up: the way out of the top one, the foot of the way
down from the stratum above): each chunk has a **role** by its ring (the chunk of the way in its ring 0, its
neighbours ring 1, and so on to 4 and beyond), a few chunks in four taking the next ring's for variety:

| purpose  | ring 0         | ring 1     | rings 2-3           | ring 4 on         |
|----------|----------------|------------|---------------------|-------------------|
| keep     | gate           | garrison   | domestic            | lord's            |
| temple   | shrine         | cloister   | domestic, cloister  | shrine            |
| cellars  | stores         | stores     | domestic, stores    | cells             |
| crypt    | shrine         | tombs      | tombs               | tombs             |
| works    | works          | works      | garrison, works     | stores            |
| halls    | domestic       | stores     | domestic, lord's    | cells             |

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

- **Water:** a **well** where the stratum has water (the plan's water, to come with the water's step: a hook, for
  now from the stratum's seed and the region): in a domestic suite's kitchen, or a well room of its own in the
  stores'; a cistern where the cellars want one.  A well is a fixed thing (its stone curb) on its room's floor.
- **Flat and straight:** a built stratum's ground is level (the plan's rock lies flat under it), so its rooms and
  passages are flat, one floor; its zones one plane each, never sloping, all at the room's floor, but a hall's
  **dais** (its last zone two feet up, a step along its front).  Passages run straight between their doors, a
  dog-leg at most.  (Mines and mixed strata keep the rock's lie: they follow the vein, the caves.)
- **Ceilings by size and use:** a great hall's high and vaulted (a barrel vault, a pointed one in a big hall), a
  chapel's high and pointed (domed if it's square), a gatehouse's high enough for its portcullis and the murder
  holes over it, a kitchen's high for its smoke; crypts, vaults, galleries and cisterns barrel-vaulted and low; the
  rest (storerooms, pantries, cells, quarters, barracks, guardrooms, a well room) flat at the stratum's own height,
  the passages' too: one height for all of them, the builders' (a chunk's passages' ceiling kept with it).  The
  bigger the room the higher, within its use.
- **Fortified or not:** a fortified stratum's way in is a **gatehouse** (the room the way in comes out in): its
  doors portcullises (raised; their winches on its side) and stout iron-bound doors, murder holes in its vault (a
  room's flag: their words the senses'), arrow slits in its walls and its guardrooms' where a passage runs beside
  them, covering the way in; its doors thicker and more often iron-bound, fewer of them open, fewer loops among the
  rooms (a choke point: one way on).  An open plan's the other way: many doors, many standing open, more ways round.
- **What's left lying about**, by the room's former use and the stratum's age (the stock's): a kitchen's pots, its
  bones and potsherds; a pantry's sacks and baskets; an armoury's rusted blades and arrows; a chapel's candle stubs;
  a barracks' or a cell's straw and bones; a guardroom's table and chair; a vault's chest and pennies; a workshop's
  tools; the theme's word for each where it has one.  Age rots what rots: the older the stratum, the more of the
  wood, the cloth, the straw and the food gone to dust (bones and potsherds last).

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

**Mining any way** (the user: "digging is just mining down, so I guess mining sideways is a thing too (or
diagonally up or down as well).  Just something to keep in mind."): a dig goes level (a tunnel, an adit), down (a
shaft, a winze), up (a raise through the ceiling, to the stratum above), or slanting up or down (an incline, its
floor rising or falling square by square, as a sloped passage's does); each a run of squares changed, their floors
at the incline's slope, kept as any change is.  (Built so far: level tunnels by the compass's four, shafts down.)

**Materials, and who judges them** (the user: "There is mining with explosives, too, and magic/psionics could be
used as well (dig spell?).  Anyway, mining should be the skill here and should apply anywhere attempted the same
way.  Stone type and hardness matters as well, but that is something for the mining skill referee and the world
builder to work out (world builder for the floor, ceiling, and wall materials, referee for how applied tools and
skills/abilities affect it)."): the maker gives everything its material as it grows it (a region's rock, its kind
and hardness, varying across the stratum; a built room's walls, floor and ceiling: brick, dressed stone, the rock
cut); the world reads what's there and asks; the referee judges how what's applied to it (Mining with a tool,
explosives, a spell, a psionic power) works on it.  (Built: as a chunk is grown, each of its rooms is given its
floor's, its walls' and its ceiling's materials, and its passages theirs (their floors, the rock they're cut in),
by the stratum's style and age (dq0m/fixtures.c): a built room's walls masonry, its floor flagged or bare earth,
the more earth the older; a cave's floor earth or stone, its walls the rock; a mine's all stone; the world's dig
takes the floor's, or the wall's, from the chunk.  The user's answers (2026-10-15): flags are a material of their
own (rs.h's MAT_FLAGS: lifted, a pick's or a mattock's quickest, a shovel's slowly, not bare hands); a shaft dug down
goes through the floor (earth two feet, flags or masonry one) and then the stratum's rock; a fall lands on the floor's
material (earth the ground, flags and masonry hard, stone broken rock).  To come: the rock's own kinds and hardness,
across a stratum.)

**Tools** (the user: "For digging, tools are important, so the referee will need to know which tools you have
equipped as well since digging stone with your bare hands doesn't usually work well (or even with a shovel for
that matter).  Digging was just one possibility"): what an actor has equipped is the referee's to know (rs.h's
RS_EQUIP, as an item is taken in hand, worn or put away), and every test that needs a tool (digging, picking a lock,
climbing with a rope) is judged by it: the world says what the work is on (digging: earth, stone, masonry), the
referee says whether anything the actor has can do it, and how much a spell of work does.

**Tools to hand** (the user: "if you are using a skill, then you are versed enough to auto-equip the right tool for
the job if you have it available.  If you carry both a shovel and a pick, digging in the dirt might choose a shovel,
but digging in a stony field might choose the pick.  If you have a sword, you could dig in the dirt with it, not as
efficiently as a shovel, but it would still work, dulling it all the way"): so nothing is equipped by hand for a
skill.  The referee is told every tool the actor carries, wherever it's carried (RS_EQUIP's whole list, sent again
as anything is taken up or put down), and a skill used takes out the one that suits the work best: by its rate on
that material, and of two as good the one the work wears less.  The reply names the tool and the wear the work did
it; the item records its wear (a blade dug with is dulled, then dull: worded in what's carried).  A tool that can't
do the work isn't used for it (a sword won't dig stone).  Built: digging so judged, its tool named as it's first
taken up ("You dig with your pick"); a game can still begin with a thing in hand (dq0 -k TOOL).  Earth to dig, where
a shovel is taken (bare hands before a sword, which the work would dull), comes with the materials the maker gives
floors and walls (dq0 -k shovel digs an earth floor, and lifts flags, till the rock under them stops it).

Each change is a **record** (a stratum, a square, what it became: its kind, its floor, its openings; who made it and
when), kept by the maker: laid over a chunk each time it's made again, and in the save.  Seen squares are kept the
same way, a chunk's map of them kept when the cache makes way for it.  A change that makes a way between strata adds
a link to both strata's records.

### Portals

The user (2026-10-14): "One important thing is portals, being doors and windows, portcullis and drawbridges (which
can close off or join two areas), and similar that can be barred, locked, be of different materials and toughnesses,
etc., and can be actively attacked/destroyed to gain access.  Magical and mechanical locks/bars both need to be
thought about."

A portal is a fixture between two areas (or an area and a passage): a **door**, a **window** (shutters, glass, bars),
a **portcullis**, a **drawbridge**, a **gate**, a **grate**, a **hatch** (in a floor or a ceiling); a record of its
own in its chunk: its kind, its **state** (open, closed; barred, and from which side; locked; warded), its **lock**
(mechanical, how complex: the referee's Lockcraft, or a key that fits; magical, how strong: the referee's magic, a
charm, a word), its **material** and **toughness** (wood, iron-bound, iron, stone: what breaking it takes, worn away
blow by blow, each one loud), and what works it (a portcullis or a drawbridge from a winch or a lever, perhaps
elsewhere; a door from either side).  Closed, it stops movement and sight (but a window's glass or a portcullis's
bars let sight through), and dulls sound; open, it joins its two areas.  The referee judges every way through: a
key, picking, forcing (Strength), breaking (a weapon's or a tool's blows against its toughness), magic (unwarding,
a knock), climbing (a window).  Its state changes are kept as any change is; denizens open, close, lock and break
them by the same rules.  (First: doors, their states, opening, closing, unlocking, forcing; the rest with weapons,
items and powers.  Grown as data, by the style's say (dq0m/fixtures.c): windows, their parts any of glass,
shutters and bars, and arrow slits, in a room's wall where a passage runs beside it (world.h's PK_WINDOW, PK_SLIT
on a T_WINDOW square); portcullises for some doors (PK_PORTCULLIS); none in a natural cave.  Seen through as the
table says (a window's shutters closed not; a portcullis up or down), and worked (dq0m/work.c: the maker keeps the
fixtures it grew, so it works them for the world, which walks the character up to them and waits): a portcullis
raised and lowered by its winch, on its room's side, or heaved up from the far side (Strength), never walked
through while it's down, but its bars bent apart (bend the bars: Strength, harder the thicker they are) or broken
through by blows (break the portcullis: as a door's, off their body, iron's resistances), and it's a way through for
good, jammed where it is (PS_WRECKED, "a portcullis (its bars forced apart)"); a window's shutters opened, closed and forced as a door's, its bars bent (Strength), its
glass or its shutters broken by a blow (the referee's impact against theirs), an open one climbed through (its
sill a climb); a slit only seen through.  Worded as what they are: "a barred window", "an arrow slit", "a
portcullis (down)".  Their states kept as a door's are, a window's parts with them.  (To come: missiles through
them, with the weapons; a portcullis's winch elsewhere; the blow struck with what's in hand.)

A **window** (the user: "Windows can be of glass and/or have shutters (lockable/wardable) or bars") is parts, any
of: **glass** (sight through, not movement, till it's broken: loud, and sharp), **shutters** (open or closed, locked
or warded, as a door is), **bars** (sight through, not movement, till they're bent or broken: the user, "given the right amount of applied strength or tools.  The referee would work that out"); each part its own
state, and the window's sill its height above the floor (a climb to go through).

**What passes** (the user: "There are smaller openings, like murder holes and arrow slits, that can be seen through and
fired through (or have boiling oil poured through), but cannot be traversed.  Barred windows don't restrict sight or
arrow fire, but do restrict movement, and glassed windows do the same for vision at least, but shuttered windows
block traversal, vision, and ranged attacks."): every portal, and every part of one, says what it lets through,
**sight**, **missiles** (an arrow, a thrown spear), **movement**, **sound**, **liquids** (boiling oil, water); the
portal's own is what all its parts let through, as they are now:

| Part, or opening    | Sight | Missiles | Movement | (and)                |
|---------------------|-------|----------|----------|----------------------|
| open (any)          | yes   | yes      | yes      |                      |
| murder hole         | yes   | yes      | no       | liquids poured       |
| arrow slit          | yes   | yes      | no       |                      |
| bars                | yes   | yes      | no       |                      |
| glass               | yes   | no       | no       | (till broken)        |
| shutters, closed    | no    | no       | no       |                      |
| a door, closed      | no    | no       | no       | sound dulled         |

**Hidden portals** (the user: "Don't forget secret/hidden doors that may "look" like nothing is there.  Only a search
or special awareness ability would reveal them on a successful referee determination."; and "Magically hidden doors
may only be detectable by magical means"): a hidden portal is the wall it seems to be (to movement, to sight, to the
map) till it's found: hidden mechanically, by a search (the referee's Awareness, or an ability it allows); hidden by
magic, only by magical means (a spell or a power of detection, the referee's).  Found, it stays found.

### Traps, pits and other nasty surprises

The user (2026-10-14): "...and don't forget about traps, pits, and other nasty surprises."  A trap is a hazard of its
own, a record in its chunk as a portal is, placed by the grower where the theme and the style say (a tomb's guarded
door, a mine's rotten floor, a faction's ambush): its **kind** (a pit: open, covered, spiked, or down to the stratum
below; a deadfall, a scything blade, darts or needles, gas, an alarm, a flooding chamber, a ceiling brought down, a
rune or a glyph), its **trigger** (a plate trodden, a wire tripped, a door or a lid opened, a lock picked wrong, a
weight lifted), its **hiding** (as a hidden portal's: mechanical, found by a search; magical, only by magical means;
or none, a pit in plain sight), and its **state** (set, sprung, disarmed, broken).  Unfound, it's the floor or the
wall it looks like.  The referee judges it all: noticing it (a glance, a search), dodging it as it goes off (Dodge,
Jumping, Acrobatics), its harm (rs.h's RS_HARM: its dice, its aspect, a fall's height), disarming it (Lockcraft,
Engineering; a dispelling for a magical one).  Denizens know their own faction's traps, and go round them; a trap
sprung is an event, heard.  (With the referee's harm: climbing's falls and combat need it too.  Grown as data
already, by the style's say (dq0m/fixtures.c, world.h's struct trap): pits, open (a T_PIT square, in plain sight:
its depth) or covered, spiked or not (hidden), where the squares round it are floor and as level as a walk, so a
chunk's ways are never parted; deadfalls, blades, darts, gas and alarms, a plate's or a wire's, and a closed
door's, set off as it's opened; hidden (a search's steps) or by magic; a natural cave's only pits.  And now set
off (dq0m/work.c, for the world): a walk worked out stops on the first it would set off, a plate's or a wire's,
and one the character knows of is gone round (a walk to the stairs or a ladder too, the user's, 2026-10-16); a door's goes off as it's opened or forced, and a door with one it
knows of is left shut.  Stepping on one, the character may notice it in time (Awareness, a smaller chance than a
search's; never a magical one); else a pit's cover gives way (a fall, its depth; a spiked one's spikes a strike),
and it climbs out; gas is resisted (Condition) or choked on, senseless a while; an alarm's a noise the denizens
hear; the rest are dodged (Dodge) or strike (their aspect, dice and place, the referee's injuries).  A search finds
hidden ones as it finds hidden doors; one known of is disarmed in two checks (the user: "one to assess the
mechanism, and one to disarm.  Assess is not deadly, disarm could be if fumbled"): its mechanism made out first
(P_ASSESS, Lockcraft, a minute a try, never deadly; a critical success makes the disarm easier), then disarmed
(Lockcraft, a minute a try; a fumble sets it off, and fumbles can be deadly); magic only by magical means; a pit not
at all).  Their states (sprung, disarmed, found) kept in the maker's change
records (TRAP_STATE), a pit's square a pit once it's given way; the ones it knows of in what it perceives.)

**Doors on the way** (the user: "auto-opening a door is OK as long as it has been opened previously, but I would say
opening needs to be explicit"): a walk stops at a closed door ("The door is shut: open it first."), but one the
character has opened or shut before (its parts' PP_KNOWN, kept with its state), not locked or barred, it opens
again as it goes.

**A portal's make** (the user, 2026-10-15: "A door should be examinable for quality, material, and other
characteristics ... Doors also don't have a size ... Door quality may be better or worse (rotten versus new) which
would make them harder or easier to break down, as would their resistances and overall "body" points"): each portal
is made with its chunk, by its own dice (the layout never moves for them: dq0m/fixtures.c), with a **size** (a
built place's doors much alike, the stratum's own, a few odd; a cave's or a mixed place's each its own, added
later), a **thickness**, a **state** (0 rotten ... 4 sound ... 9 new: as made, by the style, then worn by the
stratum's age, wood fastest, stone hardly), a door's **features** (a little window with a grille, a spy-hole, a
bar's brackets, iron studs, which side its hinges are, two leaves), and so a **body**: the net harm it takes
before it fails, by its material, size, thickness and state.  **Breaking** (break the door; a window's shutters,
glass, bars) goes by it: blows, a dozen at most a time, each an attack the thing ignores (the referee's RS_ATTACK at
RS_THING) with what the character strikes with: the best tool to hand (never a weapon, which a thing would spoil), or
a fist, a foot, its head (punch, kick, head butt the door), its impact less the material's resistance to the blow's
aspect (the less, the worse its state), off its body; the harm kept (PORTAL_HARM); a door broken down open for good
(PS_WRECKED: it won't shut).  **Bare hands, feet and heads** (the user, 2026-10-16: "Ignore how the current system
does battering of doors. Once unarmed combat is built, model it after that. There is info in the HarnMaster rules
about this."; this supersedes the user's formula of 2026-10-15, "D4 + strength mod + any armor bonus - door
resistance to blunt impact", the hand hurt by "lesser of door resistance amount and amount of damage dealt (minus hand
armor coverage)"): each blow is HarnMaster's unarmed attack (DENIZENS.md: fights; the referee's dq0harn/fight.c and
gear.c), the thing ignoring it:

- the attack: the striker's Unarmed tested (the book's hand, foot and head, Combat 3: their attack classes and
  impacts; a tool's skill and its attack class), the Ignore table's strike (Combat 12: a critical failure none, a
  marginal failure a die, a success three, a critical success four), the impact the dice and the weapon's (a gauntlet
  a point more on a punch, hobnailed boots on a kick: the book's "enhanced", Skills 18; the game's numbers);
- the thing's harm: the impact less its resistance to that aspect (its armour), never below nothing, off its body;
- the striker's own: only as a blow goes wrong (the book's H2 on a bare part that meets something hard, Combat 12; the
  user remembered self-harm on a failure only: the book has no rule for striking things, so the game's, after it), a
  marginal failure a d6, a critical failure 2d6, and half the thing's resistance to a blunt blow, less the part's
  armour (gauntlets a hand's, a helm the skull's, a kick the soles of its footwear: sandals little, shoes and boots
  more, hobnailed boots most): a blunt hit on it (rs.h's HC_STRUCK), the referee's HarnMaster injury, worded ("You
  hurt yourself on it: a minor bruise to your right foot (M1)"), on the sheet.  So skill spares the striker (an
  Unarmed 95 batters a rotten door down unhurt), armour and boots spare it, and a sturdy door hurts more than a rotten
  one.  The character stops when the referee says it's hurt enough (the game's: that part's injuries three levels or
  more, a serious one, or the shock of it).

A tool carried (a pick ...) is used instead (break the door), sparing the hands.  Forcing (Strength) is easier on a
rotten door, harder on a new one.  Its **parts** are items of their own (items.h's IW_PORTAL, put in it as the chunk's stocked):
a door's **bar** (in its brackets, on the square on its bar's side; some unbarred doors' set aside by them), a
window's **pane** and **bars**, each with its material, its state, its size and thickness: examined as any thing is,
lifted out (take the bar, lift the bar: only from its side; a pane worked out whole; bent bars lifted out) and
carried; a bar put back (bar the door, put the bar on the door: from its side, the door shut, a bar carried or in
reach that spans it, the widest; the maker's state barred, the things' bar in its brackets again); a pane broken is shards at the character's feet.  **Examined** (examine the door, the window, the
portcullis): its kind, size, material, thickness and features plain to see, its state and how stout it is the
appraiser's judgement (the referee's Timbercraft, Metalcraft or Masonry, as near the truth as the grade).  A room's
words are general at a glance (a small hall, a high ceiling, a low, rough one); its figures (its size in feet, its
ceiling's heights) come with a close look (look closely, examine the room); search is search.

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
Built (dq0p's things.c, its code in the catalogue's bank): held in a hand (hold, wield, ready: two hands at most, a
two-handed thing or a big one both), stowed in the pack, put in a container (by its room and the weight it bears;
a container in another, by its own bulk or what it holds, the more; never into itself or what's in it), taken from
it (into the pack, else a hand), hung from the belt (by its slots; only what hangs: a pouch, a sheath, a quiver, a
blade), worn and taken off (one of a kind worn); the inventory says which hand (both), what's worn, what's in or on
what, and the load.  Taking up still fills the pack first, then a hand.  **The load** (all the character carries,
worn or not, its armour too) is the referee's to judge: the things give it with the tools to hand (RS_EQUIP), and
the referee makes it HarnMaster's encumbrance (the load over the endurance, its physical penalty: the body's tests
harder, its move less, so the world's step slower), worded on the sheet (the load, the encumbrance) and in the
inventory ("You're heavily burdened.").  (To come: a scabbard's one blade, a quiver's arrows only.)

**Surfaces, and what a container keeps in** (the user: "a table or chair is a type of container as well.  Some
containers cannot hold liquids (wicker baskets), but hold clothes just fine; same with gasses (a balloon or a cloth
sack).  Something to note and make accommodations for later."): things can lie **on** an item as
well as in it (a table, a chair, a shelf, an altar: a surface, its room in area and the weight it bears; what's on
it seen with it, not hidden inside); and a container says what it keeps in by form: solids, small things (a sack
keeps coins, a basket doesn't keep sand), liquids (a skin, a bottle, a bowl, not a basket or a sack), gases (a
bladder, a balloon, not a cloth sack).  A thing put where it won't stay is refused, or (a liquid in a basket) runs
out.  Built: a kind's keeps (items.h's KF_*: a surface KF_ON) and its own form; a thing that won't stay refused (a
basket and a heap of sand; a waterskin keeps only liquids), a lit thing not put in a container (put it out first);
what's on a surface told with it ("On the table lies a candle"), taken from it, taken up from it.  Liquids and gases
as things in their own right (a skin's water, a flask's oil, lamp fuel) come with water and the items' later work.

**Light** (the character's): from the lit things it carries, the brightest's reach (a torch 20 feet, a candle 10, a
lantern 30: the kinds'), in a hand, worn or hung from the belt; lit (light X, or the first unlit) with a tinderbox
carried or another flame (one carried, or lit within reach), put out (douse X); a thing lit in the pack taken into a
hand first.  Put down lit, it lights the squares round it it has a clear line to (dq0p's view.c, its lamps), seen
from where the character stands; the room's words name what gives its light ("by the light of your candle").  The
things keep it in the hub (pc_light, pc_lamp); the world takes the time and looks again (I_LIGHT).

An item is **found** through the character's senses, as everything else is: only on a square it sees now, lit;
noticed by the referee's test of its sight (rs.h: P_NOTICE_SIGHT, or P_SEARCH when it searches), the steps from the
distance, the light, the item's size, and how it's **hidden**: by nothing, by where it lies (in shadow, among
rubble), mechanically (a false bottom, a hidden niche, a buried cache: found only by searching, at the hiding's
difficulty), or by magic (invisible, illusion-hidden: found only by magic or a critical success, the theme's say).
An item found stays known (the character's knowledge, kept with the change records); the perception record lists
the items it knows of that it sees now, the UI words them.  Denizens find things by the same rules, through their
own senses.

**Hidden by what's over it, or on someone** (the user: "some things may be hidden by clothing or trapped under a
dead person, or purposely concealed in a secret pocket"): a thing can be hidden by another thing as well as by where
it lies: under a body, a cloak, a fallen beam (seen once that's moved, or felt for by a search; trapped under
something heavy, freed only by moving it, a strength test); worn or carried under clothing (a dagger in a boot, a
pouch beneath a tunic: not seen on a denizen, nor on a body until it's searched); or concealed on purpose in a
secret pocket, a hollow heel, a false lining (a container's hidden part, found only by searching it, at its
maker's skill).  The item records where it is (in, on, under or worn beneath another item, or an actor's) and
how it's hidden; searching a body or a container is a search of what's in, on, and under it.

**Concealment** (the user: "each purposely concealed item would have a difficulty rating (0-9) to find the
concealed item, determined by the quality of the method of concealment and the skill of the concealer.  Hiding
something is an art/skill."): a thing concealed on purpose carries its difficulty, 0-9 (the item record's hide),
set as it's hidden: by the method's quality (a false bottom well made, a stone loose in a wall, a fold of cloth)
and the concealer's test of their hiding skill.  The skill is **Concealment** (the user: "The hiding skill is
called Concealment, and is for concealing items as well as people (hide in shadows anyone?)"): one skill for hiding
a thing and for hiding oneself (in shadows, behind a pillar, among the bones), rs.h's P_CONCEAL, which a ruleset's
referee maps to its own (HârnMaster has no skill of that name: its Stealth).  A search finds a thing by a test whose
steps are its difficulty's, and a hidden person by a contest against their Concealment; the character can hide
things, and itself, by the same test.

**Holes** (the user: "A hole is a container, and putting a chest in it, then filling the hole, makes for great
concealment.  A filled hole is still a container, just a hard one to find!"): a hole dug in a floor (digging's, a
foot or two down) is a container, its room by its size; filled in, it's a concealed one, its difficulty by the
filling's care and the digger's skill (and the floor: fresh earth shows on a flagged floor).  Found by a search, it
must be dug out again to get at what's in it.  Not built.

**No two things alike** (the user: "Each item is unique and should have a unique ID.  Also, each item of a type
should have unique characteristics such that 'all longswords are not the same'.  Each should have quality (how
well made), resistances to different kinds of damage (based on materials), and durability (how much damage it can
take overall and how much is left before breaking; maximum based on quality + materials)."): each item has its own
number, never another's; its quality (0 crude ... 9 a master's, most middling); its material (a kind's as a rule,
but a blade of steel if it's well made, of bronze if crudely), whose resistances to each kind of harm (blows, edges,
points, fire, cold, acid, 0-9) are its; and its durability, the harm it can take, and what's left of it, its most
by its material's toughness, its weight and its quality (a thing found in the dungeon the worse for its years).
Built: each made its own as the things task first comes to it (items.h).

**Appraising** (the user: "Examining an object (using a skill) would return some estimation (possibly way off) of
the quality, value, etc. based on expertise in the item judged by the appraiser.  Warriors would do well assessing
the quality and value weapons, but not books of magic."): what anyone sees of a thing is its kind and its plain
state (battered, nearly broken); its make, its material's worth and its value are an appraiser's judgement
(examine X): the referee tests the craft that knows that kind of thing (rs.h's P_APPRAISE, its AP_*: weapons and
armour Weaponcraft, metal Metalcraft, wood Timbercraft, books Lore ...; one not learnt at its opening level, less
a penalty), and the judgement is as near the truth as the grade: a critical success right, a marginal success near
(its make a step off, its worth a fifth), a marginal failure well off, a critical failure wild (said as surely).
Built.  Still to come: the judgement remembered with the thing, and the theme's things' values.

### Hidden things, and seeing in the dark

The user (2026-10-14): "using your "find hidden" ability, you could discover hidden items, hidden exits, hidden
denizens, etc.  Don't forget ultra- and night-vision as it impacts ability to see in the dark."

- **Searching** is one ability for every hidden thing: an item (above), an exit (a secret door, a hidden hatch, a
  concealed shaft: a square that looks like wall or floor till it's found), a denizen (one that hides: the referee's
  contest of its Concealment against the searcher's Awareness).  Each hidden thing has its hiding (how well, and how:
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

**Harm** (rs.h's RS_HARM, the referee's): a fall (its height, the ground it lands on, whether it's clear or past holds
to grab), a trap's strike (its aspect, its dice, where it strikes), a burn, a striker's own hurt by its blow at a thing; each
made the ruleset's injuries on the sheet (HarnMaster's: the book's falling, an impact on a location, its severity and
levels, the shock roll, bleeding, a limb made useless), kept in the save, told back as effects and conditions (the
summary's), the referee's words of the wounds and the UI's of the rest.  A blow at a thing (a door, bars, shutters) is
an attack (RS_ATTACK at RS_THING: above, portals) the thing's resistance taken off by the referee.  Built: falls, from a climb's
critical failure (a hold grabbed may stop it) and from jumping down a ledge or a shaft (jump down); a death ends the
game.  Armour worn (the items' armour kinds: a leather cap, an iron helm, a leather tunic, a gambeson, a ring
byrnie, a mail hauberk, gauntlets, sandals, shoes, knee boots, hobnailed boots) is taken off a trap's strike and a burn where they strike, and off a blow in a fight (DENIZENS.md:
fights).  (To come: the character's bleeding as time passes, and its treatment.)

### Seeing, and the map

The world works out what the character sees (in front of it and to its sides, a clear line in plan and over the
floors between in height, from its eyes five feet above its floor, within its sight's range, lit) and keeps a
record of it in the hub, which the UI words: the place, the openings and where they lie (as it faces) and how far
above or below, the other zones of the room, the ways on and whether they climb, the water it sees and hears.  The
map is a window on the stratum round the character (one square a character, the screen's size), drawn by the world
into the hub; a floor's height against the character's marked (1-9 tens of feet higher, a-i lower); a zoomed-out map
(a character for each 4 by 4 squares) for the large strata.

## Steps

1. (Done, 2026-10-14: and with it doors and hidden doors, crossing rooms, climbing ledges, digging, the parser.)
   **Plan and chunks**: a stratum's plan grown whole (its regions, the ways between them, its elevations), the
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
