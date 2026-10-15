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
explosives, a spell, a psionic power) works on it.  (To come: the materials are still the world's guess, masonry or
stone.)

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
a shovel or a sword would be taken, comes with the materials the world builder gives floors and walls.

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
items and powers.)

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
sprung is an event, heard.  (With the referee's harm: climbing's falls and combat need it too.)

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

**Surfaces, and what a container keeps in** (the user: "a table or chair is a type of container as well.  Some
containers cannot hold liquids (wicker baskets), but hold clothes just fine; same with gasses (a balloon or a cloth
sack).  Something to note and make accommodations for later."): noted, not built.  Things can lie **on** an item as
well as in it (a table, a chair, a shelf, an altar: a surface, its room in area and the weight it bears; what's on
it seen with it, not hidden inside); and a container says what it keeps in by form: solids, small things (a sack
keeps coins, a basket doesn't keep sand), liquids (a skin, a bottle, a bowl, not a basket or a sack), gases (a
bladder, a balloon, not a cloth sack).  A thing put where it won't stay is refused, or (a liquid in a basket) runs
out.  Liquids and gases as things in their own right (a skin's water, a flask's oil, lamp fuel) come with water
and the items' later work.

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
and the concealer's test of their hiding skill (the referee's: Stealth, or a craft's, the ruleset's say).  A
search finds it by a test whose steps are that difficulty's; the character can hide things too, by the same
test.

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
