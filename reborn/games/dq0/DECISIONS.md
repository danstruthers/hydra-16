# Dingus Quest Zero's decisions

The dated log of the design's decisions: when each was made, why, and what the user said.  The design documents
(DUNGEON.md, DENIZENS.md, SOUND.md) say how things are now; this says how they got that way.  The house rules are
HOUSE.md's.

**Oldest first**, by day.  Each entry gives the decision, the user's words in quotation marks where we have them
(a summary of what the user said has no quotation marks), what came of it where that's history worth keeping, and
**Design:** the sections it shaped.  The words come from the design documents as they were before the split
(2026-10-19), the commits on reborn-game, the plans (the detailed plan's decisions D1-D17, the game plan's G1-G6,
the command grammar's answers), and the notes kept as the work went.  The user's own notes, DesignIdeas.md, stay
theirs: they are named here, not copied.  Nothing here is the HarnMaster books' text.

## 2026-10-14

### The game

The user: "I want to make my first game. It is a text adventure game, something like Zork or Adventure, but with
more depth and completely random dungeons, similar to NetHack. I want it to be somewhat realtime as well, so if you
are not typing, the denizens of the dungeon are still moving about, menacing. The main premise is for your
adventurer to move about the dungeon trying to find pieces of a puzzle that will lead to your being able to escape
or at least be powerful enough to rule the dungeon without fear."

The answers the same day: C (cc65, the C SDK); prose, with a MAP command, on an 80 by 24 serial terminal; a clock
that never waits (what the character does takes the game's time, the denizens act while the player types), a pace
chosen at the start and a pause key; and "The story is based on the random theme of the dungeon that is generated
at the beginning of play, including what the ultimate mission is going to be, so work on the engine first, and we
can work on the content and such later."  Its name: "The name should be Dingus Quest Zero, which is in preparation
for the bigger game, Dingus Quest."  So the program is dq0, leaving dq for the bigger game, and the engine is built
for Dingus Quest to grow from.

**Design:** DENIZENS.md, the aims and the theme; DUNGEON.md, the aims.

### Sound and music

"I want there to be sound and music as part of the game as well (environmental sounds especially)."  The plan's
answers: the YM2151 always, the Vera X's PSG if it's there; sparse and atmospheric, ambience and short motifs at
moments.

**Design:** SOUND.md, the aims and the channels.

### The rules: HarnMaster, behind a referee any ruleset can fill

"Important mechanics: characters have skills and abilities. The game engine should be based on the harnmaster game
system where possible (see the danlang Harn module)."  Then: harn.dl "is not the complete rule set. The harnmaster
pdf I linked is the complete harnmaster rule set. harn.dl was just the beginning work on that."  Where harn.dl and
the book (HarnMaster 3.5) differed, the user chose "Fix harn.dl too": the C SDK's harn.h and danlang's harn.dl both
follow the book.

The tasks: "I would like to split the game into three threads at least. One for the UI and player interactions,
one for managing the actual dungeon structure and rules, and one for the denizens of the dungeon. There could be
more layers... Maybe one more for the HarnMaster rules engine, but made with a generic interface so that you could
substitute another engine (castles and crusades, d&d, basic fantasy, Open Adventure Fantasy, others) and still play
the game."  The plan's answers: Castles & Crusades the second ruleset; "Players should be able to choose the
ruleset, but there should also be a way to convert characters from one ruleset to another so that you can
substitute and test each iteration with different ones"; the pace in real seconds a combat round; defence
automatic, by a stance command; "Spells and psionics both need to work in this game eventually, so a generic power
interface may be best, implemented after the core game is working"; the full split (the UI, the world, the
denizens, the referee, the sound); death permanent; companions designed for, built later.

The character: "The character generator should be a tool that the game uses to create the character, but
standalone so you could generate one outside the game as well."  (hmchar, which the game runs.)

**Design:** DENIZENS.md, in the tasks; DUNGEON.md, who keeps what; HOUSE.md.

### Through the character's senses

"interaction with the dungeon is through the lens of the character, meaning their current facing direction, their
eyesight/hearing (which might be modified from their attributes by temporary factors), and any light sources either
carried by the character or in the environment itself."  Its facing: the compass's eight.  The map: "your
interaction with the map should include current effective attributes and facing and position" (a panel beside it).

**Design:** DUNGEON.md, the aims; seeing, and the map; perception as layers.

### The dungeon in three dimensions

"I also want to allow the dungeon to be 3D, so up, down, slopes, etc. need to be considered. The best case is that
a 3D map is generated, and each area has an elevation, and its exits join together with other exits via
transitions that can be slopes, vertical walls, level hallways, or shafts that take the elevation differences into
account. A single room with three different "zones" of elevation would be three areas with slopes between."  And:
"Movement within an area is based on movement rate, and transitions between areas of differing elevation is
goverened by distance, slope, and effective movement rate."  For "areas where the floor is sloped fairly uniformly
over the entire area ... each exit/connection point would have an elevation and slope between them would be
treated as uniform."

The answers the same day: strata, stacked layers (within one, areas don't overlap in plan, their elevations vary
freely; shafts, stairs and long ramps join them); the map a plan, its heights marked; depth in feet the truth, the
strata named by the theme, not numbered.  Then: windows are exits with an elevation above the floor; shafts in
ceilings and floors; each area's ceiling a height and a kind (flat, cathedral, domed); each stratum a style and an
age, the dungeon grown by rules that follow from them; underground rivers downhill, with waterfalls, filling lakes.
And: "each strata need not be the same dimensions as the others. One may be 200 ft. square, and then next might just
be a storm cellar, 10x10, with a ladder down to the catacombs that is miles in each direction."  So the dungeon is
made in chunks from the seed, as the character comes near, over a coarse plan that makes them fit.  The 62-mile
reach of a stratum's 16-bit squares the user found fine.  Built 2026-10-14 (step 1, 83dcc4a: the maker, plans and
chunks; the flat 64 by 20 levels gone).

**Design:** DUNGEON.md, the aims; scale: chunks; a stratum's plan; growing a chunk; moving.

### A theme, and not every denizen hostile

"Each unique dungeon also has a theme, which is aligned with the puzzle that must be solved, which helps dictate the
way the dungeon grows, the types of denizens, and their attitudes. Also, not all inhabitants of the dungeon need be
hostile. Some could become allies, controlled by the denizens task, but working possibly for the player.
Abilities/skills like rhetoric and other charisma-based skills should be able to influence the alignments of
denizens that can be influenced in that way, and skills like animal husbandry could influence animals encountered
in the same way."

**Design:** DENIZENS.md, all of it.

### The save on a card; a stratum's plan grown whole, its denizens placed with it

"the games should write to the SD card of choice to save the dungeon, game state, etc."  And: "Each stratum should
be mapped out/grown in a general sense at once, then detail added as explored. Initial denizen populations and such
should be determined at this time so encounters do not feel contrived, since the denizens of the dungeon are added
based on the theme and style."  Death stays permanent.  Then: "Start work on saving to SD and wrap up step one. Then
move on to items."  Built the same day (1778ca3: dq0 -d FOLDER, the plans grown again from their seeds, not kept).

**Design:** DUNGEON.md, a stratum's plan; the save.  DENIZENS.md, populations.

### The denizens make the dungeon too

"A dungeon strata could be generated based on movements of denizens as well as the player, so keep that in mind,
which could cause complications for the denizens if other factions hostile to them are disturbed."  Not built: the
chunks are made for the character, and the denizens away from it move only in chunks already made.  No slice of the
plans names it yet.

**Design:** DENIZENS.md, populations.

### Exits made by those in the dungeon

"I would like to support player- or denizen-created "exits" such as digging shafts down to other levels, blowing
holes in walls (either intentionally or on accident), or just denizens with natural abilities for tunneling, etc."
Built 2026-10-14 (step 1b, ec86309): the change records, digging (level tunnels and shafts down, Mining tests, the
work's time and noise), a shaft dug down a new link between strata.  Breaking and accidents wait for explosives
(slice 4.10), tunnellers for slice 4.11.

**Design:** DUNGEON.md, changing the dungeon.

### Tools, mining any way, materials

"For digging, tools are important, so the referee will need to know which tools you have equipped as well since
digging stone with your bare hands doesn't usually work well (or even with a shovel for that matter). Digging was
just one possibility, but thanks for fleshing it out."  Then: "digging is just mining down, so I guess mining
sideways is a thing too (or diagonally up or down as well).  Just something to keep in mind."  And: "There is mining
with explosives, too, and magic/psionics could be used as well (dig spell?).  Anyway, mining should be the skill
here and should apply anywhere attempted the same way.  Stone type and hardness matters as well, but that is
something for the mining skill referee and the world builder to work out (world builder for the floor, ceiling, and
wall materials, referee for how applied tools and skills/abilities affect it)."  And: "For now, continue working on
general game mechanics (generating and moving around the dungeon)."  Built: the referee told what's equipped
(RS_EQUIP), the rooms' and passages' materials by style and age (00094ab).  Raises and inclines, and the rock's own
kinds across a stratum, are not built; the rock's kinds are slice 4.10's.

**Design:** DUNGEON.md, changing the dungeon (mining any way; materials; tools).

### Tools to hand

"if you are using a skill, then you are versed enough to auto-equip the right tool for the job if you have it
available.  If you carry both a shovel and a pick, digging in the dirt might choose a shovel, but digging in a stony
field might choose the pick.  If you have a sword, you could dig in the dirt with it, not as efficiently as a shovel,
but it would still work, dulling it all the way"  Built (ed267fe): the referee told every tool carried, choosing by
rate, then by wear.

**Design:** DUNGEON.md, changing the dungeon (tools to hand); HOUSE.md, digging.

### Water levels

"Don't forget about water and water-filled spaces.  There should be a water level that dictates if water has filled
the space or not, and then swimming skill comes into play, and fatigue from swimming, drowning, spells for breathing
under water, etc.  This should be remembered and accounted for in future plans, but not worked on specifically at
this time."  Slice 4.9's.

**Design:** DUNGEON.md, a stratum's plan (water levels); still to come.

### Portals

"One important thing is portals, being doors and windows, portcullis and drawbridges (which can close off or join two
areas), and similar that can be barred, locked, be of different materials and toughnesses, etc., and can be actively
attacked/destroyed to gain access.  Magical and mechanical locks/bars both need to be thought about."  Windows: "Windows
can be of glass and/or have shutters (lockable/wardable) or bars"; bars bent or broken "given the right amount of
applied strength or tools.  The referee would work that out".  What passes: "There are smaller openings, like murder
holes and arrow slits, that can be seen through and fired through (or have boiling oil poured through), but cannot be
traversed.  Barred windows don't restrict sight or arrow fire, but do restrict movement, and glassed windows do the same
for vision at least, but shuttered windows block traversal, vision, and ranged attacks."  Built 2026-10-14: doors
(4966fa4), then windows, arrow slits and portcullises as data (00094ab); worked (raised, bent, climbed through, broken)
the next day (8a0eca6).

**Design:** DUNGEON.md, portals.

### Hidden doors, traps

"Don't forget secret/hidden doors that may "look" like nothing is there.  Only a search or special awareness ability
would reveal them on a successful referee determination."  And: "Magically hidden doors may only be detectable by
magical means".  Then: "...and don't forget about traps, pits, and other nasty surprises."

**Design:** DUNGEON.md, portals (hidden portals); traps.

### Every proportion the style's

"All of the percentages of each type of thing should be part of the style (natural caves don't usually have doors,
for instance, unless extended by sentient beings)."  Built: the maker's styles (grow.h's struct style, make.c), no
proportion in a grower's code.

**Design:** DUNGEON.md, growing a chunk.

### Items: a layer of their own

"Don't forget to include a dungeon "layer" where items are tracked.  Each area needs an inventory, just like each
denizen and the player need.  Each item in the area's inventory should also be discoverable based on visibility,
eyesight, distance, and any magical or mechanical obscurement."  Containers: "chests, backpacks, sacks, quivers, bowls,
etc. are containers (as are belts, in a different manner) that can hold a specified amount of space and weight of
items".  Surfaces and forms: "a table or chair is a type of container as well.  Some containers cannot hold liquids
(wicker baskets), but hold clothes just fine; same with gasses (a balloon or a cloth sack).  Something to note and make
accommodations for later."  Built from 2026-10-14 (1856a8d), the containers, surfaces and forms 2026-10-15 (c4402f9).

**Design:** DUNGEON.md, items.

### Hidden things, and seeing in the dark

"using your "find hidden" ability, you could discover hidden items, hidden exits, hidden denizens, etc.  Don't
forget ultra- and night-vision as it impacts ability to see in the dark."  And: "some things may be hidden by
clothing or trapped under a dead person, or purposely concealed in a secret pocket".  Things hidden under or by
other things, and on someone, aren't built; no slice names them yet.

**Design:** DUNGEON.md, items (found; hidden by what's over it); hidden things.

### Concealment

"each purposely concealed item would have a difficulty rating (0-9) to find the concealed item, determined by the
quality of the method of concealment and the skill of the concealer.  Hiding something is an art/skill."  Its name:
"The hiding skill is called Concealment, and is for concealing items as well as people (hide in shadows anyone?)"
(rs.h's P_CONCEAL, b6c458d; the HarnMaster referee's Stealth).  Holes: "A hole is a container, and putting a chest in
it, then filling the hole, makes for great concealment.  A filled hole is still a container, just a hard one to
find!"  Holes aren't built; no slice names them yet.

**Design:** DUNGEON.md, items (concealment; holes); HOUSE.md, traps, searching and hiding.

### No two things alike; appraising

"Each item is unique and should have a unique ID.  Also, each item of a type should have unique characteristics
such that 'all longswords are not the same'.  Each should have quality (how well made), resistances to different
kinds of damage (based on materials), and durability (how much damage it can take overall and how much is left
before breaking; maximum based on quality + materials)."  And: "Examining an object (using a skill) would return
some estimation (possibly way off) of the quality, value, etc. based on expertise in the item judged by the
appraiser.  Warriors would do well assessing the quality and value weapons, but not books of magic."  Built
(baabd2c).

**Design:** DUNGEON.md, items (no two alike; appraising); HOUSE.md, tests.

## 2026-10-15

### Floors, falls, and the denizens' time

The user's answers: a fall strikes anywhere, the legs when it lands on its feet; death as it was (the game ends;
quitting still keeps it); one knocked senseless wakes within the hour; a fall lands on the floor's material; a shaft
dug down goes through the floor, then the stratum's rock; flagstones a material of their own; the tools' rates as
they were; the denizens act through all the time the character's action takes; and "Something stirs" only when a
denizen comes into sight, leaves it, or changes what it's doing.  Built (aa1a562 and the denizens' second round).

**Design:** DUNGEON.md, changing the dungeon (materials); moving (harm).  DENIZENS.md, acting and time; seen and
heard.  HOUSE.md, climbing and falling; digging.

### Doors opened when asked; a trap disarmed in two checks

"auto-opening a door is OK as long as it has been opened previously, but I would say opening needs to be explicit".
And of disarming: "one to assess the mechanism, and one to disarm.  Assess is not deadly, disarm could be if
fumbled".  The meanings of store, ready, wield and equip kept as they were (the user: they sound good).  Built
(21ebe01).

**Design:** DUNGEON.md, traps; portals (doors on the way).  HOUSE.md, traps, searching and hiding.

### A portal's make; rooms in general words till looked at closely

"A door should be examinable for quality, material, and other characteristics ... Doors also don't have a size ...
Door quality may be better or worse (rotten versus new) which would make them harder or easier to break down, as
would their resistances and overall "body" points".  And: rooms told in general sizes until they're looked at
closely; rooms, doors, windows and bars examined; doors much alike in built places and each its own in caves and
mixed places; bars as items.  Built (8306a4d, f5282ec).

**Design:** DUNGEON.md, portals (a portal's make).

### Bare hands at a door (superseded 2026-10-16)

"Breaking with hands really depends on strength, but could result in damage ... to the hands", and the formula: "D4
+ strength mod + any armor bonus - door resistance to blunt impact" to the door, the hand's own hurt the "lesser of
door resistance amount and amount of damage dealt (minus hand armor coverage)".  Built (e7364d2), then replaced by
unarmed combat (2026-10-16, below).

**Design:** DUNGEON.md, portals (bare hands, feet and heads).

### Events: the denizens in the background

Playing: "the UI is slow to respond in many cases. The cursor flickers constantly, and that is a problem; probably an
artifact of time sharing with the other tasks.  I was thinking that an event-driven system might work better, where
the denizens task is doing work in the backend, and it publishes changes as events, which may or may not be
perceived by the other denizens or player depending on the perceptions of each."

What was slow, measured in the emulator at the normal pace: a command the UI answered by itself took a fifth of a
second to start and most of a second to finish, a move or a turn seven to nine seconds; idle, the CPU was busy 70%
of the time, and the status line went out whole three times a second (a third of what the serial line carries),
the cursor jumping to it and back.  The time went to the senses' look after every move and turn (some 18 million
cycles), the world waiting for it with the clock standing, the status line, and the tasks' loops.  So the denizens
weren't what held the UI up, but the user's design was the right one for them, and it was built (1a6b503): the
denizens deciding on their own time, what they do published as events, each perceiver applying its own senses.
The status line drawn only where it changes; the look in a grid of its own, its lines cast in assembly (c7cf2bc);
what wants nothing of the character answered while it's busy (93274ef); the clock going on while the world waits
for a look (c3a8aad).  A turn went from three and three quarter seconds to the prompt to one and a third, a walk
across a room from five and a half to three.  Later, after the moves of Phase 2 (2026-10-17): the look rewritten
(the grid kept between looks), the UI's words streamed as they're made, the answers timed (dq0answer).

**Design:** DENIZENS.md, events; minds and bodies.  DUNGEON.md, who keeps what (the senses).

### Sound, per creature

"Hearing sounds should be per creature, but I am not really sure what that means yet.  Sound the player hears
should be a mixture of the whole environment, but only so many channels exist, so probably need to prioritize
sounds to those sources closest/loudest and divide them up on different channels based on direction so if I am
facing N and the sound is coming from the E, then I hear it in my right ear (or, at least it is louder in that
ear).  That alone would help with locating the source."  Built (2a6c54f, merged 1c1dc90): the sound task, dq0s.

**Design:** SOUND.md, all of it.

### The Vera X, and the serial line's rate

"If the Vera is available, this should be running there, and keyboard would come from the SMC ... If the player wants
to play over serial, they can increase their serial rate to 19200 or 115200".  Built (28823bc).  The status line's
clock: the game's minutes gone only ("12 min", 54dbfd9).

**Design:** SOUND.md, without a sound chip, or a Vera X (the words the same on any terminal).

## 2026-10-16

### Answered while busy; the whole screen; the tasks left free

The commands that want nothing of the character (what it carries, its sheet, the map, the help) answered while it's
busy: fine for now, maybe more later.  On a Vera X: "Full screen is fine" (923bccd).  Three tasks left free while the
game runs: fine, since not much else runs with it.

**Design:** SOUND.md, in the tasks (the tasks' count).

### Fights, attitudes, themes and the gaps

The user chose all four next: the creatures' sheets and HarnMaster's melee; factions, attitudes, influence and
allies; themes as data with their puzzles; and the smaller gaps (traps checked on a walk to the stairs, a door barred
again, a portcullis broken and its bars bent, encumbrance).  Built and merged the same day (eb3e1e8, 71f8084,
f1d1395, 4bfb8f1).

**Design:** DENIZENS.md, the theme; attitudes; influence; allies; fights.  DUNGEON.md, portals; items (the load).

### Unarmed blows, and battering as one

Kicking a door like battering it with bare hands, its harm the foot's, boots protecting it from the worst; then
unarmed combat (punch, kick, head butt) against creatures and things alike, a helmet, gloves and boots protecting
the part that strikes, footwear's soles differing for a kick, the Unarmed skill sparing the striker; and battering a
thing an attack the thing ignores.  Then: "Ignore how the current system does battering of doors. Once unarmed
combat is built, model it after that. There is info in the HarnMaster rules about this."  This replaced the formula
of 2026-10-15.  Built (fbf8486).

**Design:** DUNGEON.md, portals (bare hands, feet and heads); HOUSE.md, portals.

### Which one, of several

"If I say open door and there are three closed doors in the room, it should give me a numbered list of choices if one
is not directly in front of me" (the same for go to, move to, examine, go through).  Built (beb7510).

**Design:** the command grammar (numbered choices, answered by a number, a selector or a name: its answer 13,
2026-10-19).

### Only what the character can see

"descriptions often tell you what lies behind you, which you can't really see ... You should only be able to tell
details about things you can clearly see.  Examining a door from across the room should only tell you its relative
size and maybe its construction material.  Closer inspection would need you to move to it ... Eyesight score should
affect the amount of detail something is described in if it is far enough away."  Built (5426dd0).

**Design:** DUNGEON.md, seeing, and the map.

### Pace, watchfulness, the character's state, small moves; a square 5 feet (D1)

"I also want a way to set my current rate of travel, and to say something like "actively search for traps as I
move", which would slow me down to a slow walk (as would carefully moving in the dark).  Faster than normal speed
(based on the player's base move) would increase fatigue rate, as outlined in the HM rules.  Your current character
state (the "status" command) should tell you your current rate of speed, active states of awareness (looking for
traps, etc.) as well as your current condition (wounds, state of shock, etc.) and current status should dictate what
information you can gather from the environment.  Running people don't see as much as those walking slowly.  Also,
when moving about a room, I should be able to say something like "move left one space" to move one 10' square at a
time, and distance traveled each movement should be based on speed and/or distance traveled."  The square's size,
asked: the user said 10 feet; decided (D1) 5 feet, "one space" one square.  Built (413c218).

**Design:** DUNGEON.md, moving; HOUSE.md, noticing; moving and tiring.

### Natural places irregular

"For natural areas and tunnels, they need to be irregular in shape (not straight tunnels, not rectangular rooms).
The dungeon "grower" should take care of that."  Built (7b6aa46, b934525).

**Design:** DUNGEON.md, growing a chunk.

### Built to a plan, and not a formula (D12)

"Also remember that purpose-built spaces are normally built to a plan, with a well for water somewhere if available.
straight, flat rooms and passages (on the same strata at least), and rooms that make sense (feast halls near
kitchens near pantries and storage).  Fortified versus not are built differently, etc.  Larger rooms tend to have
higher ceilings, and corridors and smaller rooms tend to have uniform ceiling heights, depending on original use, of
course.  These are the kinds of things that make a dungeon feel real.  Prior usage can also inform the
builder/populater what kinds of detritus or other items may be in the room."  And: "If an area is inhabited by
denizens, their current use of the space would dictate what kinds of things are to be found there as well".  Then,
on the stand-in themes' purposes: "I don't really want it to be too formulaic.  A temple could sit over the ruin of
an older keep, say.  I just want the things on the same strata to make sense."  Decided (D12): a stratum may have a
history of layers, only things on one stratum need to make sense together, and the purposes built are its first
layer.  Built (d6ff17f, 4deefcc); the histories are Phase 3D's (moved there from slice 4.10, 2026-10-18).

**Design:** DUNGEON.md, built to a plan.

### Names for things, rooms and allies

"I would like to be able to reference items by aliases, as long as the alias doesn't clash with something else.  I
would like to be able to say "alias sword as Linda" or "name my sword Linda" and then later say "Attack the goblin
with Linda".  Aliases should be allowed for rooms (name this room "The Mausoleum") and then you could say "I want to
go back to the mausoleum" and you would be on your way ..."  The way back, at the pace, as far as the character's
bearings and maps allow, and a mental map fading with time, are slice 4.2's.  Built: the names (65963f0, dcf1836).
On 2026-10-19 the command grammar made "alias" the command for text aliases (its answer 15): naming by name and call.

**Design:** DUNGEON.md, names.

### Light, tracks and smells

Rooms lit only where denizens need light and tend it (an old empty dungeon dark, cold torches in sconces); a layer
of tracks in the dust and dirt, the character's too, shown by the floor (dirt and dusty smooth stone well, rough
stone poorly), swept floors hiding them, Tracking telling how many, how recent, what kind; smells, gone sooner than
tracks are covered; lights burning down at their own rates (candles by quality, thickness and material), found ones
with some life left, lanterns refilled with oil, torches with oil or pitch.  Designed on reborn-game-trace, stopped
(it put state in the wrong tasks), kept for slice 4.1.

**Design:** DUNGEON.md, still to come.

### Bearings and the map

Travel underground disorients most kinds (people), not dwarves and the underground's own; the way the character goes
a check, not given; the map north-up for dwarves, facing-up for others; the map clear only where the character sees
now, the rest obscured and, for those poor at it underground, remembered wrong; mapping as it goes (like watching
for traps) keeping it true, with the map's materials to hand.  Designed on reborn-game-bearing (hmchar's species
landed from it), the rest kept for slice 4.2.

Slice 4.2 begun (2026-10-23: "Go ahead and push, then start 4.2"), the old design reused; the user on its plan:
"Those look really good."  Then: "If you are underground to start, and you are just waking up from a fall, you could
start out disoriented, so your "known" north could be quite far off.  Doesn't really matter, but it will shift for
non-dwarves (or other above-ground dwellers), so things will get interesting when players start using cardinal
directions they believe are true.  One more piece to ponder is the fact that going over the same area many times will
reinforce that space in the player's mind, making the map more constant in well-traveled places, similar to how smell
reinforces with repeated transits."  So a start's bearings by how the character came there (walked in by the way in,
north known; underground disoriented, a fall's waking, its north off, maybe far off, unknowing); and a place's memory
strengthened by each passing, kept better and fading slower the more it's crossed, as a scent is.  And: "I would
really like for the map to have a way to visually distinguish between the area your current vision shows versus your
memory of what else there is.  I know that is hard with text, but could color be used in some way?"  So the map in
colour (the console's VT: SGR on the Vera's screen and a serial terminal alike): what's seen now bright, what's
remembered dim, a drawn map's squares a colour of their own, the character bright; the faint glyphs kept, a setting
to turn colour off.  And: "And just remember that with better intelligence, the map stays clearer longer, but that
doesn't mean it is bright and shiny when you can't see it."  So only what's seen now is bright; what's remembered is
dim however well it's kept (a memory's quality says whether a square's shown, how true its place, at most which dim
shade).  And: "Map quality should reflect the drawing skill of the map maker (or cartography first, then drawing
second if no opened cartography skill).  If neither is open, open drawing when the first map is being drawn.
Cartography is a skill that can only be opened with the right tools and instruction.  I am not sure what the HM rules
say regarding that.  Please try to use the HM rules where they are explicit, and we can add to the house rules for
the cases where it is unclear or leaves it to the GM."  The book: Cartography a specialty of Drawing (Skills 13), a
map's quality limited by what its maker knew; a specialty chosen at the skill's ML 40, opened at its ML (Skills 2); a
teacher at least 20 higher (Skills 7); Drawing's [1 Month], Cookery's [First Use] (not defined in the text: read as
how each is opened).  So a drawn map tested as it's drawn (Cartography, else Drawing; its errors and the believed
north's in it), Drawing opened at its first map (the house's, over the book's month), Cartography only by instruction
and its tools (none in the dungeon yet: from the sheet, else later), a skill not opened tested as the book's opening
says, generally.

**Design:** DUNGEON.md, moving (watchfulness: mapping as it goes); still to come.

### Every suggestion generalized; one system for long tasks

"Please try to generalize any of my suggestions to similar things when you can. All long-running tasks should fall
into the same system, and should be interruptable either by issuing a "stop xxx-ing" (crawling, casting, etc.) or by
an interaction with denizens or the environment (triggering a trap, being ambushed, slipping and falling, and so
on)."  Words as a task goes: "walking down the corridor, corridor turns right, torch is flickering as you continue
down the corridor".  No interim walking words: built once, in slice 4.0.

**Design:** DUNGEON.md, still to come.

### Perception as layers (D3)

The user's idea: the world has layers, one for each kind of perception; everything that affects a layer contributes
to it where it is, and each sense samples it like a directional antenna, by its facing, acuity and threshold; what it
reads becomes the circumstances of the referee's test, so the referee stays neutral.  "if I am standing in the dark
(no light source), I should be able to see other light sources from much further away."  Decided (D3): the senses
work out perception for every actor by the same layers, a far denizen given a coarse answer.  Built 2026-10-17 (move
2.5, 4fdf6a2): the light layer, the lights seen far off, the eyes' adapting, the denizens asking the senses.

**Design:** DUNGEON.md, perception as layers.

### No fixed size

"I also don't want some fixed maximum size to the dungeon. It should be as big as it needs to be, given there is
enough disk space to hold it."  Built 2026-10-17 (move 2.8, 04da7cc): every store that grows with exploring paged by
region to the save folder, versioned so a crash never leaves the folder ahead of the save.

**Design:** DUNGEON.md, no fixed size.

### One authority on state: the plan realigned

The work paused and the plan realigned to the critique: one owner for every piece of state (the world the bodies, the
things, the portals, the traps, the clock and the rules' use; the maker generation only; the referee rules only; the
denizens minds only; the senses perception for all), played slices, the house rules written down.  The user: "Do not
start new work on the new plan until I tell you to".  Decided with it: D2, the item store's code in the world's swapped
parts; D3, above.  Built as Phase 2's moves, 2026-10-16 and 17: the writer check (2.1, 0e32da7), the maker pure (2.2,
e283e7d), the item store the world's (2.3, 8d99143), bodies and blows (2.4, b3cb1b9), the senses by layers (2.5,
4fdf6a2), the game's rules and the character's state out of the UI and the referee (2.6, 2.7, 8c44dfe), the stores paged
(2.8, 04da7cc).

**Design:** DUNGEON.md, who keeps what; DENIZENS.md, in the tasks; minds and bodies.

### The detailed plan's decisions (D4-D16)

All decided this day but D9 (open) and D17 (2026-10-17):

- D4: the images committed only at merges into reborn.
- D5: a spare torch and a flask of oil in the starting kit (slice 4.1).
- D6: bleeding over time, as HarnMaster has it, with first aid and bandages that stop it, and healing over days of
  rest (slice 4.3).
- D7: death permanent, with autosaves before danger as a setting: turned off, it's hard mode.
- D8: the character carries on into a new dungeon, with its skills, wounds and gear (slice 4.4).
- D9: hearing (each kind's sounds, and the range) open: tuned by ear once real environmental sounds are in.
- D10: an ally in the character's way moves aside if it perceives the character trying to pass, if it can and is
  willing; it may have a reason to stay (guarding, told to wait, cornered).
- D11: powers from HarnMaster Magic and Religion read in full, psionics from the core rules; talents latent until
  found in play; all Level I and II spells first, the higher levels later; all the deities once the Religion book is
  read.  The plans include psionics (the user asked).
- D13: Strength doesn't add to impact; it sets how heavy a weapon can be wielded without penalty, heavier versions
  hit harder, and strong characters are made with heavier weapons.  The first X pounds carried are free of
  encumbrance, X doubling every 4 points of Strength from 10 lb at Strength 10 (the user chose this curve over a
  straight line or doubling every 5).  A wrecked door, window or portcullis appraises by its state.  A slip climbing
  out of a pit ends the climb, and the player chooses what next.
- D14: morale for the character an option, like hard mode: off by default; magical or psionic fear always can.
- D15: moving quietly tests Stealth once as it starts, then again when something changes or a longest time has
  passed; the grade sets the steps' loudness (slice 4.6).
- D16: the shared ROM library for the programs' common code not yet: revisit near the release.
- For denizens: "For denizens, read the harnmaster bestiary" (before slice 4.5).

**Design:** HOUSE.md, decided, not yet built; open.  DUNGEON.md and DENIZENS.md, still to come.

## 2026-10-17

### No old saves till the release

"until the final version, I do not care if old save versions work. You do not need to support old versions of the
save."  Built 2026-10-19 (3b63bae, SAVE_VER 10): only a save of the game's own version is carried on.

**Design:** DUNGEON.md, the save.

### The answers' times measured at 115,200 baud (D17)

The responsiveness targets measured at 115,200 baud, which the user can test on the board; 9,600 accepted as slower:
"if you don't mind slow text, but that adds to the ambiance".  A command's first letter within 720,000 cycles (a
fifth of a second; the dq0answer tests).

**Design:** SOUND.md and DENIZENS.md, the tasks giving way to a command in hand.

### Phase 2 played

The user played Phase 2's end at 115,200 baud: "it looks okay for now" (reborn e3c2c26).

### Nothing told twice

A trap told twice in one look's words.  Two causes, both fixed (d806d5a): a door sometimes made twice on one square,
and the stirs telling every denizen seen again.  A door is made once; a stir tells only what's changed; alike
denizens and things are told together.  A guard in every dq0 test fails a sentence told twice in one answer.

**Design:** DENIZENS.md, seen and heard.

### Looking one way; found told as found; a long task stopped by the actor's own state

"look north", "look left", "look at the door": a look that way, further and in more detail, through whatever lets
sight through, telling what lies that way: "To the north, the room opens up and continues some distance before
ending in a stone wall with a closed door in the middle of it."  Whatever a test finds is told apart from what's
plain to see, after it, and says how it was found ("Searching about..."; searching as it goes, a trap found was
told as if plain to see).  A long task stops or asks by the actor's own state: breaking down a door "should stop or
prompt to continue if you are injured, become fatigued, a hostile enemy enters the scene".  All slice 4.0's.

**Design:** DUNGEON.md, still to come.

## 2026-10-18

### The interpreter and the history: the game plan

The user's notes (DesignIdeas.md, theirs) name two pieces the game needs: a command interpreter, and a generator of
the dungeon's theme from a history.  The game plan's decisions:

- G1, bank programs, the user's design in place of running a whole program in place: a child program at $8000 in a
  RAM bank of the calling task's own, cached there until the bank is wanted; children are subroutines, not
  sub-tasks, called on the caller's own C stack.  Built 2026-10-19 (Phase 3B, 8e1c03f).
- G2, the world's scale: a new region generated for each game, for now.
- G3, the setting: the game's own world, its own peoples and names (HarnWorld as inspiration only).
- G4, the interpreter: an abstract set of actions (VB_EXAMINE_OBJECT, VB_OBSERVE_LOCALE, VB_RETRIEVE_OBJECT ...)
  that phrases map onto; Language.txt a start only; the whole grammar designed first, with the user, then built in
  stages.
- G5, the goal kinds: all four (an artifact found, evil cleared, a prisoner rescued, a way through), maybe more, so
  the kinds are data.
- G6, the order: 3A, 3B and 3C side by side, 3D once bank programs work.  "Do not start anything yet." (till
  2026-10-19).

**Design:** DENIZENS.md, the theme (still to come); DUNGEON.md, built to a plan (the histories).

## 2026-10-19

### Phase 3

"Continue with phase 3 of the rewrite", and later "start all the remaining phase 3 items in parallel".

### The command grammar's answers

The user's answers to the command grammar's fifteen questions: "really" kept as a detail marker ("really look" a
close examination); CLOSE and OPEN actions; "look at" a glance, "examine" and "look closely at" the close look;
"take a look" a look; the single letters n, s, e, w, f, b, l, r, u, d commands, read as directions where a direction
is expected; "ready" EQUIP, "use" USE; "it" the last thing of any kind the player named (a thing, a place, a
denizen or a direction), "them" the last group; a bare "take" with nothing near and nothing named says so; "stop"
ends whatever runs, "stop digging" only that task; FIND and SEARCH two actions; speech in quotes, quoted text alone
said aloud, "say hello" without quotes refused with the form shown; possessives later, in a stage of their own; a
numbered choice answered by a number, a selector or a name; the player "me" only.  And answer 15: "alias" the
command for text aliases ("alias x is dig down"), naming things and places by name and call.

**Design:** the command grammar; DUNGEON.md, names.

### The parser a task of its own, in assembly

"if assembly is considerably faster, do it"; then the parser its own task: "go ahead with dq0c and the timing
tests".  Built (236c5d4, 7d50f36): dq0c, the grammar deciding, a command's meaning made on the 65C02, parse.c gone.
Start-up work: a one-time cost is fine, up to ten seconds in all, as long as the player is shown what's happening;
trade start-up work for each command's speed.

**Design:** SOUND.md, in the tasks (the tasks' count); DUNGEON.md, names.

### The documents split

The design documents say how things are now; this log says when and why, in the user's words; no "Built:" lines in
the design (the detailed plan's rule for documents, Phase 3A).

### The command interpreter's first build (Phase 3C)

Actions, not verbs, and verbs as data (the user's, 2026-10-18; the game plan's 3C): a line makes one command, an
action of the catalogue's and its roles, the same structure a denizen's mind is to post; each verb a line of data
(its actions by the phrase's shape, the thing it takes unsaid, its value for the game, the skill, the long task).
Built on the grammar's answers: "it" the last thing of any kind named (7), a bare take saying so when there's
nothing near (8), a question answered by a number, a selector or a name (13).  The choices made in building it, the
user's to review: a thing's word is its head, its last word before a preposition ("the iron key"); "go down the
stairs" climbs them; the lead of several is the closest, as yet; several of a name, none said, is the world's
services' choice as before, but a throw's mark and a target set must be one, so asked; "follow" an ally's request
still (the game's), not the character following; anything thrown at the book's missile ranges, a kind's reach its
short band, a hit wounding nothing till the referee's missile attack.

**Design:** DUNGEON.md, who keeps what (the command) and seeing (which, asked); HOUSE.md, throwing and commands.

## 2026-10-20

### HOUSE.md's open rules

The user's answers to the house rules' open questions (each the recommended one):
- a theme's own tests name a purpose, which each ruleset's referee maps to a skill of its own, not a skill by name;
- untrained skills stay open, every test tried at the opening level, only tools gating one (lockpicks);
- a skill never opened takes its group's penalty, as an opened one does (the book's: the universal for a craft),
  not the physical one whatever it is;
- climbing's rate goes by the face's grade, once the maker grades faces;
- bare hands dig slower than a blade, so "a sword digs earth too, poorly, and dulls" comes about: half a foot of
  soil a spell, a blade or a knife a foot; and "use of hands for digging should only work in soft soil.  Hard-packed
  soil or harder should result in a 'you make no progress' result (or the like)": soft soil a material of its own
  (a natural cave's earthen floors), the rest's earth hard-packed;
- then: "Hard-packed earth should be harder to dig for any tool (half the soft soil rate)", and "Earth floor take no
  mining skill, and hard-packed is half the rate of loose soil.  Anything harder uses mining skill, and rate depends
  on material (only a pick will work for harder materials)": earth dug at the tool's rate, untested; flags, masonry
  and stone a Mining test each spell, a pick's alone (the mattock, the chisel and the shovel's flags gone).

D9 (hearing) stays open, tuned by ear once the real sounds are in.

**Design:** HOUSE.md, tests, digging, decided but not built.

### Slice 4.0's words

The user, starting Phase 4 ("On to the next phase then."), at the slice's first step:
- where the character is, and where things are, in words relative to its facing only, never the compass;
- a long task's progress shown both ways: a bar on the status line and words as it goes;
- interruptions: "I like hard stops, soft ask, but all wounds taken should be hard stops";
- a walk's words at each change, and a line on a quiet stretch about every 30 feet.

**Design:** DUNGEON.md, still to come (slice 4.0).

### Out of reach: walk over first

The user's play note, 2026-10-20: "During play testing, one thing I found annoying was when I was trying to pick
something up, if the character was not right next to something, it would say I couldn't pick it up because it was
too far away, but I was not asked if I wanted to walk over to it first.  I think that an auto-walk should happen if I
specify I want to pick something up that is not right next to me, just as an auto-engage should happen if I target
something and attack (either by just stating `attack the goblin`, or `target the goblin and attack` or just
`attack` when there is an active target and not engaged and not using a ranged weapon)."  Built as the general
kind (the user's standing rule): any action that needs its thing within reach walks over to it first, a long task
like any walk (its bar, stoppable, stopped by what it meets), then does it; an attack on one out of reach, unengaged
and not with a missile weapon, closes with it first.

**Design:** DUNGEON.md, moving; HOUSE.md, where you are, going places, and long tasks.

### Engagement zones on every move

The user, 2026-10-20, of the walk over and the close-in: "Don't forget that there are auto-engage rules for movement
through an active enemy's engagement zone."  Built as the general kind: the book's engagement zones on every walk,
the character's and the denizens', not only a blow's close-in; a walk that comes into an enemy's zone stops there,
engaged.

**Design:** HOUSE.md, where you are, going places, and long tasks (engagement).

### Every occupation the book lists, and one of the player's own

The user's play note, 2026-10-20: "Another shortcoming I saw: only 6 character occupations available.  There should
be any that are listed in the Harn reference, as well as a "build your own" that allows for selecting some number of
starting skills (based on a fair analysis of other canned occupations), or just by adding a few more skill points (5
maybe), with the same restrictions on only 2 points for any one skill, except for max 3 in one skill only (a
primary)."  The analysis: the 55 occupations whose skills the book's table lists in full (Character 15) are worth 1
to 21 option points each (a point opening a skill at its OML, a point for each SB more; Laborer the least, Hunter/
Trapper the most), 11 at the median and the mean, over 5 skills at the median, the most in one skill mostly 4.  The
soldiers (Character 27, by culture and unit), the clerics (Character 23, by deity) and the mages (Character 25, by
convocation: their skills come with magic, Phase 5) are listed elsewhere in the book.  Asked, the user chose: a
player's own occupation worth 11 points, the median's (at most 2 in a skill, 3 in one primary: 3+2+2+2+2 over five
skills, the median occupation's shape), the usual 5 option points on top; the 3-point primary the player's own
occupation's alone (every character's option points keep 2 a skill).  And, the user: "Please make a note that we
will need to look at game balance for each occupation" (HOUSE.md, open).

**Design:** HOUSE.md, decided, not yet built.

### The decisions after slice 4.0

Asked 2026-10-20 ("Prompt me for outstanding decisions"), the user chose:
- **Occupations by class and sex:** the book's limits.  A serf's or an unguilded character's choice is the unguilded
  occupations, a guilded one's those and the guilded, a noble's all of them; Lady is for women.  hmchar -q draws
  within the limits.
- **Culture:** rolled, mostly Feudal, with the book's exceptions (Imperial, Viking, Tribal; elves Sindarin, dwarves
  Khuzan); -q rolls it too.
- **Deity:** the player's choice, one of the ten or none, the book's morality ranges shown as a guide; -q none.
- **Tiring:** strenuous work tires the character as a run does (digging, climbing, battering), and a long dig or
  climb asks ("You're tiring.  Keep digging?").
- **Engaging:** the book's optional engage initiative test is used: as a character engages an enemy, both test
  Initiative, and a defender that does better gains a tactical advantage (it strikes first, or disengages).
- **A grope while engaged:** taking a thing up, drawing or swapping a weapon while engaged is a Dexterity test; a
  failure wastes the round.
- **Denizens' places:** a denizen keeps its bearing and distance (as built), not a corner's words.
- **A look one way's reach:** kept at 31 squares for now, and the user: "I don't know that sight of 6 is
  keen...lighting will restrict what is seen more so than absolute distance usually, but depending on sight
  attribute, you might be able to see decent detail much further away.  It is the level of detail that is
  affected, unless completely blind, of course.  For example, a low-vision person might be able to only see
  movement beyond a certain distance, where a person with better vision would be able to resolve general shape
  (humanoid or not) and someone with even better vision might be able to tell the race (khuzdul, gargun,
  sindarin, human).  That should be modeled if possible, but may need to wait.  I will leave that dermination
  for later.  For now, keep it as is."
- **hylang:** its two faults found by the occupations work fixed now: read-lines on a file over 8 KB (garbage), and
  its slow reading of source.
- **A search cut short:** covers only the part done (the squares nearest first), each at the full chance, not
  every square at a harder one.
- **Battering's noise:** every blow heard, not only the first.

And the user, after: "Tiring work includes swimming, climbing, running, battering down a door.  Please add a way for
test to be determinitive if that means turning off initiative rolls with at flag, and fixing culture the same way
(with flag) sounds necessary."  So: a dq0 flag turns the engage initiative test off (the tests' runner adds it as it
adds -a, a test of it asking for it on), and an hmchar flag fixes the culture (the tests' characters keep theirs);
swimming tires when there's water (slice 4.9).

**Design:** HOUSE.md, decided, not yet built (and Open: sight's detail by distance).

### Fighting on, and the defences chosen for you

The user, 2026-10-20: "Because the games is somewhat realtime, defenses should be automatic, based on what should be
best for the situation normally.  Also, once engaged in combat, you should continue to attack your target as you can,
without actively having to say "attack" each time."  As built then: the character's defence the better of its block
and its dodge (a creature counterstrikes if it's the better at it, else dodges), and each attack one blow.  The
user chose:

- **Fighting on, once engaged either way:** after the character attacks something, and when a hostile enemy engages
  it, it strikes a blow each round as it can, without "attack" again; any command takes over; the usual soft
  questions apply.
- **The next foe:** its target beaten (dead, senseless, fled out of reach) and others still engaging it, it turns to
  the next; none left, it stops and says so.
- **Defences:** "Counterstrike should only be used sparingly, on command.  Dodge or block should be preferred, with
  block allowed for only one foe per round, per blocking weapon (if I have a shield and a sword and there are two
  attackers, I should be able to use block against both, one with the sword, and one with the shield, but not both
  with the shield).  Likewise, dodge should only be able to be applied to one foe per round as well."  So the
  automatic defence is a block or a dodge, whichever is the better against that blow, of those not yet used that
  round; a counterstrike only when the player says (defend counter), its weapon used for the round as a block's
  is (the lead's reading); none left, no defence.

And the user, after: "if using ranged attacks, you can continue attacking targets as long as they are within range or
you run out of ammo.  Throwing daggers (if you have a brace of them) would be handled the same way as arrows for your
bow; you throw/fire, grab the next one/knock the next arrow, repeat attack.  Should all be done automatically until
foes are all dispatched, you are out of ammo, or you are engaged and need to switch to a melee weapon."  So fighting
on with a missile weapon (slice 4.7's, with the missiles) shoots or throws at its target while it's in range, the
next arrow or dagger taken each time, turning to the next foe in range, till none is left, the missiles run out, or
an enemy engages the character, when it stops (fighting on in melee then, from the engagement: the lead's reading).

And then: "For ranged attacks, the player will want to be able to specify the range at which they want to
auto-target and attack hostiles, something like "autotarget with bow at medium range".  That can be an enhancement,
but the same can be done for auto-engaging enemies with melee (autoengage with sword at close range), with close range
defined specifically as within one square of normal engagement zone, and range weapon ranges defined as per the ranged
weapons tables."  So a setting, later: a weapon and a range at which a hostile begins fighting on by itself (melee's
close-in at the engagement zone and a square more; a missile weapon's at the book's bands, short to extreme).

**Design:** HOUSE.md, decided, not yet built.

### Disengaging, and tiring in a fight

Asked how a character gets away from a fight (none built: a walk away went a square, then the fight went on), whether
fighting tires, and the strenuous work rate, the user (2026-10-21): "1. For disengaging, try to have it follow the
book.  2.  Tiring for combat should happen at the standard strenous work rate.  3.  Fatigue rates for running should
be higher than for strenuous work, and sprinting should be higher still."  So:

- **Disengaging, the book's** (Combat 7): an engaged character moves a square, and then, if no longer engaged, a
  half-move more, stopped by another enemy's zone, with no attack that round; the only way an engaged character
  moves.  "disengage", "withdraw", "back away" (a way, or away from the foes engaging it); "flee" engaged the same at a
  run (the grammar's already), not engaged a run away from the nearest foe; "pass" a round given up, defending.
- **A fight tires** at the strenuous work rate, a level each five minutes of it (the book's second variant of combat
  fatigue, Combat 7).
- **The rates:** running tires more than strenuous work, sprinting more again: as built (a sprint a level a minute,
  a run one each two minutes, strenuous work and a jog one each five; the book's, Combat 6 and Skills 6).

**Design:** HOUSE.md's fights and long tasks.

### Scents, the air and rain

The user (2026-10-21), as slice 4.1 began: "For traces, some denizens have rather keen sense of smell, and may be able
to follow a scent for much longer than an hour.  Blood hounds especially so.  I am not sure how old a scent trail can
be for them to track a target, but definitely more than 1 hour.  Rain and wind would act to reduce those further.
Wind/air movement toward a potential foe could also alert those with particularly good sense of smell long before
sound or light might reach them (thus why hunters try to stay down-wind of their prey).  Smell, rain and air movement
layers will need to interact in those kinds of ways.  Please make sure the plan has this accounted for."  So:

- **A scent's age is the reader's nose's to judge** (slice 4.1): what can still be followed goes by the kind's smell
  acuity (a hound's keen nose follows a trail days old; most beasts' hours; a man's hardly beyond the freshest), not
  a fixed hour; a keen-nosed denizen smells the character in still air within its nose's reach, as it sees and hears.
- **The air layer** (slice 4.5, with the sound layer: both spread over the rooms and the openings): drafts through the
  dungeon and the surface's wind, carrying scents (and sounds) downwind and away from upwind, so a keen nose downwind
  smells a foe long before sound or light reach it, and a hunter keeps downwind of its prey.
- **Rain and running water** (slice 4.9, with water): washing scents and tracks away sooner.
- Slice 4.1's smell layer is built open to both: one place a draft widens or narrows how far a scent carries, one
  place rain or water speeds its fading.

**Design:** DUNGEON.md, perception as layers; the plan's slices 4.1, 4.5, 4.9.

### Hunger and thirst

The user (2026-10-21): "When do we get hunger, eating, and drinking?"  Nothing planned then (the kit's rations and
waterskin carried, eat and drink refused; HarnMaster's core rules have none).  Asked where, the user chose with water
(slice 4.9): hunger and thirst over time (house rules: faster at hard work, levels of penalty, then starving or dying
of thirst), eating the kit's rations and drinking from the waterskin, refilled at wells and streams; the cheap way to
manage it a standing order to eat and drink when needed, warnings ("You're getting hungry."), and the status line
showing them once they matter.

**Design:** the plan's slice 4.9 (to be drafted with it).

### Hands and limbs

The user (2026-10-21): "Also, if you have a limb injury, it should affect that limbs usefulness.  A serious fracture
to the leg should make it more difficult to walk, same idea with arm wounds and using a shield or weapon in that
hand.  Also, is there handedness (right handed versus left versus ambidextrous) in the character profile?  I think
the HM rules account for it, and ambidextrous is a rollable physical trait.  There should be a penalty for using a
weapon in your off hand unless ambidextrous of course, and the HM fumble rules should be followed for hand and arm
wounds and dropping anything held in the hand on that side."  As built then: handedness only as the sheet's medical
traits (Character 9: left-handed, ambidextrous), the hands counted but not told apart, the referee's fumble and
stumble rolls made and a grievous limb useless, but nothing done with them (a fumble's drop not even made: a fault).
Asked when, the user: "Right after 4.1, but don't forget stumbles and falling prone with leg wounds.  If prone isn't
a thing until later, make sure failed stumble rolls are added to that work."  So a slice of its own after 4.1
(the plan's 4.1b): two hands, the off-hand penalty of the book (10, not a shield's, none if ambidextrous: Combat
3), an arm's injuries hindering what its hand uses and its fumble rolls dropping what that hand holds, a leg's
slowing the walk and its failed stumble rolls laying the character prone (prone is the fights' already: getting up a
round), a useless limb a hand that holds nothing, a leg that hobbles or crawls; 4.6's body positions keep failed
stumbles too.

The user (2026-10-22), on a useless limb: "According to the rules, at some point of injury a limb is useless.  For an
arm or hand, that means no picking things up with that hand.  For a leg or hip, that means no walking, but crawling
may still be possible if one arm and one leg still work."  So a useless arm's hand takes nothing up and holds nothing;
a useless leg walks no more: the character crawls if an arm and a leg still work, else it can't move.  Then the
user: "You can stand and hop with one good leg.  Hopping would be quite slow (crawl speed), and would tire you at the
strenuous work rate.  A crutch could be used to move at walk speed."  So with one good leg the character can stand
(the book's Rise) and hop, at a crawl's speed and tiring as strenuous work does, or walk at a walk's speed with a
crutch (a staff, a pole: a thing that serves as one, held); with no good leg it crawls, if an arm still works.  And
then: "No good arms makes crawling impossible, and both arms are required if both legs are unusable.  Also, getting
up with no good arms would be difficult, should probably require an agility check or something similar."  So a crawl
needs a good arm with a good leg, or both arms with neither leg; getting up with no good arm takes an Agility test.

Slice 4.1b built (2026-10-23), the user on what was left: "Don't worry about peglegs for now.  Smells are okay as
they are for now as well...playtesting will help me figure out those numbers.  Cornered beasts can wait too.  The
house rules look pretty good."  So HOUSE.md's hands and limbs stand as drafted (an arm's injuries a step a level, a
limp a fifth more time a square a level, a hop or a crawl three times a walk's, the Rise's test, the main hand's thing
striking when none's named, a disarmed creature's fists); the book's pegleg left out; the smells' numbers and a
cornered beast fighting back wait (playtesting first).  Then: "Go ahead and push, then start 4.2".

**Design:** HOUSE.md's hands and limbs; the plan's slice 4.1b.

### A helpless character, and poison

The user's playtest (2026-10-21): a cave adder bit the character in shock again and again ("For some reason the
wounds just kept getting worse and worse"; and "the adder attacks were "stabs" instead of bites").  Found: past the
record's 12 injuries each new wound was added into the last, its words the running total (S12 ... S37); a character
in shock made no shock roll when wounded again (the book: one with every injury), so it never went senseless; by the
book only a grievous wound bleeds and only a mortal one kills, so small bites never ended it; and a bite was worded a
stab (the point aspect's words).  The faults fixed as faults.  Asked what a creature does to a helpless character
(senseless, or in shock): the user chose **by its nature**: a killer or a predator finishes it (as the character
finishes a senseless creature), one that only defends itself (the adder) stops once the threat is down and goes, a
cunning foe may rob it or leave it; each kind's way in kinds.txt.  And **poison** (the adder's venom, stings, resisting
them, their effects over time, antidotes) with slice 4.3, wounds over time.

**Design:** HOUSE.md's fights; DENIZENS.md; the plan's 4.3.

### Slice 4.1 reviewed

The user's answers on slice 4.1 (2026-10-22), after it was built: "1. push, then work on the speed,  2.  Rats should
attack if cornered, but that might be difficult to determine, so defense only is okay for now.  Bears and Cultists
are fine.  Weight should be recounted immediately after a robbery. 3.  Your suggestions here sound good.  4.  The
only numbers I need to think about are the smells (repeated passage or occupancy should enhance prior smells)."  So:
pushed (dab6cc15); the giant rat a defender only for now (one cornered, no way off but past the character, to fight
back: later, its test to be found); the cave bear a killer, the cultists and their priest killers, as built; a
robbed character's load told the referee at once; HOUSE.md's 4.1 numbers stand but the smells', the user's to think
about, and a scent strengthened by repeated passing and by a stay (a lair's keepers, a denizen resting, the
character waiting: its time there counted as passings); and before 4.1b a pass for speed and room: the main RAM
freed in the UI, the world and the senses, and a command not kept waiting while the senses look again at the
denizens (the world's stir).

**Design:** HOUSE.md's light and traces (the smells) and fights (a helpless character); the plan's slice 4.1.

### A look one way's rooms, and a timed action's outcome at its end

The user's playtest (2026-10-22): "When looking though, the text is duplicated, but in different words: You peer
ahead. Through the open door ahead you see into the nave. Down the passage ahead you see into the nave, 10 feet on."
So two ways ahead into the same room tell the room once, the second way with it ("...; the passage ahead leads there
too"), as two alike openings the same way are told once already ("the open doors").

And: "When you attempt some task and it has a two-part reply, both come out almost immediately, but the UI input is
paused.  It would be better if the first half (for example "You study the mechanism a while") coms out first,
followed by, as the task timer expires, the second part (i,e, "but can't figure out how it works").  Makes it seem
like there is actually a delay in studying the mechanism before the outcome happens."  Built as the general kind:
every action that spends time and has a test (a trap studied or disarmed, a lock picked, a door forced, a
portcullis heaved, Tracking ...) a long task of the one system, its first words at once, its outcome decided and
told as its time's done; stopped first (a blow, a wound, the player's stop), no outcome, the stop told (the lead's
reading: the user's words say nothing of a stop).

**Design:** HOUSE.md's long tasks.

### Looking at a thing, its appraisal asked for, and what opening reveals

The user (2026-10-22): "When examining an item, I want to know about its state, and for containers if they are open
or closed, and what is on/in them.  `look [at] item` instead of examine should give these things.  Examine gives too
many unimportant details (like value).  I think there needs to be a way to specify what aspect you are evaluating,
maybe with `examine value [of] item` or `determine value [of] item` and `examine quality [of] item` or `determine
quality [of] item` or maybe just `quality [of] item` or `value [of] item`.  When opening something (a door, a bag, a
chest) there should be an automatic `look in` or `look through` that happens, for instance if I open a chest, I
should get a "You open the chest, revealing a pile of gold coins", and if I open a door, I should receive text like
"You open the door, revealing a dark corridor running away from you into the darkness" or some such."  So: looking at
a thing tells what anyone sees of it (its state: open or closed, lit or out, worn, battered; what's in it or on it), no
appraisal; examining it a closer look, still no appraisal; its value or its quality only when asked ("examine the
value of", "determine the quality of", "value", "quality": the referee's appraisal as before); and opening a door, a
chest, a bag tells what it reveals (a look in, a look through), in the open's own sentence.

And (the same day): "Look at should also tell you if a light source is lit or out."

Asked for the words (the lead's options, the same day), the user chose: a bare examine a closer look, no
appraisal; assess the value; appraise both value and quality; judge, "quality of", "how well made" the quality; and
the other aspects now too: heft and weigh (its weight), study the lock, check it for traps, its condition.
**Design:** HOUSE.md's commands; the command grammar's words.

### Battering told as it goes, and bare hands stopping short

The user's playtest (2026-10-22): "when battering or kicking a door, nothing is written to the output until after it
is resolved instead of something like "You start beating on the door..." followed by progress.  If you kick or hit
the door with your bare hands or feet, you should be told, with it stopping short of injury the first time you would
injure yourself ("You punch the door with bare hands, but you stop short of injuring yourself.  Continue?"  Same for
kicking without foot protection.  As always, you should be able to avoid injury with a critical success or marginal
success (or possibly even a marginal failure) test against unarmed skill.  MF only results in no damage, and CF
results in damage to self?"  And: "After being warned of injury, continuing should only apply for that door, being
reset on the next door you try to batter down with bare hands/feet."  So: a battering's first words at once ("You
start battering the door.") and its progress as it goes; with bare hands or bare feet (no gauntlets, no boots), the
first blow that would hurt the character stops short of it, asked ("...but you stop short of hurting yourself. Keep
at it?"), and kept at, not asked again at that door (asked again at the next); each such blow the Unarmed test: a
critical or marginal success a blow that tells on the door and no hurt, a marginal failure no harm done to either,
a critical failure the hurt (the lead's reading of the user's question; theirs to confirm).

**Design:** HOUSE.md's portals (battering) and long tasks.

### Walking and searching in the dark

The user's playtest (2026-10-22): "when wandering around in the dark, you should not auto-adjust your movement
direction toward an opening you cannot see (a door not directly in your path).  Movement should be slowed when in
complete darkness or temporarily blind (half-walk speed; anything faster can be explicitly stated, but stumbles over
large objects in your path, or running headlong into a pit, a wall, a door, or off a cliff are real dangers) and you
cautiously walk forward, arms outstretched or weapon probing so as to not run into anything.  You should also be able
to search around in the dark with your hands to find nearby items, using `search`.  `search` should also be
qualifiable as `search nearby` or `search the east wall`, or `search around for a/an item`.  Text output while
searching in the dark should be qualified as well, such as "You fumble around in the dark and find nothing", and
searching in the dark should default to searching nearby only.  You should also be able to `search around on the
floor/table/other flat container` which would search nearby (in the case of the floor) or on that item (in the case
of a flat container nearby) for items or hidden things or traps."  So: in the dark (no light it can see by, or
blind) a walk goes by what the character knows and feels, never turned toward an opening it can't see; its gait a
cautious walk, half a walk's speed, feeling ahead, which finds a wall, a door, a drop or a thing in the way before it
blunders into it; a faster gait said outright goes faster, at the risk of stumbling over what's in the way and
running into walls and doors, into pits and off ledges; and a search in the dark by touch, nearby by default, worded
as touch; a search qualified: nearby, a wall, for a thing, on the floor or on a surface (things on it, hidden things,
traps).

**Design:** DUNGEON.md's moving, hidden things and seeing in the dark; HOUSE.md's moving and searching.

### alias for shortcuts, name for naming

The parser's review (the session hydra-2-89's, 2026-10-22) found "alias" taken as naming ("alias the sword as Linda",
names.c), against the command grammar's answer 15 and DUNGEON.md (text aliases: "alias x is dig down").  Asked, the
user: "alias should be for shortcuts, and name for naming things".  So "alias" defines, shows, lists and removes the
player's shortcuts (a line's first word standing for a line: "alias x is dig down", "alias x", "alias", "unalias
x"), kept with the game; naming is "name" and "call" (and rename, dub; forget, unname) alone.

**Design:** the command grammar (commands.ebnf's alias-cmd); DUNGEON.md's names.

The user (2026-10-22), of what was left to settle (the Unarmed rule for a bare hand's battering, a marginal failure
harming neither and a critical failure the part; the smells' numbers; walking in the dark's numbers): "The things to
settle are fine as they are for now".

### The thing named is what strikes

The user's playtest (2026-10-22): "with the current build, I cannot `hit the door with bar` if I have a `bar` in
hand.  It still says I am battering it with my bare hands".  The named tool was checked and taken in hand, but the
referee chose what struck (the best weapon or tool in its tables; a bar in none of them: the fists).  So: the thing
named is what strikes, at a thing or a denizen; anything that can be swung strikes as an improvised weapon (by the
book's rule if it has one, else the house's, by the thing's size, weight and material); what's too slight to hurt,
refused.

**Design:** HOUSE.md's fights and portals.

Then the user (the same day): "A bar should just be treated as a club or stick, no?" (so the book's nearest weapon: a
bar a club, a pole a staff), and "I think it should try and use whatever you request, but if it is something soft (a
piece of clothing, a noodle, string, rope, etc.) it should reply with something like `You attempt to batter the door
with the noodle, but it seems ineffectual`": so nothing refused, a soft thing's attempt made and told ineffectual,
no harm done.

And with nothing named: "I don't think it should auto-fallback to something better unless you have nothing at hand,
and then it should ask for confirmation.  If your hands are empty, but you have an axe at your belt, then something
like `You can try battering it with your hands, but would you rather use your axe?` or some equivalent saying."  So
a blow with nothing named strikes with what's in hand; with empty hands and something better carried, it asks first
(yes takes that in hand, else the bare hands); with nothing better carried, the bare hands, unasked.
