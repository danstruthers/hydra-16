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
Its gap closed (2026-10-29): a denizen told as it stirs is kept as told too (a walk stopped by one coming into sight,
the look as it stops doesn't tell it again), and a journey's next leg tells its new place whole but nothing seen
before in the same answer (lbsaid.s: a sentence known by where it's said of; a thing's and a denizen's by nothing).

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

### Searching a thing, and a whole room

The user (2026-11-02, a playtest):

"When searching a pile of straw, it says "no sign of a trap", but I am not looking for traps, but for anything hidden
in the straw.  Also, I would like `search the room` to take time to search around the whole room, poking through piles
of straw, taking a closer look at the items, etc., not just searching for hidden doors or traps.  It should report
incrementally what you are doing as well."

Built: a thing searched means what's in it or under it, no trap words unless they're asked for; a heap hides what's on
its square; `search the room` a long task of stops along the walls and across the floor, each thing and heap in turn,
told as it goes, kept as far as it went; bare `search` stays the quick minute.

**Design:** HOUSE.md's the search's forms and a room searched whole; DUNGEON.md's searching.

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

### Outfits, reach and things used up

The user (2026-10-23), on the map agent's questions (the kit, using things up, "handy and equipped"): "For starting
kit, each class should have appropriate starting kit, with weapons, armor, and other items appropriate for characters
just starting out in their occupation.  Universally, a backpack, torches, tinder box, rations x3, full waterskin, belt
with pouch and belt knife/dagger, some money.  If a bow or other ranged weapon, appropriate ammo and storage (arrows +
quiver, sling stones or bullets + pouch, bolts plus bolt case, etc.).  Swords have scabbards, daggers have sheathes
(both on the belt).  Throwing knives should have a bandolier with a number of knives (maybe 3-5) included.  Shield
skill normally includes a shield.  Starting money should be allocated to weapons, armor, and supplies, as per costs in
the HM rules.  Thieves/Lia Kavair should have tools of the trade, and probably mapping materials.  Please do a deep
analysis of each occupation and come up with a good list of starting gear for each, but keep in mind that it should be
somewhat random what specific gear is generated, but it should stay appropriate to occupation and selected skills  As
for using up material, arrows can break, and should be able to be retrieved, as should sling bullets and stones not
embedded in something, but wild misses (critical failures) should possible be lost.  Ink, pencils, charcoal, etc.,
should be used up eventually.  A pot of ink might last a really long time, as might a pencil, but a chunk of charcoal
or chalk would be used up faster.  You can build that.  Handy would be readily available, so in a backpack that is
being worn is not as readily available as one in a belt pouch.  In a sack, in a backpack is two-containers deep, so
should require longer for each level of container.  A backpack on the ground or in hand (not worn) is more readily
available and does not require a grope to remove it first before taking something out of it.  Searching a worn
backpack takes it off first, then if it was on, puts it back on, each taking time.  Just shrugging it off/dropping it
before battle should not require time.  Please go over what I have tried to lay out here and make it consistent and
as general (appropriately applying it to as many cases) as possible."  The book: a character begins with the clothes
worn, a knapsack, a moneybelt and its family wealth by class (Character 17: serf 40, 80, 160d; unguilded 60, 120,
240d; guilded 90, 180, 360d; noble 600, 1,200, 2,400d: poor, average, rich), spent at HarnWorld's price list and the
weapon and armour tables (Combat 3-4); a squire equipped by its lord.  So three general systems, every actor's:

- **Outfits**: the universal kit, then the occupation's by its skills (each weapon with its carrier: a sword's
  scabbard and a dagger's sheath on the belt, a bow's quiver of arrows, a sling's pouch of stones or bullets, a
  crossbow's bolt case, a bandolier of three to five throwing knives; a shield with Shield; armour and supplies),
  bought with the family wealth at the book's prices, random within what suits the occupation and the skills chosen;
  thieves' tools and mapping materials for a thief.  First the analysis of every occupation, for the user to see.
- **Reach**: how readily a thing comes to hand, for every taking out, putting away, search and tool's use: in hand;
  on the body (a belt's pouch, a sheath, a scabbard, a quiver, a bandolier); in a container set down or held; in a
  worn pack (taken off first, and put back on after, each its time); each container deeper its time more; a pack
  shrugged off or dropped at no time.  A tool "handy" (mapping's materials, picks, a tinderbox) is one readily to
  hand: not in a worn pack.
- **Things used up**, by each kind's data: drawing's charcoal and chalk soon, a pencil and a pot of ink long; a
  light's fuel already; missiles (slice 4.7): arrows and bolts that break, missiles taken up again where they fell or
  out of what they struck, a wild miss (a critical failure) maybe lost.

Then, on the denizens: "Denizens should be equiped appropriately for each type, with those items one their bodies as
appropriate.  If the denizens have stuff that would normally be stored where they sleep (If their mates can be
trusted, or they have lockable chests), then some of their kit may be where they sleep.  Most humanoid minions will
have little in the way of belongings other than those required for their occupations (which would usually be weapons
and armor and some small amount of coins and personal trinkets, but usually not water and food unless they are
wandering explorers).  The details will need long, hard thought.  More intelligent enemies would have more personal
items ov real value (dwarves, elves, humans), for instance.  Please put together a plan for believeably equiping the
other inhabitants of the dungeon."  So the outfits are every actor's: each denizen's kind (and role: a leader, a
guard, a wanderer) its gear on its body, its other belongings where it sleeps where its mates can be trusted or a
chest locks, a minion's little more than its trade's (weapons, armour, a few coins, a trinket; no food or water but
a wanderer's), the more intelligent peoples' more of real value; first a plan, for the user to see.

The plans written (2026-10-26: F:/.claude/hydra-2/outfits/outfits.md, denizens.md, kinds.md, denizens-kinds.md, outside
the repository), the user's answers to their questions:
1. The universal kit comes out of the family's money, strictly: a poor character may go without some of it.
2. The family wealth rolled at the character's making, 25/50/25 poor, average, rich, nudged by estrangement, kept in
   its file.
3. A soldier brings its unit's weapons and armour, free.
4. A knight's lance and horse are left at the dungeon's mouth.
5. A cleric: the book's equipment, the universal kit from its temple, weapons and shields for the fighting churches
   (Agrik, Larani, Sarajin).
6. "Some kit should be generated when denizens are created (weapons, armor) but others (personal items) can wait
   until first encountered."
7. A denizen's armour only what it wears (the fixed numbers gone).
8. Some denizens unready now, before slice 4.6: weapons sheathed or set down, a turn to ready them.
9. Coins for all folk: elves and kobolds too (silver pennies; a dwarf's gold crown as proposed).
10. A key opens its own lock, no test.
11. "Please use the denizens from the HarnWorld Bestiary, and include Khuzdul (dwarves), the Sindarin (elves),
   Humans, and Gargun (or all types)."

Then (2026-10-27), on the denizens' gear told (the gear2 brief's item 4: the UI naming what a denizen holds and
wears): "For #4, I would prefer that the game say general things when looking, more detail when examining,
including basic quality of visible equipment when assessing, as would normally be the case.  If assessing at a
distance, eyesight should dictate quality/granularity of any assessment/examination based on viewing distance,
naturally"  So three levels of detail for anything seen, things and actors alike: a look general, examining more
(what it holds and wears), assessing the quality and condition of what's visible; the viewer's eyesight and the
distance setting how much comes through, one rule for every viewer; what's hidden (beneath clothing, in a pack)
never told by looking.

### Money

The user (2026-10-28): "Is the money system modeled after the one in the HM rules?  If not, it should be."  It wasn't
(prices in whole pennies, the pennies bronze and weightless, a crown 100 pennies, a purse capped at 255, the denizens'
purses and valuables the house's ranges), so it is now, the book's where it speaks (HarnWorld's Prices and Incomes and
its price list; HarnMaster's Treasure 1-7, Combat 3-4 and Character 17), the house's where it doesn't (HOUSE.md's
money, for review):

- **The coins and the reckoning** (HarnWorld 23): the silver penny (d) the only common coin, a dram of silver (Treasure
  2); the farthing (f) a quarter penny; the shilling (12d) and the pound (240d) only names for amounts; the Khuzan crown
  the dwarves' gold coin, an ounce, worth 240 pennies, and rare (most never see one).  Every worth and price in
  farthings (the catalogue's, the outfits' prices, the family wealth: a candle 1f, a hot meal 1d, a dagger 24d); the
  appraisal's words in farthings under a penny, pennies under a pound, pounds and shillings above ("about 2 pounds 6
  shillings").
- **Prices:** HarnWorld's list (read from its PDF's table: the text extraction had shifted the farthing prices) and
  the core's weapon and armour tables (Combat 3-4, the referee's book, where the two differ: a mail hauberk 990d, a
  handaxe 72d); the house's for what the book lacks, by its nearest, marked in kinds.c.
- **A purse holds all its money:** a lot's count is a byte (250 at most, as lots join), so a big purse is several piles
  told as one ("You carry ... 600 pennies").
- **The denizens' purses and valuables by the Personal Possessions tables** (Treasure 2-3): each kind its category (a
  bandit a Brigand/Outlaw; its lord, both, the higher chances; a high priest a Cleric; the Gargun, and the kobolds that
  stand for them, Hunters/Tribesmen; the Khuzdul and the Sindarin by the human tables, as the book allows), its silver,
  gold and gems or jewelry each by the table's chance and dice, made as it's first met; the jewelry by Treasure 5's
  items and materials.  Hoards the house's still: the book says only to use the possessions tables as guides.
- **Condition by Treasure 1's table:** what a denizen carries or keeps by its Person column (most good, a few fair or
  worse, one in ten as new), what lies lost or left in the dungeon by its Found column (one in five broken or decayed;
  silver and gold the less: the house's thirty more on the roll), told by the appraisal's life words (as good as new
  now its Excellent: the life reckoned in tenths of the whole).

Then (2026-10-28): "Also change what dwarves carry for money.  Gold crowns are far too valuable for all dwarves to just
be carrying them around."  So the Khuzdul carry silver pennies like everyone, by their category's silver; a crown only
by its gold chance (a warrior's or an elder's three in ten, a miner's two in a hundred), else in hoards and vaults.
The same mistake looked for elsewhere: every denizen's gold, gems and jewelry now the table's chance (a bandit's ring
one time in four, a priest's pendant always, an elder's gold necklace always: all gone; a priest's ring of its rank
kept, Religion 9's).

Then: "each coin should include its provenance (where it was minted), with the local region mintage most common (80%)
unless belonging to an historic/anchient hoard, which should, of course, be appropriately stocked."  And: "Of note, a
single group of coins should have a mint, yes, but one "pile" may contain many different mintings."  So:

- **Mints:** the theme's (theme.h's struct tmint, its mint lines: themegen's, the stand-ins' by hand): today's, the
  region's own realm (the game's own world, G3: its name the men's tongue's, from the seed), two neighbours (one in two
  times hard to pass), the dwarves' hold (gold, taken at par everywhere, as Azadmere's are); and of old, each era's
  people who struck coin (the kobolds and the hill-men none).  Each has its name ("pennies of Velmor") and its word, an
  adjective ("Velmoran"), for the market to come.
- **A pile, many mintings:** a pile of coins is one thing, taken, counted and dropped as one; within it up to four
  groups by mint, each its share; a part taken has the same mix (the house's: proportional), two piles put together
  merge their groups (the four greatest kept, the rest folded into the greatest).
- **Where they come from:** today's coins (a purse, a hoard of now): four in five the region's own, the rest a
  neighbour's or the dwarves'; coins lying in an old era's works its people's, whole, a gold-striking people's one
  time in four crowns (the maker's stock: an old vault's, a crypt's), worn by the Found table; the present era's (held
  still) today's.
- **Told:** the inventory and a look "pennies"; examined close, their stamps ("Their stamps: most of Velmor, a few of
  Kharzad."; an era's "(old)"); assessed, their worth as a local would take it (the book's 10-20% off for coins of
  elsewhere, the house's 15%; at par the region's own and the dwarves'; hard to pass, half; an old era's their
  silver's, three quarters).

For the market, later (the user's, 2026-10-28, not built: no trader yet): "Make a note for the market later: you will
normally try to pay with the lowest-value coins first (foreign coins other than Azadmere), but the seller may require
local or full-value Azadmere coins.  You shall be able to specify which coins to use, i.e. `pay with Kanday
[coins/pennies/farthings/money]`" (the realm written as the user corrected it: "Change Kandor to Kanday").  A pile's
groups are kept by mint, each one's worth known from its mint's ways, so "the lowest-worth coins first" and "only
these mints" are a pick among them; the mints' names and words are what the parser will take.

Then (2026-10-28): "Please keep farthings as they might be useful for denizens/"  So the farthing is a coin of its
own, as the book has it (HarnWorld 23: a silver penny cut in four): a kind (KI_FARTHINGS, silver, worth a farthing,
too light to weigh), in piles with mints as pennies are (a cut penny keeps its penny's mint); the character's odd
farthings in its purse as farthings; the poor's purses (a tribe's, a kobold's, a vagrant's, a brigand's, a miner's)
some farthings besides the book's pence (the house's: the tables give silver in pence), for review.  Told and taken as
any coin ("2 farthings", "take 1 farthing"; examined, their stamps), "count" an examining ("count the pennies"; "count
the money" the pennies'), and for the market to come, change.

Left for later (DUNGEON.md's): the coins' metal value (three quarters of their face, the book's) and their discount
abroad beyond the appraisal's telling, buying and selling (second-hand clothing 30-60% of new, a mercantyler's 10% and
50%), the market's paying and its change, usurers' notes, trade goods, written works and potions in the
purses, the Khuzdul and the Sindarin as denizens (their purses written, the kinds to come).

### Travel's stop at a shut door, and waits a night long

The user (2026-10-28), on HOUSE.md's two points for the user (travel's end at a shut door it hasn't opened; a wait
over 100 minutes held to 100): "Of the behaviors to confirm, travel should stop at a closed door, and waits should be
able to last a full night's sleep, so 8 hours at least.  Some spells may take longer than that to cast, so 16-bit
minutes would be better."

- **Travel stops at a closed door** as built: a door the character hasn't opened or shut before, one locked or barred,
  a passage's shut door, ends the journey there ("The door is shut: open it first."); one it has opened or shut before
  is opened as it goes.  HOUSE.md's line settled.
- **Long durations in 16-bit minutes:** a duration past 100 minutes (the most sixteen bits of tenths hold, 60,000 of
  them) is carried in minutes, up to 65,535 (some 45 days): a wait, a sleep, a rest, and any long task's length
  (the general kind, for a spell's casting to come), its part done in minutes too.  Shorter ones keep their tenths,
  so no short action's time changed.  The clock and the ready times were already 32-bit tenths; only what carries a
  duration in sixteen bits has the unit beside it (an intent's, the long task's, an event's).  So "wait 8 hours"
  waits eight hours, "sleep 8 hours" and "rest" sleep a night, and the words tell it back ("You wait eight hours.").
  The house's ways of it (the parser's durations, a night's sleep unsaid, the long wait's speed, the coarser ticks
  while the character rests) are in HOUSE.md's long waits, for review.

### Morale: a field about every mind, and bonds to its leaders

The user (2026-10-29), on the peoples' note that a fallen leader dismays its faction four steps worse: "I read your
comment on morale, and the effect of a falled leader should be more generic.  Any intelligent creature should modify
the morale field around itself.  The strength of that effect should be based on their importance/rank as well as their
own influence skills.  A chieftan or spiritual leader has a much larger influence (positive to their own
subordinates/friends, negative to their foes), but there should also be some heirarchical influence, regardless of
distance, that trickles down from leaders to subordinates no matter how close.  That passive influence would increase
with time (new leaders having less influence than old ones, and frequency of contact reinforcing that bond).  So, there
are upward bonds to leaders above you, and environmental bonds based on how close other friendly units are to you, and
how they are doing.  The loss of a leader (that the subordinate knows about) would instantly cause a morale check with
that leader's influence removed.  The local morale field and any morale bonds act as morale check bonuses or
penalties."  Then: "Queue up the new morale work."

Built 2026-10-30 (branch reborn-game-morale), the lead's defaults where the user didn't say, for review in HOUSE.md's
morale:

- **The field:** each intelligent actor (a mind low, average or cunning; no beast, nothing mindless) and the character
  sway the courage of every actor within eight squares by their influence: their rank in their faction (kinds.txt's new
  rank: the rank and file, a captain, a lord or a spiritual leader) and their influence skill (Oratory or Rhetoric, the
  referee's), for their friends and against their foes (a mighty foe the more: its fearsomeness), less with distance, by
  how each is doing (wounded, berserk, fleeing, fallen).  A friend counts the more as one's band is old and close.
- **The bonds:** each subordinate's to its band's head and to its faction's leader above that, wherever they are, by
  the leader's rank, the time they've had together (a new leader's the less) and their contact (a band living with
  its leader every day; others as they meet), aged by the clock.
- **Reckoned only as a test is made** (the 65C02's time), or as one may be: an actor's heart as it engages (the
  world's blow carries it to the referee, its courage tested with it as it's wounded) and as one near it falls; held
  to the book's band (Combat 18: twenty points either way), the book's bad and desperate situations beyond it.
- **A leader's loss known:** one that hears it fall (the fight's din) tests at once with that leader's influence and
  bond removed; one that doesn't (out of hearing, below, senseless) keeps its bond till it's told (a runner's or a
  survivor's word: the hook, to come).  It replaces the peoples' fixed two steps and four.
- **The character** sways its allies and its foes by its own rank and skill; its own morale stays the player's (D14).

**Design:** HOUSE.md's morale; DENIZENS.md's fights and attitudes.

### Succession

The user (2026-10-30), after the morale work: "When a leader dies, the next one below him in the heirarchy
(strongest/most influential of the those directly below them) should take over."

Built the same day (branch reborn-game-morale2), for review in HOUSE.md's morale: every leader's post in the chain (a
band's head, a faction's leader) left by its death is taken by the most influential of those directly below it (the
morale's influence: rank and skill; ties by might, then the dice), from the next rank down if there's none left
directly below; the successor takes the post's rank and weight; the bonds to it begin as a new leader's, but its own
band's keep what they had with it; those who know of the death know of the successor (the told hook's too); one at
its post that the character sees or hears rallies its fellows, told ("The small orc rallies its fellows.").  The post a
successor leaves is filled the same way, on down the chain (the coordinator's reading of the user's words, the
same day).

**Design:** HOUSE.md's morale; DENIZENS.md's morale.

## 2026-10-30

### The start's levers: the card's reads and the overlays packed (2026-10-31)

The user's choices (the lead's, relayed 2026-10-31): yes to the storage driver's read loop quickened (the bits' timing
proven on the board exactly as it was, only the gap between bytes the shorter), yes to the overlays compressed (a
small, fast unpacker in a shared ROM library, not in each task's main RAM; the banks the same as today's, checked),
no to the game on the ROM disk.  Built: the start 208M cycles at seed 1 and 289M at a big seed (58 s and 81 s on the
board, from 79 s and 104 s).  With the start quicker, dq0cmd's fight at the turns pace went otherwise: a throw's
two intents (taken in hand, then thrown) let the world move its clock and give the denizens a turn between them as the
tasks' timing fell.  Fixed generally (the hub's ui_giving, HUB_VERSION 32: no turn and no clock while a command's
being given or an intent of it waits), not by re-baselining; dq0turnsame guards it.

**Design:** DUNGEON.md, the game's start; who keeps what (the turns pace).

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

### The catalogue's room (the margins' pass, 2026-11-03)

The user (2026-11-02): "Stone chisel versus wood chisel might be a good distinction for future items."  Built: a kind's
number a whole byte everywhere, room for 60 more (NKINDS 206), a stone chisel, a wood chisel, a mattock and a spade
(HOUSE.md's digging, for review).

**Design:** items.h's NKINDS, HOUSE.md's digging.

### The answers' first letters (the margins' pass, 2026-11-03)

The swings at the normal pace, found by a probe of each answer's tasks: (1) `examine` of any thing asked the denizens first
whether it named one of them, and they're the background: a second and more when a mind was at work (now only when the
word's first three letters are in an actor's name, or the player has named one); (2) a denizen's kit in hand in the world as
Enter comes can't be left part way, up to 1.1M cycles (not cured: it would take the kit's draw and its making split, the
world's loop looking at the UI's intents between); (3) a walk to a
place in the room told nothing till it got there, 13M cycles ("You set off across the room." now).  The rest is the parts'
copies: the UI's now four pages a pass (SWFAST), the chooser's pass of the store direct, not by items ().

**Design:** DUNGEON.md, the game's start; HOUSE.md's going to anything.

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

## 2026-10-31

### Leisure

The user (2026-10-31), after the denizens' daily life: "For what folk are doing, there should be a "leisure time" state
where denizens sit around reading, dicing, talking, drinking, playing other games, etc."

Built the same day (branch reborn-game-leisure), for review in HOUSE.md's leisure: folk at leisure (DO_LEISURE and its
pastime) for a part of their time about (a watch-keeper's two hours after its free watch's meal, a long day's last four
hours); each kind's pastimes (kinds.txt's: reading, dice and other games of chance, a board game, talking and stories,
singing, drinking, mending its gear, whittling, by what fits each people), chosen by the denizen's number and the day's,
not the dice; the social ones (dice to drink) its lair's together, in the room they gather in (a hall, a guardroom, a
barracks, quarters), a reader or one mending alone at its bed; told as seen ("Three soldiers are ahead, 20 feet away, at
dice.") and heard (dice rattling, laughter, singing, cups clinking); less watchful at it (the book's half Awareness for
one not paying attention: its sight and hearing halved, quartered at its cups), in drink a while after (its blows two
steps worse, struck at two easier); its pastime let go as it rises to act, told ("The soldier lets the dice fall.").

**Design:** DENIZENS.md's daily life; HOUSE.md's leisure.

### The time of day believed

The user (2026-10-31): "Timekeeping should also be a fuzzy thing, unless there is some place where you can actually see
outside through a window, skylight, vertical shaft, or the like.  Unless a person has absolute timesense, then
especially after sleeping the current time-of-day should be quite arbitrary to the player, and should drift further and
further away from known as the play progresses."  Then: "Time-of-day would be corrected whenever the sun can be seen."
Then: "...and the direction is known, or the player spends some time watching it."

Built the same day (branch reborn-game-timesense), for review in HOUSE.md's the time of day believed: the character's
time a belief, as its north is (the hub's tb: its error from the true clock and how sure it is), drifting a little each
hour awake (the referee's test, Awareness, its drift the more the longer since it last knew), much after a sleep or a
black-out (its length guessed as it wakes), narrowed by its own light burning down and its hunger since its meal; the
sky in sight (an open way out) setting it: its light alone (dawn, day, dusk, night), the sun or the stars read by the
north it believes in (wrong if its north is, unknown to it), the sun high or low if it knows it's lost its bearings,
and the sun or the stars watched a while showing the time and north both.  Absolute timesense a rare trait (one in a
hundred, hmchar's: the house's).  Told by "time", "what time is it" as believed and as sure as it is; "sleep until
dawn", "wait until morning", "watch the sun" go by it.

**Design:** DUNGEON.md's the time of day believed; HOUSE.md's.

Then (2026-11-03): the check of HOUSE.md found that the user had named windows, skylights and shafts for seeing the time,
but only an open way out showed the sky.  Asked which goes first, the user: "All three together".  **Built:** HOUSE.md's
what shows the sky.

### Poison, bad air and disease

The user (2026-10-31): "What about poisoning, chemical exposure, and disease?"  Then: "Should also include poor air
quality (CO or CO2 excess, low oxygen levels, gas pockets--which could explode!)."

Built the same day (branch reborn-game-afflictions), for review in HOUSE.md's poison, bad air and disease: one course
for every affliction, the book's (Physician 4: a poison an infection with its intervals shortened to the toxin, a
disease a Contagion Index and a healing rate, its recovery rolled daily), the venoms one case of it (their tests as
they were); poisons (a dart trap's, a contact smear's, spoiled food's), diseases (foul water's flux, a giant rat's
rat-bite fever, a ghoul's grave rot: the HarnWorld Bestiary's grave-wight's), caught unknowing and shown after their
incubation; the air in pockets (blackdamp pooled deep, firedamp in the shale, whitedamp an explosion's afterdamp), a
flame's warnings, firedamp bursting at a naked flame; the denizens keeping out of the bad air they know.

**Design:** HOUSE.md's poison, bad air and disease; DUNGEON.md's bad air; DENIZENS.md; RULESETS.md's RS_TIME.

### Ways off the compass

The user (2026-10-31): "Is tunnelling sideways a thing?"  Then: "What about passeges, tunnells, rooms, etc. that are not
set on the 4 main compass coordinates?  Tunnells and natural caverns especially, or grids that are not aligned to the
compass are exmaples."  Offered the lead's three steps (diagonal passages as true ways; a passage told by its real
heading; built complexes at an angle; free angles advised against), the user: "Yes, please queue up 1-3".

Built (branch reborn-game-angles), step 1 first, for review in HOUSE.md's diagonal ways: a diagonal way, a step between
two squares that share only a corner, open where the grid says so (a corner cut, world.h's T_CUT: rock a diagonal way's
corner cuts through, neither corner a square to stand on, never squeezed between two solid corners), walked by every
actor (the character's walks, its ways across a room and its journeys, the denizens' steps, a hunter's scent), seen
along, told among a passage's ways ("ahead to your right"), drawn on the map ("/" and "\"), and dug ("dig northeast":
a square corner to corner, seven feet); the cave grower's zigzag tunnels made the diagonals they are.

**Design:** DUNGEON.md's diagonal ways; HOUSE.md's diagonal ways.

## 2026-11-02

### Moving: walking straight, going to anything seen, inspecting from afar

The user's words (a playtest): "Can you please go [over] how moving around works?  It is still difficult to navigate
without saying `15 feet west`, etc. and I still can't approach things other than doors and window, or say `cross the
room`.  It is frustrating to be on one side of a room facing the far side, and if there is a closed door on that side
(even if I can't see it) it says "the door is closed, you have to open it first" (or something like that) and I do not
move.  I would like `f` to move me forward until I reach a wall, closed door, or other obstacle.  Also, since I can't
move to an object yet, it is hard to inspect things."

Asked whether the compass words should walk the same as the relative ones: "Answering your question, yes, all
directions by themselves should work the same".

More from the same playtest: "if you cannot see something (it is outside your light bubble), you may still be able to
walk to, inspect, etc. even though it is not seen.  Doors you can't see can also be enumerated in the `which door...`
response.  One last thing, if you say `walk to the door on the left` it doesn't always do what you expect."

And: "When you go to something to pick it up, if you have to walk to it, it says "You walk up to <item>", then it gives
a full look, then says "there is a <object> at your feet" and then says "Taken." or the appropriate "You are
overburdened" if you are carrying too much."

And: "Playtesting, I see that a barred door is not shown as such, just closed.  You also can't `unbar the door`."

And: "When moving about in the dark, when you meet a wall, it tells you so, then says something like "It is dark; you
feel your way. You are in a passage. It goes to your right and behind you, rising gently." Since it is dark, when you
meet a wall, you don't really know which way the passage goes.  It might be better to just say "You feel a wall to your
left, and nothing to your right" or some such.  What do you think?"  The lead proposed feeling your way telling only what
the hands and feet could tell (HOUSE.md's walking and searching in the dark); the user: "That sounds great!"

The lead's reading, built: what it sees now, or has seen and remembers, can be named and walked to (a door out of the
light listed in "which door?", the look not telling it); what it has never perceived is neither listed nor found.

**Design:** HOUSE.md's going places (straight walks); DUNGEON.md's going places in a room.

### One path for every actor: the common structure (Phase 3C)

The user (2026-11-02): "Go ahead and start the command interpreter changes now".  On the lead's survey (the
character's line becomes a command, struct cmd, which the UI resolves into the world's intents, struct msg's I_*;
a denizen's mind posts those same intents), the user chose the intent as the common structure: a denizen's mind posts
the character's own intent for the same action (its kind and its roles: the door's square and what's done to it, the
thing's square and its record), and the world carries it out by the same code for every actor.  The verbs and the
words (struct cmd, VB_*: selectors, "it", questions) stay the player's: a mind names things by their squares and
their records, not by words.  How the catalogue's actions become the world's intents (the UI's, main.c's command):

| Actions (VB_*) | Intent (I_*), its roles | The minds post it |
|---|---|---|
| MOVE_DIRECTION, MOVE_TOWARD, APPROACH_OBJECT | I_GO (a heading, the door or mouth meant), I_GOTO (a square), I_TRAVEL (a place named) | I_STEP a square at a time, I_GOTO a dash |
| STEP_DIRECTION | I_STEP (a heading, squares) | I_STEP |
| FACE_DIRECTION | I_TURN | (its face, every intent's x[1]) |
| CLIMB, JUMP | I_CLIMB (up, down; I_JUMP) | I_CLIMB (an ally come along) |
| DIG | I_DIG (a way) | I_KIT's KT_DIG (a tunneller's spell) |
| OPEN, CLOSE, UNLOCK, LOCK, FORCE, BREAK, BAR, RAISE, LOWER, BEND, DISARM, SEARCH, EXAMINE (a portal) | I_PORTAL (PA_*, which, its square) | I_PORTAL (a door: PA_OPEN ... PA_BREAK, a lock picked by a robber's kind; a portcullis: PA_RAISE, PA_LOWER, PA_BEND: 2026-11-02) |
| RETRIEVE_OBJECT (out of reach) | I_TAKE (its square, its record) | I_TAKE (its wants: the world takes the first in its reach); I_ROB's FE_TAKE (an ally's errand) |
| ATTACK_OBJECT, ENGAGE | I_ATTACK (whom, the aim), I_CLOSE (walked up to first) | I_ATTACK |
| THROW_OBJECT, SHOOT_OBJECT | I_ATTACK, AIM_SHOT | I_ATTACK, AIM_SHOT |
| LOAD_OBJECT, GATHER_OBJECT | I_LOAD, I_GATHER | |
| DISENGAGE, FLEE | I_DISENGAGE | (its steps away) |
| WAIT, SLEEP | I_WAIT | I_WAIT |
| STAND, SIT, LIE_DOWN, CROUCH | I_RISE | I_RISE (lain down to sleep, up again: 2026-11-02) |
| LIGHT_OBJECT, EXTINGUISH_OBJECT | I_LIGHT | |
| OBSERVE_DIRECTION, TRACK, SMELL | I_LOOK, I_TRACE | |
| STOP, META_QUIT | I_STOP, I_QUIT | |
| the rest (things handled, words, the meta) | the things' requests (IO_*), the sway (SW_*), the UI's own | I_ROB (a robbery, an errand; FE_DROP, FE_PASS, FE_THROW, 2026-11-04: by the character's let fall, its hands, its throw) |

Built (branch reborn-game-onepath) in the brief's order: first what the denizens lacked, the doors (opened, shut,
a bar lifted and set, unlocked and locked with their keys, forced, battered: the character's parts, wdoor.c and
wblow.c, with the actor's own tests), and the doors' keys for every actor; the minds using them; then taking things by the
character's take (one take rule, the minds' I_TAKE).  Steps, digging and climbing stay two codes, the rules' tables
shared (DUNGEON.md's one path for every actor says why).

**Design:** DUNGEON.md's one path for every actor and its portals; DENIZENS.md's doors; HOUSE.md's denizens and doors.

### Reach told and timed a step at a time

The user (2026-11-02, a playtest): "Also, when you have to take off your backpack to get something from it, it should
read "Taking off backpack", pause, "retrieving <item>", pause, then "putting your backpack back on".  Taking just as
long as it might take when really doing it.  Grope (take off backpack) followed by Grope (get something from backpack)
then grope (put the pack back on), each one taking its time."

The user (2026-11-02): "Dropping your pack in a fight should be automatic (no grope test), but taking it in hand should
be."

The user (2026-11-02, a playtest): "Also, `light a new torch` should drop the old one and get a new one and light it."

The user (2026-11-02): "oh, and lighting a torch is a grope (take off backpack, if necessary), grope (take the tinder box),
grope (to use the tinder box, penalized roll in the dark, continue until successful, reporting as it goes along), then
grope (put away the tinderbox), then grope (put backpack back on).  If the tinderbox is empty, then say so once you get to
that step (or if remembered), and if the tinderbox is in your belt pouch, no first and last grope to take on/off
backpack."

The user (2026-11-02): "Applies to any tool use, like sharpening your sword (if you have a whetstone), and similar tasks."

The user (2026-11-02): "Fumbling about in the dark should be difficult, so re-dipping a torch (`refresh torch` or
`dip/redip torch` should work), but you need to blow out the torch first unless you have another, and then re-dip (in the
dark), then re-light it (in the dark), all appropriately done.  So lighting a new torch, blowing out the old one,
re-dipping, putting it away, etc. should be the preferred way to "redip|refresh" your torch, if it is still lit and you
have a spare."

Built as the general kind: every reach is a sequence of timed steps, told as each begins, the world changed at each's
end, for everything the reach rule covers; "new", "fresh" and "another" a selector (the thing not in a hand, not
burning) for every handling, and a light's own run (the old let fall, a spare got out, lit).  **Design:** HOUSE.md's
reach, handy tools and things used up; DUNGEON.md's reach.  A whetstone built, the kinds made 146, "sharpen" the fill's verb: HOUSE.md's.

### Digs told why (the user's playtest, 2026-11-02)

The user (2026-11-02): "One other thing I notices is that tunnelling at the end of a passage doesn't seem to work if there is a hidden door there that hasn't been discovered.  `dig f` does work some times, but without the right tools, especially trying to do so through stone walls, seems like it needs a "doesn't seem to be working" type of response, or "You need better tools".  Maybe a chisel would work, but without a hammer, seems moot.  Stone chisel versus wood chisel might be a good distinction for future items."

**Design:** HOUSE.md's digging (a chisel and a hammer, told why, what isn't there to dig, a hidden door); DUNGEON.md's still to come (the two chisels).

### The kit: light by money, what starts full, a light from a light, hammer and chisel, turning's words (2026-11-03)

The lead asked: "Should the starting light be a lantern for everyone, or go by family wealth?" (recommending comfortable
families or better a lantern and two flasks of oil, the poorest more torches, a warning at about 10 minutes of light
left). "Must the hammer be in your hand to use a chisel, or is carrying it enough?" "Should `turn r` say "You turn right,
to face west"?" "Should a new torch be lit from the old one before the old one is dropped?" "Can tablets be read by touch
in the dark?"

The user (2026-11-03): "2. Light source by available money, 3. Yes, hammer and chisel in each hand, 4. I guess the turn info is OK keeping both,
5. Of course, light the new torch from the old one before it is extinguished.  6. I wouldn't worry about reading tablets
in the dark, especially if you are illiterate (as most fantasy world folk would be)."

And: "If you have a waterskin, it should be full to start."

Earlier (2026-11-02): "Wandering about in the dark is really difficult.  That is good, but it makes torches or lanterns
very important.  Maybe starting with a lantern and extra flasks of oil for it would be better?"; "`light a new torch`
should drop the old one and get a new one and light it."; "So lighting a new torch, blowing out the old one, re-dipping,
putting it away, etc. should be the preferred way to "redip|refresh" your torch, if it is still lit and you have a spare."

And (2026-11-03): "When running status, I would like to see my hunger and thirst state as well."

And (2026-11-03): "Another thing, I seem to acquire fatigue if overburdened even when sleeping.  Probably not a good thing."

And, on standing under a load, asked by the lead (2026-11-03): "Yes, if you are awake and standing with the burden, you should still get fatigued."

**Found:** the outfit's waterskin was already full (four pints, stoppered, so not listed); "fill the waterskin" said "You
fill your waterskin." all the same, and nothing told how full it was: so it seemed empty.  And no code charged fatigue by a
load at all, asleep or awake: what rose in a long sleep was hunger's and thirst's fatigue levels (very hungry or very
thirsty one, starving or parched two, wasting or dying four, not rested off), under a Condition line that named the load and
not the want.  **Design:** HOUSE.md's a load borne, a new light, the kit's light, a chisel and a hammer; DUNGEON.md's status.

### Looking in the dark; a thing taken from a fixture (2026-11-03)

The user (2026-11-03): "When in the dark, if I say `look around` it should really fell around, looking for things in the environment (on the floor, the
walls) to see if there is anything nearby to grab/use."  And: "`feel around` already works, so that is nice".

The lead asked: "In the dark, what should `look around` and plain `look` do?"  The user chose: the same as feel around; in the light they stay as they
are; check it finds things on walls as well as the floor, and that you can take what it finds next.  Blind counts as dark.

The lead asked: "Should "take the torch from the sconce" with a hand free go in the hand?"  The user chose: to a free hand; the pack only if both hands are
full.  Any thing taken from a fixture alike.

**Design:** HOUSE.md's looking in the dark, taken from a fixture.  What the look told at once it tells still (the walls felt, a light seen far off), then the feel around: for review.
### The round after HOUSE.md's check: the user's answers on rules (2026-11-03)

The lead asked, after the four readers' check of HOUSE.md against the game: "The status line shows true minutes gone ("767 min"), and every game starts at 18:00, so you can work out the real hour even when the character shouldn't know it. What should it show?"

The user (2026-11-03): "Hide the minutes"

The lead: HOUSE.md says denizens move at a walk, but bandits, soldiers, kobolds and ghouls cover ground at twice your walking speed.

The user (2026-11-03): "walk unless hurried, and closing in to attack should be hurried unless they are cautious."

On a miss's stray, the book having both 50% (a mark engaged in melee, Combat 15) and 20% (a nearby target, Combat 17), the user (2026-11-03): follow the book: 50% if the mark is engaged, 20% otherwise.

The lead: "`dig down with the knife` while carrying a pick uses the pick."

The user (2026-11-03): "Suggest the better tool if one is available, but allow them to continue with the less effective tool."

On a thing's value or make asked again at once, a new die each time (a chest's 25, 85, 45, 11 pennies in one minute), the user (2026-11-03): keep the first answer per thing, until the skill improves.

The lead: "HOUSE.md says your footprints vary by footwear, but only the words change, not the prints' tread. What should happen?"

The user (2026-11-03): "Build it (Recommended)": "Prints really differ by footwear. It's a small change."

The lead: "Burrowers' claws dig shale, against the rule "only a pick will work for harder materials". They also dig a bare wall in soft rock when at work but not when chasing prey. What should the rule be?"

The user (2026-11-03): "Soft rock only": "Shale is too hard for claws; the same soft-rock rule at work and in a chase."

The lead: "HarnMaster has a rule for aimed blows, high or low, but no command says it, so it can't happen. What should be done?"

The user (2026-11-03): "Leave unbuilt for now (Recommended)"

The lead: "Which of these odd rules about water and money should I change? Any you leave unticked stay as they are, marked for review."

The user (2026-11-03): "Still water, Stream current, Farthing goods, Haggling pump"

The lead: "Which of these odd rules about dungeons should I change? Any you leave unticked stay as they are, marked for review."

The user (2026-11-03): "Naming the rock, Dust and flee"

(Left as they are, marked for review: a builder walling up a room the character dug into; carrying on renewing fuel and oil.)

The lead: "HOUSE.md credits you with "each step about a named one", on dipping two torches, but it's not in your recorded words. It looks like my wording. Did you say it?"

The user (2026-11-03): "Not mine, correct it (Recommended)"

**Built:** the status line without the minutes (dq0 -m, the tests' and debugging's, puts them back).  A kind's step its walk (kinds.txt's numbers doubled: they were the jog's), hurried its jog (fleeing; coming at its foe, but a cautious one), every actor's by wbody.c.  A stray's chance by the book.  The tool named is the one that digs (the referee's P_DIG, both), a better told of once.  An appraisal's die a hash of the thing, the skill's mastery and the seed, no store; a door's and a wall's rock too.  The character's prints by its footwear.  Claws in rubble, earth and soil, no rock, one rule at work and at prey.  Filling from still water: the flux's risk as drunk there.  One chunk of a stream in eight six feet deep, by the seed.  A price under a penny in farthings; buying never under two thirds of a thing's worth, selling never over.  The first good spell names the rock.  A place of no age no dust, a century or two a skin.  "flee" with no foe it sees: "You see nothing to flee from.", no run, a foe near or not.  **Design:** HOUSE.md's their paces, a miss's stray, a tool named, appraising a thing, what the floor shows, a burrower's claws, the diseases, the water in the dungeon, the market, told as it's come to, the time of day believed.

**Doubt, for the user:** with shale out, no rock in the game is soft enough for claws, and the maker's walls are stone or masonry: burrowers now dig rubble and nothing else.  A softer rock (chalk, marl, clay) would give "soft rock" something to mean.

The user (2026-11-03), asked whether to add a softer rock: "There should be softer materials, so digging would make sense, but claws shouldn't work trying to dig in shale, at least not without some damage.  I guess it depends on the animal.  Obviously it would be spelled out in the monster's sheet if they could tunnel through rock, so I think we are fine without generally allowing claws to dig rock.  Clay or softer would not be very stable without shoring anyway, so existing tunnels, except in a shored mine shaft, would not have lasted anyway."

**So:** no softer rock added for now: claws dig rubble, earth and soil, and no rock; a kind that tunnels through rock has it said on its sheet.  Damage to claws tried on shale is not built.

## 2026-11-03

### HOUSE.md's review: the user's answers

The lead read HOUSE.md against this file and the game, and put what it found to the user: 27 questions, most important first, numbered here A1 to A27 as HOUSE.md calls them, and 13 bundles to accept.  A question with the options offered named is one the user answered in words of its own; the rest are the option chosen, its label and its words as offered.

**A1, Tongues.**  The lead: "A character speaks its own tongue and common, and the character maker gives no second one.  Bandits, cultists and soldiers speak common; kobolds, dwarves, elves and all orcs have tongues of their own.  So a human can never persuade, threaten, deceive or haggle with a dwarf, an elf, an orc or a kobold, and can only reach them by gifts and bare buying.  Should that change?"  (Offered: "Leaders speak common (Recommended)", "Characters learn tongues", "Both of those", "Leave it as it is".)

The user (2026-11-03): "What do the HM rules say about languages spoken/written?"  Told the book's rule, the user chose "Book, plus leaders": occupations the book gives a 2nd Language get one, its mastery capping a skill used through it, as the book says; elders, kings, captains and wardens also know common, so each people has someone to talk to (that part the house's).  Widens the character sheet.

**A2, Who trades.**  The lead: "There's no merchant kind yet, so any thinking, non-hostile person will buy your things and sell you what it carries that it isn't using: a soldier or a bandit is a walking shop.  Should that stand?"

The user (2026-11-03): "Friendly folk only": "Only people friendly to you will trade; indifferent ones refuse, so trading has to be earned first."

**A3, Skill growth.**  The lead: "A skill you use is marked, and after eight hours' rest each marked skill gets one improvement roll, told as you wake.  However often you used it, it's one roll; nothing improves until you rest.  Is that how skills should grow?"  (Offered: "After a night's rest (Recommended)", "At once, as used", "Extra roll, hard use".)

The user (2026-11-03): "Improvement rolls should only happen after a minumum number of uses once the skill is opened, not after each use.  I would say 5 uses = 1 improvement roll.  I don't know what thre HM rules say, so might be better to use their numbers."  Told the book's rule (a roll for a use in a dangerous or special situation, none for routine use; more for a very dangerous one; about 3 practice rolls a month), the user chose "Stress, or 5 routine uses": a stressful use earns a roll; 5 routine uses earn one too.

**A4, Healing rest.**  The lead: "Wounds heal by a roll every five days, and each roll is lost unless the character rested 60 hours of those five days.  It starts with three days' rations.  So a serious wound means staying put for days with food to find; a night's sleep heals almost nothing.  Keep that?"

The user (2026-11-03): "Keep it (Recommended)": "It's the book's pace and your "healing over days of rest"; wounds matter, and first aid and care with fights matter more."

**A5, Autosave.**  The lead: "The game saves as you enter a new level and as you quit.  If you die, you go back to that save, which may be hours of play earlier.  You also asked for autosaves "before danger"; those aren't built.  A save stops the game for its time on the card, and a real-time fight would begin with that pause.  What should happen?"

The user (2026-11-03): "Stay as it is (Recommended)": "Level entry and quit only until a playtest shows the lost play hurts; no pause as a fight starts."

**A6, Asleep.**  The lead: "A sleeping character notices things exactly as one awake: a sound or a denizen in sight wakes it as it would stop a wait.  Denizens asleep hear at half and don't see, as the book has it.  Should your character sleep as lightly as it does now?"

The user (2026-11-03): "Half awareness asleep (Recommended)": "The book's rule, already built for denizens; where you choose to sleep starts to matter."

**A7, Shield skill.**  The lead: "Blocking with a shield improves the skill of the weapon in your other hand, not your Shield skill, so Shield never gets better by using it.  Change that?"

The user (2026-11-03): "Block improves Shield (Recommended)": "The skill used is the skill marked, as everywhere else; a small change."

**A8, Carried gear.**  The lead: "When a character who got out carries on into a new dungeon, its wounds and skills come along as they are, but everything it carries is made new: a battered sword comes back whole.  (Fuel and oil being renewed you've already left as it is; this is about wear and damage.)  Keep that?"  (Offered: "Made new (Recommended)", "Wear and damage stay".)

The user (2026-11-03): "Until there is some inter-dungeon "town time", there should probably be a random amount of time between dungeons, and healing should probably happen accordingly (auto-rolled by the game), and weapon and gear maintenance as well, with the appropriate cost (including food, housing, etc.),and skill improvement should be handled as per the book if > 30 days passes."

**A9, Early foes.**  The lead: "Six creatures still have the game's own stand-in numbers rather than the Bestiary's: the giant rat, cave spider, ghoul, skeleton, cave bear and kobold.  They're the first foes most games meet, so they set how fights feel.  The rest already follow the Bestiary, as you asked.  Move these six too?"

The user (2026-11-03): "Book's figures now (Recommended)": "Your rule for every denizen, and fights feel right from the first rat; many fight tests must be re-baselined, which is real work."

**A10, Worn blades.**  The lead: "A blade's wear only changes the words ("its edge dull"): a dull sword hits as hard as a sharp one, a crude one as hard as a fine one, and sharpening with a whetstone changes nothing in a fight.  (Checked in the code.)  The book's rule that a weapon striking something hard may break isn't built either.  What should wear do?"

The user (2026-11-03): "Both of those": "Dull edges cut less and weapons can break; the most work and the most to go wrong in a fight."

**A11, Your menace.**  The lead: "A creature's courage weighs how fearsome its foe is, but the character's figure is a fixed placeholder, so a knight in mail frightens a rat no more than a beggar does; beasts in particular are never put off by you.  Should the character's real might count?"

The user (2026-11-03): "Reckon it properly (Recommended)": "By its size and its weapon skill, as the creatures' is; beasts break off from a strong fighter sooner; a small change."

**A12, Hiding.**  The lead: ""hide" only hides a thing.  You can't hide yourself, though you named the skill for both ("hide in shadows anyone?"), and it was put off until denizens could look for you, which they now can.  Should it be built?"

The user (2026-11-03): "Queue it next (Recommended)": "It's the thief's other half beside moving quietly; denizens' senses need a new check, so it's moderate work."

**A13, Foes & traps.**  The lead: "Denizens never set off a trap, on a door or underfoot, so you can't lure a foe over one, and a rat runs safely across a pit trap you know is there.  Each faction knowing its own traps is planned, not built.  Should that wait?"

The user (2026-11-03): "Leave for now (Recommended)": "It waits for that plan, and for rules for a denizen hurt outside a fight, which don't exist yet."

**A14, Morale tests.**  The lead: "A creature tests its courage only when it's wounded, or hears a friend or leader fall.  The book tests before every action.  So a band never quails just from the look of you, however badly outmatched.  Keep that?"

The user (2026-11-03): "Also on first facing you": "One more test as it first engages you, so a weak band can break before a blow; a little more time per fight."

**A15, Looting mail.**  The lead: "Taking armour off a dead body takes no longer than picking the same piece up off the floor, so a dead bandit's mail comes off in a moment.  The plan had it at twice the time to take off your own.  Which?"

The user (2026-11-03): "Twice taking-off time (Recommended)": "Stripping a body takes real time, so it's a risk with others near; the plan's way."

**A16, Rope.**  The lead: "Rope and grapples are in the game and climbers start with them, but nothing uses them: "tie the rope" does nothing, and a shaft mouth more than 9 feet up simply can't be reached.  What should happen?"

The user (2026-11-03): "Queue rope rules (Recommended)": "Tying off, climbing aided, a grapple thrown up; players will try it at once.  New commands and rules, so real work."

**A17, Found keys.**  The lead: "Keys found lying in the dungeon open nothing; only keys carried by a door's or a chest's owner fit.  A player who finds a key will carry it round trying doors for nothing.  What should happen?"

The user (2026-11-03): "Fit something nearby (Recommended)": "A found key opens a door or chest in its neighbourhood, so finding one means something; the dungeon's growing must tie them, moderate work."

**A18, Sky shaft.**  The lead: "Some shafts go up to the open sky.  You can climb one, but until the dungeon's mission is done an iron grating bars the top.  So you can see daylight and not leave, and the puzzle can't be skipped.  Keep that?"  (Offered: "Keep the grating (Recommended)", "Climbing out wins", "Out, but no win".)

The user (2026-11-03): "It is not about winning or losing.  It is about solving a puzzle.  That means climbing out and not solving the puzzle is not a loss, just a disappointment.  We will need to build the rest of the game that takes you to the dungeons in the first place, which are usually to find some bit of knowledge, or some item, or to kill some local scourge, of just because someone mentioned that they found the dungeon and were afraid to enter.  There could be lots of reasons, but that part of the game is not built, and we need to plan for that.  The inter-dungeon time will make it feel more realistic and should tie the whole character arc together."

**A19, Firedamp.**  The lead: "Marsh gas gathers in shale.  A weak pocket shows a blue cap on your flame as a warning, but a strong one explodes as you walk in with a torch or candle: a burn of 3 or 4 dice, your lights out, no warning first.  Is that fair?"

The user (2026-11-03): "Warn before bursting (Recommended)": "The flame shows a warning a step before the burst, so a careful player can back out; still deadly if ignored, a small change."

**A20, Reading.**  The lead: "Anyone can read anything if there's light; no skill is tested.  (Checked in the code.)  You've remarked that most folk here would be illiterate.  Should reading need the Script skill?"  (Offered: "Leave it open (Recommended)", "Reading needs Script".)

The user (2026-11-03): "Eventually, part of the game should be to collect clues and take them back to a library or to a scholar to determine the next thing to do.  Each clue might be in a unique script that needs a specialist to help decipher, or time in the library if the character is literate.  The HM rules describe languages and scripts known, so that makes scribes and scholars viable character classes.  Put a plan together for this, but for now, leave it open."

**A21, Relic's end.**  The lead: "In a dungeon whose goal is to end a curse, laying the relic on the altar ends the game there and then, though the cursed dead are still walking about.  Keep that?"

The user (2026-11-03): "Keep for now (Recommended)": "The dead's response to it hasn't been built, and the ending's words carry it."

**A22, North adrift.**  The lead: "When the character's sense of north is an eighth of a turn off, typing "north" in a passage often answers "You can't go that way", since the way it believes is north is really a diagonal.  "Forward" and "left" still work.  So the compass misleads rather than quietly going the nearest way.  Keep that?"

The user (2026-11-03): "Keep it (Recommended)": "Your "things will get interesting" when players trust their compass; the words a player uses as it faces never fail."

**A23, Far journeys.**  The lead: ""Go back to the Mausoleum" only works on the level you're on; from another it says "The Mausoleum isn't on this level."  Should journeys reach other levels?"

The user (2026-11-03): "Same level, for now (Recommended)": "You walk to the stairs yourself and then travel; a journey between levels raises hard questions about what's remembered."

**A24, Tinderbox.**  The lead: "Striking a light with a tinderbox asks for no free hand: with sword and shield both held, you can still light a torch.  Dipping a torch, and working a chisel, do ask for hands.  Should striking a light?"

The user (2026-11-03): "Needs a free hand (Recommended)": "The same as your hammer and chisel, and lighting up in a fight costs something; a small change."

**A25, Chisel.**  The lead: "Other tools get themselves out of your pack by the usual steps as you use them, as you asked for any tool.  A hammer and chisel don't: you're told to hold them yourself first ("hold the chisel", "hold the hammer").  Should they work like the rest?"  (Offered: "Get them out (Recommended)", "You hold them".)

The user (2026-11-03): "If your hands are busy and your want to ready tools or other things, it should probably put the other things away first (in their appropriate place) then take the tools in hand.  A torch would be placed in a nearby empty sconce or laid on the ground, a sword put in its scabbard, etc."

**A26, Tool stopped.**  The lead: "Stop halfway through using a tool you took from your pack (a blow, or "stop"), and the tool is back in the pack at once, though you never put it away, and your pack may be lying at your feet.  Should it stay in your hand?"

The user (2026-11-03): "Stays in hand (Recommended)": "The world stays true, as you asked of stopped steps; it needs a little work to find its way back later."

**A27, Start hour.**  The lead: "Every game starts at six in the evening, so a player soon knows that night folk are waking and day folk going to bed as each game begins.  Should the hour vary?"

The user (2026-11-03): "Vary by game": "Each game's seed picks an hour, so the same trick doesn't work twice; a small change, and some tests need the hour fixed."

**The bundles.**  The lead: "Each line: accept these as they stand until a playtest says otherwise."  Everything in the questions above is carved out of them; rules marked the user's or the book's are in them only for their numbers.

- **B1.  Tests, noticing, searching, traps; the second ruleset; test flags.**  Which skill each purpose tests, senses as numbers, how pace affects noticing, what's hidden how well, a thing hidden's grades, disarming's steps, a whole room's search and its times, a heap's hidden things; the null ruleset's numbers; the tests' -N, -J, -F switches.
- **B2.  Moving, tiring, climbing, positions.**  Paces and the fatigue table, inclines told, a load's tiring, a sleeper lying down, encumbrance's words, climbing rates, falls with no armour counted, senselessness, a pit; crouching, kneeling, sitting and lying.
- **B3.  Digging and the rock.**  Tool rates and wear beyond your own, chisel and new tool prices, words for why a dig fails, a dig finding a hidden door, a named tool's refusals, rock beds and hardness, rotten rock's fall (1d6), age's collapses and rubble, mixed strata, diagonal digging.
- **B4.  Doors, windows and looking.**  Lock, force and bar steps, portcullis work, battering's beats, time and words for looking, examining, each aspect asked by name, what opening reveals.
- **B5.  Fights and missiles.**  Improvised weapon classes (club, stick, soft), who defends how, what's outnumbered, armour quality, courage's table, health words, the wound record's folding; throw ranges and reaches; launchers' times and ranges underground, the cover, strays' one die, arrow breakage, denizens' shooting and throwing, falls told, auto-target; weapon versions' Strength limits; a pass tiring.
- **B6.  Wounds, poison, disease, air, hunger.**  First aid's house rules, self-treatment, what you may do in shock, creatures' healing and binding, venom's spread; poisons, diseases and their rates, a dart trap in four poisoned, the air's strengths and times, a flame's warnings, where bad air lies, antidotes; swimming and instant drowning, hunger, thirst and meal numbers.
- **B7.  Reach, tools and lights.**  Every second of the pack steps, a run of handlings, dipping two torches, a new light's order and its 2026-11-03 cases, the whetstone's numbers, the tinderbox's thirty lightings, a dig tool's choice among tools, things used up, engaged steps a round each, a wound not stopping them.
- **B8.  Outfits, money and the market.**  The kit's light thresholds, what starts full, what's bought in what order, makes, weapons by skill, armour by bent, tools, an outfit that fits; money's prices, jewels' values, mints and mixes, what coins are worth; a gift's worth and test, reputation; prices, haggling, remedies' prices, entertaining.
- **B9.  Denizens' things and peoples.**  Where each kind lives and how many, gear and blows, unready weapons, a shield's block, beds, stores, dens' remains, hoards, purses, what a look, examine and assess tell of a denizen; names, sheets, senses, attitudes, tongues, homes and gear of the peoples.
- **B10.  Denizens' minds and day.**  Morale's influence, field, bonds and succession numbers; allies' errands; denizens' tiring, paces, water and hunger; hours, watches, sleep, meals, work, rounds, leisure and drink; tunnellers and builders; working doors and portcullises, keys, what they pick up, ways found, bodies found.
- **B11.  Senses and belief.**  Sound's reach and the background (hearing stays yours to tune by ear), noise drawing foes, moving quietly's grades, the draught; when bearings are tested and how they drift, memory's fading, drawing a map, what it costs; a journey's choices and its forty legs; waiting's words and speed; the time of day's drift, clues and words.
- **B12.  The dungeon's shape.**  How often and where each thing is grown: history's leavings and ruins, water, windows and skylights, shafts, daylight's reach, diagonal ways, a passage's slant and its words, diagonal passages and rooms, great caverns, long slopes.
- **B13.  Your playtest requests as built, and commands.**  Straight walks, going to anything, leaving a room, inspecting over a distance, what can be named, "the door on the left", walking over for a command, a door's state, feeling your way, looking in the dark: all your own words, marked only "did it come out as you meant?"; a hostile unaware not engaging; "take all"'s eight, which of several, shortcuts' limits; first-use skills told, the ending's words.

The user (2026-11-03): all thirteen accepted, "until a playtest says otherwise".

**So:** settled as they stood: A4, A5, A13, A21, A22, A23.  To build: A1, A2, A3, A6, A7, A9, A10, A11, A12, A14, A15, A16, A17, A19, A24, A25, A26, A27.  To plan: the game between dungeons, with what takes a character to a dungeon in the first place (A8, A18), and clues, scripts and scholars (A20); the rules stand till then, and reading stays open.  **Design:** HOUSE.md's guide at its head (what's accepted, what's carved out, what a *for review* mark means from now on), each rule named there, and its Decided, not yet built.

### A word as they begin (2026-11-04)

`open the door` (4.8M cycles to the first letter) and `listen` (6.2M) worked out the outcome before they said anything.  The choice put: a word as they begin, or the outcome told first.  The user: "Word as they begin."

Built for the kind, every command over the 720,000: words that hold whatever comes of it (HOUSE.md's a command's first words).  And an answer's first words go out as they're said, however short: many were made in time and held for the rest of the world's news.

**Design:** HOUSE.md's a command's first words.

## The game between dungeons, the plan answered (2026-11-04)

(Steps 4 to 6 built: HOUSE.md's between dungeons, for review.)

The plan (F:/.claude/hydra-2/plans/between-dungeons.md, sections 1-7) put ten questions; the user chose among the options as put:

- Q1 Can every dungeon be left? "Every dungeon": out by a sky shaft if it has one and by the way in, unless the reason says it's shut; a shut dungeon always has another way out to find.
- Q2 Does solving end the dungeon at once? "No, walk out": told it's done, then walk out with what was found; dying on the way out still counts as solved in the life's account.
- Q3 Can a character go back to a dungeon it left? "Not yet, keep it": not in the first version; each dungeon's folder kept so it can be added.
- Q4 Who chooses the month's practice skills? "Filled-in question": one question with the answer filled in (most used, then occupation's): Enter takes it, or name your three.
- Q5 Can a character die off-stage, of an infection? "As the roll falls": the book's roll stands; a character can die in the account. (NOT the recommended long fever.)
- Q6 When the money runs out? "No debts": sell spare goods, live rough, work at half wages (days added), go on with what it has.
- Q7 In the first version, should selling, mending and restocking just happen? "Just happen": in that order, valuables last, what you named never, until the town's menu gives the choice.
- Q8 The town? "A menu"; a walked town left for Dingus Quest itself.
- Q9 Should an illiterate really be unable to read in the dungeon? "Yes, no puzzle needs it": from step 9; a writing is a hint; the theme checker proves each puzzle solvable unread; carry or copy.
- Q10 What's in 1.0, what goes first? "1-6 first, 7-12 in 1.0": steps 1-6 before Phase 5; 7-12 in 1.0 with a one-link clue; the long thread in 1.0 only if there's time, else first in Dingus Quest.

**So:** the steps 1-6 first, before Phase 5: leaving unsolved, solving without ending, the life record, time and healing, monthly practice, food and upkeep. A18's grating goes with step 1. Reading stays open until step 9.

## The swapped parts, the study answered (2026-11-04)

The study (F:/.claude/hydra-2/swapstudy/report.md) measured the copies at 12-15% of all cycles in play and 16% of a first letter. The user chose among the options as put:

- A second swap area in the world, about 1,700 bytes? "No, keep the room".
- Go ahead with: "Fix the fight soak", "Smaller stubs", "Two small speedups", "Move rare code out".
- A first word on plain walks and steps? "No word, leave them".

**So:** no true cache and no second area; 6-byte stubs, the record walk, SWFAST in dq0d, rare code out of hot parts; each landing apart from game work.

## The last questions of the round (2026-11-04)

Put to the user after the life soak's faults, with their answers:

- Should long world jobs hold back once typing starts? "Hold back".
- One G5 cut killed 6 of 20 hunters by infection: should that stand? "Try in play first".
- A death in the account between dungeons, final in both modes? "Final in both".
- Working toward a repair of something out of reach, skip it? "Skip it".
- A first-dungeon death before any save says "Your last save is kept" and nothing is: save on arriving, or change the words? "Change the words".

**So:** the world's long jobs wait from the first key typed until the first word is out, measured so the world isn't starved; infection left as it is until it's been played; no time goes on repairs out of reach; the words at a death tell the truth about the save, and no save is made on arriving.

## After the long delays (2026-11-06)

Put to the user with the lead's recommendations; the user: "Yes, use your recommendations on the two outstanding questions".

- Robbers hiding 3 squares from a resting character re-hide for hours rather than come and rob it: should they come and rob? Yes.
- Should every look make only the chunks a line from the eyes reaches (fewer chunks, but lairs placed later, 18 tests to re-baseline)? No, not for now; again only if the pauses of about 2 s a chunk bother the user in play.

**So:** a denizen that wants something of a character at rest comes for it, as a robber comes to rob a sleeper; looks stay as built at 2dc5439d.

## Climbing down a well (2026-11-06)

The user, in play: "What about climbing down a well?"  A well was only its curb (drink, fill, a rope tied, "The well's shaft drops away into the dark."), nothing below.  Asked what a well's shaft should be, the user chose "A real shaft": a narrow shaft down to water, climbed with a rope tied to the curb by the rope and climbing rules, the bottom in the water, some wells with a way on (a cistern, a passage to a lower level).

## Designators for things and denizens (2026-11-06)

The user: "I am wondering if it might be easier to work with things if they had some sort of designator, like nethack uses.  Please research how nethack manages inventory and items in the environment so we can make interacting with them easier, and produce a plan to do something like that to make life interacting with objects easier."  And: "We could possibly do something similar for denizens we can see (list them, with designators, for when we want to interact with them)."  The plan: the Claude Doc "dq0: letters for things and denizens" (https://claude.ai/artifact/92kFwatKMsoNbizMWE86oR).

Having read it: "I think capital letters for in-the-environment things (items and denizens, unique, up to two characters) with denizens numbered first (designators only for seen items or creatures), and lower-case letters for items carried/worn (one or two characters).  When shown, the designators should be in parens or brackets, like `(A) a gargun`.  Multiple items should be takable from the environment with a prefix (`take 3A`) with the appropriate warning if less than three are in the stack."

Then, asked: capitals shown "Look and a list" (in the look's sentences and a list of all in sight); the inventory "A list, one a line", replacing the sentence; a thing seen again "Keep it a while" (its capital kept while it's remembered).

**So:** six steps (the doc's), after the torch fixes merge, since both change how a thing is chosen.

### Stowing asks where (2026-11-06)

The user: "To add to the designator work, when stowing something, it should ask where if there is more than one place it could go (pouch(es) or backpack), of course depending on size.  Stowing a weapon with a sheath should choose the sheath if worn."

Made general in the designators plan (step 7): a holder made for the thing (sheath or scabbard for its blade, quiver for arrows) is chosen first when worn and free; otherwise, if more than one place could hold it by size, the question asks with designators.

## The torch playtest: which thing, the first words (2026-11-06)

The user's playtest, verbatim:

> When I say `drop torch` it says `You reach for the torch.` followed by `Put down.`  I think it was because there was a lit torch on the ground.  Also, I had a lit torch, and I said `drop the spent torch` and it dropped the lit one.  When I said `take spent torch from backpack` it first said `You reach for the torch.` followed by the reach stream.  One other issue was when I was in the dark with the spent torch in my hand I said `light a new torch`; the response was the reach you would expect, but the `You put the torch in your backpack` was concerning because I thought it was putting the newly lit torch in there, but no, it was the spent torch.  One more thing: if I say `put <obj> in backpack` when <obj> is in hand, the response leads with `You reach for the <obj>.`; same if <obj> is not in hand and is not around anywhere.  One last thing: when you dip a torch, one line of the reach is `You set to filling the torch.` which should be `You set to dipping the torch.` or the like.

> If there are many items of the same type around, it can be hard to get the one you want.  If there are three torches, one in hand, one in my backpack on my back, and one on the ground, if I say `pick up the torch` it should take the one I don't already have (from the ground).  But if all three are on the ground, I should get a prompt for which one, with longer descriptions of each, something like `Which one?  1. a torch (lit), 2. a torch (spent), or 3. a torch (almost spent).`

> One other strange thing: I was trying to light a torch that was in one hand, with the tinderbox that was in the other hand, it finished the reach sequence with `You put away the tinderbox.` even though the tinderbox remains in hand.

> If I have no torches in my backpack, but there are some on the floor and I say `take torch`, it first takes off my backpack, then takes a torch from the ground, then puts the backpack back on.  The torch ends up in the backpack.  It doesn't just take it in hand.

> I can't specify `take torch from the floor` or `take torch from the ground`.  It says `You see no [floor|ground] here`.

**So:** the first word follows what will happen (HOUSE.md's a command's first words); a thing is named by what tells it apart; "spent" and "full" choose by state; dipping is its own word; a tool in hand before stays in hand; a plain take goes to a free hand; the floor and the ground are places to take from and put on.

### A line typed during a reach (2026-11-06)

Asked whether a line typed while a reach's steps are still going (the pack going back on, without a question) should run between the reach's steps or wait for them all, the user: "I think that `stop` and other similar commands should be acted on immediately, but others could be held until after the reach."

**So:** the commands that halt or break off (stop, and its kind: halt, enough, flee and the like) act at once, between the reach's steps; every other line waits until the reach is done, as now.

## Commands that name a thing always answer about that thing (2026-11-06, as built, for review)

The user asked for a fresh look at the command processor (COMMANDS-CRITIQUE.md and ENGINE-CRITIQUE.md, beside this file), then: "Go ahead and start work on these changes."  The first step is the critique's 4.6 and its migration step 4.

- **A sweep of every verb at every kind of thing** (tests.js's dq0sweep): 17 verbs at the kitchen's pot, well, sconce, stairs and wall and the carried backpack, torch and sack, 134 lines, one rule: the answer to a line that names a thing names it too.  Before: 80 of the 134 broke it.  "sit on the pot" sat down; "climb the pot" climbed the stairs; "light the stairs" lit a new torch; "take the stairs" asked "Take what?"; "kick the pot" said "There's nothing like that here to fight."; "touch the pot" said "You can't touch in this game yet."
- **The rule, as built (refuse.c, overlay 1):** a thing named that its verb would pass over, or do to something else, is refused, said back from the line as typed: "You can't sit on the pot in this game yet.", "You can't climb the pot.", "You can't take the stairs.", "You can't kick the pot.", "There's nothing written on the stairs."  A verb the game doesn't do yet names its thing too ("You can't touch the pot in this game yet."); one it doesn't know at all still names only itself ("I don't know how to bribe.").  Each verb's rule is a row of a table: what it refuses for a thing named by its word, and the parts of the place it does (climb: stairs, a ladder, a shaft, a window, a ledge; a well and a rope by their words).
- **After:** 4 of the 134 left, listed in the test (a look at the stairs the character stands by, a sniff toward them, "light the torch" with the torch in hand lit).
- **Found, not mended (the designators' work is in the same chooser):** "take the backpack" while it's worn, and "take the sack" while it's in hand, say "You see none of those here."  The take looks only at what lies about; a thing carried should be answered as one ("You wear the backpack already.", "You hold the sack already.": the things' SA_WORN, SA_HELD).
- **Overlay 1 has 1 byte free after this** (the old o_unknown moved into refuse.c with it).  Every overlay of the UI's but OV22 (the letters', 912) has under 500 bytes free: ENGINE-CRITIQUE.md's first point.

## The build places the swapped parts (2026-11-07, as built, for review)

The user: "Go ahead with the auto-packer" (ENGINE-CRITIQUE.md's Â§1.2, after the parts' stubs and config were made by the build).

- **The parts' banks are the build's.** parts.txt's bank is a part's wish.  The build reads every segment's size from the objects before the link (od65).  A bank whose wishers don't fit moves out the fewest bytes (the smallest part that makes room), each to the bank of the same file with the least room that holds it: the world's dq0w.rop, read only as ropes are first wanted, keeps its own.  A growing overlay pushes part images elsewhere instead of failing the build.
- **Calls are routed by part** (pack.js's route, every build): s_NAME called from outside its part becomes NAME, its stub; NAME called from inside its part becomes s_NAME, straight.  So a function moved is its pragma alone, whatever its callers write.  Two of the UI's stubs went (called only from inside their part).
- **A part past the swap area has pieces moved out by the build** (pack.js's plan).  A piece is a stubbed function and what only it reaches (its static helpers, its tables).  It moves the piece with the fewest calls to the rest of its part, to the part with the most calls to it whose bank has room, in the same file.  It won't move a piece whose calls across the new boundary would carry a pointer (in the prototype), or that stores an address in a global the other side reads, or that assembly touches, or a move that takes a function's estimated stub calls in calls past SWDEPTH.  The moves are listed in the room table.  Today nothing is past its area, so nothing moves.
- **Limits, for review:** the swap-depth estimate is static and high (the world's main loop serves requests as it waits, so its estimate is 29 against SWDEPTH 12); where it's already past, it can't judge, and the tests are the guard.  A part that's one big function (the world's part 6, s_blows, 3,630 bytes) can't be split.  The world is bound by its banks, not its parts: only one bank has more than 1,100 bytes free.
- **Tested:** with dq0d's parts 2 and 9 and seven of the world's parts given less room (DQ0_PACK_SQUEEZE=TASK:PART=BYTES,...), ten functions moved, and all 412 tests passed.  A part forced into a full bank (dq0d's 13) went to another and linked.  banksChecked and planChecked hold the rules each build.

### Taking a thing you carry, lighting the torch you hold lit (2026-11-07, as built, for review)

The two wrong answers left from the sweep (dq0sweep), mended now that the designators' work in the take is in.

- **"take the backpack" while it's worn, "take the sack" while it's in hand** said "You see none of those here."  Now, when nothing of that name lies about but it's worn or in hand, the take gives the wear's or the hold's words: "You wear the backpack already.", "You hold the sack in your right hand already."  (The world's take, things.c's serve.)  One that's only in the pack is still "none here": taking it out is a hold, and that's the player's to say.
- **"light the torch" with the torch in hand already lit** reached into the shut backpack for a spare.  HOUSE.md's rule (2026-11-04) is "the one in a hand first, whichever way it's lit, none put away for another", so now: "The torch is lit already."  A torch unlit in the other hand is still lit first, and "light a new torch" still gets a spare out.  The UI's first words follow the same rule (sway.c's chooser): "You raise the lit torch." instead of "You reach for your backpack."
- **Room:** the UI's overlay 20 has 53 bytes left after this; the catalogue bank's code (where the world's light is) had too little room for the check, so it's in part 92 instead.

### The stairs looked at, examined, sniffed toward (2026-11-07, as built, for review: COMMANDS-CRITIQUE.md's step 6, its first slice)

The sweep's last three gaps, and the first of the features as things (the stairs, a ladder, a shaft, the way out: the senses' openings).

- **"look at the stairs", "examine the stairs"** turned the character and told the whole room again.  Now the one meant (the chooser's, which asked of several) is told in the room look's own words for it: "You stand by the stairs up to the way out."; the way out's with the theme's words for its door, shut or open ("The way out is shut fast. Whatever opens it is somewhere below. A door of black stone closes the stair, a round hollow in its face.").  One only remembered, out of sight, still gets the look about.  (way.c's opened; words.c's one-opening mode.)
- **"smell the stairs"** said only the air's words.  Now it names what it sniffs toward first: "You sniff toward the stairway.", then the draught and the smell that way as before.  (Its turn to face them comes after those words, as the turn is carried out later; it reads well enough.)
- **The sweep has no gaps left**: every one of its 134 lines names its thing, and the take, light, look, examine and sniff answers are held word for word.
- **The UI's banks: a last resort, measured and kept out of use.**  At one point these few hundred bytes left no UI bank with room for part 17, even with one part moved out for it, so the bank packer has a last resort: every part of that file placed again, the biggest first, each in the bank with the least room that holds it.  It worked, but the packed overlays came out 3.5K bigger (the parts scattered pack worse), and the start went 1.7M cycles over its budget (dq0starttime: 226.7M of 225.0M).  So it stays a last resort, and part 17 was trimmed instead (8 bytes): nothing moves today.  The UI's banks have about 4K free between them, in pieces; the next sizeable UI feature needs a bank more (as overlay 23 was for the designators) or space won back (the refusal table, features as data).

### The resolver's choice kept (2026-11-07, as built, for review: COMMANDS-CRITIQUE.md's Â§5.1, step A)

A survey found six choosers deciding what a name means, and twelve ways they disagree (COMMANDS-CRITIQUE.md's Â§5.1).  The first: the UI's resolver chose one of several things lying about ("the closest", "the largest", an answer to its "Which do you mean?"), then sent the world only the name, and the world chose again by its own rules.  "take the closest torch", with a lit one at your feet and an unlit one a step off, asked "Which one? (E) a torch (lit), (F) a torch".

- **Now** the resolver's choice of several is sent as that thing's id (letters.c's des_seen adds it to the line's handles, as a letter's is), and the world takes that one: "take the closest torch" takes the lit one at once.  Test dq0closest (it fails on the old code with the "Which one?").
- **One candidate alone** is still left to the world, whose rule puts what's in hand first: "light the torch" with one lying about and one lit in hand is still "The torch is lit already." (the sweep caught this when every choice was sent).
- **Next** (Â§5.1's order of work): "it" and "take all" carrying ids; then one chooser for things in the UI, over what's carried and what's seen together, which will need a staging overlay bank.

### The carried thing chosen once (2026-11-07, as built, for review: COMMANDS-CRITIQUE.md's Â§5.1, step C's first half)

The user approved a staging overlay bank (overlay 24) and step C.  What's built:

- **The thing carried that the first words choose is the one the world is asked of.**  The UI already chooses a carried thing to say its first words at once ("You reach for your backpack."), by a copy of the world's ranking; then it sent the world the name, and the world chose again by its own.  Now that choice goes as the thing's id, as a letter's does, for put down, hold, stow, put in or on, wear, take off, light and put out.  The two can't name different things any more.
- **Left to the world:** a state said ("the spent torch": the world knows the state better; the UI's flags call a lit torch with no fuel left not spent, the world's spent); another thing named ("put out the torch in the sconce", "put the sack in the sack": the world's rule that a thing isn't put in itself goes by the names); a lit thing to light ("The torch is lit already.", the world's words).  The suite caught all three.
- **Not done, and why:** a chooser of its own in overlay 24 copied what's carried a second time and made the first words 30,000 cycles slower, past the user's 720,000 for a first letter (dq0torchwords); so the choice is made final in sway.c, where the first words already make it.  Things lying about stay the world's to choose: the UI's sight record holds only six of them, and the world also chooses among things remembered in the dark.
- **Overlay 24** is in the game, empty of code for now; the bank packer moved part 6's image into it when overlay 20 grew by this change.  It's for step D (one "which?" question).

### One "Which one?" for things and denizens (2026-11-07, as built, for review: COMMANDS-CRITIQUE.md's Â§5.1, step D)

The user: "Do D then finish C, and if you can do them in parallel, do that."  (Step C's second half is being done alongside, separately.)

- **The resolver's question is now asked the designators' way.**  Before: "Which do you mean: 1. the giant rat ahead to your left, on the right, 2. the giant rat ahead to your left, on the left, or 3. the giant rat ahead?"  Now: "Which one? (A) the giant rat ahead to your left, on the right, (C) the giant rat ahead to your left, on the left, (B) the giant rat ahead".  The capitals are the things' and denizens' own (a thing keeps its capital, so they aren't always in list order); the world's "Which one?" (a take of several that differ) was already worded so.
- **One answer reader for both** (letters.c's which_in): a capital, the choice's number, a way ("left", "ahead", "north"), "closest" or "nearest", a word of one choice's alone ("bones", "spent"), each with "the" before it or not.  The answer remakes the command on the chosen one's capital ("take" then "the bones" is "take E"), so it goes through the same path as a letter typed; the resolver's own answer reader and its kept copy of the command are gone.
- **Not yet:** the doors' and openings' question (choose.c's "Which door: 1. ..."), answered by a number or a way; doors have no capitals until features are lettered (step E).  And "Drop what? [a, d, f]" keeps its own form.
- **Room:** the new code is in overlay 24 (which.c); resolve.c's overlay 11 went from 7 bytes free to 1,097.

### A state said, the carried thing chosen once too (2026-11-07, as built, for review: COMMANDS-CRITIQUE.md's Â§5.1, step C)

The carried thing the first words choose is now the world's with a state said too ("drop the spent torch", "drop the full pot", "hold the open sack"), as the step before left out.  What's built:

- **The UI's flags are the world's chooser's.**  The world makes what the UI knows of each thing carried (s_ix_make); it now makes each state flag by the tests its own chooser uses (pick_ and unlike), so the UI's tests (sway.c's ix_pick) take what the world would.  Where they differed: "open" (the world needs a kind that closes: a basket isn't open, it doesn't close); "full" (only a container, a surface or a fuel is full: potsherds aren't); "spent" (an empty container is spent too, and a light burnt out though it's still marked lit); "shut" (closed, whatever the kind).  The kind that closes is a bit in the kinds' flags the UI copies (in K_BELT's place, which the UI never reads); "full or empty" is the bit that said "a container" (CX_HOLDS, read by nothing).  The words that tell things apart ("You put down the spent torch.") read the same flags and say what they did.
- **Not to light or put out.**  The world's lighting doesn't choose by its chooser but by the name ("light a new torch" lets the old one in hand fall by its name); given the id, it put the spent torch in the pack instead.  That's what failed dq0torchwords when this was first tried, not the flags.  So a light lit or put out with a state said is still the world's to choose.
- **Test dq0statesel:** potsherds before a pot of pitch in the pack, "drop the full pot" puts down the pitch (with the old flags the UI chose the sherds and the world, given them, said "You carry none of those."); "drop the open basket" puts nothing down; "drop the spent torch" is the one burnt out in hand.
- **Room:** the UI's overlay 20 has 2,876 bytes free (73 used by this), the world's part 102 931 (60 used).  The start 225.8M cycles at seed 1 (its budget 226M).

### The doors' "Which door?" the same question (2026-11-07, as built, for review: COMMANDS-CRITIQUE.md's Â§5.1, step E)

The user: "Finish up 5.1, please, on parallel threads."  (Steps B and F are being done alongside, separately.)

- **The question about doors and openings is now asked and answered the same way as the things'.**  Before: "Which door: 1. the door ahead to your left (closed), 2. the one to your right (open), or 3. ...?", answered only by a number or a way, and only when the answer read as a move.  Now: "Which door? (1) the door ahead to your left (closed), (2) the one to your right (open), (3) ...", answered by the same reader as the things' question: a number, a way ("left", "north"), "closest", or a word of one choice's alone ("open", "double", "barred").  The answer makes the command again as it was typed, its door chosen.
- **Not lettered.**  Giving doors and openings capitals would change every room's words ("a door (G) ahead"); the designators' design gave capitals to things and denizens only.  So doors are numbered in the question, and nothing else about them changed.  If you want them lettered too, that's a small step from here.
- **Room:** choose.c's own answer and kept command are gone (overlay 7: 40 bytes free to 255); the new code is in overlay 24.

### One name match (2026-11-07, as built, for review: COMMANDS-CRITIQUE.md's Â§5.1, step F)

Six rules decided whether a word typed names a thing: the world's chooser's (a word of the name or its start, a plural, the end of the name: "pack" a backpack), the world's other copy without the end, the resolver's (whole words, a plural, and a qualifier checked), the first words' (a copy of the chooser's, and `strstr` for things seen), and the blows' `strstr`.  "pack" found the backpack in the world and nothing in the resolver.  What's built:

- **One rule, the world's chooser's,** in the UI as `name_is` (nameis.s, the main RAM's, so every overlay and part reaches it): a word of the name or its start ("key" an iron key, "bon" the bones), that word made plural by s or es ("sacks", "torches"), or the end of the name for a word longer than 3 ("pack", "skin").  With the qualifier check only the resolver had (help.c's, gone): the word before it in the line, if it's one that makes one kind another ("black", "giant", "iron", or its start), must be in the name too, by the same rule.
- **Used by** the resolver's gathering (things and denizens seen, the denizens remembered), the first words (the kinds carried, `ix_pick`; the things seen, and "You look them over."), the blow's denizen (blows.c), the tending's patient (tend.c) and a walk's new things (task.c's new_words).  **The world:** its `named` took the end-of-name rule and the chooser's copy (`named4`) went into it, so the chooser, the light, the market and the search ask one rule.
- **Left with their own, on purpose:** `o_may_den` (three letters anywhere in an actor's name: looser, as it only says "maybe, ask the denizens"); letters.c's `which_in` (a word of a "which?" choice, not a name); main.c's `bow_named` (a launcher's kind by `strstr`: "bow" names a shortbow there, which the rule's end of a name, 4 letters at least, wouldn't).
- **Looser in the resolver now, for review:** a word's start names a thing, so "go to the rat" with rations lying in sight would ask which of the two (the world's take already took rations for "rat").
- **Test dq0namematch:** the backpack put down, "target the pack" and "target the bon" find it and the bones (before: "You see no pack here.", "You see no bon here."); "target the black rat" finds none of the giant rats.  No other test's expectations changed.
- **Room:** in assembly, as the C was 550 bytes and left the UI's main RAM under its 2,048: the main RAM 2,560 bytes free to 2,194; overlays 1 from 1 to 443, 11 from 1,097 to 1,507, 20 from 2,876 to 3,197.  The world's main RAM 2,894 to 2,791, its part 4 134 to 447.  The start 225.3M cycles at seed 1 (its budget 226M); all 421 tests pass.

### "It" and "take all" by the thing, not its word (2026-11-07, as built, for review: COMMANDS-CRITIQUE.md's Â§5.1, step B)

Two more places where the UI knew which thing was meant and sent the world only its name, for the world to choose again (the survey's disagreements 8 and 12).

- **"It" is the thing last named, by its id where that's known.**  Before, the resolver kept the last thing named as its word alone, so "take it" after "look at the closest torch" (a lit one at the feet, an unlit one a step off) asked "Which one? (E) a torch (lit), (F) a torch".  Now its id is kept with it (it_id, in the main RAM) when a handle had it: a letter's or a capital's, the resolver's choice of several, or the carried thing the first words chose (sway.c's bind).  "It", and a verb's unsaid thing that's the last one named ("drop", "take" after "examine the book"), make that id the line's handle again, as a letter's is, so the world is asked of that thing ("#" and its id).
- **Only while it's still carried or in sight.**  The id is checked against the letters of what's carried and the things in sight; a thing gone (eaten, burnt, or joined to a pile of its kind as it was taken up, its record gone with it) or out of sight is named by its word, the world's to choose, as before.  Without that, "take E" of a ration lying about and then "eat it" would have found none.
- **"Take all" takes each thing it listed, by its id.**  It lists the things it sees lying about, nearest first, then took each by its name, and the world took the nearest of that name it knew: with a torch hidden at the character's feet and a lit one a step ahead, "take all" took the hidden one and left the lit one lying.  Now each one's id is read from its store record as it's listed; the first is the line's handle, and each next one's is set as it's handed on to be taken (taken_next: the line's other handles let go then, a "take all" line has none of its own).
- **Test dq0it:** both, in one game at seed 3; it fails on the old code with the hidden torch taken, and "Which one?" for "take it".
- **Room:** the handles' adder (letters.c's des_add) is in overlay 24 (542 bytes of it: 1,736 left), as overlay 22 has 570 and the code reads the store's records and the letters itself; resolve.c's overlay 11 used 171 (926 left), sway.c's overlay 20 27 (2,849 left); the main RAM 13 bytes (2,547 left).  The start 225.5M cycles at seed 1 (its budget 226M); all 421 tests pass.

### Â§5.1's last disagreements, and the start's budget (2026-11-07, as built, for review)

The user: "Don't worry too much about the startup time.  If you need to rewrite some of the portions in assembly, do so.  Do whatever cleanup you can, and then start the next piece of the critiques work."

- **The start's budget** (dq0starttime) is now 235M cycles at seed 1 and 315M at the big seed (it was 226M and 305M; the start is 225.8M and some 302M now), so the work to come isn't held to the last megacycle.  The measure is kept: it still fails a start that grows a lot.
- **The disagreements the AâF steps didn't close**, closed: the first words of a thing named by a capital or another handle never borrow a carried thing of its name (7); "attack the rat with the other sword" takes the other sword in hand, the instrument's own selector, not the rat's (9); "attack the rat" with several rats in sight goes for the nearest, the resolver's choice, its square the fight's to go by, not the first in the actors' table (11); "You look them over." is said only when two of those seen differ, as the world then asks which; two alike, the one taken is reached for (5, test dq0leftovers).  "The second" settles as one order for each kind: carried things the UI's, things lying about the world's, denizens the resolver's (2).
- **Clean-up:** the three parallel agents' worktrees and branches removed (one folder under .claude/worktrees still held open by a process, left to go); stray single-byte section signs written into four sources made UTF-8.

### The second sweep: every command that names a thing answers about it (2026-11-07, as built, for review: COMMANDS-CRITIQUE.md's Â§5.2, step G)

The next piece of the critiques' work, after Â§5.1: the dispatch (Â§5.2).  Its first step is a second sweep of the verbs the first one didn't cover, the same rule (an answer names the thing named), and the gaps it found closed.

- **Test dq0sweep2:** eat, drink, fill, look in, tie, untie, wear, push, pull, unlock, lock, raise, lower, disarm, force, bend and bar, each at the pot, the well, the sconce, the stairs, the wall, the backpack worn, the torch in hand and the sack carried: 136 lines, 55 of them broke the rule, none now.
- **What changed:** "eat the pot" said "You're not hungry."; now "You can't eat the iron pot." (a thing named must be food, its kind's key, or water, a waterskin, the well; one not to hand, "You have no apples."); not hungry with food named, "You're not hungry. You leave the rations be."  "raise the pot", "bend the pot", "bar the pot", "disarm the pot" went looking for a door or a trap; now "You can't raise the pot." (a door's or a window's own part by its word still taken: "lift the bar", "bend the bars").  Eat, drink, fill, tie, untie, wear and look in at the stairs or a wall: "You can't fill the stairs." (look in still works at a door, a window, a passage, a shaft).
- **For review, odd but named:** "tie the torch" says "You can't close the torch."; "wear the torch" says "You hold the lit torch in your left hand already."
- **"take all" holds its next thing while a line is being typed.**  The test of the user's rule for a line typed during a reach (dq0reachstop) caught "stop" typed during "take all" acted on only after the next thing was reached for and taken: the next was started as soon as the UI was free, whatever was being typed, so the order turned on timing.  Now the next waits while a line is half typed; a stop, once entered, ends the take as the rule says; any other line is held till the take's end, as before.

### Refusals as data (2026-11-07, as built, for review: COMMANDS-CRITIQUE.md's Â§5.2, step K)

The same refusal was worded, or built, in many places: "You see no %s here." in eight, "out of reach" four ways, a dozen "<verb> what?".  What's built:

- **One table, one routine.**  refuse.c's `said[]` is the table of refusals, its first rows the line's (U_VERB, U_YET, U_CANT, U_NOTE, and now U_WHAT), then the A_* (dq0.h): "You see no %s here.", "You have no %s." (each with a nameless form: "You see none of those here.", "You carry none of those.", said when there's no name to put in, none or a handle's "#"), "There's nothing here to take.", "You haven't seen a(n) %s here." and "... that way.", "%s is out of reach: go up to it first." ("the" put in before a name that isn't capitalised, the sentence's start capitalised), "You can't get near enough to %s.".  Said by `say_no (a, w)` (w the thing's name, 0 "it"; copied first, so it may be lb, any of the main RAM's, not another overlay's own), or `unknown (U_*)`, from the line, as before; `A_LB` adds it to lb, unsaid, for the sway's words and a blow's note, which are built there.  Each site calls it in place of its own `sayf`, nothing round it moved (thirty-five sites, fourteen files).
- **refuse.c is overlay 24's now** (overlay 1 had 286 bytes; the table and say_no wouldn't fit beside the title and the help).  One more stub in the main RAM (11 bytes).
- **Every wording that changed** (old, then new; where):
  - "You see none of those here." â "You see no pumpkin here." (the thing named): take (main.c's s_took_up), hide behind a thing (s_hide), lift a bar with no door by it (s_lift), search for a thing (search.c).
  - "You carry none of those." â "You have no pumpkin.": drop (main.c), hide a thing (s_hide), throw (resolve.c).  "You have no such thing." (give, with nothing named: errand.c) â "You carry none of those.".
  - "The giant rat is too far off: you'd need to be within reach." (give, a beast's ways: sway.c; a trade: errand.c), "It's out of reach: go up to it first." (a blow's note: blows.c), "You'd have to be beside the rat." (first aid: tend.c) â "The giant rat is out of reach: go up to it first.", "It is out of reach: go up to it first.", "The rat is out of reach: go up to it first.".  ("... is too far off to hear you." kept: talking range isn't reach.)
  - "You can't get any nearer to it." (a walk to a thing that stopped short: main.c's reach_go) â "You can't get near enough to it." (the fight's close-in's words, task.c's).
  - "Take what?", "Put down what?" (main.c), "Read what?", "Copy what?" (read.c), "Name what?" (names.c) â the line's verb and "what?", as "Drop what? [a, b ...]" was already: "Take what?", "Get what?", "Put down what?", "Drop what?", "Read what?", "Copy what?", "Name what?", "Call what?" (in play the take's and the drop's are seldom reached: "take" alone asks which, "drop" alone lists the letters).
  - "There's nothing written on it." (read, a part of the place named with no word: read.c) â "There's nothing written on the way out." (U_NOTE, the line after its verb, as refuse.c's rows say it).
  - "You can't eat the pot." (feed.c, from its word) â the same from the line (U_CANT): "eat pot" says "You can't eat pot.".
  - Not changed in wording, only where it's kept: "You see no %s here." (resolve.c twice, words.c twice, way.c, tend.c, errand.c twice, sway.c), "You have no %s." (errand.c, feed.c twice), "There's nothing here to take." (resolve.c twice), "You haven't seen a(n) %s here/that way." (way.c, choose.c).
- **Left as they are, on purpose:** the questions with a hint or another shape ("Throw it at what? (throw the dagger at the rat ...)", "Give it to whom?", "Fetch what?" (an order: its line isn't "fetch"), "Name it what?", "Use what, and on what? ...", "Combine what with what? ..."); the "Which one?" headings (caps.c, which.c: a question's start, its choices after it); the per-action templates of Â§4.5 (refuse.c's rules[] rows are what there is of them).
- **Tests:** expectations changed only where a wording did: dq0hide ("You see no ... here", either form), dq0firstword ("take the pumpkin": "You see no pumpkin here."), dq0moving ("can't get near enough"), dq0parse (the gift out of reach), the repeats allowed in dq0engage (both forms).  No behaviour changed, so no new test.
- **Room:** the main RAM 2,150 bytes free to 2,139 (the stub); the banks, overlays and parts together, 8,619 free to 8,723 (the build repacked the parts among the banks, so single overlays moved a lot: refuse.c's 700-odd bytes out of overlay 1, the four strings out of tell.c's overlay 14).
- **All 424 tests pass,** dq0sweep and dq0sweep2 among them.

### One path for things, one walk over (2026-11-07, as built, for review: COMMANDS-CRITIQUE.md's Â§5.2, steps H and I)

A survey found some fifteen handlers each doing part of what `handle()` does for a thing (ask the world, walk over if it's out of reach, ask which if several differ, word it, charge the time), and the walk over then the command again written four times.

- **One path:** the asking, the walk over and the "which?" are now one routine: main.c's `things` with `IO_GO`. Out of reach: walked over to first, and the command carried out again. Several that differ: which, asked. Either way it answers `IR_GONE`, and the verb has nothing more to do. Every verb that acts on a thing goes through it: handle's (hold, stow, put, take from, wear, take off, light, put out), take, drop, hide, search for or on a thing, look in, fill, tie, open, close and lock a thing, look at, appraise, read, climb down a well, and a blow's weapon taken in hand. Each keeps its own words and special cases (take's portal part, drop's counts and "Put down.", hide's cover). The time stays each verb's own. `after()`'s light check matters only to the handling verbs, and sharing it would swap part 2 in for every look.
- **One walk over:** `reach_go`, by `walk_a` (I_TAKE's a). 1: the command kept and carried out again (every verb, and the door's I_GOTO). 0: a take's, the world taking the thing up as it gets there. 2: a "go to", walked on as it gets nearer, nothing after. Take's own walk and way.c's own I_TAKE are gone. **Kept apart:** a denizen's approach (I_CLOSE: up to it as it moves, a fight's) and a place in the room (`place_go`: not a thing, its own words, nothing after).
- **Visible changes:**
  - **"hide the bones" with the bones lying beside the character** said "You carry none of those." (the hide looked only at what's carried). Now they're taken up first, then hidden where the character stands, as HOUSE.md's hiding a thing puts it down at its end: "You look for the bones. Taken. You set to hiding the bones. ... You hide it well enough."
  - **"hide the dagger" with the dagger lying out of reach** said the same. Now: "You look for the dagger. You go over to the dagger. Taken. You set to hiding the dagger. ...".
  - **For review, untested:** a blow's or a draw's weapon lying out of reach ("attack the rat with the sword", the sword across the room; "draw the sword") is walked over to first, then the command again. Before, the world's words for the hold were said and nothing was done.
  - Nothing else visible: the world answers "which?" only for a take and a stow, and those already asked it. The sweeps (dq0sweep, dq0sweep2) pass unchanged.
- **For review, the hide's words:** "You look for the bones." comes first (the hide's first words, said before the world answers that nothing of that name is carried), then "Taken.".
- **Test dq0onepath:** both hides, at seed 3. On the old code "hide the bones" said "You carry none of those.".
- **Room:** the main RAM 2,150 bytes free to 2,144 (`things` rewritten to reach the request by its fixed address, which paid for `IO_GO`). Part 2 683 to 777, part 13 2,274 to 2,234, part 17 1,915 to 1,926, part 18 1,748 to 1,648, part 10 1,397 to 1,405, part 25 510 to 513; overlay 16 49 to 61. (The build placed part images differently, so some overlays' numbers moved: 5 255 to 215, 14 23 to 128, 18 102 to 10, 19 17 to 10.)

### One path for features (2026-11-07, as built, for review: COMMANDS-CRITIQUE.md's Â§5.2, step J)

A door, a window, a portcullis, a passage, the stairs, a ladder, a shaft or the way out is acted on in the same steps: found, walked up to if it's out of reach, faced, the first words said, then the act.  There was one full copy of those steps (the door verbs') and seven partial ones, with two ways of turning (an intent, and way.c's own bare message to the world) and two tests of what's in reach.  What's built:

- **One routine, main.c's `feature`** (part 13, where `door_go` was): it finds the feature (choose.c's `pick`, which asks of several), walks up to it if it's out of reach and has the command carried out again on arrival (I_GOTO), takes a blow's weapon in hand, faces it (the intent, I_TURN), and says the first words.  The caller then does its own last step.
- **One reach test, set by what the last step needs.**  The door work's (the default): one beside it by the four ways, or on the walls of the room it stands in (the world's door work walks up to that one itself).  A close look's (FG_NEAR): beside it, by the eight squares around (the UI reads the record itself, so there's no walk in the world's work to rely on).  Never (FG_FAR): a look one way, or a move that has its own walk.
- **Its users:** the door verbs and everything that reaches them (a window climbed through, check for traps, search, attack, climb); going to a door, or through one or a passage; leave; examine (way.c's `inspect`); look, listen and sniff at a feature or a way (`s_look_way`); and lifting a door's bar or a window's pane (`s_lift`, its door found by the part's square, then faced through the routine).  way.c's `turn_to` and `inspect`'s own reach test are gone.  The errand's door (errand.c) and the blow's check for a door (choose.c's `s_wield`) still only find: the character doesn't act on those.  A look at the stairs (way.c's `opened`) only finds too, as before.
- **What a player sees change:**
  - *Examine, assess or judge a door, window or portcullis faces it first*, as look at and check for traps did.  Before, it turned only to one behind it.  Now "examine the door to my right" says "You turn to face east. ..." (the turn's look) after the door's description, because the description is the UI's own and comes back at once.
  - *Exit or leave by a door that's out of reach walks up to it first*, as "go through the door" does.  At seed 3 in the passage off the cell, "exit the door", 1, said "You can't go that way."; now it says "You head for the door." and then the door's own answer ("The door is shut: open it first.").
  - *Examine walks up with I_GOTO, not I_TAKE's walk*, the same as the door verbs' walk: the same words ("You head for the door."); it stops before the door.
  - *Lifting a bar or a pane, turning to a way, a listen or a sniff:* the same turn as before, now sent as the intent through `intend2`.  The only difference: the character is busy while it turns, as with every other intent.
  - Every other answer is unchanged, word for word (the door tests, both sweeps, the whole suite).
- **Test dq0features:** both changes above.  It fails on the old code ("examine the door to my right: not turned to face the door"; "exit the door (1): not walked up to it: You can't go that way.").
- **Room:** see COMMANDS-CRITIQUE.md's row J and the commit.  Part 2 683 to 824 bytes free, part 17 1,915 to 2,182, part 24 3,161 to 3,240, part 13 2,274 to 1,997 (the routine): 210 bytes won back in all.  The main RAM is unchanged at 2,150 (one stub replaced by another).
- **A test's premise changed by step J** (found merging, 2026-11-07): dq0choose's second game examined a door (answered "right"), then "go through the door" and expected "Which door?".  Examining a door now faces it, so the door is ahead and the go goes through it without asking, as it should.  The game now turns away ("face south") before the go, so it still tests the question.  (The turn after "right" goes northwest, the door's own bearing, where the question said "to your right", the look's stored way: the two can differ, as the chooser survey noted.)

### The dispatch table, stage 1 (2026-11-08, as built, for review: COMMANDS-CRITIQUE.md's Â§5.2, step L)

Which function carried out a verb depended on where there had been room: `s_command`'s switch (part 2), then its default's `chores` (part 2), then `more` (overlay 16), then `rarer` (part 24), each tried in turn, and "You can't ... in this game yet." if none took it.  Stage 1 puts a table in front of them and calls each verb's handler by its row, the handlers' work unchanged.  What's built:

- **A row a verb:** actions.c's `rows[]`, in overlay 11 (beside the resolver), in actions.h's order, four bytes each: its handler (dq0.h's `H_*`), the kinds of thing it takes (`TK_*`: so far only `TK_CLOSER`, a thing that closes, worked first, for open, close, unlock, lock and force), and two bytes kept for stage 2 (its refusals, refuse.c's rules[]) and stage 3 (its request, where `cmd.aux` holds one).  Written `ROW (VB_*, H_*, TK_*)`; the build holds the rows to actions.h (games/dq0/make.js), so a verb added without its row stops the build.
- **One dispatcher:** the end of `s_command`, after the resolver, the refusals and the puzzle: the row read (`verb_row`, an overlay stub), then a switch on the handler's number.  Part 2's handlers are called straight; overlay 16's through `run16 (h)` and part 24's through `run24 (h)`, each that bank's one entry, which switches on the number.  Not a table of addresses: that would need a stub a handler in the main RAM (6 or 11 bytes each, ~35 handlers), and the main RAM has ~75 bytes to spare.
- **The handlers:** `s_command`'s and `chores`' cases, made functions where they're more than a call (`do_go`, `do_door`, `do_examine`, `do_fight` ...; `do_portal` the door's work several end in, once the `portal:` label); `more`'s cases, now by number (`o_run16`); `rarer`'s (`s_run24`).  `chores` is gone; `more` and `rarer` are renamed `run16` and `run24` and dispatch nothing: each is called only for its own handlers' numbers.  The hide still calls overlay 16's work by number (`H_TIE`, `H_HIDDEN`), the door verbs `H_SHUT`, the climb part 24's `H_CLIMB`.
- **Dropped, unreachable:** chores' `VB_OBSERVE_OBJECT` (s_command's examine took it first); `more`'s open, close, unlock, force, lock and hide, and `rarer`'s climb, as verbs (s_command took each first: their work stays, called by number).
- **Verbs with no handler:** 50 rows say `H_NOT_YET`: the 42 the survey found, and 8 seen to before the table (step, set pace and pace, pace.c's; the target's three, resolve.c's; use, read, combine, the puzzle's).  Their answer is the same "not yet" as before.
- **Nothing a player sees changes** (the whole suite).  The cycles: the row costs an overlay call (~100 cycles) and the switch fewer compares than the old chain (which went through `chores`, an overlay call to `more` and a part swap to `rarer` before "not yet").  A verb of part 24's (a dig, a sway) no longer calls overlay 16 first.
- **Room:** the main RAM 2,133 bytes free to 2,122 (`verb_row`'s stub).  Part 2 905 to 1,059, part 24 3,238 to 3,342, overlay 16 146 to 184, overlay 11 1,367 to 738 (the table, 568 bytes).
- **For review:** the two bank entries (`run16`, `run24`) are where stage 3 will meet the question of a handler moved between banks: its `case` moves to the new bank's entry.  And whether the table should hold stub addresses after all once stage 3 has the handlers in few banks.

### The dispatch table, stage 2: the refusals in the rows (2026-11-08, as built: COMMANDS-CRITIQUE.md's Â§5.2, step L)

refuse.c's list of refusals by verb is now part of the dispatch table: each verb's row says what it refuses for a thing named by a word and for a feature of the place, and which features the verb does.  One table says what a verb does, what it takes and what it refuses.  Nothing a player sees changed (the whole suite passes).

### The dispatch table, stage 3: the handlers by meaning (2026-11-08, as built: COMMANDS-CRITIQUE.md's Â§5.2, step L)

The command handlers were where the old chain cut by size had put them: part 2 (main.c's `do_*` and one-line cases), overlay 16 (words.c's `run16`), part 24 (main.c's `run24`).  Stage 3 puts them in files by what they're about, each group's place its pragma's.  Nothing a player sees changed (the whole suite: all 426 tests pass).  What's built:

- **Five files, one a group** (cmds.h lists them): `hthings.c`, the things' verbs (take, drop, the handling verbs, hide, eat and drink) and their timed works (fill, look in, tie, a thing that closes, a hide begun); `hplaces.c`, the features' and the doors' (a door's work, climb, examine, search, a look one way, leave); `hmove.c`, moving (go, face, jump); `hdens.c`, the denizens' (a blow, the missiles, disengage, the sway verbs, give and trade); `hself.c`, the body's and the meta commands (the limbs, a feel about, a look, wait, stop, tend, dig, inventory, sheet and map, time, pause and save, help, quit).  dq0.h's `H_*` are in that order, each group's numbers together.
- **Where:** all in part 2, the commands' own (a handler there costs no copy of a part before its answer), but the timed works, in overlay 1 (overlay 16's till now, 131 bytes from full) through one stub, `things_work`, which the hide (part 18) calls too.  A group moved is its pragmas alone: the build routes a part's entry straight from inside its part and through its stub from outside.
- **The dispatcher:** the row read, then the group's entry by the handler's number (five compares), each entry a switch on it.  No table of stubs: the part-2 entries are called straight, and the overlay's is one stub.  `run16` and `run24` are gone, and with them part 24's copy for a dig, a jump, a climb, a sway, a gift or a trade.
- **The request byte:** the door verbs' rows carry their `PA_*`, put in `cmd.aux` by `verb_row` when the line gave none: lock's (the parser's 0), so the door's work and `H_SHUT` no longer test the verb for it.  Not filled where it doesn't pay: the handling verbs (wear and take off are two requests each), the sway verbs, search and examine.
- **Found, for review:** an empty name (`""`) an overlay gives `othings` is read through the window after `othings` has put the hub there: it was overlay 16's address, which falls past the hub's header, in the room bank 0 keeps free, where nothing is kept.  The moved timed works give `nil`, an empty name in the main RAM, so where they land doesn't matter.  resolve.c (overlay 11) does the same with IO_FIND, left as it is.
- **Room:** the main RAM 2,117 bytes free to 2,128.  Part 2 1,059 to 659, part 24 3,342 to 3,729; overlay 16 131 to 1,073, overlay 1 1,288 to 424, overlay 11 307 to 278.

## One line of sight (2026-11-08, as built, for review: ENGINE-CRITIQUE.md's Â§4 and step 2, its first part)

ENGINE-CRITIQUE.md's Â§4 said there were three lines between two squares that might disagree. They did. What each did
before:

- **The look (dq0p/cast.s's ray, in its grid):** Bresenham's squares from the character's eyes (a step along if
  2e >= -|dy|, across if 2e <= |dx|), the eyes 5 feet above the floor (3 crouched, kneeling or sitting, 1 prone) and the
  line ending at the far square's floor.  A square stops it if it is rock or a wall, a shut door or window (any portal
  not open but a portcullis, cast.s's `rdoors`), or its floor is above the line.  A lowered portcullis is seen through,
  and so is glass.
- **A denizen's sight of the character, its smell past the sound layer, the lights the character sees far off
  (dq0p/eyes.c's `clear_line`):** other squares (from the denizen's end, x and y each truncated, i |dx| / dist), no
  heights at all, and its own rule: a door square stopped it if its portal wasn't open, *a lowered portcullis too*;
  *a window never did*, closed shutters and glass included.
- **A missile's line (dq0w/wshoot.c's `lof`, `passes`):** Bresenham's again but with the ties the other way (2e > -|dy|,
  2e < |dx|), tried from both ends, no heights, a wall's corner let through when the character saw the target, and its
  own portal rule: a portcullis's bars and a grate let a missile through, glass and closed shutters didn't.

So a denizen beyond a lowered portcullis could be seen and shot by the character and not see it, and one could see the
character through closed shutters the character couldn't see through, or over a rise.

**What's built.** One line, dq0w/line.inc (6502 assembly), assembled into the world (dq0w/line.s) and the senses
(dq0p/line.s), with C's view in line.h:

- `line (sn, x, y, tx, ty, mode)`: 0 clear, else what stopped it.  The modes: **LN_SIGHT** (the eye), **LN_FIRE** (a
  missile), **LN_AIR** (a smell; no heights, the air fills the place).  Bresenham's squares as cast.s steps them; the
  line from `ln_eye` feet above the first square's floor to the last square's floor, as cast.s works it out, for the eye
  and a missile; `ln_way`, if set, asked of each square (the world's: someone standing in the way).
- `ln_portal (o, mode)` and `ln_pass (sn, x, y, mode)`: one rule for what a square lets through (0 no, 1 yes, 2
  narrowly: a slit, a window, the world's cover).  No square, rock, a wall: nothing.  A door, a drawbridge, a gate, a
  hatch: open or broken down.  A portcullis or a grate: bars, up or down, let everything through.  An arrow slit:
  everything, narrowly.  A window: its shutters closed, nothing; its glass in, the eye alone; else everything, narrowly.
- **A line between the character and anyone is drawn from the character's end**, from its eyes by its body's position,
  as its look's lines are.  At a tie Bresenham's squares from one end aren't the same as from the other, so this is
  what makes a denizen see the character just when the character would see it (the light aside: each side's own light
  rules are as they were).  A line between two denizens (an ally's throw) is drawn from the shooter.
- **Its users:** the senses' `s_sense_actor` (a denizen's sight, LN_SIGHT; its smell past the sound layer, LN_AIR),
  `glows` (the lights far off, LN_SIGHT with LN_CACHED: a chunk not in the cache stops it, none made); the world's line
  of fire (`lof`, LN_FIRE, someone in the way by `ln_way`, cover by the square beside the target), a missile flying on
  past its mark (`on_through`: `ln_pass`), a smell's way (wtrace.c's `airy`: `ln_pass`, LN_AIR); the sound layer's
  doors and windows (sound.c's `kind_of`: `ln_portal`, LN_AIR); and the look's own doors and windows (cast.s's `rdoors`:
  `ln_portal`, LN_SIGHT).  `clear_line`, `passes`, `lof1` and the copies of the portal rule in sound.c, wtrace.c and
  part 68 are gone.
- **The look keeps its grid** (cast.s's ray: thousands of lines a look, reading the grid with no `at`): the same
  squares, heights and portal rule, worked out there (dq0looks holds the look's every answer to what it was, below).
- **Not yet in the paged ROM** (ENGINE-CRITIQUE.md's Â§1.4 wanted it there): the line reads the chunks through each
  task's own `at` (its cache in the hub's banks, the maker asked for a chunk not made), which a ROM module can't call
  back into.  It's a stage: one source, assembled into the two tasks that draw lines.  It goes to ROM with the chunk
  lookup.

**What the game now means by a clear line, mode by mode:**

| | Sight | Fire | Air |
|---|---|---|---|
| Rock, a wall, no square | stops | stops | stops |
| A door shut (a hidden one too) | stops | stops | stops |
| A door open or broken down | passes | passes | passes |
| A portcullis or a grate, up or down | passes | passes | passes |
| An arrow slit | passes | passes (cover) | passes |
| A window, shutters closed | stops | stops | stops |
| A window with glass | passes | stops | stops |
| A window open, or bars alone | passes | passes (cover) | passes |
| A floor above the line | stops | stops | (no heights) |
| Someone standing in it | (passes) | stops, unless the target's as far beyond again | (passes) |

**What a player sees change:**

- *A denizen sees through a portcullis's bars*, as the character does: one beyond a lowered portcullis that the
  character can see (and shoot) now sees the character too.  (dq0denportc's dwarf, drawn to the lowered portcullis,
  now sees the character through it, heaves it up and comes at it: the game runs on longer, the character knocked
  out, so its cycle budget went from 3,000M to 7,000M; it passes as before otherwise.)
- *Where a denizen stands out of the character's sight, it's out of the denizen's too*, so it behaves as one that
  hasn't seen the character.  (dq0tiring's dig at seed 3: a giant rat in the passage below saw the character before
  through a door the character's own look didn't, and stayed watching; now it doesn't, wanders in and out of view, and
  its stirring cut the first ten-minute spell short.  The test now sends the rats off as each dig begins, dq0Alone's,
  as other scenes do: its subject is the fatigue.)
- *A denizen no longer sees through a window's closed shutters*, nor does a smell pass them or glass.
- *A denizen's eyes and a missile are stopped by a floor above the line* (a rise, a ledge, steps), as the look is;
  and a crouched or prone character's lower eyes see, and are seen, over less.
- *The squares between are the look's*: at a slant a denizen may now not see the character where it did, or the
  reverse, just where the character's own look agrees.  At seed 3 (dq0looks's first game) a giant rat in the passage
  south of the start room, at 32718,32724 with the character at 32723,32722, saw it before through the room's door at
  32722,32723; from the character's end the line goes through the door at 32720,32723, shut, as the character's own look
  does, and it no longer does.  Another (32722,32727, the character at 32724,32722) saw it through the door at
  32723,32723; the character's line cuts the wall's corner at 32724,32723.
- *A missile's line is the character's look's* when the character shoots or is shot at: one way, from its eyes, no
  wall's corner let through (the look already decides what it sees), and a floor above the line stops it.  Glass and
  closed shutters stop a missile as before; bars and slits let it through as before.
- *The lights far off* are seen by the same line: closed shutters hide one, a portcullis doesn't, a rise does.
- Unchanged: the look itself (dq0looks: every reply the senses give the same as before up to the first denizen's
  answer that changed above, in two of its games, both at seed 3, and the rest byte for byte; tests/dq0/looks.ref
  written again for those two); a sound's way and a smell's trail through doors and windows (the same rule, now ln_portal's); a
  missile's flight on past its mark.

**Test dq0line** (tests.js's `dq0LineTest`): seed 1's guardroom (dq0tunnel's scene), the portcullis let down, the
character's lantern lit a step back from the bars, a dwarf miner held four squares off at the far end of the passage
beyond, the character hidden (its hiding poked, so it's silent: a denizen's hearing answers before its eyes are asked).
The character sees the dwarf through the bars; the dwarf's senses must say it would see the character (SN_LOOK).  On
the old code: `its senses [0]` (clear_line stopped at the portcullis): "the dwarf the character sees through the bars
wouldn't see it".

**Performance** (cycles, the same games on the old build and the new, a profile of the senses' task by routine):

| | Old | New |
|---|---|---|
| A look (`view`, seed 3 / seed 7 / seed 3 again) | 355,776 / 295,865 / 378,339 | 355,997 / 296,952 / 379,529 (+0.1-0.4%: `rdoors` calls `ln_portal`) |
| The lights far off (`glows`, a call) | 46,975 / 48,584 / 48,700 | 47,551 / 49,108 / 49,083 (+1%) |
| A denizen's senses (`sense_actor`, a call) | 92,610 / 129,342 | 97,689 / 128,929 |
| One line (`clear_line`, then `line`) | 3,783 / 4,529 | 4,413 / 5,035 (it reads the two ends' floors) |

The start (dq0starttime, its first look and the denizens' first senses in it): seed 1 225.4M cycles to 225.3M,
seed 123456789 302.4M to 302.7M.

**Room:** the world's main RAM 2,791 to 2,536 bytes free (ln_pass, ln_portal and the line's variables); its part 65
178 to 227 (the line itself, in place of lof1, lof and passes), part 68 972 to 1,192, part 29 50 to 281 (their portal
rules gone).  The senses' main RAM 2,634 to 2,435 (the whole line, in place of clear_line); part 1 1 to 4, part 5 4,028
to 4,136.  (Written in C first, cc65 made it 1,185 bytes; in assembly it's 766 in the world, 219 of them in the main
RAM and the line itself in part 65, and 892 in the senses, with the cache's checks.)

## The game's own library in the paged ROM (2026-11-09, as built, for review: ENGINE-CRITIQUE.md's Â§1.4, Â§4 and step 2)

ENGINE-CRITIQUE.md's Â§1.4 wanted the code the tasks each had a copy of in a shared library of the paged ROM, called by
XCALL (some 120 cycles a call: for routines that do real work, or are called seldom).  Its first contents are the two
Â§4 copies that were simplest and safest: the overlays' loading and `num`.

**A module of its own, not an addition to unpack.**  modules/dq0lib (rom.txt, after unpack: a library module,
HT_LIBRARY, its jump table at `$A030`).  The unpack library is the system's (libraries.md lists it with numbers, math
and asm: packed banks read from any file, nothing of the game's in it); the game's own code, its overlays' files'
names, its later line and router, would make it the game's.  The cost is a bank of the paged ROM (a library has its
own `.cfg`, so the build doesn't pack it beside another module); the ROM has the room.  dq0lib calls unpack by XCALL
in its turn (found by MODINFO, as the tasks found it before): XCALLs nest, each putting its caller's bank back.

**How a task calls it** (games/dq0/dq0lib.inc, which each task's bank.s includes in place of ovunpack.inc, with
`OVL_BANKS`, `OVL_DOT` and `OVL_FILE` set before it):

- `unsigned char ovl_load (const char* dir)`: finds the library (MODINFO, its directory's entries read into the parts'
  area, nothing there yet), keeps its bank and page (two bytes), and calls its `OVL_LOAD`: the path made in the parts'
  area, the file opened, the banks had (a run), their numbers written to `_ovbank`, the unpack library called, the file
  closed, the window's bank put back.  0, or not 0.  The UI gives the fd find_dirs opened (its high byte 0) and a `.`
  after the first twelve banks (`OVL_DOT`: its start's progress, as before).  dq0harn's ovl_in takes its two banks
  more after it.  Every task calls it first, so the library's found before anything else calls it.
- `unsigned num (const char* s)`: `NUM`, a number's digits (32 bits in the library, 16 here).  dq0d's, dq0p's (its
  `s_num` in part 3) and dq0w's `num` and the UI's `dnum` (bank.s: `_dnum = _num`) are gone, and dq0m's `atoi` (with
  the C library's ctype that came with it).  dq0harn keeps `atoi` for its sheets' numbers; its arguments are `num`'s.
- Not moved: dq0life's and dq0end's `num` (32-bit): RAM programs with some 20,000 bytes free, which don't find the
  library (finding it would cost more than their `num`).

**Room** (the main RAM's free bytes, the floor 2,048): the UI 2,128 to 2,307; the world 2,536 to 2,624; the denizens
2,399 to 2,512; the senses 2,435 to 3,122; the maker 2,414 to 2,879; the referee 2,197 to 2,329.  More than Â§1.4's
measure ("a few hundred bytes a task"): a task's `ovl_load` was what brought the C library's `open`, `strcpy`,
`strcat` and the like into its main RAM, and the senses and the maker used them nowhere else; the maker and the referee
had a 64-byte path besides.  The parts: the world's part 3 84 to 166 and overlay 1 86 to 168, the denizens' part 23
2,473 to 2,547 and overlay 2 31 to 105, the senses' part 3 4,966 to 5,048 and overlay 3 2,209 to 2,291 (their `num`s).

**Performance:** the start (dq0starttime) 226.8M cycles to 226.6M at seed 1, 305.9M at seed 123456789; every task's
banks as ld65 made them (dq0OvlBanks).

**The line of sight and the chunk lookup, looked at, not moved** (ENGINE-CRITIQUE.md's Â§4 has the plan).  The obstacle
noted before ("a ROM module can't call back into the task's `at`") isn't the real one: a ROM routine can `jsr` to its
caller's RAM.  The real ones: the ROM can't link to the task's symbols, so `at` and the rest must be handed over at run
time; the line keeps some 45 bytes over its calls of `at`, which may change the call registers, and a library has no
RAM; it reads cc65's C stack, whose runtime is at another address in each task; and `at`'s quick way (some 80 cycles,
thousands a look) can't be an XCALL.  The plan: a fixed zero-page block for the line (`$50-$7F`: the tasks' own zero
page ends at `$41`) with the callbacks' vectors, laid out in one include for the ROM and the tasks; stubs in the task
that take cc65's arguments into it; the hub cache's search as the library's `CS_FIND`.  It would free some 770 bytes of
the senses' main RAM and the world's part 65, for some 150 cycles a line (3%).  Left for now: neither task is short of
room, the one line is already one source, and the router (step 2's next) needs the same contract of callbacks into
`at` from the ROM, so it's best designed once, for both.

## One way-finder (2026-11-09, as built, for review: ENGINE-CRITIQUE.md's Â§4 and step 2, its second part)

ENGINE-CRITIQUE.md's Â§4 listed the step rule and the way-finding as copies that might disagree: chunk.c's `step_ok`,
the world's `step_by` (its own copy), ways.inc (the same source, compiled into the world and the senses), the minds'
`open_sq` and `taken`; the minds' `route` (a search) and `step_for` (greedy), the world's `ways`, `s_go_to`,
`s_path_ways`, `s_ways_go`, `s_step_to`.  What each decided before:

- **A step, a step at a time** (chunk.c's `step_ok`, slant.s's `slant`: the world's walks and steps, a denizen's body's
  step as the world carries it out, wbody.c, the minds' steps): a square can be stood on if its kind can (T_PASS: not
  rock, a wall, a window, a pit, a cut; rubble, steps, a stairway's square can), a door's only if open and not hidden;
  a rise or fall of STEP_MAX feet, STAIR_MAX on steps at either end; a diagonal only a diagonal way's, past two corners
  neither of which can be stood on (cornk: rock, a wall, a door, a window, a pit), one of them cut.  One source
  already, compiled into each task.
- **The minds' steps** (dq0d.c's `step_for`, `s_coarse`, `s_way`, three copies of one test): that rule, and the square
  within its reach of home, no one on it (`taken`: the character, where it's going, another actor, a denizen out near
  it), no water deeper than it wades unless it swims or flies, no bad air (`wades`).
- **The minds' way found** (`route`, part 18: a search over a 32-square field from the mark): the same kinds, heights
  and corners, a shut door a way for hands (worked as it gets there: door_way), not for a beast, a hidden one none;
  **no notice of water or bad air**; a diagonal past a door's corner refused once that door's square had been reached
  (the order the search came to it), let through before; and its fields **shared by every walker** bound for the
  same mark, whatever its rules.
- **The character's ways** (dq0w.c's `s_ways_go` and `step_by`, part 21: a search over the room's floor and walls, or
  25 squares about it in a passage): the same kinds, heights and corners (its own copy); a door's square a way in a
  room whatever its state (hidden too), none along a passage, open or shut; no square a denizen stands on, nor one
  with a trap it knows of, a hidden trap the way's first; its times by its pace.
- **The character's straight walk across a room** (`across`, part 33): any of the eight headings over the room's
  floor, a diagonal over open floor and past corners too; a door straight on opened if it's handled one.
- **The placing's `open_sq`** (where a denizen is put, an ally come along, a cornered one's dart): only floor, a
  passage, steps.

So they agreed on slopes, steps, rubble, windows, pits and the diagonal ways' corners everywhere a player can reach
(the world's copy of the rule was the rule).  They disagreed on water and bad air: **a denizen's way led into water
deeper than it wades, or a pocket of bad air, where its steps won't go**, and it stood at the edge for good (its way's
next step refused, the greedy step no nearer, the same way found again); a beast could follow a field found for one
with hands to a shut door; and the diagonal past a door's corner hung on the search's order.

**What's built.** One source, dq0w/walk.inc (C), compiled into the world (dq0w.c's part 21) and the denizens (dq0d.c's
part 18), not yet the paged ROM (it reads the chunks through each task's own `at`; it goes there with the chunk
lookup, as the line does):

- `wk_cell (t, q)`: a square's cell by the rule, for the walker's rules (`wk_rules`): WK_HANDS, a shut door (not a
  hidden one) a way, to be worked as it gets there; WK_NODOOR, no door's square a way.  The rule with neither is
  `step_ok`'s.  What else a walker leaves out its reader takes off the cell as it reads the squares in: the
  character's, a denizen's square and a trap it knows of (a hidden trap marked, WC_TRAP); a denizen's, water deeper
  than it wades and bad air (dq0d's `wet_ok`, `wades`'s own test) and a chunk not made.
- `wk_search ()`: the one search over a grid of 32 by 32 cells (a chunk's squares, the world's; a field about the
  actor and its mark, the denizens'), the floors beside them, from the cells queued, the four ways then the diagonal
  ways, the step's rise (STEP_MAX, STAIR_MAX on steps) and the diagonal's corners checked there, the first way to each
  square kept (the way it was come into); within a box (the room's, the world's), stopped at a square (the actor's,
  the denizens').  The world's times and its traps on the way by WK_REACHED, its hook.
- **The world**: `s_ways_go` reads the room's squares in (the character's rules: WK_HANDS in a room, WK_NODOOR along a
  passage) and searches; `step_by` gone; the ways' bank laid out anew (cells, floors, times, traps, a 512-entry
  queue, then the path and the travel's legs as before).
- **The denizens**: `route` reads the field's squares in by the actor's rules and searches; each field kept with the
  rules it was found by (WKEY: its water's, its hands), a field shared only by walkers whose rules are the same; the
  minds' one test of a step, `legal` (the rule) and `may` (its reach, no one there, `wades`), in place of their three
  copies in `step_for`, `s_coarse` and `s_way`.
- Unchanged, on purpose: `step_ok` and `slant` (the rule a step at a time, and already one source); ways.inc (the
  same source in the world and the senses: a passage's breadth, the ways on); `open_sq` (a stricter rule: where one is
  put, not where it may step); `across`'s diagonals over open floor (a straight walk, the character's own; see below).

**What a player sees change:**

- *A denizen goes round water deeper than it wades, and round bad air*, where its way led it in and it stood at the
  edge for good.  (dq0route: a hostile dwarf miner four squares off across the guardroom, a pocket of bad air on the
  square between, comes round it to the character; on the old code it stood beside the pocket, two squares off, for
  the three minutes the test waits.)
- *A beast doesn't follow a way found for one with hands*: one bound for the same mark finds its own (round, or none:
  its greedy step then, as before), not a way through a shut door it can't open.  The fields are found more often
  where walkers of different kinds go for the same mark (each at most every ten seconds, as before).
- *A diagonal way past a door's corner* is a denizen's way whatever order its squares were found in, as its step
  already allowed.
- *The character's ways*: unchanged but for one thing no player could see before: a hidden door's square is no longer
  a square of the room's ways (every caller had left hidden doors out already).
- Timing: the world's search now reads the room's squares once (each square's denizen and trap looked up once, not
  once for each of its eight neighbours) and searches the cells; the first letter of a walk's answer is a little
  later, the whole a little sooner (below).  As cycles move, so do the turns' dice in long fights: dq0antidote's
  adders bit the character senseless a third time within one answer ("Everything goes black again." told twice: the
  game's own wording, blows.c's harm_told, "again" after the first), which the repeats' check (DQ0_REPEATS) now lets
  pass with the fight's other falls.
- Left as they were (a difference kept): the character's straight walk across a room (`across`) goes diagonally over
  open floor, and between two corners in a cavern, which neither a denizen's step nor anyone's way does (a diagonal is
  a diagonal way's only); a dart and an ally come along want floor, a passage or steps (`open_sq`), not rubble nor a
  stairway's square.

**Test dq0route** (tests.js's `dq0RouteTest`): seed 1, dq0line's scene (the dwarf miners' guardroom), the character's
lantern lit; a dwarf miner made hostile four squares off along a row of floor (the others sent off: dq0Alone), a pocket
of bad air poked into the chunk on the square halfway (its reach 0: that square alone); three minutes waited.  The
dwarf must come within a square of the character and never stand in the pocket.  On the old code: it stood at
32708,32795, the pocket's edge, every time it was looked at ("the dwarf never came round the pocket of bad air").

**Performance** (cycles; the same games on the old build and the new):

| | Old | New |
|---|---|---|
| A denizen's decision (`decide`, a call: dq0denportc, its 15,219 decisions the same / dq0tunnel / dq0atbay) | 97,446 / 53,868 / 106,511 | 97,810 / 54,121 / 105,730 (+0.4%, +0.5%, -0.7%) |
| Its step (`step_for`, a call: the same three) | 5,671 / 6,516 / 6,917 | 5,737 / 6,426 / 7,148 (`may` and `legal` called, not inline) |
| A walk's answer (dq0answer: "walk to the door ahead to my left", "walk to the window"), its first letter | 623,716 / 481,606 | 633,207 / 488,461 (+1.5%: the room's squares read in first) |
| The same, the whole answer | 5,926,259 / 5,902,481 | 5,717,516 / 5,669,342 (-4%) |

The start (dq0starttime): seed 1 226.8M cycles to 227.2M, seed 123456789 302.2M to 305.2M (+1%: by the tasks, the
world +0.6M, the denizens +1.9M, in their own doings as the start's timing moved, more dice thrown and intents posted:
no way is found in the start).

**Room:** the world's main RAM 2,536 to 2,471 bytes free (the ways' lookups, way's and path_ways', by the new layout);
its part 21 1,641 to 1,402 free.  The denizens' main RAM 2,399 to 2,348 (`legal`, `may`, `wet_ok`: the three copies
of the step test gone from step_for's, s_coarse's and s_way's), part 18 95 to 89, part 17 2,718 to 2,870.  The senses
unchanged (2,435).

## The hub's numbers, made by the build (2026-11-09, as built, for review: ENGINE-CRITIQUE.md's Â§2 and step 3, its first stage)

ENGINE-CRITIQUE.md's Â§2 found the hub's numbers kept in step across languages by hand: games/dq0c/hub.inc mirroring
twenty of hub.h's (held to them by dq0c's make.js), actions.h mirroring the catalogue (held by make.js), struct kind's
size, the attitudes, a square's feet, the sound's kinds and some sixty of struct theme's offsets typed again in the
make.js files with nothing holding them, the tests compiling C expressions with cc65 as they ran to find the structs'
offsets, and the writers' table a comment a test read.  Stage 1 makes each of them come from one place; it doesn't
yet type the messages' payloads (stage 2) nor change a field of the hub.

**The C headers stay the schema, read as the compiler sees them.**  games/dq0/hubgen.js, called by build.js
(`hubNumbers`) before the games are built and by `build.js prog`: it reads hub.h and the headers it includes (rs.h,
level.h, ../dq0w/world.h, items.h, theme.h) with dq0.h and life.h (the command tools' names), and commands.actions.
Every object-like #define and every enum's member by parsing; every named struct's fields by parsing (a field of a
struct type, or of an anonymous struct, flattened into its own paths: `tb.at`, `door.rel`, an array's first element;
a #define among the fields left out); then one C file of `const long vN = (long) (EXPR);` lines, one a name, a
field's offset and size, an array's element size, a struct's size and each region of the writers' table, compiled by
the build's own cc65 and read back from its `.dword`s.  A line cc65 can't make a constant of (a #define of code) is
dropped and the file compiled again: two of 2,166 (KEPT_CLOCK, FIGHT_LB).  2,164 names, 134 enums, 80 structs with
1,516 fields; some 0.3 s, made again only when an input's hash changes.  No second schema file: the headers are what
the C is compiled from, so they can't disagree with it, and stage 2's payload table is the place to decide whether a
declarative file should write the C instead.

**What it writes** (obj/gen, beside apigen's files, on the build's cc65 and ca65 include paths now):

- `dq0hub.json`: the names (`C`), the enums in order, the layouts (`S`: each struct's size, each field's offset, size
  and dimensions), the writers' map (`own`) and the tasks' names (hub.h's T_*).  `load()` reads it, with `off (struct,
  path)` (indices numbers or names: `faction[2]`, `mreq[ASK_UI]`), `size`, `words` (an enum's members as words, each
  checked its value its place) and `value`.  Read by dq0's theme compiler (make.js: struct theme's and struct kind's
  every offset and length, the TM_*, P_FORT, K_*/KF_*, TR_*, TE_*, END_*, G_THROUGH, TL_RUINED, AT_*, ST_* it typed),
  dq0w's make.js (struct kind's size, its name's, cap_weight's place, NKINDS, CQ_WORDS) and outfits.js (MT_*), dq0d's
  (ST_*, AT_*, SND_KINDS, P_*, SQ_FT) and dq0s's (SND_*) make.js, sim/lib/dq0meaning.js (its 400-odd named offsets and
  the names for its events), sim/lib/hubown.js (the map, the table, the tasks), tests.js (hubown's `evaluate`, nine
  calls, now offsetof, sizeof and names from the layout; four numbers it had typed) and commandwords.js (its own
  header parser gone: the names, CQ_REL, CF_*).
- `dq0hub_NAME.inc`, an assembly include per assembly file, each with the names it uses (hubgen.js's `ASM`: a header's
  name, `STRUCT.PATH` an offset, `sizeof STRUCT[.PATH]`), `.include`d where its hand-typed lines were: dq0c.s (the
  hub's places, the request's, its constants: hub.inc's), cmdmean.s (struct cmd's CM_*, struct cthing's T_*, CF_*,
  CQ_REL), line.inc (T_*, struct portal's kind, state and parts, PK_*, PP_*, PS_*), items.s (struct item's size and
  four fields, NITEMS, ITEM_BANK (0), ITEM_BANKS), letter.s (NLET, LT_REC, struct cix's size and let: an `.assert`
  that it's 4, its two shifts), pulse.s (SND_GRID, SND_W), at.s (CW) and lbsaid.s (dq0.h's TOLD).  The hardware's
  HUB_U and HUB_BANKREG, hub.c's literals, moved from hub.inc into dq0c.s.
- `actions.h`, from the catalogue: VB_NONE, its [actions] in order (each its `(* *)` words), NACTIONS.  The catalogue
  has the roles and the result codes, so it is the one list; games/dq0/actions.h is gone, and the C's `#include
  "actions.h"` finds the build's (VB_OBSERVE_IN's words moved into the catalogue).

**The writers' table** is the block between hub.h's `own:` and `own.` lines, in its comment as before (readable, a line
a region).  The build reads it (a line in the block that isn't an `own` line, a blank or a line's words going on
stops the build), works out each region's bytes in the same cc65 pass (the C expressions hubown.js compiled at test
time) and writes them into the JSON; hubown.js paints them and checks the writes as before.  The map was held to the
old one byte for byte (26 banks, 231 regions), as were the recorder's offsets and the 1,768 names the old parser found.

**Gone:** games/dq0c/hub.inc and dq0c's make.js checks (hub.inc to hub.h, cmdmean.s to hub.h, actions.h to the
catalogue), hub.h's HUBO_*/CQO_* and dq0c_fits, actions.h, things.c's (the world's and the senses') store_fits (items.s's
numbers to the headers), obj/hubown.json and obj/dq0meaning.json (the tests' caches of what they compiled).  Kept: the
C's own asserts (cmd_fits, theme_head: C to C, for the C), make.js's rowsSame (actions.c's rows are code, held to the
enum, now the JSON's), dq0c's limits (its make.js to dq0c.s's, the parser's own).

**No behaviour change:** every value the generator gives is the one that was typed (checked as each was replaced), and
the images came out byte-identical: bin/* (the ROMs, the SD card's image, the page) and the games' programs and data
in obj/games, 112 files (hydra.lib differs in ar65's file times alone, as it does build to build).  One difference
that isn't in the images: a theme's kind may now name every material and craft the game has (MT_*: wax, tallow, gold;
AP_*: rock), the theme compiler's lists being the enums' (they had stopped at `other` and `glass`).  The build took
1 min 23 s (1 min 27 s before).

**Left** (not the hub's, or not numbers): the denizens' own records in dq0d/make.js (dq0d.c's struct dkind, faction and
dkx, wtrace.c's struct tkind, lore.h's records: their headers aren't the hub's, and dq0d.c's structs are in a .c),
outfit.h's PL_* and places in outfits.js and dq0w/make.js, themegen.c's TK_*, line.h's LN_* in line.inc, and the
tables built from world.h's T_PASS (slant.s's t_kinds, cast.s's): the generator can take them as their headers are
added to its list.  `node sim/tools/commandmatch-test.js --phrases` doesn't link (resolve.c's letters, des_add and
the rest, not in cmdres.c's stubs: nothing of this change's), and isn't part of the suite or the tiers.

## The messages' payloads, declared (2026-11-09, as built, for review: ENGINE-CRITIQUE.md's Â§2 and step 3, its second stage: the intents)

Every intent and event travels in one `struct msg { kind, who, a, b, w1, w2, t, x[4] }`, and what each slot meant for
each kind was written only in hub.h's comments, often packed by hand at both ends (I_WAIT's w2: `k << 8 | (n & 0x7F) <<
1 | 1` in the UI, `m->w2 >> 8` and `m->w2 >> 1 & 0x7F` in the world).  Stage 2 states each kind's fields once and makes
the rest from it, the layout untouched; this first part does the intents.

**A declaration file of its own, not a block in hub.h** (games/dq0/msgs.def).  The writers' table lives in a hub.h
comment because it reads as a table of regions beside the structs it names; a payload table is a grammar of its own
(slots, bit ranges, constants, enums, forms, inclusions) that would need escaping inside a C comment, and hub.h's long
comments beside each kind stay what the fields mean, msgs.def where they sit.  It is read by hubgen.js in the same pass
as the headers (its hash in the cache key), so it is checked against the very structs and constants cc65 compiles.  Its
syntax (its head says it at length):

    struct msg
    I_WAIT          a how; w1 tenths; w2(busy) task:8-15@LT_NONE what:1-7 quiet:0
    I_STEP          w1 head:0-2@H_N keep:8=STEP_KEEP; w2 squares
    I_STEP:den      < I_DEN; a heard@DE_MOVED; b loud; w1(way) head:0-2@H_N coarse:9=STEP_COARSE scent:10=STEP_SCENT;
                    w2(sense) one:0 feels:1=STEP_FEELS; x[2] nose

a line a kind (indented lines go on), each slot of the struct then its fields: a name alone is the slot whole, `name:N`
or `name:N-M` its bits; `=CONST` a one-bit field's constant from the headers (checked to be that bit: the hand names
STEP_KEEP and the rest stay, tied to the declaration); `@FIRST` an enum whose members name the values in the tests;
`SLOT(name)` a packed slot named whole too (to test it for none, copy it, or set it by its packer); `KIND:FORM` another
shape of the same kind (the denizens' I_STEP, a search's I_PORTAL, I_KIT's digs, I_ROB's errands); `< KIND` another's
fields included (I_DEN, the denizens' intents' head: doing and face, not a kind).  A slot of flags the headers already
name (I_WAIT's WA_*, I_KIT's KT_*, I_RISE's POS_*) is one field, its slot whole, and the code tests it by those names.
The same syntax is meant for the events and for struct req (a `struct req` line; its multi-byte x[] wants a slot range).

**What the build makes of it.**  obj/gen/dq0msg.h, which hub.h includes after struct msg (written before cc65 reads
hub.h, so the pass's own compile sees it): for each field an accessor `I_WAIT_TASK (m)` over a struct (a pointer's `*m`),
an lvalue when it's the slot whole (`I_GO_X (m) = tx;`); for each packed slot a packer, `I_WAIT_W2 (task, what, quiet)`,
its fields in the order written (one bit: its bit or none; the lowest field as given, the top one cut by the slot, one
between masked); for each form a builder, `I_PORTAL_SET (m, who, act, which, x, y, bar, strike, hand)`, setting kind,
who and each slot it names in the struct's order (a comma expression).  The generator stops the build on a slot the
struct hasn't, a field past its slot's bits or over another of its form, a name the headers have already, the same
name meaning two things in two forms, or a constant not its bit.  The macros were tried against cc65 first: `(&m)->`,
`(*m).` and `m->` are the same code, constant packers fold to their value (`I_WAIT_W2 (0, 0, 1)` is 1), a comma builder
is the same as its statements; one difference found by the images (a word's low byte as `w1 & 0xFF` loads both bytes,
`(unsigned char) w1` one), so a field of bits 0-7 of a word is accessed as the byte.  Into dq0hub.json go the payloads
by offset (`msgs`: each form's fields, offset, bytes, bits, enum), each slot's width held to cc65's; `load ().payload
(kind, form)` and `load ().decode (bytes, o, kind, form)` read a message's fields by name, a one-bit field a boolean, an
enum's value its member's name.

**Every intent declared and switched.**  The 31 kinds after I_NONE, in 45 forms.  The UI: its intents go out by
`intend2 (kind, w1, w2)` and `intend (kind, a)`, slot by slot, which stay (their slots plain values: the meaning is the
kind's line in msgs.def); a packed slot is made by its packer (`intend2 (I_WAIT, t, I_WAIT_W2 (k, n, 1))`, the quiet
waits `I_WAIT_W2 (0, 0, 1)`, `I_STEP_W1 (d, keep)`, `I_TRACE_W1 (TRC_SMELL, h)`), and the hand-built messages by the
builder (s_work's I_PORTAL_SET) or the accessors (s_intend, shot, reach_go's and place_go's walk, search.c's searches,
pace.c's journey): 61 lines in main.c, search.c, pace.c, way.c, hthings.c, read.c, tend.c, blows.c, feed.c.  The
denizens (dq0d.c, 177 lines): every `im.SLOT =` is its kind's accessor, the steps' packed slots by their packers.  The
world (184 lines): the receivers in dq0w.c (intent, intent_few, intent_more, task_word, close_go, fight_go,
disengage_go, passed, travel_go, a trace's work), wbody.c (the denizens' ring), wport.c, wtake.c, wfetch.c, wfight.c,
wdig.c, wlimbs.c, outfit.c, wgame.c, wtrace.c, and the world's own I_WAIT (things.c) and I_ASKs.  Left as slots, by
design: `kind` and `who` (every message's), the generic senders' bodies (intend2's, post's, act's head is I_DEN's), and
a few clears of slots a form doesn't use (`m.w1 = m.w2 = 0`).

**The tests.**  The recorder (sim/lib/dq0meaning.js) records the intents as the UI's ring (the hub's iq) and the
denizens' (di) show them: each `{ line, ring, kind, who, ...its fields by name, raw: { a, b, w1, w2, t, x } }`, a
denizen's by its :den form if its kind has one; dq0Turns gives each turn its `intents`.  dq0longwait reads "wait 8
hours"'s I_WAIT (`tenths` 480, `how` WA_MIN, `task` LT_NONE), dq0tend "tend my wounds"'s (`task` 'LT_TEND', `quiet`).
The events' decode merges a declared payload too, so declaring the events needs no change there.

**No behaviour change:** the images came out byte-identical (bin/*, and every program and data file in obj/games; the
objects differ only in their debug lines, hub.h two lines longer), and the suite passes.

**Left** (ENGINE-CRITIQUE.md's Â§2, "Left of stage 2"): the events (E_*: forms chosen by a field's value, E_TASK's TE_*,
E_BEAT's WB_*, in place of dq0meaning.js's hand switch; their names clear of the raw slots the event records keep at the
top), then the referee's requests (struct req: multi-byte slots, a request's fields and its reply's apart).

## The events' payloads, declared (2026-11-09, as built, for review: ENGINE-CRITIQUE.md's Â§2 and step 3, its second stage: the events)

The intents' mechanism (the entry above) carried over to the events (E_*), the world's news to the UI, the denizens
and the sound task, in the same struct msg.  What an event's slots mean often hangs on one of them: E_TASK's b and
w1's high byte are the enemy's entry for TE_ENGAGED, the light's kind for TE_LOW, the robber and how for TE_ROBBED, the
errand's outcome for TE_FETCHED; E_BEAT's high byte is a side, a heading and a gait, a thing's kind or a search's step
by its WB_*.  dq0meaning.js's hand switch (forty lines of `if (e.te === ...)`) was where that was written down.

**A selector, `FIELD==CONST[,CONST]`.**  A form of a kind (msgs.def's `KIND:FORM`, as the intents' :den forms) is chosen
by a field's value:

    E_TASK          b named@LT_NONE; w1 te:0-7@TE_STOPPED task:8-15@LT_NONE
    E_TASK:engaged  b by:0-5+1 quiet:6=TE_QUIET still:7=TE_STILL; w1 te:0-7==TE_ENGAGED,TE_FAR,TE_AWAY,TE_FIGHT@TE_STOPPED
                    task:8-15@LT_NONE
    E_TASK:robbed   b by+1; w1 te:0-7==TE_ROBBED@TE_STOPPED purse:8 caught:9 asleep:10

`==` and not `=`: a one-bit field's `=CONST` is already its constant (`quiet:6=TE_QUIET`), checked to be that bit; a
selector names the field's value.  The tests' decoder (hubgen's `load ().decode`) reads an event by the first form whose
selector its field matches, else by the kind's own form (E_TASK's: TE_NOT's named, and the rest).  One selector a form,
of the form's own fields (not an included one's), none on a kind's own form; its values the headers' names, held to the
field's bits.  In the JSON a form has `sel: [field, [values]]`; the C has no use for it (the code tests the field).

**Its accessors, `KIND_FORM_FIELD`.**  The intents' rule is a name the same field in every form of a kind; the events'
forms reuse names for different bits (E_TASK's `task` is w1's high byte, but TE_LOW's and TE_OUT's its low six bits, two
flags above; `by` is b's low six bits, or four, or all of it), and the tests read them by those names.  So a chosen
form's field is `KIND_FORM_FIELD` (`E_TASK_LIGHT_TASK`, `E_TASK_ROBBED_BY`), unless the kind's own form has it the same,
slot and bits (`E_TASK_TE`, `E_TASK_TASK`, `E_BEAT_WHOSE`); its packers and builder are the form's as before
(`E_TASK_LIGHT_W1`).  A selector of one value isn't an argument: its constant is in its place
(`E_TASK_ROBBED_W1 (purse, caught, asleep)`, `E_BEARING_SKY_W1 (refusal)`), so a sender can't build a form with another
form's value; of several, it is (`E_TASK_ENGAGED_W1 (TE_FAR, LT_NONE)`).

**Three more marks, for what the hand switch worked out.**  `!` a signed byte (E_LEDGE's rise: the C accessor
`(signed char) (m).w1`, the tests' value negative); `+K` a value stored plus K (an actor's entry plus 1: the C reads and
writes it as stored, the tests read it less K: `by`, the entry); `@A,B` and `@a/b/c`, names of several kinds (E_FIGHT's
out, `@O_MISS,FO_GONE,FO_BLED,FO_BOUND`: the referee's O_* or the world's FO_*; TE_BREAK's how and TE_FREE's none,
`@back/flight/run`, words for 0, 1, 2, where the headers have no names).

**Every event declared:** the 21 kinds after E_NONE (the critique's 23 counted E_NONE and NEVENTS), in 41 forms:
E_TASK in ten (its own; engaged, hurt, nofoe, break, free, light, robbed, fetched, begun), E_BEAT in nine (its own; feel,
met, hit, thing, limb, rise, swat, search), E_HARM:struck, E_DIG:notool, E_BEARING:sky.  The field names are the hand
switch's where the tests read them (te, task, by, still, quiet, lamp, seen, ft, beat, side, thing, fa, head, gait, de,
def, out, ff, how, feet ...); a square's x and y are `sx` and `sy`.  Not declared: E_PORTAL's a and b (a search's reach,
an opening's what and room), E_TASK's b for the forms not given (TE_NO_SHOT's why, a journey's place: `named` as
before), E_BEAT's high byte for the water's and the walk's own beats (WB_WATER, WB_POSED: `side`, as before).

**The tests' records keep the raw slots at the top (the choice the critique left open).**  The intents' records moved
them under `raw`; the events' tests read `e.a`, `e.b`, `e.w1`, `e.w2`, `e.t` in some forty places (a denizen's DE_* by
`e.a`, a dug square by `e.w1`, an event's time by `e.t`, a fall's whose by `e.b`), so the events keep them where they
were, and no field of an event is named as a slot is (kind, who, a, b, w1, w2, t, x, line, den: dq0meaning.js's layout
stops on one).  The hand switch was kept beside the declared decoder for one run of the suite, every field it gave held
to the new one's.  The differences: fields it set where they meant nothing (E_BEAT's fa, thing, head and gait on every
beat past WB_FEEL, whatever it was; E_HARM's stopped on a fall; E_DIG's feet for D_NO_TOOL, what it's into, now
`into`); `by`, which the fights' and the robberies' tests read, first named otherwise (named so again); and values the better for it (E_DEN's
`seen` and TE_NO_FOE's `shock` the byte, not a boolean; E_BEARING's `brg` named with a refusal in w1's high byte;
E_FIGHT's FO_BOUND named).  Three test reads changed for them (dq0dig's two of D_NO_TOOL, `into`, dq0succession's `!!seen`);
then the hand switch went: dq0meaning.js's decode is the slots and `H.decode`.

**Switched:** 239 lines.  The world's senders (155: dq0w.c's `event (kind, who, w1)` by the packers, `E_CLIMB_W1
(K_BEGUN, cl.need)`, `E_TASK_W1 (why, HUB->pc_task.kind)`, `E_BEAT_THING_W1 (WB_SPENT, upk)`, the evb of a form by its
packer, `evb = E_TASK_ENGAGED_B (e, 0, 1)`; the hand-built ones by the accessors, `E_DEN_SX (m) = x`, and the packers:
wport.c's, wbody.c's and wdig.c's E_DEN, wfight.c's E_FIGHT and TE_ROBBED, wfetch.c's TE_FETCHED, wlimbs.c's, wroom.inc's
WB_SEARCH, the E_NOISEs, E_LIGHTs, E_VIEWs, E_LEFT, E_TAKEN, wtime.c's sky).  The UI's receivers (60: main.c's events
and heard, s_came, s_limb_words, s_news, s_noise_told; blows.c's fight words, caps.c's den_heard, search.c's
search_said).  The denizens' (dq0d.c, 10: E_NOWAY's N_DEN, E_NOISE, E_DEN, E_FIGHT), the sound task's (dq0s.c, 13) and
the senses' light layer (light.c, 1).

**Left as they were, by design or by the code they'd cost:** an argument that is a word whole (`task_words (m.w1, m.b)`,
`beat_words (m.w1)`, `trap_words`, `harm_words`, `pose_words`, `water_words`, rs_tell's and told's w: their callees read
it by hand), a whole-word test (`m.w1 == N_SHUT`, `m->w1 != PO_NOTHING`, `m->w1 == BRG_LOST`), and the shapes cc65
codes otherwise: the low byte as `w1 & 0xFF` inside a `||` or a `?:` or as an argument (the accessor's cast is shorter
code, so the images would change: E_ARRIVE's A_UP or A_DOWN, dig_words' and climb_words' feet), a field between two
others unmasked (E_HARM's band, WB_HIT's fa, WB_FEEL's head, E_PORTAL's pk, TE_ROBBED's and TE_OUT's flags packed by
hand: the packer masks it), and E_ARRIVE's sender (its link and flags by conditions).  The tests read nothing new; the
recorder's events are what they were, by declaration.

**No behaviour change:** the images came out byte-identical (bin/*, and every program and data file in obj/games: 154
files; the .s files the same less their .dbg lines; dq0's .lbl differs in one function's local labels' numbers alone,
its addresses the same), the UI's main RAM as it was, and the suite passes.

**Left** (ENGINE-CRITIQUE.md's Â§2, "Left of stage 2"): the referee's requests (struct req: multi-byte slots, a request's
fields and its reply's apart), being done beside this.

## The referee's requests' payloads, declared (2026-11-09, as built, for review: ENGINE-CRITIQUE.md's Â§2 and step 3, its second stage: the referee's requests)

The referee is asked in `struct req { RQH; actor, other; steps; a, b, c; w1, w2; x[32]; grade, result; text }`, and
answers in the same slots: what each meant, the request's and the reply's, was rs.h's comments, and the code at both
ends read `r->w2`, `r->x[18]`, `*(unsigned long*) (r->x + 4)` by hand.  This part does for the requests what the first
did for the intents, by the same file and the same generator, extended where requests need it.

**The same file, a second struct.**  msgs.def's `struct NAME` line already named the struct its kinds travel in; a
`struct req` line after the intents starts the requests, each slot checked against cc65's struct req (its width, an
array's bounds, its offsets in dq0hub.json).  Four things are new, all in msgs.def's syntax and hubgen.js's reading of
it, nothing in the parts the events will want:

    RS_HARM         a cause@HC_FALL; b ground@GR_FLOOR; c how; w1 feet  -> result caught; b band; w2 out; x effects
    RS_TIME         a about; b pc; w1 minutes; w2 bleeders; x[0] fighting; ...; x[16-17] spent; x[18] aff@AK_NONE ...
                    -> result died; b health@H_FINE; c bound; grade news; w1 dead; w2 out; x[4-7] next
    RS_ATTACK:thing c run; w2(flags) shy:0; x[0-3] resist[]
                    -> result(struck) stop:0 part:1 short:2 broke:3; a drop; b aspect@AS_BLUNT; c worst; w1 harm; w2 out

- **A reply after `->`**, a form of its own (`RS_HARM:reply`, `RS_ATTACK:thing_reply`): the referee writes its answer
  over the request's slots (RS_ATTACK's a is the aim asked and whose fumble it was answered; its w1 the range of a shot
  and the two healths back), so the two are declared apart, each checked for overlaps on its own, and the accessors of
  both are the kind's (`RS_HARM_FEET`, `RS_HARM_BAND`: one name, one slot, or the build stops).  A reply with no request
  before it is a form of a reply alone (`RS_TEST:swim -> grade; w1 squares; w2 breath`: a swim's test is asked as any
  test is, its reply its own).
- **Slots spanning bytes**: `x[4-7]` an array's elements spanned, a word for two bytes and a long for four, the accessor
  a cast of its first byte's address (`RS_TIME_NEXT (m)` is `(*(unsigned long*) ((m).x + 4))`, the cast rswant.h wrote
  by hand), an lvalue; any other width, or a field written `name[]`, is its bytes, the address (`RS_ATTACK_RESIST (*r)[k]`
  in the loop that fills them); `x` alone the array whole (a reply's struct rs_effect list, a new actor's struct
  rs_summary: `(const struct rs_effect*) RS_ATTACK_EFFECTS (*r)`).  Bit fields go in a span of four bytes at most.  The
  overlap check knows spans: `x[4-7]` and `x[5]` in one form stop the build.  In dq0hub.json a field's seventh
  element is the events' signed mark (1: their `!` byte, and a signed slot whole, a reply's grade) or 2, a span's bytes
  (`name[]`), the decoder giving its bytes; the eighth the events' `+K`; a span's offset its first element's, its size
  the element's times their count.
- **A slot alone is its field of its own name** (`steps`, `grade`): the reply's grade is a grade, a test's steps steps.
- **No builder for a request.**  A request is filled slot by slot around what the code computes (the world's blow: the
  aim, then the creature's heart, then the flags by the two bodies' positions, the missile's afterwards), and a builder
  sets its slots in the struct's order: the stores would move, and the images with them.  hubgen.js makes a builder only
  for a struct with a `who` (struct msg).

**Every kind declared.**  All 27 RS_* kinds: 17 with fields, the 10 not asked yet (RS_DROP_ACTOR, RS_TEST_MANY,
RS_REST, RS_INITIATIVE, RS_EXPERIENCE, RS_POWER, RS_EXPORT, RS_IMPORT, RS_RESTORE, RS_QUIT) a `-`; 60 forms, 35
requests and 25 replies.  The forms by purpose, cause and target: RS_TEST's `:dig` (b the material; the reply's rate and
the tool's wear), `:appraise` (b the craft, w1 the thing's number; the craft's mastery back), `:read` (the writing's
number), and the replies of a swim, the bearings (the drift) and drawing (Drawing opened); RS_ATTACK's `:missile` (b
the band, c the kind shot, w1 the squares), `:thing` (at RS_THING: the blow's run, the thing's resistances; the harm it
takes, the part's hurt), `:rise` (O_RISE: still down); RS_HARM's `:strike` (HC_STRIKE, HC_FIRE) and `:struck` (the
striker's own part); RS_TREAT's `:done`.  Packed slots: RS_ATTACK's flags (w2, `shy:0` beside the RS_* and MS_* flags
the headers name), its reply's two healths (w1), RS_MORALE's leaders (w2: its band's head, its faction's), RS_TIME's
meal (x[14]: meals, a ration begun) and drink (x[15]: pints, a remedy taken, foul water), RS_DICE's a (seeded).  The
names are what rs.h's comments said: RS_TIME's `minutes`, `bleeders`, `fighting`, `drowned`, `aff`, `dose`, `air`,
`flame`; its reply's `died`, `health`, `bound`, `news`, `dead`, `out`, `next`.

**Every sender and receiver switched.**  The senders, hub_ask's callers: the world (dq0w.c's fall, a dig's test and its
query, the roof's and the air's harm, a swim's reply, the wield, the engage contest, a grope, a map drawn; things.c's
tests, wblow.c's blow at a thing, wfight.c's blow, wdig.c, wlimbs.c's RS_EQUIP and the harm's effects, wtend.c's
RS_TIME, outfit.c's RS_OUTFIT, wrope2.c, wrope3.c, workpart.h's harm's reply), the UI (main.c's character loaded, the
hello, the things to hand; read.c, tend.c, way.c, feed.c), the denizens (dq0d.c: a new actor, the stealth and notice
tests, the sway's tests, morale).  The receivers: the HarnMaster referee (dq0harn.c's answer and its own tests, fight.c,
missile.c, gear.c, heal.c, sheet.c, afflict.c, want.c, outfit.c) with the rulesets' shared rshouse.h and rswant.h, the
null ruleset's (dq0null.c), and the referee's unit tests (tests/ram/t_rules: RS_DICE's faces and sides, RS_LOAD_PC's
reply; its other requests are built from rules.txt's slot names, as data).  642 lines: the world's 147, the UI's 32,
the denizens' 42, the HarnMaster referee's 273 (rshouse.h and rswant.h with it), the null's 138, t_rules's 10.  Left as
slots, by design: `kind`, `actor`,
`other`, `text` (every request's: who asks about whom), the generic senders' bodies (the world's `ask_w (kind, a, b)`,
the UI's `ask_b`, the denizens' `ask`, workpart.h's `harm (a, b, c, w1)` for a fall or a strike alike), and the few
places an accessor would change the code: the minutes copied byte by byte (`x[16] | x[17] << 8`, rswant.h and
dq0null.c), dq0harn.c's swim using `result` as scratch, the refusals' `result = 1` and the defaults' `0xFF`.

**The tests.**  The recorder (sim/lib/dq0meaning.js) reads a request as it's posted (its state REQ_POSTED, every slot
written) and keeps its fields by name; each reply's record gains `rs` (its kind's name), `ask` (the request as it was
asked, its kind's form) and `reply` (its `:reply` form's), beside the raw slots the tests already read.  dq0well reads
the jump's fall as `r.ask.cause === 'HC_FALL'`, dq0swim the swimmer's time as `r.ask.about`, dq0hardthrow a throw's
steps as asked (`r.ask.steps`, not the reply's slot).  A form by a field's value (a dig's `mat`, read as RS_TEST's
`how`) is not chosen yet: the events' selector (`==`) would do it, but it renames a chosen form's own accessors
(RS_TEST_MAT to RS_TEST_DIG_MAT, RS_HARM_IMPACT to the strike's and the struck part's), and RS_ATTACK's missile (a bit
of its flags) and RS_TREAT's done (a 1) have no constant of the headers' to be chosen by.

**No behaviour change:** the images came out byte-identical (bin/*, every program and data file in obj/games, and
obj/tests/t_rules.hyx), and the suite passes.  The accessors were held to the code they replace: `((*r).b)` is `r->b`,
`(*(unsigned long*) ((*r).x + 4))` the hand cast, `&RS_TIME_NEXT (*r)` the address memcpy took, `((*r).result & 0x8)`
`r->result & 8`; RS_MORALE's packer `(l1) | (unsigned) (l2) << 8` the same as the hand's `l1 | l2 << 8`.

**Left** (ENGINE-CRITIQUE.md's Â§2, "Left of stage 2"): the requests' forms chosen by a field's value, a pass of its
own (the accessors it renames, the constants it wants).

## Wear for what's worn (2026-11-09, as built, for review: the user's, "wear" restricted to articles that can be worn)

"Wear" was equip's: commands.verbs' `wear = don`, `don` the verb of `VB_EQUIP_OBJECT` with aux `IO_DON`, the world's
`IO_DON` (things.c's `don`): a thing of a kind that's worn (the catalogue's `K_WEAR`: clothes, armour, a cloak, a
backpack, a belt, a bandolier, a shield or a bow slung, a ring, a brooch, a necklace, a mask) put on; one that isn't,
but is held (`K_HAND`: a sword, a torch, a lantern), taken in hand instead ("wear the sword", "You take the sword in
your right hand.", or "You hold the sword in your right hand already."); anything else refused, "You can't wear the
sack." (the catalogue's `SA_NO_WEAR`).  "Wear what? [...]" (letters.c's) already offered only the `K_WEAR` kinds not
worn.  What's built:

- **Wear is a verb of its own, told apart by its aux.**  commands.verbs' `wear thing=VB_EQUIP_OBJECT unsaid=last
  aux=IO_DON_WORN`, `wear = wear`; equip, don, put on, ready, wield and brandish as they were.  `IO_DON_WORN` (items.h)
  is no request of the world's: hthings.c's `H_HANDLE` sends it as `IO_DON` with `to` `DN_WORN`, and the world's `don`
  refuses a thing that isn't worn whether or not it's held: "You can't wear the sword.", "You can't wear the lit
  torch.".  letters.c's "Wear what?" takes `IO_DON_WORN` as `IO_DON`.
- **Why the aux, not an action or a manner.**  The aux is the verb's own value and already tells the handling verbs
  apart within one action: wield is `VB_EQUIP_OBJECT` too, told from equip by `IO_HOLD`.  A `VB_WEAR_OBJECT` would be
  a row of the dispatch table, a line of commands.actions, the grammar's, the corpus's and the reference compiler's,
  and main.c's hand phrase and letters.c's tests of the action, for the same handler and the same request; the corpus
  ("wear the mail hauberk", `VB_EQUIP_OBJECT`) stays true.  A manner is how a thing's done (`MN_*`, the adverbs'), and
  no verb sets one.  `IO_WEAR` was taken (a tool's key worn by a body), so the name is `IO_DON_WORN`.
- **The refusal is the catalogue's, as it was.**  "You can't wear the %." (kinds.c's `said`, `SA_NO_WEAR`) is the
  world's, said after the thing's found by name, as the sack's always was; refuse.c's `said[]` and `say_no` are the
  line's refusals, before the world's asked, by a word, and whether a thing's worn is its kind's, which only the world
  knows of a thing named (one lying about too).  No new words.
- **Left as they are, for review:** "don" and "put on" (equip's still: a sword donned is taken in hand); "Equip
  what?" offers only what's worn, as before.  If "don" should be wear's, it's one line: `don = wear`.
- **Room:** the UI's main RAM 2,307 bytes free, as it was (the changes in overlays and parts).  The world's
  catalogue bank (its code at most 6,400 bytes, into the outfits' index) was 6,399 and went 6 over; take_from's and
  doff's tail (carried, into the pack or a hand, its words) made one routine, `carried`: 6,371.
- **Test:** dq0wearonly (a sword and the torch in hand, a sack: refused; "Wear what? [k]", the cloak's letter alone;
  the cloak put on by its letter; equip and wield the sword, taken in hand).

## The verbs not done yet: the rows, the aliases and the powers' refusals (2026-11-09, as built, for review: the plan for the 50 H_NOT_YET verbs, its batch B0)

Fifty verbs' rows in the dispatch table (actions.c) said `H_NOT_YET`, "You can't ... in this game yet."  The user's rule: each verb is decided on its own, and each should do something in the world rather than say "not yet".  This is the shared groundwork, so the later batches (B1 the fights, B2 the noise, B3 the minds, B4 the things, B5 the self and the powers, B6 moving, B7 the line; grapple its own) can each work in files of their own.  What's built:

- **The rows seen to before the table** say so: the target's three (resolve.c's) and read and combine (the puzzle's, theme.c's: read keeps its row's refusals, asked before the puzzle) are `H_NOTHING`, never reached; "sequence" too (parts.c splits the line first).  The pace's verbs are pace.c's when it knows the words; a line that comes past it now does something: a step that names a thing ("step onto the stairs") goes to it (`H_GO`); "pace" alone, a pace it doesn't know ("double"), and "status" with more words than pace.c takes are a new `H_STATUS` in hself.c, pace.c's "status" words (its described (0)) by the same path.
- **"sneak north" moves**: pace.c set the creep and stopped there; now the sneak is one of the moves it lets on to the command (as "run north" was), so the pace is set and told, then the row's `H_GO` goes.  "sneak" alone still sets the pace alone.
- **The aliases are the parser's** (commands.verbs' [words], the verb a word says): "negotiate" is persuade's, "sense a lie" examine's, "activate" use's; the three verb lines that made `VB_NEGOTIATE`, `VB_JUDGE_SINCERITY` and `VB_ACTIVATE` are gone.  Their rows stay, for any line that still makes them, pointing where the alias goes (`H_SWAY` with persuade's `SW_PERSUADE` as its request, `H_EXAMINE`, `H_USE`).  "negotiate with the rat" is a persuasion now ("The giant rat is an animal: words won't reach it ..."); "sense a lie from the rat" a look at it ("The giant rat seems wary of you."); "activate the torch" the use's own words ("Use what, and on what? ...", the puzzle's).  Not added to commands.aliases (the player's listed shortcuts): [words] is enough for the parser, and the listing stays as it was.
- **The powers refused, true to the character** (the user's wording), as data: refuse.c's `said[]` has three refusals more, said by `say_no` from a new `H_POWERS` (hself.c), the refusal the row's request byte (`ROWP`'s, as the door verbs' `PA_*`: `A_NO_MAGIC`, `A_NO_PSI`, `A_NO_GODS`):
  - spells (cast, memorize, unmemorize, learn a spell, research, dispel, counter, release): "You've no magic: you never studied the convocations."
  - psionics (a talent, possess, link minds): "You reach for it; nothing stirs."
  - invocations (invoke, learn an invocation, a call for intervention): "You know no invocations of Peoni's.", the character's own deity; with none, "You know no invocations." (the refusal's nameless form, as "You see no %s here." has).
- **The deity's name is the referee's**: `RS_DESCRIBE` with b `RD_DEITY` (5) answers the name of the deity the character worships (HarnMaster's: its sheet's `att dty`, hmchar's choice, `harn_deity_name` through part 2's stub; "" none), the null ruleset "" always.  A call for intervention by the world's pantheon's name ("beseech Imma") would be refused by the character's own deity's: the refusal is about what the character knows.  For review: whether a call for intervention should be refused so, or be prayer's (B5).  Not tested: in the test's world "beseech ugrath" isn't understood (the parser's, the world's deities' words) and "call on X" is the names' ("That name is taken ..."), both as before.
- **Stubs for the batches**: each verb to come has a handler of its own (dq0.h's `H_*`, its row pointing at it), a function in its file by meaning that says "not yet" (`unknown (U_YET)`, the words as before) till its batch replaces it: hthings.c `H_USE` (use, and activate), `H_PUSH`, `H_PULL`, `H_TOUCH`, `H_TASTE` (B4); hplaces.c `H_KNOCK` (B2); hmove.c `H_SWIM`, `H_FOLLOW` (B6); hdens.c `H_CHARGE`, `H_THROW_WAY`, `H_ENGAGE`, `H_SPLIT` (B1), `H_SAY` (B2), `H_WAKE`, `H_TAUNT`, `H_ORATE` (B3); hself.c `H_PRETEND` (B3), `H_PRAY` (pray and pray for), `H_FAST` (B5), `H_AGAIN` (B7).  Grapple's row is left `H_NOT_YET`, the grappling's own.
- **The shared words**: hub.h's `I_NOISE` (a noise made: a its loudness, b what it is, w1 w2 its square, 0 the character's; declared in msgs.def; the world does nothing with it yet, its E_READY as any intent's), `SW_WAKE` and `SW_TAUNT` (after `SW_BEFRIEND`, so `SW_ATTACK` ... `SW_GIVE` ... move up two: dq0d.c's `cap[]` sized to `SW_TAUNT`, and its `s_sway` answers both `SR_CANT` till B3 builds them, never the attitude's table past its end); rs.h's `P_ACT` (Acting), `P_ORATE` (Oratory), `P_RITUAL` (the deity's Ritual), before `P_THEME`, with dq0harn.c's purpose table (`HARN_ACT`, `HARN_ORA`; Ritual by the sheet's deity, `HARN_RAGR` + it, a critical failure with none).
- **The count**: of the fifty, 28 are done (the 14 powers refused; the 11 rows seen to before the table or now somewhere, status and sneak with them; the 3 aliases), 21 are stubs for their batches, and grapple's row is its own.
- **Room**: part 2 (the handlers) has 237 bytes left of 659 (the stubs and the refusals' asking: some 17 bytes a stub), overlay 24 (the refusals) 132 of 292, part 4 (pace.c) 13 of 17; dq0's main RAM unchanged (2,307 bytes free), dq0harn's 2,203 (the stub and the Ritual's test), dq0null's 5,453.
- **The corpus**: no lines added.  commandcorpus.js checks the corpus's form and that each action has a case; its expected actions are the grammar's (commands.ebnf), not the meaning commands.verbs gives, so the aliases change nothing there and the rows-only verbs and the powers have their cases already.
- **Test dq0notyet**: the refusals' words (a spell, a talent, an invocation invoked and learnt, the deity's name and none), the aliases (activate, negotiate, sense a lie), "status" and "pace", and "sneak north" a creep and a move.

## Grappling (2026-11-09, as built, for review: the verbs plan's B1g, the user's "full HarnMaster grappling")

The user, of the 50 verbs the game answered "not in this game yet": grappling gets HarnMaster's full rules, "grapple
attacks, holds, throws, pins, chokes, breaking free", by the Unarmed skill, the referee gaining them.  VB_GRAPPLE_OBJECT
was an H_NOT_YET row.  HOUSE.md's grappling has the rules as built; this is how, and where it's simpler than the book.

**How it's carried.**  A grapple is a blow of another kind, so it rides the fight machinery whole: the UI sends
I_ATTACK with its b `AIM_GRAPPLE | GM_*` (the move: hold, throw, pin, choke; the world's own GM_LETGO and GM_BREAK);
the world strikes it as a blow (wfight.c's blow: the same slot, ring, E_FIGHT, effects, morale, senselessness, death),
asking the referee RS_ATTACK with RS_GRAPPLE in its flags and the move in its a (msgs.def's new form,
`RS_ATTACK:grapple a move; c(hold) choked:0-6 pinned:7=GH_PIN`); the reply is a blow's, its outcome one of nine new
O_* (O_HOLD ... O_STRUGGLES, and the world's O_LETGO), so the UI words it (blows.c's grip_words, overlay 19) and the
denizens and the sound task take it as they take a blow.  The character fights it on as it fights a blow, round after
round, till its move is done (held, thrown, pinned, choked senseless: then its fight ends, the world's pc_cut), a hold
first if it doesn't hold that one already.  While it holds one, the character doesn't fight on by itself (the world's
fighting on, begun by a hostile one striking at it, isn't begun): the next move, a throw, a pin, a choke, a blow, a
let go, is the player's, or a hold would be beaten out of its hands by its own fists.  No new request kind, no new handler id (the row is H_FIGHT's, the
handler's first test the verb: the shared files' edits kept small, B0's in parallel).

**Who holds whom** is the world's (wfight.c's grip: the actor plus 1, GH_PIN; the rounds choked): one hold, the
character's.  A held denizen's intent (wbody.c's, through part 15's grip_held: part 14 has no room) is its try to
break free, its round, if it's anything but a wait or (not pinned) a blow: so it never walks away, and whatever its
mind wanted (to flee, to keep off, to rise) becomes the struggle.  Held but not pinned, its blows are four steps the
worse.  Let go: as either is knocked senseless or dies (the blow's, the bleeding's), as the held one leaves or is found
apart (its next intent), as the character moves off (the world's intent: a walk, a step, a dash, a climb, a take, a
close-in, a disengage, travel: told, "You let go of the giant rat."), at "let go".

**The referee** (dq0harn's grapple.c, a new overlay, the ninth: the others full; its two calls of fight.c's through
new stubs) tests each move as an opposed test, and the null ruleset answers each with a flat test.

**Where it's simpler than the book, and why:**

- **The book's grappling tables aren't in the repo** (no rulebook text: only the house's notes), so the moves are
  built from HarnMaster's grappling as the brief gives it, each an opposed test in the book's levels (CS, MS, MF, CF:
  the attacker's success must beat the defender's level; the defender has the ties) rather than a grappling table
  of outcomes.  Why: the opposed test is the book's general rule for one skill against another (Skills 5), and it
  needs no table the repo doesn't hold.
- **A hold is Unarmed against Unarmed or Dodge, the better;** a throw, a pin or a choke Unarmed against Unarmed or
  Strength (five times), the better; breaking free the held one's Unarmed or Strength against the holder's Unarmed.
  The book weighs Strength in grappling; five times it is the house's way of making an attribute a skill's peer (as
  the house's Stealth, a dig's).
- **Size two steps a difference,** both ways (the bigger the better), where the book bars throwing a much bigger foe
  outright.  Why: no table of what can be thrown; two steps keeps a bear hard and a rat easy.
- **A creature's Unarmed is its own attack's mastery** (its sheet's first weapon, its penalty off it): the bestiary
  keeps no Unarmed.  So a sword-armed bandit grapples at its sword's skill.
- **The throw's fall is two dice of blunt impact** (the sizes' difference a point a die) at a place the middle's strike
  table gives, its armour off it, as a blow's strike is; the book's fall impact by height isn't used (a throw is a
  fall from standing, too short for the falling table's bands).  The hold is let go as it's thrown (the thrower
  doesn't fall with it), so a thrown foe's next turn is its getting up, the book's Rise.
- **A choke is an Endurance test** (three times it) each round it's choked, ten less for each round before in this
  hold, a pinned one two steps the easier to choke; failed, it's knocked senseless as a blow's shock does (the book's
  shock rolls till it comes round, a minute at least: an unhurt one would come round the next round).  The book's fatigue from a choke isn't kept: the creatures' fatigue is tire.c's,
  by the clock, and a level more a round would knock it out no sooner.  Bones that move (BF_MINDLESS) can't be
  choked.
- **A pin** lays it down and holds it there; pinned, it can't strike (its blow too is a try to break free), four steps
  the worse to break free; the book's further holds from a pin (a lock, a choke's ease) are only the choke's two steps.
- **Held, its blows four steps the worse;** its dodge as ever.  The book's penalties for the held are by the hold;
  four steps is the house's one figure.
- **One hold at a time,** the character's: the denizens don't grapple the character (the brief's optional part, left:
  their minds would need a move to choose, and the character's intents a break-free of their own); a second hold on
  another lets the first go.
- **Grappling needs the foe beside the character** (out of reach: "go up to it first"), no close-in as a blow has: the
  grammar's own note ("the target must be engaged"), the book's a melee attack at one engaged.
- **A hold isn't kept in a saved game** (the world's own, not the save's): a game kept and taken up again has no hold.
- **The words "grapple", "wrestle", "tackle", "pin", "choke", "strangle", "throttle" are one verb** (commands.verbs'
  grapple), its move read from the line's first word (blows.c's grapple: p a pin; c, s and th a choke; t a throw;
  else a hold), as commands.aliases says a manoeuvre is: dq0c's records were full (255: five verbs made 258).  The
  grammar (commands.ebnf) gained "let go [of X]", "strangle" and "throttle"; sim/tools/commandcompile.js the same.  "Hold
  the goblin" and "throw the goblin" are a grapple when a denizen of that name is beside the character (else a thing
  held, a thing thrown, as ever); "let go" ("let go of the goblin") a verb of two words.  "Release the goblin" stays
  VB_RELEASE_PRINCIPLE's (a principle released, the magic's), its row B5's.

**The images** grew by the overlay (dq0harn.ovl's ninth bank), two of the UI's stubs, two of the referee's; the world's
part 15 the rest (its room's).  The tests: dq0grapple (a hold, a walk away failing, breaking free, a throw, a pin, a
choke to senselessness, "let go"); dq0rules' grappling cases; dq0parse's "grapple the rat" no longer "not yet".

## Prayer, fasting, swimming and following (2026-11-10, as built, for review: the verbs plan's B5 and B6)

Five of the verbs that said "not yet" now do something in the world: pray and pray for (one handler, `H_PRAY`), fast
(`H_FAST`), swim (`H_SWIM`) and follow (`H_FOLLOW`).  Their work is a part of its own, part 27 (hself.c's and hmove.c's
`SW27CODE` blocks), because part 2 has no room.  Part 2 keeps the stubs, each now a call.  What's built:

- **Pray** (the user's): to the character's own HarnMaster deity, named by the referee (`RS_DESCRIBE`, b `RD_DEITY`).
  "You kneel and pray to Peoni." begins a timed work of an hour (`I_WAIT` with `LT_WAIT`, its command kept: main.c's
  reach 3).  "stop" or an interruption ends it with no more said than a stopped wait's.  At the hour's end it makes a
  Ritual test (`P_RITUAL`), and its grade picks a calming line that names the deity, for example "You rise from your
  prayers, calmer, sure that Peoni has heard you."  The effect is the words alone.  "pray for X" is the same, with no
  bonus.  A character with no deity prays the hour too ("You kneel and pray, though you know no god to pray to."), and
  its end has a line of its own, with no test.
- **Fast** (the user's): "fast" is a vow for a day (864,000 tenths of the clock).  While it holds, eating is refused
  in feed.c, before anything is looked for, with the hours left and a warning: "You're fasting, with 24 hours of your
  vow to go: you leave the food be. ("break my fast" ends the vow.)".  Drinking isn't refused.  Hunger counts on as
  ever (the referee's), and there's no piety effect.  The vow ends with "break my fast", "stop fasting", or a plain
  "stop" when no long task is under way (H_STOP asks part 27's `fast_stop` first).  Once the day is out, the next meal
  says "Your day's fast is over." and goes ahead.  The vow is kept in the UI's main RAM (4 bytes); as built it wasn't
  in a saved game.  *The user's, 2026-11-11: a vow in force is carried in the save,* through save and restore and the
  time between dungeons, till its day is up or "stop" (below, "The user's review of five defaults").
- **Swim** (the plan's default, for review):
  - With a way or a thing ("swim north", "swim to the far bank") it is a move, `H_GO`'s, and the world's water rules
    do the rest (wading, swimming out of its depth, `P_SWIM`).  The UI knows only the water where the character stands
    (the hub's `pc_wet`), not the square beside it.  So "swim north" from dry ground walks north and swims if the water
    there is deep, rather than asking whether deep water is beside it.
  - "swim" alone, or "tread water", in water five feet deep or more treads water ("You tread water."), a minute.  The
    world (`I_WAIT`'s new a bit, `WA_TREAD`) tests the minute's swim at once: dq0w.c's part 73, `swim_still`, makes
    wade_t's test due.  A critical failure goes under ("You go under!"), and under, a tread comes up ("You come up,
    gasping.").  This is HOUSE.md's "not yet" of a swim's test as the character treads water, built for this verb
    only; an ordinary "wait" in deep water still isn't tested.
  - "dive" or "swim down" goes under on purpose (`WA_DIVE`: the minute's test first, then under, the breath held a
    minute, `WB_UNDER`, the work three times a swim's).  It is a timed work of a minute (reach 3), and at its end the
    character strikes out for the surface: a tread of moments, whose test brings it up or keeps it under.  Under past
    its breath, it drowns, as a sinking does.  "swim up" when under is the same tread.
  - With shallower water or none: "There's no water deep enough."
  - The parser: "dive" is swim's word (commands.verbs' [words]), and "tread water" a two-word swim; the grammar has
    both.  The handler tells a dive by the line's "dive" or the way down.
- **Follow** (the plan's default, for review):
  - "follow the X" trails a denizen it sees (the resolver's square, matched to the sight's record: its actor's `den`
    kept).  It walks after it with the world's close-in, walked alone (`I_CLOSE` w1 1, the world re-aiming it as the
    denizen moves), and its command is carried out again as each walk ends (main.c's reach 1).
  - Caught up, it waits a round beside the denizen, and goes on if the denizen goes.  If the denizen stays, the follow
    ends: "You stop beside the giant rat."
  - A walk that got nowhere (no way, or no nearer) ends it: "You can't keep up with the giant rat."
  - Out of sight but within six squares (gone behind it), the character turns to look for it once.  Not found then,
    or further off, or on another stratum, it's lost: "You've lost the giant rat. ("track" may find its trail.)"
  - A thing that isn't a denizen: "The sack isn't going anywhere."
  - "follow me" is the ally's order still.  The parser now gives every "follow X" `VB_FOLLOW_OBJECT` (commands.verbs'
    `thing=`), and the handler sends "me" (and nothing named) to the sway, as `VB_ALLY_COME`.  "follow" alone is
    `VB_ALLY_COME` in the parser as before.  The grammar already had "follow X".
  - *For review:* the world's "You can't get near enough to the giant rat." is told on each leg that falls short of a
    denizen that keeps its distance.  "tail" stays the grammar's alone: a [words] alias would have been a parser
    record more.
- **The parser's records**: 253 of 255, as before.  "dive" shares swim's record, and the two-word phrases ("tread
  water", "break my fast") are verbs of several words (119 of 255).  The corpus is unchanged at 645 of 660.
- **Room**:
  - The banks were full: part 27's 3.1K fitted in none of the 24 overlay banks.  So there is a 25th (dq0.cfg's OV25,
    bank.s's OVMAX, dq0.h's ovbank), and part 27 wishes for it.  The packer put part 3 there too.
  - Part 2 has 102 bytes left (110 before).
  - dq0's main RAM has 2,202 bytes free (2,272 before: the stubs, the new part's record, the vow and the follow's
    state).  The world's has 2,477 (2,483 before: `swim_still`'s stub).
- **Tests**:
  - dq0pray: Peoni's hour twice, the clock an hour on each; the vow, eating refused, "stop", "break my fast",
    "stop fasting" with none; no deity's prayer.
  - dq0tread: "swim" out of water and on the landing refused; a tread; a dive and its coming up, the world's beats;
    "tread water"; "swim south" out.
  - dq0follow: "follow me" to a befriended rat, the ally's order (SW_ORDER OR_FOLLOW, SR_OK); "follow the sack"
    refused; a shy rat followed, walked after, and lost.

## Worn places (2026-11-10, as built, for review: the user's, "Wearing should extend to rings, necklaces, earrings, and other jewelry, as well as scarves, and should keep track of where worn")

The user's words: "Wearing should extend to rings, necklaces, earrings, and other jewelry, as well as scarves, and should
keep track of where worn (as should all worn items that can be worn in more than one place)."  The user's decision:
rings by the hand only (left or right), up to four a hand, not by the finger.  The design's part (A) (the worn places;
the bandages and the materials are other work).  What's built:

- **The place is the item's own, its sn.**  A worn thing's sn was used by nothing (a held one's is its hand, IH_*; one
  on a square's, its stratum).  A worn thing of a kind worn in more than one place keeps its place there (items.h's
  WP_*): a HarnMaster location (harn.h's HARN_SKULL ... HARN_FOOT, the low four bits), WP_RIGHT its right side (harn.h's
  HARN_RIGHT, 0x80), WP_PAIR (0x40) a pair (earrings), and WP_AT (0x10) its place given.  WP_AT is the build's, not the
  design's: an outfit's worn thing is made with sn 0 (outmake.c's: its hand, none), which is HARN_SKULL, "on your head";
  without the mark a scarf drawn into an outfit would be on its head.  An sn without it (0x20 clear) is its kind's own
  place.  outmake.c was left as it was: its part (58) has 4 bytes free.  Nothing in the save changes size.
- **The table is kinds.c's** (its wplaces and wroom), each multi-place kind its place unsaid, the locations it may be
  worn at (a 16-bit mask), and each location's room (of everything worn there with a place): a ring the hand (four a
  hand); a brooch the breast (two), a shoulder (one), the neck; a necklace and a pendant the neck (three); a talisman the
  neck, a wrist (two); a scarf the neck, the head (one), the face (one), the waist (one); earrings the ears (a pair: the
  head's location with WP_PAIR, so a scarf on the head and earrings don't take each other's room).  The table is linked
  into part 107's image (its rodata's segment), not the catalogue's bank: that bank's code was 29 bytes from its end.
  Everything else worn (clothes, armour, a cloak, the mask, a pack, a belt, a bandolier, a slung shield or bow) keeps
  sn WP_NONE and the one-of-a-kind rule as it was.
- **The rule's in a swapped part, reached by its stub.**  things.c's don (the catalogue's bank: 6,371 of 6,400 bytes)
  lost its one-of-a-kind loop to part 107's worn_at (wworn.c: worn already, SA_WORN; one of a kind for the rest; the
  place said checked and its room counted; unsaid, the kind's own, a ring's the off hand's and then the other's), and
  sets the item's sn from its answer: the bank's code 6,345 bytes now, the four new kinds' throws included.  Its refusals
  are the part's own words: "There's no room left on your left hand for the ring.", "There's no room left on your hands
  for the ring." (unsaid, both full), "You can't wear the ring about your neck.".
- **Defaults when no place is said:** a ring the off hand (the referee's limbs: the left, the right if left-handed),
  then the main hand when the off hand has four; a brooch the breast; a necklace, a pendant, a talisman and a scarf the
  neck; earrings the ears.
- **Phrasing.**  The grammar's `<worn-phrase>` (commands.ebnf: on, over, around, about, at, in; "my"; left or right; the
  part, an open noun, or "head", "hand", "hands", "face", which are the grammar's own words) after `<equip>` and `<put>`.
  The meaning the parser makes of it is uneven ("over" and "head" are little words; "wear the scarf over my face" makes
  the face the thing), so the UI reads the place from the line as tend.c reads "left" and "right": worn.c's worn_said
  (overlay 14, sway.c's place_said's first): a wear, a don, a put on or a put whose line has " my " then (left, right)
  a part of the body (head, ears, ear, face, neck, throat, shoulder, breast, chest, waist, hands, hand, finger, wrist) is
  a don there, its thing's word the one before the preposition if the parser took the part for it; the other the place
  in the world's words ("@", the location as a letter, the side l, r or -).  "put the ring on my pack" is still a put.
  main.c's hand_said no longer makes "on my left hand" a hold (a line with prep on).  "don" and "put on" take a place as
  "wear" does (their aux IO_DON; wear's DN_WORN refuses a weapon as before).
- **Display.**  The inventory: ", on your left hand", ", about your neck", ", in your ears", ", over your face", ", at
  your breast", ", worn" for the rest; alike things worn in one place together ("(j) four rings, on your left hand"),
  in two places apart.  A look: "Your scarf is worn on your head."  A denizen examined: "It wears earrings in its ears, a
  ring on its right hand, a belt ...".
- **Which, by place.**  "take off the ring" with rings worn in two places or more: the world's which (IR_WHICH, as a
  take's), each place's first by its id, its state WS_AT and the place's words after it; caps.c words it, "Which one? (E)
  a ring, on your left hand, (F) a ring, on your right hand", and a capital, a number, or "left"/"right" answers.  For
  it, the UI's first words no longer fix a take off's thing by its own choice (sway.c's bind: not for IO_DOFF), and both
  choosers (the world's pick_, the UI's ix_pick) put a worn thing last for a wear and an unworn one last for a take off:
  "wear the ring" with one worn takes another, not "You wear the ring already.".
- **Denizens.**  A denizen's thing's sn is the stratum it's filed in, so its place isn't kept: it's drawn from the
  thing's own number (its id) among its kind's places, and its side by the id's second bit; the same each time.  The
  outfits' jewelry worn (jewel@worn, the talisman, the ring of rank) shows at its place.
- **kit.dat** keeps the place in its ofit's in (meaningless for a worn thing): WP_KIT (0x80) | the location | WP_PAIR,
  0x20 its right side; never a container's entry (NOFIT, 64, at most), so outmake, wfetch's fit and dq0acct read it
  as none.  Part 63 lays it on the thing as the kit's carried on.
- **New kinds** (items.h's, after KI_COPY; kinds.c's): scarf (6d, worn; MT_CLOTH till the materials' split: wool),
  earrings (40d, silver, a pair, worn), linen sheet (24d; MT_CLOTH: linen), towel (4d; MT_CLOTH: linen).  outfits.txt:
  a bandit's bed a linen sheet (40 in 100), a bandit lord's and a priest's always; a scarf for a bandit lord (30); earrings
  for the people of standing (25), lords (20) and a bandit lord (25), a scarf for the people of standing (30).  A
  bandit's scarf was left out: its section's 255 bytes (outdraw.c's) were 5 over.  town.txt's market sells the scarf and
  the towel.
- **Left for review:** veils and kerchiefs (optional in the design) aren't added; a brooch at the shoulder and a talisman
  at the wrist are the house's; the place isn't changed by wearing a thing worn already somewhere else ("You wear the
  ring already."): take it off first.  Taking off by place ("take off the ring on my left hand") isn't read: the which
  asks instead.
- **Room:** dq0's main RAM 2,262 bytes free (2,272 before), dq0w's 2,408 (2,483); part 107 2,167 bytes of 3,648; overlay
  14 153 free.  The build's packer moved parts among the banks to make room.
- **Tests:** dq0worn (four rings on the left hand by wear, put on and don, a fifth refused; a ring at the neck refused; a
  ring unsaid on the right hand; a scarf on the head, a necklace around the neck, earrings by default; the inventory and
  a look by place; "take off the ring" asks which by place, "right" answers; the scarf over the face), dq0wornkit (a
  ring on each hand and a scarf over the face kept in the save and into the next dungeon), dq0wornden (a bandit's
  jewelry examined at its places).

## Words said, a knock, the minds swayed (2026-11-10, as built, for review: the verbs plan's batches B2 and B3)

Six of the stubs (say, knock, wake, taunt, orate, pretend) now do a thing in the world, the user's rule.  Their words are
minds.c's (the UI's part 27, a part of its own in a new bank, overlay 25: the UI's banks had nowhere 3.8K bytes left,
part 2 only 110), the minds the denizens' (dq0d.c's part 24, `s_minded`, new: parts 2 and 16 full), the noise the world's.

- **I_NOISE, the world's** (part 37): the intent made the denizens' E_NOISE (its loudness, what it is, its square or
  the character's), two seconds' work.  The denizens' `hear` took the character's noises already: each actor in hearing
  (the senses' sound layer, a sleeper's hearing halved) turns to it and stands a while, and a sleeper that hears it is
  woken (its rouse).  New: words or a knock (E_NOISE's b `NZ_VOICE`, `NZ_KNOCK`, hub.h) draw a foe that hears them on
  to where they were made (drawn: it comes to look).  The sound task plays no sound for a voice (dq0s.c), a knock a blow's.
- **say, yell, whisper** (and a line in quotes: y"..." yelled, w"..." whispered, one ending in ! yelled): the words said
  back ('You say, "hello there"'), the noise 3, 6 or 1.  The words are the line's (cl): the parser keeps none.  A
  denizen awake, near (within the loudness in squares) and facing the character, is asked as a talk (SW_TALK, the one
  meant by den_meant), its words said only if it's taken so (a beast, a tongue not shared, as far as it goes: nothing
  said).  "say" and "yell" with no words the grammar doesn't take, as before.
- **knock**: on a door (a portcullis, a window), walked up to and faced first (feature's), "You knock on the door.",
  noise 3, heard beyond it by the sound layer, so a foe comes to look.  Along a wall ("knock on the wall", and the
  grammar's new "knock on the north wall": commands.ebnf, commandcompile.js), a short search by ear: the world's wall
  search, half a minute, its b `SF_KNOCK`, the referee asked P_NOTICE_SOUND for each hidden door and door trap in reach,
  not P_SEARCH (the world's `srch_p`, set by `work`: wtrap.c's part 8 had 10 bytes); noise 2.  On a thing to hand a
  rap (noise 2); else "You can't knock on ...".  For review: a door hidden by magic is tested by ear as any other (the
  referee bars only P_SEARCH from those; no room in the world's part 8 to bar it); a wall's side is the heading rounded
  to the four ways (the search's own rule).
- **wake**: "wake" alone asleep is a stop (task.c's `free_cmd` takes it at once, as a stop); awake, "You're awake."
  "wake the bandit": the one of that name beside the character; seen further off, walked over to first (the reach's,
  the command again as it's there); then SW_WAKE: not asleep, "The bandit isn't asleep."; asleep, roused up and
  watching, turned to the character, drawn on to it, and its attitude decides what follows: a foe beside it strikes
  ("You shake the bandit awake. It comes to with a snarl, and turns on you!"), wary and worse "eyes you warily", the rest
  "blinks up at you".  No test: a shake wakes anyone.  (Not tested: the walk over; in the test's scene the knock had
  woken the far sleeper first.)
- **taunt**: SW_TAUNT, a test of Rhetoric (P_PERSUADE; a beast's Animalcraft: one can goad an animal), no tongue wanted
  (jeers carry, as threats do), within earshot (12 squares, a beast 8), at steps the easier the less it likes the
  character (indifferent 0, unfriendly +2, friendly -1, an ally -2), its kind's sway, a low mind +1, a cunning one -1;
  hostile already, as far as that goes.  A success makes it hostile, an ally no more, drawn on to the character from its
  post (a foe drawn on is before its day's doing in its mind); a failure leaves it as it was ("It ignores your jeers.";
  a critical failure "It only laughs at you.").  For review: the book's contest is opposed; here the character's test at
  steps, as persuasion is.
- **orate** ("orate", "address ..."): noise 4, then one Oratory test (P_ORATE) in the tongue of the nearest minded one
  awake in earshot (common to a leader, as words are); a tongue the character doesn't speak, the sway's refusal.  The
  grade is applied to every minded one awake within 12 squares whose tongue it is, as a persuasion's: a critical success
  two steps, a success one, a critical failure a step back, never past friendly (a speech wins no ally): "You hold forth
  to those about you, in common. Your words carry: 6 of them seem the friendlier for it."; half a minute.  None to hear,
  "There's no one here to hear you."  (The hub's sway: SW_PERSUADE with order `SWO_ALL`, the reply's ix those swayed, iy
  those it reached, word2 the tongue.)
- **pretend dead or asleep** (the plan's default, for review): a pretence naming death, sleep or senselessness lies
  prone (I_RISE, unless prone already), then an Acting test (P_ACT) for each foe (unfriendly or hostile) awake that
  senses the character, the cunning -2, a low mind +1, the mindless +2 (SW_DECEIVE with `SWO_ALL`); then a wait lying
  still, ten minutes.  The fooled take it for helpless (HOUSE.md's rule, `decide`'s: killers strike, robbers rob, the
  rest leave) while it lies prone on that square in its wait (dq0d's `pcfeign`); moved, up, the wait over or stopped,
  the pretence is over till it's feigned again.  Foes that come later, or weren't fooled, see through it.  "The bandit
  is taken in." / "It isn't taken in." / "No foe is watching you."  Any other pretence ("pretend ignorance") is a
  deception of the nearest (deceive's own words).  For review: the book would have Acting against each foe's Awareness;
  here the character's test at steps.
- **The shared files**: hub.h (`NZ_VOICE`, `NZ_KNOCK`, `SF_KNOCK`, `SWO_ALL`, I_NOISE's comment), msgs.def, cmds.h
  (`minds`), dq0.cfg (OV25, `#@SW27`), bank.s (OVMAX 25), dq0.h (`ovbank[25]`), parts.txt (27), dq0d/parts.txt (24);
  dq0d.c's `cap[]` comment (wake and taunt aren't influence's), `s_sway`'s SW_WAKE, SW_TAUNT and `SWO_ALL` sent to
  `minded`.  sway.c's `tried[]` stays to SW_BEFRIEND: its words are never asked for wake or taunt (minds.c words a reply
  SR_OK itself, the sway's words only its refusals).
- **Room**: dq0's main RAM 2,260 free (2,272 before), part 27 495 of 4,352 free, overlay 25 1,269 (the map's part 3 moved
  there by the packer); dq0d's main RAM 2,184 (2,457: `hear`'s drawing, `decide`'s fooled, the pretence's state), its
  part 24 2,026, part 2 18; dq0w's 2,482 (part 8 3, part 37 1,869); dq0harn's 2,169, unchanged.
- **Test dq0noise**: say taken as talk, a whisper heard at 1, a yell at 6 by the bandits about, a quoted line, a knock
  on the door heard at 3, a knock along the wall, "wake" awake, ignorance feigned (a deception), death feigned (the
  bandit taken in, robbing the "body"), a hostile sleeper shaken awake, a taunt, a speech; seed 7's hall, its doors made
  secret, the door found by knocking along the north wall, and a speech with no one to hear it.

## Cloth, bandages and dressings (2026-11-10, as built, for review: the user's bandages, the worn-places-and-bandages design's parts M and B, its rip and wash)

The user: "A bandage's location should be known, and different material should have different suitability as bandage
material.  'rip' and 'tear up' verbs should be allowed to turn a garment, bed sheet, towel, etc. into bandages ...
differing effectiveness (for material type and cleanliness)."  Decided with the user: no cotton; dressings soil over
days and are changed; wash, not boil (boiling when there are fires).  HOUSE.md's bandages and dressings has the rules as
built; this is how, and what's the house's for review.

**What the code said before** (checked): heal.c's treat and bonus asked only `has ("bandage")`, the table's bonus
whole or none; the reply's "\nbandage" made tend.c ask the world's IO_USE, s_use_key using up the first lot keyed
"bandage"; harn_heal_roll's bonus always 0; join merged any two lots of a kind whatever their material or wear;
nothing kept a cloth's cleanliness; bandages one kind, MT_CLOTH.  s_tools sent each tool's quality as its digit.

- **Materials** (items.h): MT_LINEN, MT_WOOL, MT_SILK after MT_GOLD (the numbers before kept: the outfits' file, the
  stores' records); MT_CLOTH is hemp and sacking (its name "hemp": appraise.c's and things.c's).  Linen: shroud,
  bandages, surcoat; wool: cloak, blanket, robe; hemp: sack, rope, and what wasn't named (clothes, a gambeson, paper,
  a net, a letter) *(for review: clothes and the gambeson might be linen or wool)*.  The outfits' file may name them
  (outfits.js's MATS).  The worn places' scarf, earrings, linen sheet and towel are another branch's kinds: their
  materials and their yields (wcloth.c's table, commented) on the merge.
- **Cleanliness is the wear byte**, each band one value as the bandages have it (CL_WORN 64, CL_SOILED 192,
  CL_BLOODY 255); join's guard: bandages join only of one wear and one material.  Where a cloth's comes from, the
  house's: found lying in the dungeon (make_own: linen, wool, silk) soiled; a garment torn up worn at best; a shroud, a
  denizen's bedding, soiled.  *Not built*: don marking a garment worn (don is in the catalogue's bank, 29 bytes free,
  and the worn places' branch is in it), a denizen's outfit worn as it's made (outmake's part 58 full): the tear's
  rule stands in for both.
- **The grade to the referee**: s_tools' digit for a bandage lot is CL_GRADE (items.h: its suitability by its
  cleanliness), not its quality; the referee's bonus (dress.c's) is the table's times the best bandage's grade over 9;
  the reply's key carries the grade ("\nbandage9"), and s_use_key reads a digit after a key as the grade wanted (a lot
  of that grade alone; "water", "food" as before).  A bloody bandage's grade is 0: never used.
- **The dressing per wound** is the referee's (pc_dr, a byte a wound: its grade plus 1, DR_SOILED once the days have
  soiled it), kept in step wherever pc_by is (sheet.c's room, heal.c's by_drop), saved as a "dressed" line, each
  dressing its grade and its wound's location ("dressed :ag"), read back onto the first undressed wound there: the
  account (dq0acct) keeps the line as it is, and its days at rest heal wounds and move them down, so the location,
  not the place in the list, finds the wound.  As built the days between dungeons didn't soil it, and the sheet's
  words were "dressed, clean", "dressed", "dressed, poorly", "dressed, soiled", the cloth not named.  *The user's,
  2026-11-11: the sheet names the cloth and its cleanliness ("dressed in linen, soiled"), and the days between
  dungeons soil it as the dungeon's do* (below, "The user's review of five defaults").
- **Soiling** (heal.c): at each day's mark (after that day's rolls) a dressing on a serious or grievous wound three
  grades the worse, told once it's soiled.  The healing roll's bonus +5 (grade 7-9), 0 (4-6), -10 (0-3): a poor hemp
  dressing counts as a soiled one.  Creatures: the treatment's bonus alone, no dressing kept.
- **Changing** rides first aid: tend.c sends the part said with a * before it when the line has "change"
  (commands.verbs' "change the bandage on", "change my dressing" ... are tend's words: several words each, none a
  record of dq0c's); the referee's plan TR_CHANGE (RS_TREAT's c 5), five minutes, no treatment roll; its reply's key
  "bandage9+", the + the old dressing's: s_use_key makes a bandage of the lot it used from, one, bloody, beside it (at
  the feet if the lot was in hand).  A wound tended without a bandage can be dressed this way too ("You dress ...").
- **Rip** is one verb, VB_RIP_OBJECT (commands.actions' last line, actions.c's last row: H_HANDLE), aux IO_RIP; "tear",
  "tear up", "rip up", "cut up" its words.  **Wash** is the same verb, "wash = rip", its request the word's (hthings.c:
  the line has "wash", IO_WASH), as grapple's moves are: dq0c's word records are at 254 of 255 with rip's, and wash's
  own verb would be the last.  The world's IO_RIP and IO_WASH are part 107's (wcloth.c, s_textile): the yield table,
  a blade (the one said, else any carried: a knife's or a sword's family), a minute a bandage with one and two by
  hand, wool and a gambeson wanting one; the time a timed wait (the handled things' x), not a long task *(for review:
  a big sheet's sixteen minutes can't be stopped)*.  Wash at running water or a well beside, three minutes, worn.
- **For "use"** (B4's H_USE, another branch): "use the knife on the cloak" is `handle (IO_RIP, "cloak", "knife", 0)`
  (s_handle, the UI's part 2); "wash" `handle (IO_WASH, "bandages", "", 0)`; "use the bandage on my arm" is first aid:
  cmd.action VB_TEND, the part in cmd.obj.word ("arm"; the line's "left", "right" and "change" read by tend.c from cl),
  then `tend ()` (s_tend's stub, part 12; s_early's 1 for VB_TEND, its own patient found).
- **Room**: the referee's first aid (remedy, bonus, at_part, to_treat, treat) moved from heal.c (overlay 6) to dress.c
  (overlay 9) with a copy of the library's Treatment table, and sheet.c's o_spent to overlay 9 (overlay 1's room for
  the dressing's words and room's step); two new stubs (treat, dress_line).  Main RAM: the referee 2,135 free (2,169
  before), the world 2,448 (2,483), the UI 2,272 (unchanged); dq0c's records 254 of 255.
- **Tests**: dq0rip (a worn robe, a leather vest, a rope refused; a wool cloak without a blade refused; a linen
  surcoat torn by hand into 4, eight minutes; the cloak cut up with the dagger into 6; two worn lots), dq0dress (a
  dressing, the sheet's "dressed, clean"; another wound's soiled dressing from the file, "dressed, soiled", changed,
  a bloody bandage in the pack (the days' soiling dq0rules': the denizens wake a sleeper here); wash refused with
  no water, done at running water, worn), dq0rules' dressings (the bonus by linen, hemp, soiled linen, none; the
  sheet's words; a day's soiling, two; a change and its refusals; the five-day roll's +5 and -10).

## The things' verbs: use, push, pull, touch, taste (2026-11-10, as built, for review: the verbs plan's B4)

Five rows that said "not yet" (B0's stubs in hthings.c) now do something in the world.  The user's rule: each verb a
meaningful action; push and pull include moving furniture.  What's built:

- **Use** (and "activate", its alias).  The theme's puzzle is still asked first (theme.c's o_puzzle: its rules for the
  thing, or for the thing on another; the way out).  What it has no rule for is no longer "Use what, and on what?" nor
  "Nothing comes of using ..." but the handler's, by the thing's kind (the things' IO_FIND): a bandage tends (the
  thing it's used on its patient or its place: "use the bandage on my arm" is "tend my arm"); food is eaten; a
  waterskin, a vial, an antidote, a remedy is drunk; a light is lit; a tinderbox lights what it's used on; a key or the
  lockpicks unlock what they're used on, none said the door (the one there is, or "Which door?"); a rope or a grapple
  is tied (to what it's used on, or what's nearest that holds); a whetstone sharpens what it's used on, none said the
  blade in hand ("Sharpen what?" with none); a thing held in a hand (a weapon, a tool) is wielded.  Anything else:
  "You can't see how to use the X.", or with another "Nothing comes of using the X on the Y."  Each is that verb's own
  command carried out (hthings.c's as_verb: the action changed, its row's handler called), so its words, its timed work
  and its walk over first are the verb's.  **Rip is the materials' batch's:** a TODO at the head of the routing marks
  where "use the knife on the cloak" goes (VB_RIP_OBJECT, IO_RIP), before the kinds.  (Wired as the batches merged,
  2026-11-10: a thing but the bandages used on a thing whose kind is cloth, MT_TEXTILE, is the textiles' IO_RIP, the
  cloth cut up with it, wcloth.c saying what won't cut; dq0rip's "use the dagger on the towel".)
- **Push, pull.**  A door pushed opens, pulled closes (a window's shutters alike); a portcullis pushed is raised,
  pulled is lowered (the winch loosed); a door's bar pulled is lifted out (the word "bar" first: the resolver makes it its door) (PA_LIFT), pushed is set in its brackets
  (PA_BAR); a rope pulled is untied, so from below a grapple is shaken loose (wrope2.c's, HOUSE.md's rule); there are
  no levers.  Anything else named is furniture, the world's new **IO_DRAG** (dq0w/wdrag.c): a thing lying about whose
  kind is 40 pints or more (DRAG_BULK) and isn't fixed (a chest, a table, a cask, a body) is shoved a square away from
  the character (on its own square, the way it faces), or dragged a square toward it, the character stepping back a
  square (I_STEP, its facing kept).  Out of reach, walked over to first.  Strength against its weight, what's in it
  too (P_FORCE, two steps the better, a step the worse each 10 pounds: an empty chest one step worse), five seconds,
  a noise of 3 (E_NOISE, heard whether or not it moves); failed, "You strain at the chest, but it won't budge."  It
  goes only where a walker could step (step8's), not into a doorway, not onto anyone, not onto another heavy thing;
  smaller things: "The pot isn't a thing to shove about: pick it up.", a fixed one "The well is fixed where it stands." (each answer names its thing, the sweeps' rule)
- **A shoved thing blocks a door.**  A door is a T_DOOR square with its portal; furniture is an item on a square.  A
  heavy thing on either square that leads through a door (the four ways from it: the walls' have none) braces it shut:
  opening it, by anyone (the character, a denizen: wdoor.c's PA_OPEN, a new PO_BLOCKED, "Something heavy stands
  against it: it won't open."), or a known door opened as a walk goes through (dq0w.c's door_on), is refused till it's
  moved.  Told as it's shoved there ("It stands against the door.").  The maker's own chests and tables beside a door
  brace it too (a chest stands where it stands, whoever put it there): *the user's, 2026-11-11, reviewed and kept.*
  **For review:** forcing a braced door isn't
  refused (its lock or bar bursts, it stays shut); a heavy thing can't be shoved into a doorway, so passage is
  blocked only by the door kept shut, not by a chest in an open doorway (walks don't treat things as obstacles).
- **Touch** ("feel" is its word too).  In the dark, a wall, the floor or nothing named: the felt words, a feel about
  (H_FEEL's: "It is dark. You feel ... Your hands find ...").  Else the thing examined (H_EXAMINE), which the hands do
  in the dark as the eyes do in the light (its make, its state, its lock).  For review: its first words are the look's.
- **Taste**: a sip, not a dose (the world's IO_DRAG with DG_TASTE: the thing to hand, what's in it, that one's wear;
  nothing used up; told "You taste the X. ...": a thing in a pack or a sack isn't, only a vessel's liquid).  An antidote or a remedy: a Lore test (P_LORE) names it and what it's for ("a theriac, an
  antidote against poisons and venoms"; "a physician's draught, a remedy against disease"), failed "Bitter, and
  strange: you can't tell what it's for."  Water (a waterskin's, or the water it stands in): fresh, or "Flat and
  stale: still water, not fresh. It may carry the flux." (the water's wear, still water's mark, feed.c's).  Food:
  "still good" (nothing goes off in this game).  An empty vessel: "There's nothing in it to taste."
- **The flux:** as built, a taste of still water didn't risk it (a sip too little; tasting told, drinking risked).
  *The user's, 2026-11-11: a sip can bring on the flux,* at a sip's small dose (below, "The user's review of five
  defaults").
- **Room.**  The UI's handlers are part 29's (27 as built, the merge's number) (hthings.c, part 2 full: its five stubs one call now); theme.c's overlay 5
  smaller.  fixt.c's overlay 4 had a byte free: alias.c's o_alias_said moved to overlay 12 for PO_BLOCKED's words.  The
  world's IO_DRAG and the brace are part 109's (107 as built, the merge's number; wdrag.c, 2,600 bytes), in a new overlay bank, the world's 29th in
  dq0w.ovl (the rope file's three now OV30-OV32: OVBANKS 32; ropes_in reads the last three as ever): no bank of either
  file had room.  Main RAM free: the UI's 2,237 (2,272 before: the part's stub, things_use's), the world's 2,465 (2,483: the stubs drag, braced), the others as they were; part 2 169 of 659 (its five stubs gone), part 27 2,050 bytes; the world's catalogue bank untouched (things.c's serve a case more, part 92).
- **Test dq0things**: use routed to each kind; a door pushed open and pulled shut; a chest shoved against a door, the
  door refused, the chest dragged away and the door opened; a portcullis pulled down; a bar pulled out; a grapple's
  rope pulled from below; touch in the dark; taste an antidote, a remedy, an empty vial, and still water.
  dq0notyet's "activate the torch" now lights it (the use's).

## Charge, a throw a way, engage, a split attack, and again (2026-11-10, as built, for review: the verbs plan's B1 and B7)

Five of the stubs the verbs plan left saying "not yet" (B0's) now do something in the world.  What's built:

- **Charge** ("charge", "charge!", "fast charge"; hdens.c's `H_CHARGE`): the nearest hostile denizen the character sees ahead (ahead, or ahead to either side: REL8 7, 0, 1; seen, the senses' last look's) is run at.  A war cry first ("\"Charge!\"  You run at the giant rat."): `I_NOISE` with loudness 6 (a yell, SOUND.md's), b 0, the character's square; the world does nothing with it till the noise batch (B2) builds it.  Then the close-in (`I_CLOSE`, a blow as it gets there) with a new field, `run` (w2: msgs.def's `I_CLOSE ... w2 run`), the pace its walk is planned at: `GA_RUN`, "fast charge" `GA_SPRINT` (the world's `runs`, the flight's way of setting a pace for one walk; the player's own pace isn't changed).  The engage test is the walk's own, as it comes into the foe's zone.  One beside it already is struck at once ("You fall on the giant rat.").  None: "There's no foe ahead to charge." (the grammar's `ERROR:NO_TARGET`).  For review: only the first leg is at a run (a foe that moves on, the later legs at the character's pace: the world's part 24 had no room for more).
- **A throw a way** ("throw the dagger north", "throw the dagger 3 north", "throw the dagger hard south"): the parser makes these `VB_THROW_OBJECT` with a heading (no grammar makes `VB_THROW_DIRECTION`), so `H_MISSILE` passes a throw with a heading to `H_THROW_WAY`, whose row stays for the action.  The mark is that many squares along the line (none said, a dozen: as far as it'll go), then the world's throw at a place (`IO_THROW`, the referee's Throwing).  **The world stops a thrown thing short on its line** (things.c's `s_throw_at`, the character's throws: each square from it to the mark asked of `ln_pass`, a missile's: rock, a wall, a door shut, glass stop it; it falls on the last square it got to).  That's every throw of the character's, at a thing too (one seen through glass now falls at the glass); a denizen's throw is as it was.  Words: "You throw the dagger south: it lands 2 squares off.", "...: it strikes the wall and falls 4 squares off.", "...: it strikes the wall and falls at your feet.", ", but it falls short, 9 squares off." (past its extreme range).  ("The wall" for a door shut too, for review.)
- **Engage** ("engage the rat"): `I_CLOSE` with its walk alone (w1 1, "go to the rat"'s), no blow; the engage test the walk's; engaged by another on the way, the world's refusal (`TE_ENGAGED`).  Beside it already: "You're already engaged with the giant rat."  A door, a window, a portcullis: "You can engage only a creature."; none of that name: the fight's words.
- **A split attack** ("attack the rat and the goblin"; attack, fight, hit, strike): parts.c no longer cuts such a line in two at its "and" (a blow's verb first, a determiner after the "and"), and `parts_op` (its `PO_AGAIN` look at every command) makes the parser's attack in a line with " and " and no "with" `VB_ATTACK_SPLIT`.  The parser's thing is the last named (the second); the first is the word before the "and".  Both beside it: one `I_ATTACK` with a new field, `split` (w1: the second, plus 1), "You strike at the giant rat and the giant bat by turns."; the world's fighting on (`fsplit`) strikes at the other each round while both stand (its target down, the other's alone; the other down, its target's).  The second not beside it, the first is attacked alone; the first not beside it, "The rat is out of reach: go up to it first."  For review: a close-in to the first, then the split, would want `I_CLOSE` to carry the second too; two of one name can't be told apart by name (the second word finds the same one: the first's attack alone); "punch the rat and the goblin" is still two attacks (a blow's own word not among the split's).
- **Again** ("again", "repeat"; hself.c's `H_AGAIN`): the last line typed that didn't say "again" is run again, its parts queued first, before any line typed ahead, each carried out as if typed.  term.c's `last` (cursor up's) is `t_last` now, and a line with a word begun "again" or "repeat" isn't kept in it (term.c's `said_again`, overlay 16's), so "north then again" can't run itself for ever.  Nothing typed yet: "There's nothing to do again."  As built "g" stayed give's (commands.verbs' `g = give`); *the user's, 2026-11-11: "g" is again's* (Infocom's), give spelt out (below, "The user's review of five defaults").
- **Room.**  The handlers' work is part 24's (the commands seen to less often), through two stubs (`fights`, `again_go`): part 2 is 89 bytes from full (110 before); a third stub, `said_again`.  Overlay 6 is 67 bytes from full (183 before: parts.c's split), overlay 16 25 (88).  Part 24's state is the part's own (`bss-name` SW24RODATA: none kept across a call of another part).  The UI's main RAM 2,202 bytes free (2,272 before).  The world: part 24 (fighting on) full, the charge's pace set in part 37 (the intent's).  The UI's banks are close to full (some 4,000 bytes free over all, the most 514 in one): a later batch may need parts moved, or a bank more.
- **Test dq0fightverbs**: again with nothing, charge with none, the throws (two squares; the wall behind, at its feet), turn left then again and repeat (three turns), a charge (the war cry, the close-in at a run or the blow, a blow struck), engage and already engaged, a split attack (both struck at).

## The UI's room won back (2026-11-11, as built, for review: ENGINE-CRITIQUE.md's Â§1)

The UI (games/dq0) was at the edge: its main RAM 2,069 bytes free against the floor of 2,048, part 2 (the commands'
handlers) 102 bytes from the swap area's 4,352, its overlay banks some 4K free in pieces of 500 at most.  The ask: at
least 3,000 bytes of main RAM, a margin in part 2, and room in the banks for a new part of 3-4K.  Nothing the game
does or says changes; what's built:

- **The overlays' stubs 6 bytes, not 11** (bank.s's OVFN and its new ovcall): a stub is `jsr ovcall` and its two
  numbers after it (the overlay, the function's address), as the swapped parts' stubs have been since 2026-11-04;
  ovcall reads them off the return address, puts the overlay in (ovl_in's work, done in it, not called), calls the
  function, and puts the bank back (ovl_out), its arguments in .A, .X and its value as they were.  93 stubs, some 465
  bytes, less ovcall's 50.  The cost: some 30 cycles a call more (a stub's work was some 60 before, its function's own
  aside); with ovl_in called from ovcall it was 60 more, and the start at seed 123456789 took 5M cycles more, a sign how
  many calls there are.  The same in the referee's (dq0harn, 52 stubs) and the maker's (dq0m, 23) bank.s.
- **The status line out of the main RAM, into a 27th overlay bank** (dq0.cfg's OV27, filled; bank.s's OVMAX 27; dq0.h's
  ovbank[27]): main.c's status is o_status (its stub status, called as ever), term.c's o_t_status beside it, and the
  line as shown and as it's to be (sl, sn: 256 bytes) the bank's own data, last in it (OV27BSS: the overlays' check,
  dq0OvlBanks, stops there, as at OV22BSS).  An overlay's code can't read the hub in the window, so o_status reads its
  few bytes through hub_mem (theme.c's way, and bar_text's already); its left side is the character's name itself, not
  a copy (o_t_status is in the same bank, so the name's literal in it reads right); its right side stays in the main
  RAM (bar_text, overlay 3's, writes it).  A side effect, for review: called from overlay 3 (a pause, task.c's o_meta)
  the old status read the hub's fields from the window as it was, overlay 3's bytes, till the next second's redraw;
  through hub_mem it reads them right.  s_t_init's "draw it whole" is t_status (0, 0).
- **Buffers that needn't be the main RAM's:** s_size's 100 (the console's settings read as the screen's set up) on
  the C stack (called from the main loop and the start only, shallow); start's path (56) and hub_spawn's arguments
  (64) part 1's own (`bss-name` SW1RODATA: nothing else is swapped in while they're used; part 1 207 bytes free to 94).
  (character's two, part 7's, can't: it calls hub_run and t_init, part 1's, with them.)
- **Part 2's room:** the climb (s_climb, 229 bytes: hplaces.c, part 24's till stage 3) and the powers refused
  (s_powers, 73: hself.c) are part 24's, the commands seen to less often, through their stubs.  A climb or a spell
  refused swaps part 24 in and part 2 back (some 50,000 cycles, under a fiftieth of a second at 3.58 MHz); none of the
  timed commands (dq0answer's, the letters' tests') is either.
- **Measured** (the build's room table):

  | | before | after |
  |---|---|---|
  | dq0's main RAM | 2,069 | 3,378 |
  | part 2 | 102 | 404 |
  | part 24 | 1,520 | 1,218 |
  | part 1 | 207 | 94 |
  | overlay 16 (o_t_status's till now) | 842 | 1,179 |
  | overlay 27 (new) | - | 7,032 |
  | the overlays' free bytes, all told | 9,834 | 17,090 |
  | dq0harn's main RAM | 2,135 | 2,341 |
  | dq0m's main RAM | 2,879 | 2,940 |

  The bank packer placed some parts' images anew as the banks changed (the room table's "parts not in parts.txt's
  banks"), as it does.  The world (2,336), the denizens (2,184) and the senses (3,122) have no overlay stubs; their
  room is unchanged.
- **Not done, for later:** the swap area (4,352) is the biggest part's, part 4's (s_pace_cmd, one piece of 4,339, its
  pointer argument pinning it), so it can't shrink without splitting that function by hand; main.c's events and main
  loop (2,240 bytes) read the hub at every turn and stay; names.c's helpers shared by parts 5 and 6 (some 700) would
  want part-local copies (Â§1.3) and part 5 has 113 bytes free.
- **The start** (dq0starttime): seed 1 234.7M cycles to the first prompt, its budget 235M; seed 123456789 310.7M, its
  budget 315M.  The 27th bank's bytes are read in as it starts, and the stubs' calls cost a little more: seed 1 is
  0.3M from its budget, so the next bank added (or much more code in this one) will want the budget raised, which the
  user's said is fine (the start's time isn't a concern).
- **Tested:** all 442 tests pass.

## Narration as data, stage 1 (2026-11-11, as built, for review: ENGINE-CRITIQUE.md's Â§3.2 and step 5, its first stage)

The words of what came of an action were written by hand, a function for each feature (33 of them), each its own
switch, its own strings and its own state.  This stage builds the generic mechanism, the critique's outcome event, one
renderer and a phrase table, and moves the first five wording functions onto it.  The words the player sees are the
same, letter for letter.

- **The outcome event (msgs.def's E_OUTCOME).**  Not a new kind but a head, as I_DEN is the denizens' intents': the
  outcome events include it (`E_DIG < E_OUTCOME; ...`), and the action is the event's kind.  Its fields: `who` the
  actor (the struct's own); w1's high byte the outcome, its low byte n1; b n2; x[0-1] the thing done to (obj) and x[2-3]
  the thing done with (instr), as handles (a thing's id, 0 none).  This is where E_DIG, E_CLIMB and E_TRAP already kept
  their outcome and their number, so those three needed nothing from the world; E_PORTAL's w1 was the other way round
  (PO_* low, the portal's kind high) and is now `pk:0-6 shows:7 po:8-15` (hub.h's PO_SHOWS 0x80, not 0x8000; its
  senders: dq0w.c's three and its revealed, workpart.h's tail and its told's denizen branch (`wk.out = w1 >> 8`),
  wroom.c's and wtrap.c's searches).  A kind names the head's fields its own way: hubgen.js lets a form's own field take
  the place of an included one in the same slot and within its bits (E_DIG's `how` is its outcome and `feet` its n1;
  E_TRAP's tk, tt and touch split n1; E_PORTAL's b is `room`, its search form's `touch`), so the tests read them by the
  names they always have (dq0dig's `how`, dq0portc's `po` and `pk`) and no test changed.  The JSON's new `outcome` lists
  the kinds that include the head.  E_PORTAL's a, b and w2 are declared now too (an opening's `what`, `lit`, `room`,
  `far`; a search's `reach` and `touch`, a form by its po).  **Choice, for review:** the action is the kind, not a byte
  of its own in a single E_OUTCOME kind.  That keeps the tests, the denizens' and the sound task's switches as they were,
  and leaves the struct's bytes for the handles; a new action is a kind (an enum member in hub.h) and a msgs.def line.
  **The handles are always 0 so far:** the world's dig works with the referee's tool names, not item ids, and a portal
  has no handle yet (COMMANDS-CRITIQUE.md's Â§4.8: features lettered later); the UI gives the words for them (below).
- **The phrase table (games/dq0/said.txt), made by the build** (games/dq0/saidgen.js, called by build.js's hubNumbers
  after hubgen.js, into obj/gen/dq0said.h; made again only when said.txt, saidgen.js or the headers' numbers change).
  A row is `KIND.OUTCOME[COND] "text"`: `DIG.begun`, `PORTAL.opened[pk=PK_WINDOW,PK_SLIT]`; the outcome's name is its
  enum member's (msgs.def's `@D_BEGUN` gives the prefix); a kind's own outcomes, rows the UI chooses itself, are declared
  (`own CLIMB rises falls`: SAID_CLIMB_RISES, a ledge come to).  `+"..."` says on after what's said, else the row is a
  line of its own (msg's); `once` says it as a look's sentences are.  In the text, in braces: `{feet}` a field by its
  msgs.def name (`far*SQ_FT`, `feet-121`, `feet>120`), `{feet?a|b|c}` a choice by its value, `{obj}` `{instr}` `{text}`
  the UI's words (capitalised `{Obj}`; `{instr?a|b}` by whether one's given), `{actor}` `{Actor}` `{s}` "you" or the
  denizen and the verb's ending, `{arg?...}` the UI's choice, `{a}` "a" or "an" by the word after it, `{rel F}` a
  heading from the way the character faces, `{room F}` `{light}` the theme's names, `{trap F G}` a trap's name, `{F!...}`
  said only as F changes (a dig's feet to go), `{forget}`, `{nl}`, and `{@NAME}` a piece (`@reveal`, "..., revealing a
  passage, 30 feet of it", the four openings' and the windows' rows).  The critique's `%`-names became braces: a
  template's letters and its codes are told apart without escapes, and choices nest.  123 rows, 11 pieces.
- **Packed:** a kind's rows its outcome, a condition (a field and the mask of its values), a flag and its text, the
  text's codes a byte each (a field by its place in a table of its byte, shift, mask and operation, so the renderer
  reads any kind's fields without knowing the kind), its pieces apart and called; and the 26 words most said (" the ",
  "portcullis", "shutters", " already" ...), chosen by the build by the bytes they save, a byte each.  7,107 bytes
  without the words, 5,153 with them: the overlay's room, and the bytes read in as the game starts.
- **One renderer (games/dq0/said.c, overlay 27: outcome_words).**  It finds the kind's row for the event's outcome (the
  first whose condition holds, then the one with none; none, nothing said), writes the text into lb, and says it with
  say, a `{nl}` as msg would, or with lb_said for a `once` row (lbsaid.s's no-sentence-twice rule a row's flag; no row
  moved yet wants it).  The UI's words for a row are `said_s` (obj, instr, text) and its choice `said_n`, in the main
  RAM, set by the caller; a field's last value for `{F!...}` is the main RAM's too (4 bytes, zero as the game starts, as
  dig_words' dig_feet was).  What's worked out for a row stays with its caller, the part that's truly the feature's own:
  main.c's s_news sets the dig's tool (dug_with) and the tool newly taken up (dig_with, "bare hands" its hands now, set
  in s_dig_used), and for a dig that won't begin which of six reasons (said_n, by what it carries: s_dig_used, part 14,
  which already refreshed the tools' list for it) and the better tool's name; a soft thing's name for a blow (PO_SOFT);
  a ledge's rise made a CLIMB row (`rises`, `falls`).  E_TRAP's words are said from the main loop as trap_words' were
  (untarget set there).
- **Moved and deleted:** dig_words (intro.c, overlay 19), climb_words (intro.c, overlay 12), portal_words (intro.c,
  overlay 10), fixt_words and its shows (fixt.c, overlay 4), trap_words (fixt.c, overlay 10): five of the 33, their stubs
  gone (dq0.h's CW_RISES and CW_FALLS too).  intro.c keeps the title and harm_words; fixt.c the portal looked at and a
  denizen in the way.  **The world's told copies:** none could go yet.  Each sends kinds not moved: workpart.h's (in five
  parts) sends E_NOISE, E_HARM and E_BEAT besides E_PORTAL and E_TRAP; wdig.c's and wport.c's are E_DEN (a denizen's
  digging, its door work: "denizens worded alike", stage 2), wfetch.c's and wshoot.c's (with told68) E_TASK, wfight.c's
  E_FIGHT.  Their only change is E_PORTAL's w1.
- **Room.** Overlay 27 is the renderer's and its table's: said.c some 2,400 bytes of code and the table 5,500, 282 bytes
  free.  The status line (main.c's o_status, term.c's o_t_status and its two 128-byte copies) moved from it to overlay
  19, into the room dig_words left (OV19BSS, its data last, dq0.cfg's; the stubs' overlay 19): **for review**, it was
  put in 27 this morning; 27 is the narration's now, for the stages after this.  The build's room table:

  | | before | after |
  |---|---|---|
  | dq0's main RAM | 3,378 | 3,399 |
  | overlay 27 | 7,032 | 282 |
  | overlay 19 | 297 | 1,452 |
  | overlay 4 | 54 | 594 |
  | overlay 10 | 304 | 49 (part 18's image put there by the packer) |
  | overlay 12 | 1,263 | 236 (part 17's image put there) |
  | the overlays' free bytes, all told | 17,090 | 18,138 |
  | part 14 (s_dig_used) | 1,821 | 1,218 |
  | part 11 (s_news) | 3,259 | 3,270 |
  | dq0w's main RAM | 2,336 | 2,336 |
  | dq0w's part 8 | 3 | 0 (E_PORTAL's packing, a few bytes in a few parts) |

  The packer placed some parts' images anew as the banks' room changed (20 and 25 have 2,898 and 5,111 free now).
- **The start** (dq0starttime): seed 1 234.1M cycles (234.7M before, budget 235M), seed 123456789 311.0M (310.7M,
  budget 315M): the words packed, the bytes read in as it starts fewer than the five functions'.
- **Checked:** the suite's output kept test by test before the change (a temporary dump in sim/test.js, not committed)
  and after it, its sentences compared: the same, but where the status line's redraws fall.
- **Left (stage 2 and on):** the other 28 wording functions (task_words, beat_words, harm_words, limb_words,
  throw_words, fight_words, shot_words, water_words, felt_words, sky_words ...) and the 277 inline strings; the world's
  told copies as their kinds move; SA_'s refusals as rows; the first words (D17) from a command's *begun* row; the
  denizens' actions by the same rows with the actor swapped (E_DEN's DE_* as outcomes: `{Actor} dig{s} into the wall.`);
  the sound task's event-to-sound mapping a column of the table (dq0s.c's world_sound); the handles filled (a dig's tool
  by its item id, a portal's once features have them) and `{obj}` worded from them.  Overlay 27's 282 bytes won't take
  much more: the next kinds want the renderer's code smaller (it's cc65's C; in assembly a third of it) or the table
  in a bank of its own, read through hub_mem.
- **Tested:** all 442 tests pass.  Every dq0 test's output before and after: no sentence of the five functions' said
  differently (the only sentences new or gone are the real-time fights' rolls in dq0antidote and dq0denpace, which go
  by the clock, and lines typed ahead).

## The tests: split by area, scenes, the runner (2026-11-11, as built, for review: ENGINE-CRITIQUE.md's step 10, Â§9)

The suite took 14 to 17 minutes on the 8-core desktop (442 tests, 8 at a time), and every merge waits on it.  Measured first (obj/testtimes.json, and a dq0 test's lines' cycles one by one):

- **Where the time goes.**  6,809 test-seconds in all (each test's process's wall time, summed); the dq0 tests 6,132 of them (90%), 286 of the 306 billion cycles.  The longest are long for what they do, not their start: dq0hold, dq0housefaults, dq0looks, dq0packsteps (2 to 6 billion cycles, or a watcher on every instruction) take 90 to 140 s each.  A typical dq0 test (dq0parse, 417M cycles, 6.3 s alone): the boot 11M, hmchar 48M, **the game's start (its first dungeon grown) 254M, 61%**, its 20 commands about 100M.  Every test also spent about a second before its first cycle making its ROM image (romimg.build's ROM disk volume, the same for nearly every test), and a fifth of a second loading the runner and tests.js in its own process.
- **The runner already ran in parallel** (base/sim/test.js: a process a test, -j the cores, the longest started first, the reports in the list's order): 856 s of wall for 6,809 test-seconds is 8 processes busy to the end.  So what's left to win is work, not waiting.

What's built:

- **tests.js split** (tests/suites/*.js by area: the system's, HyForth's, BASIC's, hylang's, the numbers', the sound's, the PC tool's, the screen's, hmchar's; tests/suites/dq0/*.js the game's by subject, by areas.js's NEAR: map, parser, gear, fight, denizens, senses, body, time, save, theme, screen).  The helpers more than one suite uses are in tests/lib/common.js and tests/lib/dq0.js, a suite's own at its top.  tests.js is the index: the suites in order, and the same exports.  Every test's name, fields and functions are as they were (compared one by one against the old file); the order is the suites' (the system's first, then the game's by subject).  The selector reads the helper modules too (select.js: a helper module's line runs the tests whose entries use it, as a suite's own helper's does), and every test's areas and helpers came out the same as from the one file.
- **Scenes** (sim/lib/scenes.js).  The whole machine (machine.js's state(): its memory, the CPU, the devices, the output so far, the SD card's blocks written) and the test's watchers (the hub's recorder, the hub writers' check) are saved at two points of a test's input: the prompt waits just before and just after its first dq0 line (booted and the character made; the game begun, its first dungeon grown).  The next test whose input is the same up to there is taken up from it.  This is exact, not an approximation: while the input waits for a prompt, nothing after the wait reaches the machine (acia.js looks only at the wait), so the machine at that moment is every such test's.  A scene's key is the input to the point, the images' sha1 (the ROM's, the BIOS's, the SD card's), the seed, the chip, the RAM modules, the core, HUBOWN, and the emulator's and the runners' sources; a rebuild makes new scenes, and obj/scenes keeps the newest 400 (some 0.75 MB each).
- **Why the machine and not the game's saved games.**  A game carried on (dq0 -d) isn't the same game as one begun: other words, other dice, the start's events missing.  The tests would have to change, and some couldn't.  A snapshot of the emulator changes nothing the test sees, and it skips the boot and hmchar too.
- **A test's watchers.**  A test with start(m) (dq0Poke and the like) saves scenes of its own (its name and its suite's sources in the key), so its pokes are never left in another test's machine.  It can be taken up from its own or from one a test without watchers saved.  As it's taken up, its watchers are made and given what the serial port gave before the point (each byte read and shown, at its cycle; the hub not there before it was made).  A test with budgets, send, a /pc or a machine of its own is never taken up, nor one marked scene: false.
- **Held to runs without.**  SCENES_TRACE=DIR writes each dq0 test's cycles, output hash, failures and notes.  For all 292 dq0 tests, cold (173 from their start, 92 taken up at rc's prompt, 27 in the game) and warm (279 in the game): the same, to the cycle and the byte, but for two.  dq0layers and dq0looks have watchers that see the hub's writes as the game grows, which a scene can't give them.  They're scene: false and start from the beginning as before.
- **The runner** (base/sim/test.js, sim/test.js).  A test's image is kept in obj/imgcache, by its inputs' sha1 (the modules, the init, the hardware test, the BIOS, the ROM disk's files, the image tools' sources): made once and read after.  The parallel runner starts the longest first by each test's last wall time (obj/testtimes.json's `all`), not its cycle budget.  A process for each test stays: no test's state can reach another's.

**Measured** (the whole suite, 442 tests, 8 at a time).  Other agents' suites shared the machine through every run, 1.5 to 4 times over, so the walls are rough.  The CPU-seconds (each test's process's own, now in obj/testtimes.json) hold whatever else runs.

| | CPU-seconds | that, over 8 cores | wall measured |
|---|---|---|---|
| before | some 4,450 (the run without scenes, and 0.55 s a test for its image) | 556 s | 856 s (14m 16s) |
| split, images kept, no scenes (SCENES=0) | 4,209 | 526 s | 1,436 s (the machine 3 times over) |
| scenes cold (images new: scenes made as the run goes; 93 taken up at rc, 33 in the game) | 4,012 | 502 s | 1,963 s (4 times over) |
| scenes warm (nothing rebuilt since the last run: 277 of the 292 dq0 tests taken up in the game) | 2,899 | 362 s | 1,256 s |

Warm, the dq0 tests' CPU time is a third less (2,626 s, from 3,929 without scenes); cold, 5% less.  A dq0 test of some 400M cycles (dq0parse) runs in 1.5 s from its scene, 6.2 s from its start.  The longest tests (dq0housefaults, dq0packsteps, dq0hold, dq0looks: 1 to 2.5 minutes each) are long for what they do after their start, so a whole warm run on a quiet machine is bounded by the busiest core, not by the scenes.

**For review:**

- **Cold runs gain less than warm ones.**  After a merge the images are new, so the run is cold, and in a cold run a scene only helps tests that share a start with one run before them (most share the boot and hmchar; few share a dungeon).  The lead's run after the merges is cold, its RULES=null run after it (the same images, another dq0 line, `-r null`) gains only the rc point.  An agent's runs while it works, and every rerun, are warm.
- **Disk.**  obj/scenes holds some 300 MB at its fullest (400 scenes).
- **The emulator's snapshot** (base/sim/lib) refuses what it doesn't hold: the VERA, the input controller, the RTC, the sound, a /pc host, CA1's pulses, the debugger's things.  A test using one runs from its start.
- **Next** (Â§9's): scene constructors (a room, its things and its denizens declared, not reached by commands), and the recorder's offsets from the schema.

## The placers' table and the keyed dice (2026-11-11, as built, for review: ENGINE-CRITIQUE.md's Â§5 and step 8)

The maker placed each fixture in code of its own with dice of its own: the doors their own hash (pass.c), fixtures.c
its `dseed`/`dr` (windows, portcullises, the fortified doors, the portals' make, the traps from a local array),
sconce.c a copy of them, and stock.c the rooms' marks and the kitchens' wells in a loop of its own.  This step gives
the maker one keyed dice API and one table of placers.  **Every seed's dungeon is byte for byte what it was:** the
maps, the fixtures and their places, the things.

- **The dice (games/dq0m/place.c).**  `dkey (purpose, where)` keys them for chunk L, `dr (n)` rolls (0 to n - 1) as
  often as the decision wants; a purpose (grow.h's DP_*) is what a number decides: DP_DOOR, DP_MAT, DP_WINDOW,
  DP_PORTC, DP_TRAP, DP_DTRAP, DP_MAKE, DP_FORT, DP_SCONCE.  **Each purpose's key is defined as its placer's old
  stream:** `((cx << 20) ^ (cy << 8) ^ (a << 28) ^ where) * 2654435761 + c`, a the old decision's number and c the old
  constant (the doors' a 0 and c 1: pass.c's own hash was the same formula; the fixtures' c 7).  The old streams were
  already keyed by place, never by call order, so none had to be kept apart.  DP_SCONCE's key is DP_MAKE's (sconce.c's
  copy keyed its rooms with fixtures.c's sixth number, the portals' make's): two purposes, one derivation, as it always
  was.
  **The old keys are the chunk's place alone**, not the stratum's seed: two strata's chunks at the same place have
  the same fixtures' numbers (a door on the same square the same lock), as before; kept, for byte-identity.  A purpose
  from DP_SEEDED on has its stratum's seed and its own number in its key, so a new placer gets fresh numbers that move
  nothing else and differ from stratum to stratum.  **For review:** whether to move the old purposes onto the seeded
  key some day (every dungeon would change, and the tests' scenes with it).
- **The streams (`rstream`).**  The growers' (built.c, caves.c: a layout's rolls one after another, call order by
  design) and the stock's (stock.c's things lying about) keep their dice.h streams; their seeds are made in place.c
  now (RS_GROW: the stratum's seed ^ (cx << 16 | cy) * 40503; RS_STOCK: ... * 69069 ^ 0x5EED), as make.c and stock.c
  made them.  They stay streams: their numbers depend on the order of the layout's work, which a key can't reproduce
  cheaply.
- **The placers (`placers[]`, `place (stage)`).**  A row: its stage (PG_WALLS and PG_TRAPS fixtures.c's, PG_STOCK
  stock.c's, PG_SCONCES sconce.c's), its sites (SI_ROOM; SI_WALL, a room's four sides; SI_DOOR, a portal that's a
  door), its dice's purpose (DP_NONE none), its odds (a chance in `sides` of a byte of the style, by its offsetof), what
  a site mustn't be (a room's R_* flags, a door's PS_* state), the room's use it wants (U_ANY any), qualifiers
  (PQ_DEFENCE: always in a fortified stratum's gatehouse and guardrooms; PQ_WATER: only where the chunk has water),
  where in the site it goes (WH_*), what it makes (KI_*, PK_*, or 0: the hook's choice) and its hook.  The one placer
  goes over a stage's rows, each over its sites: the site filtered, its key set, its odds rolled, the hook called.
  Rows of a stage that follow one another over the same sites are placed site by site (a room's mark, then its well),
  as the old loops ordered them, so the stock list's order is the same.  Seven rows:

  | row | sites | dice | odds | what it makes |
  |---|---|---|---|---|
  | window | a room's side | DP_WINDOW | windows in 20 (a fortified gate's always) | a window or a slit (put_window) |
  | portcullis | a door, not hidden | DP_PORTC | portc in 20 | a portcullis (put_portc) |
  | room trap | a room | DP_TRAP | traps in 20 | a pit, or a plate's or a wire's trap (put_trap) |
  | door trap | a door, closed | DP_DTRAP | d_trap in 20 | a door's trap (put_dtrap) |
  | mark | a room, its middle | none | always | its use's mark (put_mark: uses.c's) |
  | well | a kitchen, by its west wall, where there's water | none | always | KI_WELL (put_mark) |
  | sconce | a room not lit by its own light | DP_SCONCE | its hook's (kept, or built with them) | a sconce and its torch (put_sconce) |

  The well is data now: uses.c's US_SIGN 0x80 case (a kitchen's well where there's water) is gone, the row's use,
  PQ_WATER and KI_WELL saying it.  The hooks are the old code, each the part that's truly the fixture's own (a
  window's square along its wall, a trap's square with a walk round it, a sconce's square beside a wall, and their
  makes).  **Not rows:** the doors (placed by the grower's own stream as it joins the rooms, pass.c; only their
  details keyed); the materials, the fortified stratum's doors and the portals' make, which are decisions over every
  room or portal rather than placements: steps of fixtures.c's, between its two stages, with their own purposes.
- **A C table, not a text file.**  The rows hold hooks and the style's offsets, which a generator would have to write
  out as C anyway, and seven rows read plainly as C.  If the fixtures grow (COMMANDS's furniture with compartments, a
  fountain, a shelf), a placers.txt read by the build as said.txt is would be the next step.
- **Room.**  The placer, the table, `dkey` and `rstream` are overlay 9's (sky.c's, which had the room), called through
  bank.s's stubs (place, dkey, rstream, defended; the hooks put_window, put_portc, put_trap and put_dtrap in overlay
  5, put_mark in 7, put_sconce part 3's stub); `dr` is the main RAM's (it's called most).  A hook in another overlay
  can't read the table in overlay 9, so the row it's called for is copied to the main RAM first (`pr`, PR).  First
  built all in the main RAM, the placer in cc65's C was 1,477 bytes and left 1,424 free, under the 2,048.  The build's
  room table:

  | | before | after |
  |---|---|---|
  | dq0m's main RAM | 2,940 | 2,737 |
  | overlay 2 (pass.c) | 623 | 947 |
  | overlay 5 (fixtures.c) | 28 | 845 |
  | overlay 6 (the parts' images) | 66 | 508 |
  | overlay 7 (stock.c, uses.c) | 11 | 165 |
  | overlay 9 (sky.c, place.c) | 1,908 | 477 |
  | part 3 (sconce.c) | 2,043 | 2,485 |

  Overlays 1, 3, 4 and 8 and parts 1 and 2 as they were; no other task touched.
- **The start** (dq0starttime): seed 1 233.8M cycles (234.1M before, budget 235M), seed 123456789 311.4M (311.0M,
  budget 315M).
- **Checked byte for byte** (a scratch script, not committed): every run of bytes each task wrote to the hub's shared
  banks (the maker's chunks into the cache, its stock lists, the seen maps; the world's and the denizens' writes too),
  hashed, as a sorted list per seed, over 21 seeds (1, 3, 5, 7, 8, 11, 13, 21, 42, 77, 99, 117, 256, 374, 517, 1000,
  2026, 4242, 31337, 70000, 123456789), each in twelve games (as it is, -T, -T -T, -T -T -T, -M, -A, -D, -V, -P, -M -T,
  -A -T -T, -V -T -T -T: the top stratum and those under it; built, natural and mixed; the diagonal passages, the
  rooms at 45 degrees, the great caverns, the long slopes): 252 games, 902 chunks made (8,109 doors, 892 windows, 233
  slits, 915 portcullises, 770 traps in 489 chunks), 29,202 runs of bytes.  Before and after: the same, every seed.
- **Left:** the styles' proportions (uses.c's styles[4]) into the theme (Â§5's step 3); the private hashes that aren't
  placers' dice (caves.c's caverns, water and rubble, sky.c's skylights, plan.c's water, uses.c's ring, make.c's mixed
  third) as purposes of `dkey`'s, each defined as its own formula (they'd move nothing, but each wants its own
  derivation, or the seeded key and new dungeons); the hooks' square-finding as a few shared finders (a square along
  a wall, one in the open, one by a wall) once more fixtures want the same squares; a text source for the table if it
  grows.

## Narration as data, stage 2 (2026-11-11, as built, for review: ENGINE-CRITIQUE.md's Â§3.2 and step 5, its second stage)

Stage 1 built the renderer and the phrase table and left overlay 27 with 282 bytes.  This stage makes room for the
table to grow, moves four more wording functions and seven of task_words' own sentences onto it, and takes out the
world's told () copies that only sent events.  The words the player sees are the same, letter for letter.

- **Room: the table's rows in a bank of their own (overlay 28), read a byte at a time.**  dq0.cfg's OV28 holds
  obj/gen/dq0said.h's said_rows alone (OV28RODATA).  The renderer (said.c) stays in overlay 27 with the small tables
  (the kinds, their fields, where the rows and the pieces begin, the words most said), and reads a row's bytes through
  bank.s's said_b: 21 bytes of main RAM that put bank 28 in the window, read the byte and put the window back, some 40
  cycles a byte with the call.  A row now begins with its length, so the renderer steps from row to row without reading
  their texts.  Before, PORTAL's 2.5K of rows were walked a byte at a time.  **Chosen over** two others:
  - the renderer in assembly: a third of its 2.4K back, but the table, 5K and growing, is what fills the bank;
  - the renderer as a swapped part: the window would be free while it runs, but every sentence would cost a part's
    copy (some tens of thousands of cycles), with part 11 swapped out and back around it.

  **Loaded as the game starts, not lazily.**  Measured: seed 1 234.1M to 234.3M, seed 123456789 311.0M to 310.9M
  (budgets 235M, 315M).  The first build of this stage, with the same bank and fewer rows, measured 233.5M and 310.6M.
  The bank adds bytes to read in, but the four functions' code and strings are gone and the texts are packed tighter.
  Seed 1 rising while seed 123456789 falls is the game's own start, not the loading, which is the same for both.

  **The packer (parts.js's banks) puts no part in a bank whose config line says "no parts"** (OV28's).  The first build
  had put part 16's image there, so the bank's room went to the parts and not the table.
- **More words most said.**  The 26 one-byte codes were spent.  The build now goes on choosing words, as SC_WORD and a
  byte, while one saves any (up to 200; 115 now).  Their letters are in overlay 27 (said_dw 668 bytes, said_do two bytes
  each).  The rows are 7,565 bytes without them and 6,280 with them.
- **Kinds older than the head (said.txt's `kind KIND FIELD`).**  E_BEAT, E_HARM and E_TASK keep their bytes where they
  have always been: their outcome in w1's low byte, not E_OUTCOME's high one.  A `kind` line names the field that is
  their outcome (`kind BEAT beat`), and the renderer reads each kind's outcome by its field (said_ko).  **For review:**
  the alternative was to move each onto E_OUTCOME's layout.  That would have changed every sender of three of the
  world's busiest kinds, and the tests' readings of them, without changing any word said.  The head stays the shape
  for new actions.  Two forms were declared in msgs.def for the rows' fields: E_BEAT:water (wet, runs) and E_TASK:shot
  (shot, arms).  Nothing moved; the tests read beat and te as they did.
- **New template pieces:**
  - `{kind F}`: a thing's kind's name, from the catalogue (the critique's handles, for a kind: "dagger");
  - `{name F}`: an actor's name as it is, by a field (E_BEAT's whose);
  - `{-s}`: the word before made singular ("1 arrow");
  - a row for any outcome (`HARM.any`), tried after the kind's other rows;
  - a condition on one value past 7 (`TASK.loaded[arms=KI_CROSSBOW]`).
- **Moved and deleted:**
  - limb_words (main.c's part 23): the limbs' news, every actor's, worded by its body;
  - water_words (water.c, overlay 4: the file is gone);
  - harm_words (intro.c, overlay 12);
  - shot_words (shots.c, overlay 1: the file is gone);
  - their stubs (bank.s's OVFN harm_words, water_words and shot_words) and dq0.h's lines.

  What stays with the callers is what's truly theirs.  s_news (part 11) gives:
  - the body (the row's arg);
  - a fall not seen (the UI's own outcome, BEAT.unseen);
  - the referee's words of the wounds (text, at lb + 200: the referee's hurt[] is under 200 bytes);
  - whether the character is dead (arg).

  It calls state_words where harm_words did.  A body's position stays pose_words'.
- **Inline sites moved:** task_words' own sayf and msg sites for TE_STOPPED, TE_HIT, TE_NEAR, TE_ASK_SEEN, TE_ASK_HEARD
  and TE_ASK_TIRED (what it was doing is their text, copied into the main RAM), TE_CANT, TE_ASK_HURT, TE_NO_FOE and
  TE_FREE, and the missiles' four (TE_NO_SHOT, TE_LOADED, TE_GATHERED, TE_READIED).  That is 48 rows more: 171, and
  15 pieces.
- **A stage 1 bug found:** the renderer's denizen name ({actor}) was copied by hub_mem into the overlay's own BSS, which
  is in the window.  With the hub's bank in the window, the name went into the hub's header, and the renderer read back
  nothing.  No stage 1 row ever named a denizen (the world tells no actor's portal work), so nothing showed.  The limbs'
  rows do ("The unslings its buckler."), so the name now goes into lb, in the main RAM.
- **The world's told () copies.**  Gone:
  - workpart.h's, in five parts (wdoor.c, wblow.c, wwin.c, wtrap.c, wroom.c): the work's events, E_NOISE, E_HARM,
    E_BEAT, E_PORTAL and E_TRAP;
  - wfetch.c's;
  - wshoot.c's two (told and told68), both E_TASK.

  **One sender:** dq0w.c's event, now the world's and not static, its b in evb.  It now maps the hub's header itself,
  as each copy did.  Beside it is work_told, the work's sender (wk.m: its a; its w2 the character's y; E_NOISE's w1 its
  x).  wfetch.c's and wshoot.c's told are now macros of event.  workpart.h's told is work_told, except in the parts that
  also work an actor's door (W_FRONT: wdoor, wblow, wwin): there told only keeps the actor's outcome, and otherwise calls
  work_told; it no longer sends anything itself.  dq0w's main RAM went from 2,336 to 2,243 bytes free, the two senders
  now resident.

  **Left:**
  - wdig.c's and wport.c's told (E_DEN: a denizen's digging, its door work).  Each works out where, how far and whether
    seen in its own way, with its own copy of apart.  One resident den_told would cost the world's main RAM some 170
    bytes more, past its margin over the 2,048 floor.  They go with the denizens' actions worded by the rows (E_DEN's
    DE_* as outcomes), when E_DEN's shape changes anyway.
  - wfight.c's told is the only E_FIGHT sender: the blow's record, not a copy.
- **Room** (the build's table):

  | | before | after |
  |---|---|---|
  | dq0's main RAM | 3,399 | 3,396 (said_b) |
  | overlay 27 (the renderer) | 282 | 4,351 (the rows out; the words most said, 115 now, its) |
  | overlay 28 (the rows) | - | 1,912 (the rows 6,280 bytes) |
  | overlay 1 | 369 | 1,118 (shot_words gone) |
  | overlay 3 | 34 | 440 (task_words' lines) |
  | overlay 15 | 24 | 1,736 |
  | overlay 26 | 323 | 4,333 |
  | overlays 4, 12, 20 | 594, 236, 2,898 | 76, 66, 767 (the packer's: parts 7, 24, 17 placed anew) |
  | the overlays' free bytes, all told | 18,138 | 28,177 (overlay 28's 1,912 among them) |
  | part 11 (s_news) | 3,270 | 3,059 (the limbs' and harm's callers) |
  | part 23 | 2,084 | 4,007 (limb_words gone) |
  | dq0w's main RAM | 2,336 | 2,243 (event and work_told the world's) |
  | dq0w's parts 5, 6, 7, 8 (workpart.h's) | 311, 16, 239, 0 | 376, 81, 304, 90 |
  | dq0w's parts 66, 68 (wshoot.c's), 72 (wfetch.c's) | 2,150, 1,192, 720 | 2,212, 1,227, 775 |
  | dq0w's overlays' free bytes, all told | 7,044 | 7,571 |

- **Checked:** every test's output was kept before the change and after it (a temporary dump in sim/test.js, not
  committed), and their sentences compared, the status line and the start's progress left out.  No sentence of the
  moved words is said differently.  The sentences that are new or gone, in 21 tests, are:
  - the real-time fight in dq0antidote;
  - lines typed ahead, landing before or after "Time passes." or "Something stirs.";
  - three tests whose denizens move on the clock: dq0answer (a rat heard twice less), dq0jobs (a squeak "to your left",
    not "ahead to your left") and dq0skykin (a cave adder "gone from sight" three times more).  The UI's cycles changed
    (the renderer's reads, event's mapping), so the world's clock meets the lines typed at other moments.

  The first run of the moved words caught two bugs, both fixed: the name above, and a one-way choice that should have
  been two (`{stopped?| You stop ...}`).
- **Left (stage 3 and on):**
  - the other wording functions (the rest of task_words, beat_words, felt_words, fight_words, sky_words, throw_words,
    sway_words, state_words, robbed ...) and the inline strings;
  - wdig.c's and wport.c's told, with E_DEN;
  - the SA_ refusals as rows;
  - the first words from a *begun* row;
  - the denizens' actions by the rows, the actor swapped;
  - the sound's mapping as a column of the table.

  The rows' bank has 1,912 bytes left.  The next bank, when it's needed, is a line in dq0.cfg (OV29, no parts) and
  said_b choosing the bank by the row's place.
- **Tested:** all 442 tests pass.

## Narration as data, stage 3 (2026-11-11, as built, for review: ENGINE-CRITIQUE.md's Â§3.2 and step 5, its third stage)

Stage 2 left the rest of task_words, the walk's beats, the denizens' doings and the last two told () copies.  This
stage moves those onto the phrase table, gives the rows a second bank, takes out the two copies, and words the
denizens' doings by the same rows as the character's, the actor swapped.  The words the player sees are the same,
letter for letter.

- **The rows' second bank (overlay 29).**  The rows grew to 9,879 bytes, past one bank.  dq0.cfg's OV29 is a bank of
  its own ("no parts", so the packer keeps out of it) whose MEMORY line starts at **$A000**: the second half of the
  rows (said_rows2, OV29RODATA) is linked there, so a row's offset past 8,192 is simply an address past $A000.
  bank.s's said_b (main RAM, 9 bytes more) reads an address under $A000 from overlay 28's bank, one past it from
  overlay 29's at that address less $2000.  The renderer is unchanged: it still adds an offset to said_rows.  saidgen
  places each kind's rows, and each piece, wholly in one bank, the first with room, in their order (bank 28 8,186
  bytes, bank 29 1,693: DEN's rows and later pieces there).  OVMAX is 29; the game reads one more bank as it starts.
  **Chosen over** a bank number in each row's offset (said_at would grow a byte a kind, and the renderer would carry
  the bank through every pointer it walks).
- **The world's told () copies: none left.**  wdig.c's and wport.c's (E_DEN: a tunneller's dig, a builder's work, a
  denizen's door work) are one sender, wport.c's `s_den_told` in part 80, called directly there and through its stub
  (`den_told`) from wdig.c's part 79.  What each copy did its own way are flags in its loud byte: `DT_SEEN` (a dug
  square seen as it was, before it changed: wdig.c's 0x80 before) and `DT_DOOR` (a door's work: what lies between
  measured from the door, and seen if the body's own square is: wport.c's).  Its event is on the C stack, as
  wport.c's was (not wbody.c's bev).  dq0w's main RAM 2,188 â 2,182 (the stub alone); part 79 622 â 929 (its copy out), part 80 181 â 122.
  **Not resident**, so stage 2's 170 bytes weren't spent.  **Not in part 79, though it was built there first:** part
  79's bank is overlay 32, one of dq0w.rop's three, read in only at ropes_in; a denizen's door work before that
  copied an unread bank in and the world hung (dq0allydoor, dq0daily's garrison, dq0builder, dq0hoard).  wbody.c's
  told_den stays: it's every step's (DE_MOVED), the bodies' own, and a part's copy a step would cost thousands of
  cycles each.  **E_DEN's shape is unchanged**: the sound task (dq0s) and the denizens read it as before, and the
  tests' fields are the same.
- **The denizens' doings worded by the rows** (said.txt's `kind DEN de`: E_DEN worded by its own outcome field, as
  BEAT, HARM and TASK are).  Each DE_* that's told seen has a row: rally, snore, talk, alarm, dig, dug, and the 23 door
  and portcullis doings (opens ... picks); a pastime seen is DEN.any, its words tell.c's as before.  A row's
  `{arg?seen|heard}`: its arg 0 the sentence, "\nThe dwarf miner hangs a new door.", the actor swapped in by `{Actor}`
  ("The giant rat", "Grub") and the verb's ending by `{s}`; its arg 1 what it sounds like ("hammering"), left in lb by
  said.c's new `said_q` (the words not said: the caller's), for caps.c's heard words.  **The character's own words and
  a denizen's share a row's piece where they are the same words:** `@open` is `{Actor} open{s} the door`, PORTAL.opened's
  ("You open the door, revealing ...") and DEN.opens's ("The bandit opens the door.").  That is the one doing whose
  words are already the same both ways today: the character "closes" a door where a denizen "shuts" it, unlocks "with
  its key", lifts a bar "out of its brackets"; making them one would change what the player sees.  The other DEN rows
  are written for any actor ({Actor} ... {s}), so a row the character comes to share will read right.  main.c's
  s_door_words (part 18) and noise_told's own strings are gone; noise_told (part 14) keeps the weighing and the heard
  words' way.
- **Moved and deleted:**
  - the rest of task_words' words (task.c, overlay 3): what's not doing, the stop with nothing to stop and between the
    reach's steps, engaged, backed away, fighting on and turned to, a disengage, a light low and out (its plural, one
    seen, "One of your torches"), an ally's errand (fetched[]), a timed work begun (a journey, the mechanism, tracks,
    the air, a climb, battering with a thing), a bare part's blow stopped short, "Keep at it?", a journey there, the
    way forgotten, lost bearings, astray, elsewhere, drawing learnt.  task_words keeps what's truly its own: the stop
    (parts_in, reach), the questions and their answer, what the foe, the place, the light are as they're said;
  - robbed (task.c, overlay 15) and its OVFN stub: its words TASK.robbed and the UI's own TASK.coins, its purse kept
    in task_words;
  - the reach's 25 first words by verb (main.c's s_reach_said, part 19: first[]), TASK.step with its verb as its arg
    (`@steps`, a piece: a row is at most 254 bytes), and the grope, the pack's place, a fire-maker's try (TASK.fumble,
    .packed, .kindled);
  - beat_words (task.c, overlay 16) and its stub: a walk's beats (BEAT.quiet ... BEAT.on, a bend's sixteen bearings);
  - felt_words' dark beats (task.c, overlay 17): a side's wall, setting out in the dark, what the hands meet, a thing
    underfoot, running into a wall, an edge, a swat, a thing used up.  felt_words keeps the look's two (FW_NEW, FW_TOUCH);
  - state_words (task.c, overlay 7) and its stub: HARM.state, said by s_news (part 11) as the old loop did;
  - main.c's s_door_words (part 18).

  The walk's beats now go through news () (part 11), where the limbs' already did: main.c's events() calls news for
  every E_BEAT of the character's.  In all, 86 rows more: 257 rows of 8 kinds, 20 pieces, 31 fields of 32.
- **The table's compiler.**  A field named in more than one of a kind's forms is now the row's outcome's form's (E_BEAT's
  fa, 8-15 in its met form, 8-11 in its hit form), else the kind's own form's (E_TASK's task, 8-15, not its light form's
  8-13 beside its seen and another bits): the forms' selectors are in the hub's JSON (`sel`).
- **Room** (the build's table):

  | | before | after |
  |---|---|---|
  | dq0's main RAM | 3,383 | 3,415 (said_b 9 bytes more; the stubs of robbed, beat_words and state_words gone) |
  | overlay 27 (the renderer, the words most said) | 4,351 | 911 (the words most said, 156 now, 115 at stage 2; their letters and the renderer) |
  | overlay 28 (the rows' first bank) | 1,912 | 6 (8,186 bytes of rows) |
  | overlay 29 (their second bank: new) | - | 6,499 (1,693 bytes of rows) |
  | overlay 3 (task_words) | 408 | 2,121 |
  | overlay 7 (state_words gone) | 387 | 722 |
  | overlay 10 | 49 | 2,761 (the packer's: part 18's image placed anew) |
  | overlay 15 (robbed gone) | 175 | 311 |
  | overlay 16 (beat_words gone) | 865 | 1,672 |
  | overlay 17 (felt_words' beats gone) | 128 | 329 |
  | overlays 4, 18, 20 | 1,020, 1,982, 320 | 4,086, 758, 2,898 (the packer's) |
  | part 11 (s_news: the beats, the state) | 3,061 | 2,349 |
  | part 14 (noise_told) | 1,097 | 1,643 |
  | part 18 (door_words gone) | 1,640 | 2,582 |
  | part 19 (reach_said) | 2,604 | 3,541 |
  | part 23 | 4,007 | 3,560 (the packer's) |
  | dq0w's main RAM | 2,188 | 2,182 (den_told's stub) |
  | dq0w's parts 79, 80 | 622, 181 | 929, 122 |
  | dq0w's overlays 24, 32 | 291, 1,086 | 232, 1,393 |

- **The start** (dq0starttime): seed 1 233.2M â 234.0M cycles (budget 245M), seed 123456789 311.4M â 310.6M: the one bank more read as it starts, the four functions' code gone.
- **Checked:** every dq0 test's screen kept before and after (a temporary hook in sim/test.js, not committed), their
  sentences compared (the status line dropped, wrapped lines joined, the order of whole sentences ignored).
  8 of 294 differ, all noise: the real-time fights in dq0antidote and dq0denpace (dq0denpace's new run shows the moved
  rows said right where its fight went another way: "The bandit goes through your things as you lie helpless, and
  takes what it finds.", "The torch gutters and goes out.", "You fight back against the bandit."); denizens met at
  other moments as the cycles moved, dq0answer (a scurrying heard once more), dq0jobs (a squeak "to your left" once
  where it was "ahead to your left"), dq0mine (a muffled scraping heard once less), dq0prompt (a giant rat "to your
  right", not "ahead to your right", as it goes from sight); dq0passes' "Something stirs." once more; dq0sway, a
  key typed ahead echoed at a line's end a moment sooner ("behind you._N", not "behind you.N").  No moved sentence is
  said differently, nor more or less often, in any test.  **The first run caught one fault, fixed:** s_den_told built
  in part 79 hung the world on a denizen's door work before the ropes were read (above).
- **Tested:** all 444 tests pass.
- **Left (stage 4 and on):**
  - fight_words (blows.c, overlay 6), sway_words, sky_words (time.c's sky[] and its sky2 by what shows the sky: wants
    E_BEARING's sky byte split, its phase and its opening, two fields), throw_words (resolve.c: a request's reply, not
    an event: wants the throw an outcome event, or a kind for the UI's own replies), the state's and the facing's
    sheet, and the other inline strings (s_came, s_pose_words, hide's, the background's);
  - the SA_ refusals as rows, and the first words from a *begun* row: both are the UI's own, not the world's events,
    and want a kind of the UI's (a row keyed by a refusal or a verb) before they move; not built;
  - the denizens' doings that the character does too in other words (shut and close, unlock "with its key"): one
    wording each, the user's call, then the rows shared (the user chose one wording: done, "One wording for the player
    and the denizens" below);
  - wbody.c's told_den, the bodies' step: kept resident in its part for its pace;
  - the sound's mapping as a column of the table (untouched, as asked).

## The user's review of five defaults (2026-11-11, the user's)

The user reviewed defaults this file had marked "for review" and asked for five of them changed; two more were kept.
Each entry above now says so; what's built:

- **"g" is again's** (Infocom's convention), no longer give's.  commands.aliases: "g again" (beside "repeat again"),
  "hand give" and "offer give" (they were "hand g", "offer g": g was give's root), "give g" gone; commands.verbs'
  [words] "g = again" in place of "g = give" (the same record: dq0c's records as they were); commands.ebnf's
  `<GIVE> ::= "give"`.  Give must be spelt out ("give", "hand", "offer").  Nothing else read "g" as give (the grammar's
  `<GIVE>` was its only use; letters.c makes a lone "g" after a verb a carried thing's letter, "drop g", as before).
  term.c's `said_again` (overlay 16) keeps a line out of `t_last` when "g" is a part's first word (the line's start,
  after "," ";" "." or "then"), so "g" runs the line before it, never itself; a "g" elsewhere ("drop g") is kept.
  The corpus has "g" (VB_REPEAT).
- **A vow of fasting is carried in the save.**  No file grows and `SAVE_VER` stays 45: game.dat's header byte `plain`
  had one bit used (the map in plain letters); its bits 1-7 now hold the vow's time left as the game was kept, in
  steps of 12 minutes rounded up (life.h's `FAST_STEP`, 7,200 tenths: a day 120 steps), 0 none (an old save's).  As a
  game's carried on, the vow is set again on the clock it's carried on with.  Between dungeons: life.dat's head had six
  bytes of room; one is now `fast`, the vow's end in steps after the midnight that begins the head's date, dq0life's
  (`ended`: from the ended dungeon's save and its clock, world.dat's), 0 if it's over by then, and 0 once the time
  between (dq0acct's) has moved the date on a day or more (a day's vow can't outlast a whole day more).  The UI takes
  what's left of it after the hour the next dungeon begins (its seed's, as dq0life reckons it).  "stop" and "break my
  fast" end it as before.  Test dq0fastkept.
- **A sip can bring on the flux.**  Tasting still water (the water it stands in, or a skin filled from it) tells the
  referee it's drunk foul with no pints (RS_TIME's drink 0x80: hthings.c's taste, as feed.c's drink tells it).  The
  figure: *a sip is a tenth of a drink's dose, so the Contagion test comes only one time in ten* (d100, 10 or under:
  afflict.c; then the drink's own test, CI 5 times Endurance), some 2-5% where a drink is 25-50%; the null ruleset the
  same, one in ten (`random (10)`).  Spoiled food doesn't exist yet (nothing goes off in this game: HOUSE.md's "not
  yet"), so tasting food stays "still good"; when it does, its taste takes the same tenth.  The world's DG_TASTE is
  unchanged (it reports the wear).  Tests: dq0rules' three sip cases (a miss, its one die; the tenth come and caught;
  a pint from the UI, no tenth's roll), dq0things' sips (the referee told, the antidote's taste not).
- **The sheet names a dressing's cloth.**  pc_dr's spare bits 4-6 hold its cloth (dq0harn.h's `DR_MAT`, `DM_HEMP`
  ... `DM_SILK`; 0 not known).  The world's tools text gives a bandage's cloth after its grade ("bandage9S": R hemp,
  S linen, T wool, U silk; things.c's s_tools), the referee's equip_all keeps it in tqual's high bits, and the dressing
  takes the cloth of the best bandage (the first lot of that grade, the one the world uses up).  The sheet: "dressed in
  linen, clean" (the grade its cloth's whole), "worn" (0.7 of it or more), "soiled" (less, as it went on or by the
  days: linen worn after a day, soiled after two).  The "dressed" line, compatibly: a letter after a dressing's place,
  'q' plus its DM_* (r, s, t, u), none for a cloth not known; a line without it reads as before and its sheet's words
  are the old ones.  One reader and writer for the referee and the account (dq0harn/dressln.h); dq0life, dq0end,
  dq0schol and dq0town don't read it, nor any test's decoder.  Tests: dq0rules' four cloth cases, dq0dress's words.
- **The days between dungeons soil dressings.**  Every day between passes through dq0acct's `o_day` (at rest, on the
  road, the lull, the town's days, the work's), and there each dressing on a serious or grievous wound loses three
  grades, its cloth kept (`DR_SOIL`, the referee's heal.c's the same).  The account now reads the "dressed" line into
  its own pc_dr (sheet.c keeps the line whole in the main RAM's `dressed`; body.c reads and writes it, overlay 1:
  overlay 3 had 20 bytes) and keeps it in step as wounds heal and go (`by_drop`).  Untold in the account: the next
  dungeon's sheet says it.  The town sells bandages, and "change the bandage" in the dungeon puts a fresh one on as
  before.  Test dq0dressbetween (a serious stab dressed in clean linen, four days between, soiled to grade 1, its cloth
  kept, the line written back as the account ends in an infection's death: the account heals what it can before the
  next dungeon, up to ninety days, so a dressed wound seldom lasts to it).
- **Kept, reviewed:** generated chests brace doors (the maker's own chests and tables beside a door brace it, as a
  shoved one does: "The things' verbs"); and the looser hmchartongues check (hmchar.hl's native script among its
  scripts, not first: its dice can't be seeded; the cloth branch's change) is accepted.
- **Room:** dq0's main RAM 3,386 free (3,399 before); part 1 406 (the packer moved s_size to part 12), overlay 16
  418 (1,179), part 29 1,647 (1,804), part 14 1,097 (1,218).  The referee 2,264 (2,341: the tools' cloth), overlay 1
  194 (353), overlay 8 32 (69), overlay 9 1,683 (1,829).  dq0acct 2,152 (2,264: pc_dr and the line), overlay 1 319
  (1,194), overlay 3 9 (20: its line buffer now path).  dq0life 18,318; dq0null 5,035; the world's main RAM unchanged
  (2,336), its overlay 5 265 (328).  dq0c's records as before; the aliases 1,896 bytes (1,889).

## The referee's skeleton (2026-11-11, as built, for review: ENGINE-CRITIQUE.md's Â§8 and step 9)

**The user's choice, 2026-11-11: option (a), swappable rules are a real goal.**  Â§8 found the referee interface
halfway: an indirection the game paid for, and HarnMaster everywhere behind it.  (b) would have said HarnMaster is the
house system and dq0null a test double.  The user chose (a): a referee skeleton in a library so a ruleset is its
answers, the interface trimmed, HarnMaster's lists behind the referee, and the meta programs talking to "the ruleset".
Built in five stages, each its own commit:

- **1. rs.h trimmed, P_ one thing.**  Ten requests no one made (RS_DROP_ACTOR, RS_TEST_MANY, RS_REST,
  RS_INITIATIVE, RS_EXPERIENCE, RS_POWER, RS_EXPORT, RS_IMPORT, RS_RESTORE, RS_QUIT: each checked, no sender and no
  answer in any task) and `struct rs_neutral` deleted, with their msgs.def lines; 17 requests left.  The tests that
  had request numbers typed in (denizens.js's and senses.js's RS_TEST = 4, RS_ATTACK = 7) take them from the layout
  by name.  `P_` now means only rs.h's purposes: theme.h's stratum purposes are `SP_*` (SP_HALLS ... SP_WORKS,
  SP_FORT), dq0town's places `TOWN_*`.
- **2. The skeleton.**  games/dq0/referee.h (the interface) and games/dq0/rf/rf.c, rf/sheets.c.  **Where it lives:
  a shared C source each referee compiles, not the paged ROM.**  The ROM library (modules/dq0lib) is assembly with no
  RAM of its own, called by XCALL at some 120 cycles; the skeleton is C, its loop and its `put` called constantly, and
  nearly all of it was each referee's main RAM already, so moving it to ROM would have meant rewriting it in
  assembly for little room (dq0harn's whole skeleton is some 1,300 bytes, much of it data and BSS it keeps
  anyway).  Each referee has a file of its own that #includes the shared one with its #pragmas first
  (dq0harn/rf.c in its main RAM, dq0harn/rfsheets.c in swapped part 2; dq0null/rf.c), the way hub.c already was.
  The skeleton: `main` (the hub attached, its dice seeded from the game's and the role, each slot answered as rung,
  the denizens' slot with DTEXT of text, the dice by who asks: dice.inc's `dice_to`, no DICE_BEGIN/DICE_IN macros),
  the answer's prologue (tp, tend, result 0), `put`/`putn` (dq0harn's: a string in the window read with its bank),
  `tools`/`tflag`/`tqual`/`equip_all`/`has` (dq0harn's, with its marks, qualities, versions and cloths: dq0null's
  `held[]` became `tflag & T_HELD`), `bseed`, `sheets_in` (the creatures' sheets read back once, a game carried on,
  as a creature's asked for) and the save's order (the character, then the creatures' sheets), `sheets_open`
  (beasts.dat).  **A ruleset is four functions:** `rf_start` (its arguments, its overlays and banks), `rf_begin`
  (its own beginning, once its dice are seeded), `rf_answer` (each request; RS_SAVE's the character alone),
  `rf_sheets` (its creatures' sheets in beasts.dat).  Each referee's own main, prologue, `den_slot`, `sheets_in`,
  `put`, `putn`, `equip_all`, `has`, `tools` and `bseed` went.  dq0null needs no HarnMaster header (it never
  included one; it now also has no copy of dq0harn's code), and passes the tests that run it (dq0antidote,
  dq0air's, dq0dressbetween's null games, dq0outfit's).  dq0null got `getbank`/`setbank` in its dice.s, and `num`
  as atoi (it doesn't load the game's library).
- **3. The house's rules as linked functions.**  rshouse.h is declarations; the code once each in games/dq0/rf/:
  dig.c (`digger`, `house_dig_tool`), notice.c (`house_notice`), state.c (`house_pace`, `house_watching`,
  `house_light`), bdice.c (`bdice`), want.c (`want_of`, rswant.h's), air.c (`air_of`, rsair.h's).  rswant.h and
  rsair.h are gone.  Each referee compiles the ones it uses where its room is, a file of its own that #includes the
  shared one, its #pragmas first: dq0harn's hdig.c (part 3, beside dig_of), hnotice.c (overlay 1, beside sheet.c),
  hbdice.c (overlay 5, beside bearing.c), hwant.c (overlay 7), hair.c (overlay 8); dq0null's house.c (its main RAM).
  The macro hooks became the ruleset's functions: `want_say` and `air_say` (its words into its news); AIR_LEVELS
  went (dq0harn sets `aff_breath` from the house's `air_lv` after `air_of`: they were always equal).  What's an
  expression stayed a macro, now unconditional: `HOUSE_SWING (k)` (what a thing does swung: the referee's gear.c and
  the world's two parts each their own use, as before), `QUIET_IN`/`OUT`, `JUDGE_IN`/`OUT`, `DIG_TESTED`.
- **4. HarnMaster's lists behind the referee.**  The outfit tags (`OT_*`: cultures, classes, wealth, sexes) are
  dq0harn.h's; rs.h says only that x[0], x[1] are the bits of outfits.txt's tag words, the ruleset's to set.  The
  strike locations (`BP_*`) are gone from rs.h: nothing outside the referees read them (an effect's x[0] is the
  ruleset's own number, HarnMaster's harn.h location, the null's 0; the game reads only the words), and t_rules names
  them for its cases itself.  **The grades are a generic scale:** `G_BADLY`, `G_FAIL`, `G_PASS`, `G_WELL` (were
  G_CF, G_MF, G_MS, G_CS, HarnMaster's level names), the mapping dq0harn's `grade ()`; renamed everywhere (the UI,
  the world, the denizens, the maker, both referees, t_rules' cases), the values the same.  **The ruleset's own
  stance:** `D_COUNTER` is `D_OWN` (HarnMaster's counterstrike; a ruleset with none takes it as D_AUTO).
  `C_STUNNED`'s meaning is the ruleset's (stunned: in no state to do more than rest or walk slowly away;
  HarnMaster's shock).
- **5. The meta programs' interface.**  games/dq0/rsmeta.h: **the rulesets' table** (`RULESETS`: each referee and its
  own program for the time between dungeons, "/../acct/dq0acct" for dq0harn, none for dq0null) and **the sheet as a
  meta program sees it** (`meta_line`: a line of the sheet read, what the ruleset keeps of it, the rest to the
  program's own; `meta_ml (MK_*)`: a mastery by what it's for, MK_LANGUAGE, MK_SCRIPT, MK_FOLKLORE, MK_RECKON,
  MK_ASTROLOGY, MK_HERALDRY; `meta_taught`: what a master's month opens a tongue or a script at; `META_TONGUES`,
  `META_SHEET`).  dq0life runs the program the save's ruleset names (`accounts_of (sv.referee)`), not dq0acct when
  the referee is "dq0harn".  dq0schol includes rsmeta.h, not harn.h nor dq0harn.h: HarnMaster's implementation is
  dq0harn/meta/meta.c (in a folder of its own, so the referee doesn't compile it), compiled in by dq0schol's rsmeta.c.
  **dq0acct is HarnMaster's own account program** (the table names it as the ruleset's): it still includes
  dq0harn's headers, as part of that ruleset.

**Room** (main RAM free, before â after): dq0harn 2,264 â 2,209; dq0null 5,036 â 4,605 (dq0harn's fuller
`equip_all` and `put`); dq0life 18,318 â 18,198 (the table and its lookup); dq0schol 5,405 â 5,333 (its overlay
591 â 630); the UI (3,383, 3 less than before stage 1: the request numbers), the world (2,243), the denizens, the
account and the others as they were; dq0harn's overlays 1 194 â 170, 3 110 â 174, 7 11 â 47, 8 32 â 22, part 2
1,574 â 1,564.  The start at seed 1: 233.3M cycles (233.9M at stage 2); at 123456789, 310.6M (budgets 245M and
325M).

**A race found on the way: dq0reachstop.**  Its "halt" and "stop" are typed as a step's words show, at the turns pace;
whether they land in that step or the next turned on some thousands of cycles between the parser's answer (some 1.3M
cycles after Enter) and the world's next turn.  Each stage's few cycles flipped it (stage 1 game 7, stage 3 game 2).
Holding the turns pace while a busy-time line is classified (`ui_giving` from its Enter) didn't help (the step had
ended before the UI's loop saw the Enter), and holding from the first key is what the turns pace was made not to do;
left for review (see below).  **So the suite: 444 of 444 at stage 2; 443 at stages 1, 3, 4 and 5, dq0reachstop the one
(at stage 5 its game 2: "halt" lands in the reach's next step, "You stop there.", the rations taken).**

**For review:**
- The skeleton is a shared source, not the ROM library: worth moving `equip_all`/`put` to dq0lib only if the
  referees' main RAM gets short (some 400 bytes in dq0harn).
- The world's worn places are still harn.h's locations (dq0w/kinds.c, items.h's WP_*, worn.c's words): the largest
  piece of HarnMaster left in the engine.  The UI's "counter" word and its "in shock" words, outfits.txt's skill
  sections and tag words, the bestiary (RULESETS.md) are the others.
- dq0schol's d100 test is still the book's (its own `test`), and its scribes' and sages' figures.
- dq0reachstop's race: a test that types as words show at the turns pace depends on the parser's speed; it wants a
  scene that holds the world while the line's parsed, or a pace that waits on a line entered.
  **The user's, 2026-11-11: accept either step.**  Game 2's check passes the stop landing in the step after the one
  it was typed in (the rations taken in hand, then "You stop there."), the pack still at its feet and nothing after it
  done (tests/suites/dq0/gear.js); the game unchanged.
  **The user's, 2026-11-12: game 7 the same.**  Its "stop" typed at "take all"'s first "Taken." may land one thing
  later (the record store's timing flipped it): the tinderbox reached for and taken, then the stop, the pack in hand,
  no third thing taken.

## The step planner and the activity stack (2026-11-11, as built, for review: ENGINE-CRITIQUE.md's Â§3.3, Â§7 and step 6)

**What the survey found.**  The reach's steps were already planned from the containment tree, lazily: each time a
handling is asked (the UI's command again, after a step's wait), things.c's `s_reach_step` walks the thing's
containers out from the hand (`rs_walk`) and begins the next step owed (a pack on the back off, a level reached into,
held open, the thing got out), and the step's end (`RO_END`, `lwork`'s `LW_REACH`) does what it changes.  So the edges
were there (`rs_do`, thirteen kinds); what was hand-coded was (1) the step's record, spread over six globals, three
of them other jobs' scratch bytes aliased (`rs_k` was `nin`, `rs_c` `dropped`, `rs_t` `far_y`); (2) its words: 33 LT_*
kinds (LT_PACKOFF ... LT_UP2), nine of them a pair's other half picked by a ternary (GETOUT/GETNEW, PUTAWAY/PUTOLD,
TAKEUP/TAKENEW/UP2, SETDOWN/DOWNNEW, BLOWOUT/BLOWOLD, LIGHTFROM/LIGHTNEW, LETFALL/LETOLD, DIP/DIPOLD); and (3) what's
done after the work, a bit set (`rs_redo`'s RR_STOW, RR_RELIGHT, RR_D1, RR_D2 with `rs_down`, `rs_down2`, `rs_two`)
read in `rs_after`'s fixed order.  "What the PC is doing" besides `pc_task.kind`: `move.on` (the walk, a spell of
digging); the queued continuations `take_n`, `gath`, `pfoe`, `srch`, `lwork`, `trav` (the main loop's free-again
chain, in that order); the long task's own going on, `fwant` (and LT_BATTER's `batter_on`); its stop, `pc_cut`; and
two that aren't the character's at all but periodic work, `burn_ask` (the lights' minute) and `chart_late` (the map
making's late job).

**Built in three stages, each its own commit, the suite whole at each:**

- **(a) The step record and the first queue.**  things.h's `struct step {edge, say, what, thing, apart, time}`; the
  step under way `rstep` (was `rs_do`, `rs_k`, `rs_what`, `rs_c`, `rs_wn`, `rs_t`: no more aliased scratch bytes);
  the edges `SE_*` (were RS_OFF ... RS_SLING, the referee's prefix): a pack taken off, a level reached into, put on,
  let fall or set down, a spark struck, blown out, lit from another, stowed in the pack, taken up, a tool got out to
  hand, put back, put in its place (a sconce, a sheath), slung.  **The first family on a queue of step records:**
  what a redip or a new light sets down to free a hand is taken up again after the work by a step pushed on a stack
  (`aside[2]`, `naside`) as it's set down and popped as the work's over, the last set down first; the other of two
  alike marks the first's entry ("You take up the other torch.") as it's pushed.  Gone: RR_D1, RR_D2, `rs_down`,
  `rs_down2`, `rs_two`.
- **(b) The step kinds out of LT_*.**  The 33 reach kinds are gone from the LT enum (it ends at LT_DIP).  A step's
  say is a verb and a word: hub.h's `SV_*` (25 verbs, `LT_STEP` 0x20 set, so a long task's kind past LT_DIP is a
  step) | `SM_*` (`SM_NEW`, `SM_OLD`, `SM_OTHER`).  The nine variant kinds are a verb and a word (GETNEW is
  `SV_GETOUT | SM_NEW`, PUTOLD `SV_PUTAWAY | SM_OLD`, UP2 `SV_TAKEUP | SM_OTHER` ...), the ternaries a bit or'd.
  **The word is the first words' alone:** the long task's kind (I_WAIT's task, `pc_task.kind`) is the verb, as
  E_TASK's light form carries 6 bits of it (its 14 and 15 its own flags), and the world's own step's TE_BEGUN takes
  its say from `rstep` (rs_for's RF_TELL is bit 15, SM_OLD's place).  The UI: reach_said's 25 sentences (33 before),
  the word put into the name the way the AW_* words ("lit", "spent") are and never with them, as before; task.c's
  doing words by verb (`DOING`); "stop sharpening" names SV_HONE, a dip SV_DIP.  The event bytes are the same but for
  a step's kind byte.  Kept as verbs of their own, being other sentences: SV_DOWN2 ("You set the other torch down."
  isn't "down at your feet" with a word), SV_LAYLIT2 and SV_INPACK2 ("too"), SV_OFFDOWN, SV_BLOWDARK, SV_LETBURN; and
  the other hand's words in s_room (`SV_LAYLIT ? SV_LAYLIT2 : SV_INPACK ? SV_INPACK2 : SV_SETDOWN ? SV_LETFALL`), a
  verb's choice, not a word's.
- **(c) The activity.**  games/dq0w/activity.h's `struct pc_activity act`: what's queued after the long task, `take`,
  `gath`, `foe`, `srch`, `work`, `trav` (were `take_n`, `gath`, `pfoe`, `srch`, `lwork`, `trav`), and the long task's
  own `fight` (fighting on, was `fwant`) and `cut` (its stop, was `pc_cut`): the flags of four files and two statics
  in one record, every reader and writer by name (dq0w.c, things.c, wgame.c, wshoot.c, wfight.c, wblow.c, wport.c,
  wroom.inc).  **The free-again chain is a pop:** dq0w.c's `act_next` takes the first queued, `take` and then the
  bytes `gath` ... `trav` in their order, each its handler from a table (`next_[]`), where the main loop had
  `take_n ? take_up () : gath ? ... : trav ? travel_on () : 0`.  LT_BATTER's next blow and fighting on stay after it,
  the long task's own going on.

**What it isn't yet (for review):**
- **The queue is an order, not a stack pushed.**  The continuations are set by their own handlers and popped in a
  fixed order, the chain's.  A real stack (each pushed as it's planned, popped last first) is the same only if no two
  are ever queued out of that order, and the order matters where two are set at once (a dig's tool put away, `work`, before
  the journey's next leg, `trav`).  Moving each to a push is the next step, one at a time.
- **The reach is still planned a step at a time** (each ask walks the tree again), not a whole queue up front: the
  world changes between steps (a denizen takes the pack, a grope fails) and the handling's own checks come first.
  The after-work take-ups (stage a's) are the one family queued ahead; the pack put back on and a tool put away
  (`rs_after_on`, `rs_using`, RR_STOW, RR_RELIGHT) are still flags `rs_after` reads in order.
- `burn_ask` and `chart_late` are periodic work, not the character's: stage (d), Â§7's scheduler table, wasn't built.
- The denizens: their minds post intents the world carries out; `struct pc_activity` and `struct step` don't name
  the character, so a denizen's record would be the same shape, but none was moved.
- Containers (COMMANDS-CRITIQUE.md's Â§4.9): the edges are the planner's; the compartments aren't there (a thing's
  `where`/`x` its holder alone), so the walk (`rs_walk`, `level_of`) still knows a container by K_CONTAINER, KF_BACK,
  IS_CLOSED and K_HANG.  With Â§6b's compartment table the walk reads `(holder, compartment)`, each compartment's
  `closes` and noun pick the edge (open, reach in) and its words, a row per verb in said.txt (the narration's)
  rather than reach_said's table.

**The words.**  Every dq0 test's screen was saved before and after each stage (sim/test.js's TXDIR hook, not
committed) and compared sentence by sentence (the status line and the typing row dropped, wrapped lines joined, the
order of whole sentences ignored, as the timing noise moves them).  Stage (a): 6 of 294 differ, all noise
(dq0antidote's real-time fight; dq0jobs' and dq0skykin's denizens met at other moments; dq0stopwords' "Something
stirs."; an Initiative improvement told in dq0robrest and not in dq0restbusy, a denizen's timing).  Stage (b): 5
(dq0answer's scurrying, dq0robrest's improvement, dq0skykin, dq0stopwords, and dq0reachstop's race: game 2's "halt"
lands in the step it was typed in, "You stop getting out the rations.", the side the user accepted on 2026-11-11).
Stage (c): 10, all noise (dq0antidote's and dq0denpace's real-time fights; dq0answer's, dq0jobs' and dq0mine's sounds heard from denizens met at other moments; dq0skykin; dq0passes' and dq0stopwords' "Something stirs."; dq0robrest's improvement; dq0reachstop's race on the side the user accepted).  No step's words changed: each reach sentence, each word ("the old torch", "the other torch", "the new torch") and each "You stop ..." is said as often as before in every test but dq0reachstop's race.

**Room.**  dq0w main RAM 2,243 â 2,220 (a: the step record and the stack) â 2,220 (b) â 2,188 (c: the pop's table and
loop, 32 bytes more than the ternary chain); its part 86 234 â 212, 87 2,203 â 2,188; overlays 17 134 â 119, 26
114 â 92.  The UI (dq0) 3,383 throughout; its overlays 171 bytes freer and its parts 209 at (b) (the sentences;
ovlpack's placing).  The start at seed 1: 233.3M â 233.5M (a) â 233.7M (b) â 233.2M (c) cycles (245M the budget);
at 123456789 310.6M â 311.0M (b) â 311.4M (c).

## The meta programs' library, their state record, one theme compiler (2026-11-11, as built, for review: ENGINE-CRITIQUE.md's Â§9 and step 11)

The programs between dungeons (dq0life, dq0acct, dq0town, dq0schol, dq0end; themegen and themec beside them) each had their own copy of the same helpers, passed the town's state to each other through long argument lists and a file of their own, and the theme compiler was written twice, in JS for the build and in C for the Hydra, the tests holding the two to the same bytes.  Four stages, each its own commit:

**1. libdq0meta** (games/dq0/meta/meta.c).  One C source a program compiles in, a section at a time:

    #define META_WORDS
    #include "../dq0/meta/meta.c"
    #undef META_WORDS

so each program takes only what it calls and puts each section where it wants it (dq0town's ask, width_read, seed_of and launcher in its overlay 1; dq0acct's width_read in its overlay 2, value_of in 4, the launcher in 5), the names a section reads (path, ans, asks, said, tw, rbuf ...) declared by the program first.  The sections: fail (META_NAME's "NAME: why", exit 1), in_ (a path), num, seed_of, the words by the console's width (say, sayn, amount, say_flush; width and ob, META_WB and META_OB the buffers' sizes), width_read, ask, words_of and words_read (the "key | value" text, META_TEXT_ON/OFF round each reading of it: dq0town's bank), value_of (a rumour's value: the first, or with META_VALUE_N the n'th), and the launcher (META_LAUNCH: the system's SPAWN and WAIT themselves, named META_SPAWN and META_JOIN: the UI's hub_run and hub_join are the library's; META_RUN: meta_run, both; META_ARG, META_ARGN: arg1, arg, argn).  META_LINK, empty or static: shared with the program's other files (dq0acct's words, its overlays call them) or not.

| helper | was in | now |
|---|---|---|
| say, sayn, amount, the words' buffer and wrap | dq0acct, dq0town, dq0schol (the town's and the scholar's 6 of 68 lines apart: the buffers' sizes, static or not, say_flush) | META_WORDS (META_WB, META_OB) |
| the launcher | dq0life run, dq0acct's town.c arg/argn and SPAWN/WAIT, dq0town run and arg, the UI's hub_run and hub_join | META_LAUNCH, META_RUN, META_ARG, META_ARGN |
| words_of, the text read | dq0town (its bank put in and out), dq0schol | META_KEYS (META_TEXT_ON/OFF, META_LINE, META_TEXT_MAX) |
| value_of | dq0town (the n'th value), dq0acct's quest.c (the first) | META_VALUE (META_VALUE_N) |
| num | dq0life, dq0end | META_NUM |
| in_ / in | dq0life, dq0acct, dq0town, dq0schol, dq0end; themec, themegen | META_PATH |
| fail / die | dq0life, dq0acct, dq0town, dq0schol | META_FAIL |
| seed_of | dq0life, dq0town | META_SEED |
| width_read | dq0acct's o_width, dq0town, dq0schol | META_WIDTH |
| ask | dq0town, dq0schol | META_ASK |

Kept local, as they differ in what they do: dq0acct's ask (dq0auto, " > "), dq0end's words (its spaces kept, an indent stands), dq0life's out (no wrap), hmchar's in (another signature, under its floor), the rumours' files read (dq0acct's and dq0town's: the town counts themegen's).  Every program's words the same: where the copies differed it's a parameter or the difference did nothing (dq0schol's ask's word_out before out_flush).  One change in what a program does: value_of now stops at a key line with no bar inside the rumour's own lines (dq0town's way; dq0acct's read on past the rumour's end), which no rumour file has.

**Why a shared C source, not the paged ROM's library** (modules/dq0lib).  Measured against what the ROM would save: a ROM call is an XCALL to an assembly entry (dq0lib has no RAM of its own: the words' buffer, their column and the console's width would be the caller's, passed in), its stub in each program (dq0lib.inc, as the tasks have it) some 130 bytes of main RAM, and the helpers here are 100 to 400 bytes a program.  The meta programs aren't the game's tasks: but dq0acct (its main RAM at 2,152, its words in main RAM for its overlays to call, the same as before) they have thousands of bytes to spare, and a shared source puts each helper where its program had it, so nothing moves.  The UI's hub_run and hub_join the library's too, its image byte for byte the same.

**2. One state record** (games/dq0/records.def, struct between, between.dat).  What the account (dq0acct) gives the town (dq0town) was seven arguments (-t -i -p -k -h -r -n: the town, the inn, the days passed, the keep so far, the habit, Rhetoric's and Intrigue's EMLs), the town gave the scholar's house (dq0schol) its name and first command as arguments (-t, -x), and town.dat (struct townrec, in dq0acct/town.h) went from the town to the account and between the town and the house.  Now one record in the save's folder, declared in records.def (a sibling of msgs.def): its fields, their types, what each is, and the file's name, magic and version.  hubgen.js reads it with the headers: it writes obj/gen/dq0rec.h (the struct, BETWEEN_FILE, BETWEEN_MAGIC, BETWEEN_VER and the defines it wants), lays it out with cc65 with the rest (dq0hub.json's S.between) and records each field's type (dq0hub.json's records), so the tests' decoder is the same pass's: `load ().record ('between', bytes)` gives { town, inn, passed, keep, ..., learnt: [..], ok } (a char array its string, an array its numbers, ok the magic and version).  The flow: the account fills it and writes it, runs `dq0town -d folder -l lib`; the town reads it as it starts (none, or another magic or version: it fails, said), writes it before it runs `dq0schol -d folder -l lib` (the first command in it) and reads it back after, and writes it as it's left; the account reads it and removes it.  The scholar's house reads it (none: a stay begun, all 0) and writes it back; its -t, -b, -c, -w (the tests' scenes, run alone) set the town, the town's kind, the money and the writings over the file's, so dq0scholar types what it did.  An old folder's town.dat is passed by (read by nothing now; it was the town's, removed by the account as it read it).  dq0life's (-d, -s, -l), dq0acct's (-d, -l), dq0end's and themegen's arguments are where things are or what to make, not state passed on: as they were.

**3. One theme compiler: themec's (C).**  What uses it: themegen runs it on the Hydra as a game begins (hy_exec in its place) to make a game's theme.dat from the theme.txt it just wrote, the history's, from any seed; a hand-written theme is tried with it from the shell.  So the game needs a compiler on the Hydra, and the only one it can have is the C one: the JS one could be the only compiler only if themegen made the record itself (a compiler in C again).  Kept: themec, games/themec/comp.c.  The build runs it in the emulator (tools/themec.js; no host C compiler is on the build's machines, and the emulator is the build's already: hysnap's snapshot): a paged ROM of the modules and t_rc, an SD card made for the run (themec, bestiary.dat, the themes' texts), "themec -l /sd/0 -o tN.dat tN.txt" typed for each, its status and words read off the console and the records off the card (hydrafs).  Each answer is kept (obj/themec, by the hash of themec, the bestiary, the modules and the text).  Who runs it:

- the build, for the stand-in themes: games/dq0/make.js's after (build.js calls a make.js's after once every game's built: dq0's make.js runs before themec is built), obj/games/dq0t.dat themec's records with their count and file's name put in, as make.js did;
- the build, for the goal kinds: games/themegen/make.js holds each goal block made a theme to themec (compiled and checked, no record), after it writes bestiary.dat;
- the tests: tests/lib/dq0.js's THEMEC (themes) where they compiled with make.js's: dq0themegen (themegen -s 3's theme compiled and checked, theme.dat held to its record, byte for byte), dq0history (theme.dat's story), dq0why and dq0handtheme (the theme.txt a theme); dq0themec holds the records compiled on the Hydra to the build's (dq0t.dat's, their count 1 and no file's name: make.js's standIns), the same three, byte for byte.

Gone: make.js's compile and check (some 300 lines) and bestiary.  The stand-ins' records came out byte for byte the same (themec's in the emulator held to make.js's before it went; obj/games/dq0t.dat the same as before), and every image (the paged ROM's chips, the SD card) the same as stage 2's.  The build's time: some 3 s more for the emulator's run (the three stand-ins, measured on a quiet machine; the six goal kinds about as much), nothing when the themes, themec, the bestiary and the modules haven't changed (kept); the whole build 2m 27s.

**4. tools/dq0build.js.**  The readers the make.js files shared by copy: keyLines (the "key | value, indented continuation" reader: dq0/make.js's lines, themegen/make.js's read), blocks (dq0d/make.js's, on keyLines), rows (the one-liner: dq0d's, dq0s's, themegen's bestiary's), and packOnly (dq0harn's, dq0m's and dq0p's make.js, the same three lines: each now `module.exports = require('../../tools/dq0build.js').packOnly('dq0harn')`).  The build's outputs byte for byte the same.

**Room** (bytes free; before, after):

| program | main RAM | overlays |
|---|---|---|
| dq0 | 3,415, 3,415 (its image the same) | the same |
| dq0acct | 2,152, 2,157 | 4: 254, 250; 5: 2,166, 2,008 |
| dq0life | 18,198, 18,095 | |
| dq0town | 3,211, 3,379 (its arguments' parsing gone) | 1: 6,493, 6,443 |
| dq0schol | 5,333, 5,143 (the record's 208 bytes, the first command's 64 among them) | 1: 630, 630 |
| dq0end | 20,959, 20,959 (the same) | |
| themec | 902, 902 (the same: under the floor, a warning, as it was) | |
| themegen | 712, 712 (the same: as above) | |
| hmchar | 559 (untouched) | |

**Held to:** every dq0 test's transcript before and after each stage, sentence by sentence (the scratch's sent.js, a hook in sim/test.js not kept): RESULTS.  The stand-ins' records, themec's in the emulator against make.js's before it went: the same bytes.  The suite: all 444 tests pass after stage 1 and after stage 4.

**For review:**

- **The record's reach.**  between.dat holds what passes in the town.  The other files the programs share (life.dat, kit.dat, townkit.dat, kit0.dat, clues.dat, heard.txt, rumours.txt, quest.txt, lives.dat) are as they were: life.dat is life.h's struct life (the game's too), the kit files the world's; records.def could declare them next (life.h's struct life is the first candidate, its layout read by the tests by hand now).
- **The emulator in the build.**  The stand-ins and the goal kinds are compiled in the emulator: a build with themec, the bestiary, a goal or a stand-in changed runs it (some 3 s each); one without reads the kept answers.  A host build of comp.c would be quicker, if a C compiler is ever on the build's machines: the same source (it reads its files with open and read).
- **The tests' theme oracle** is now the same compiler as the game's: a theme compiled on the Hydra is held to themec's run alone, not to a second compiler's.  What the two compilers' agreement checked before (that each read the format the same) has no second reader to disagree with now; comp.c is the format's statement.

## One wording for the player and the denizens (the user's, 2026-11-11; the choices as built, for review)

**The user's decision:** an action has one wording, the character's and a denizen's, said with the actor swapped
("You open the door." / "The bandit opens the door.").  Stage 3 shared one row's piece (@open); every other doing the
character and a denizen both do was worded twice.  Now each is one piece of said.txt, used by the character's row
(PORTAL's, DIG's, TASK's, BEAT's) and the denizen's (DEN's, BEAT's), and the duplicates are gone.  The player's
wording was kept where it could take an actor (all but one, a thing dropped); where it couldn't ("The lock gives with a click.", no one doing it),
the actor was put into it, the player's words kept after it; the denizen's was taken where it was the more exact.

- **The renderer** (said.c, saidgen.js):
  - `{its}`: the actor's, "your" or "its" (SC_ITS, 0xEF); `{'s}` after `{Actor}`: "r" or "'s" ("Your", "The
    bandit's": SC_POSS, 0x7F).  Codes, not fields: the table's 31 of 32 fields are as they were.
  - A kind's actor may be a field of its own: said.txt's `kind BEAT beat whose` (said_ka, a byte a kind): E_BEAT's
    whose (0 the character, else the actor plus 1), not the event's who.  `{Actor}`, `{s}`, `{its}`, `{'s}` and
    `{actor?a|b}` read it.
  - A piece two kinds use is compiled for each (its fields are the kind's), and kept once where the bytes are the same
    (saidgen.js): @open, @close ... are PORTAL's and DEN's, one copy.
- **The UI:** task.c's TE_BEGUN gives what's forced or heaved at as obj (noun: "the window"); s_noise_told (part 14)
  clears obj for a denizen's row (its piece's "the door"); s_news (part 11) gives the character's limbs' rows its
  body, BD_TWO (said_n 0: it was left as the last row's).

**The pairs, before and after** (the character's row, the denizen's; *chosen* the one now said by both):

| action | the character's, before | a denizen's, before | one wording now (piece) |
|---|---|---|---|
| open a door | You open the door{, revealing ...}. | The bandit opens the door. | unchanged (@open, stage 3's) |
| close a door | You close the door. | The bandit shuts the door. | {Actor} close{s} the door. (@close: the player's) |
| unlock with its key | You unlock it with its key. | The bandit unlocks the door. | {Actor} unlock{s} the door with its key. (@unlock) |
| lock with its key | You lock it with its key. | The bandit locks the door. | {Actor} lock{s} the door with its key. (@lock) |
| bar a door | You set the bar in its brackets: the door is barred. | The bandit drops a bar across the door. | {Actor} set{s} the bar in its brackets: the door is barred. (@bar: the player's) |
| lift its bar out | You lift the bar out of its brackets, and set it down. | The bandit lifts the door's bar. | {Actor} lift{s} the bar out of its brackets, and set{s} it down. (@lift: the player's) |
| forcing begun | You set to forcing the door. (TASK.begun's) | The bandit strains at the door. | {Actor} set{s} to forcing {the door}. (@forcing: the player's) |
| forced | With a crack the door bursts open{, revealing ...}. | The bandit bursts the door open. | With a crack {actor} burst{s} the door open{...}. (@burst) |
| forcing held | The door holds. | The bandit can't force the door. | The door holds against {actor}. (@held) |
| broken down by blows | It gives way, and crashes open{, revealing ...}. | The bandit breaks the door down. | {Actor} break{s} the door down: it crashes open{...}. (@broke) |
| picking begun | You set to picking the lock. (TASK.begun's) | The bandit works at the lock. | {Actor} set{s} to picking the lock. (@picking: the player's) |
| picked | The lock gives with a click. | The bandit picks the lock. | {Actor} pick{s} the lock: it gives with a click. (@picked) |
| heaving begun | You set to heaving at the portcullis. | The dwarf heaves at the portcullis. | {Actor} set{s} to heaving at the portcullis. (@heaving: the player's) |
| winding begun | You set to winding the winch. | The dwarf winds the portcullis's winch. | {Actor} set{s} to winding the winch. (@winding: the player's) |
| raised (winch, by hand) | The portcullis grinds up, and catches, raised{...}. / The portcullis rises, and catches, raised{...}. | The dwarf raises the portcullis. | {Actor} raise{s} the portcullis (@raise), the character's rows adding ": it grinds up, and catches{...}." / ": it rises, and catches{...}." |
| lowered | You loose the winch: the portcullis crashes down. | The dwarf lets the portcullis fall. | {Actor} loose{s} the winch: the portcullis crashes down. (@lower: the player's) |
| bending begun | You set to bending the bars. | The dwarf strains at the bars. | {Actor} set{s} to bending the bars. (@bending: the player's) |
| bent apart | The bars bend apart: there's room to get through. | The dwarf bends the bars apart. | {Actor} bend{s} the bars apart: there's room to get through. (@bent) |
| digging begun | You set to digging: some 12 feet of it, by the look of it. | The dwarf miner digs into the wall. | {Actor} set{s} to digging (@digging: the player's; its feet the character's row's) |
| the roof come down | The rotten rock splits, and part of the roof falls into the hole. | The dwarf miner is showered with rock from the roof. | The rotten rock splits over {actor}, and part of the roof falls into the hole. (@roof) |
| a thing dropped | Your sword falls from your hand. | The bandit drops its sword. | {Actor} drop{s} {its} {thing}. (BEAT.drop: the denizen's) |
| a thing broken | Your sword breaks! | The bandit's sword breaks! | {Actor}{'s} {thing} breaks! (BEAT.broke: the same words) |
| up, not up, fallen | You get to your feet. / You struggle, but can't get up. / You fall. | The bandit gets to its feet. ... (by its body) | {Actor} get{s} to {its} feet ... (BEAT.risen, .not_up, .fallen: the same words; a creature's body its own) |

**The choices, for review:**

- **"with its key", not "with your key".**  The key is the door's (a key fits its own lock alone), and a chest's are
  said so already ("You unlock the chest with its key.", hthings.c's): "with your key" would have told the door and the
  chest two ways.  The pronoun is there (`{its}`) if the user wants "your key" / "its key".
- **The door named, not "it".**  "You unlock it with its key." read after "unlock the door"; a denizen's needs the
  door named, so the character's names it too ("You unlock the door with its key.").
- **An impersonal sentence given its actor.**  "The lock gives with a click.", "The door holds.", "With a crack the
  door bursts open", "It gives way, and crashes open", the portcullis's "grinds up": no one in them to swap.  Each keeps
  its words and takes the actor: "You pick the lock: it gives with a click.", "The door holds against you.", "With a
  crack you burst the door open", "You break the door down: it crashes open", "You raise the portcullis: it grinds up,
  and catches" ("raised" gone: the verb says it).  The shutters' (a window's, the character's alone) are as they were.
- **A timed work begun is the character's words** ("sets to forcing the door", not "strains at the door"): the
  character's are said by TASK.begun's rows, now one for each of the five a denizen does too ([task=LT_FORCE],
  LT_PICK, LT_HEAVE, LT_WIND, LT_BEND), their words the same as the general row's were.
- **A raise is one doing for a denizen** (E_DEN's DE_RAISES, by the winch or by hand), two for the character: the
  piece is the clause they share, "{Actor} raise{s} the portcullis", the character's rows adding how it went.
- **Lifting the bar is PORTAL.lifted's**, not PORTAL.unbarred's ("You lift the bar and open the door"): a denizen lifts
  a bar on its side first (dq0d.c's PA_LIFT), then opens the door, two doings told each.
- **A thing dropped is the denizen's words** ("You drop your sword.", not "Your sword falls from your hand."):
  WB_DROP is a fumble, but also a weapon let go to take up a bow (wshoot.c's), which "falls from your hand" misreads;
  and the table has no verb agreement by the thing, so the first build's "The soldier's rations falls from its hand."
  (dq0denhands) read wrong, where "drops its rations" can't.  The player's own words had the same fault for a plural
  thing ("Your rations falls from your hand.").
- **A named denizen's limbs** now read "Grub gets to its feet." (was "The Grub gets to its feet.": the row put "The"
  before the name).

**Not shared** (one side's alone, so one wording already): a rally, a snore, talk, the alarm, a door slammed as its
denizen flees (the character closes), a denizen's blow at a door (the character's blows are told by what they do:
"Your blows begin to tell on it."), a door hung, a way walled up, a pastime; a way dug through, told from where it opens
(its digger may be out of sight: "A fresh tunnel opens ahead to your left."); the denizens' thing snatched up, drawn,
readied, eaten, handed or thrown (the character's are its commands' replies, not the table's); the character's own
refusals and its windows.  TASK.away ("Quicker, the rat backs away from you.") and TASK.break ("You back away from the
rat.") already say the same doing the same way.

- **Room** (the build's table):

  | | before | after |
  |---|---|---|
  | dq0's main RAM | 3,415 | 3,415 |
  | overlay 27 (the renderer, the words most said) | 911 | 695 (SC_ITS, SC_POSS, the actor by kind, said_ka) |
  | overlay 28 (the rows' first bank) | 6 | 0 (8,192 bytes of rows: the packer fills it, a kind's rows wholly in a bank) |
  | overlay 29 (their second bank) | 6,499 | 6,577 |
  | the rows, all told | 9,872 bytes, 257 rows, 19 pieces | 9,807 bytes, 262 rows (TASK.begun's five), 38 pieces (20 shared, kept once), 31 fields |
  | overlay 3 (task.c: what's forced or heaved at) | 2,121 | 2,081 |
  | part 11 (s_news: the character's body) | 2,349 | 2,346 |
  | part 14 (noise_told: obj cleared) | 1,643 | 1,637 |
  | overlays 15, 18 (the packer's) | 311, 758 | 308, 752 |
  | dq0w's main RAM | 2,182 | 2,182 (the world untouched) |

- **The start** (dq0starttime): seed 1 234.0M â 233.4M cycles (budget 245M), seed 123456789 310.6M â 311.4M.

- **Checked:** every dq0 test's screen kept before and after (a temporary hook in sim/test.js, not committed), their
  sentences compared (the status line dropped, wrapped lines joined, the order of whole sentences ignored).  41 of 294
  differ.  Every sentence new or gone is one of the rewordings above: a door broken down (dq0batter, dq0battering,
  dq0doorfar, dq0highkinds, dq0strike, dq0wordsfix), forced (dq0moving, dq0pure, dq0timed, dq0traps, dq0watch), locked
  and unlocked with its key (dq0denkey, dq0foundkey, dq0keysteps), a portcullis raised (dq0portc), bars bent (dq0break),
  the roof come down on a dig (dq0dig), a thing dropped (the character's, in 19 tests: "You drop your torch."), and the
  denizens' (dq0allythrow, dq0denportc: "sets to heaving at the portcullis"; dq0denkey: "sets to picking the lock",
  "sets to forcing the door"; dq0tunnel: "sets to digging").  The rest is the known noise: the real-time fight in
  dq0denpace; denizens met at other moments (dq0answer, dq0jobs, dq0mine, dq0prompt), dq0answer's a key typed ahead
  echoed after a sentence's stop ("some bones.."); "Something stirs.".  **The first build** said "The soldier's rations
  falls from its hand." (dq0denhands): a thing dropped is the denizen's words since (above).
- **Tests' strings changed** (the words the decision changed, each check's meaning kept): tests/lib/dq0.js's
  DQ0_REPEATS (a dig's "The rotten rock splits over you ..."; a fumble's "You drop your ...", "... drops its ...", a
  named one's too); denizens.js (dq0tunnel's and dq0mine's "sets to digging", dq0denportc's "sets to heaving at the
  portcullis", dq0denkey's "You lock the door with its key.", "sets to picking the lock", "sets to forcing the door");
  parser.js (dq0wordsfix's "You break the door down: it crashes open"); map.js (dq0foundkey's "You unlock the door with
  its key", the old words' alternative gone).
- **Tested:** all 444 tests pass.

## One record store (2026-11-11, as built, for review: ENGINE-CRITIQUE.md's Â§6 and step 7)

**What the survey found.**  Not five stores paging by region but seven, in three tasks, and each its own table, its
own least-lately-wanted loop and its own copy of vfile.inc (the one thing they shared: the save folder's versions):

- the world's: the change records (store.c, wfile.c: `rgSXXYY.dat`, pages of the hub's change banks, their table
  the world bank's `cpage`), the strata's records (sfile.c: `stSS.dat`, a slot each), the traces (wtrace.c,
  wtrfile.c: `trSXXYY.dat`, a directory of chunks, `THD`'s `ch`), the item store's paging (ipage.c: `itSXXYY.dat`,
  `IPG`'s `r`); the traces and the things both keep a region's file of chunks' groups (each its own `others ()`);
- the denizens': their regions (dq0d.c: `dnSXXYY.dat`, `hd`'s `reg`, and an unused copy of its lookup in part 4);
- the maker's: the seen maps (changes.c: `smSXXYY.dat`, `sreg`), the change records as it lays them over a chunk
  (rg read), the big plans' file's name (`plSS0000.dat`); and the chunks' cache's slot for a new chunk (make.c's
  `pick`);
- every reader of the chunks (chunk.c's `find`: the world's, the denizens', the senses'): the cache's lookup.

The UI has none.  Their eviction agreed (the least lately wanted, its 16-bit stamp the oldest, round as it wraps,
`s - t > 0x8000`), but for the chunks' cache, which compares plainly (`<`): kept as it was (RTF_PLAIN), as changing it
would change which chunk is made way for once the clock wraps, and so the seen maps' writes.  What each keeps besides
differs: a free entry first or not, the one at hand, a box about the character (two chunks each way, one, or a region
each way; with dq0 -C none, or only its own), an actor's things (ipage's `kept_for`), a slot a task's reading or one
stocked and not yet marked (the cache's).

**What's built: the record store in the paged ROM's library** (modules/dq0lib's `RP_*`, games/dq0/rp.h its C side,
dq0lib.inc's `rp_x`: a block of the caller's a call, its first byte the entry):

- **One file-naming scheme and one save-and-restore path** (`RP_NAME`, `RP_OPEN`, `RP_MAKE`, `RP_DONE`; games/dq0/rp.inc
  the four calls in C, `rp_name`, `rp_open`, `rp_make`, `rp_done`, a swapped part's copy as vfile.inc's was): a
  region's file `preSXXYY.dat`, written whole as a new version over a slot that's neither the save's nor (three slots)
  the newest, its header last; read as the newest valid version by the hub's vsaved and vbase.  The save is the
  stamps moved on (wfile.c's `store_commit`, unchanged).  **Each kind's count its own** (`struct rpk`: vlast,
  vcount, the main RAM's), as each store's copy of vfile.inc had its own, so every stamp, and so every file's bytes,
  is as it was; one count a task would have given the same files other stamps.
- **One lookup** (`RP_FIND`): an entry by its key (a mask of an entry's first eight bytes, as the tables are laid out:
  their layouts, in items.dat, seen.dat and dens.dat, unchanged), its stamp made the clock's next if the store wants
  it, a free one among the first n, from an entry on (a region's pages one after another).
- **One eviction policy** (`RP_OLDEST`): the least lately wanted, round or plainly, but the key's own (`RTF_HAND`), a
  box about a centre (`r` each way, one byte or two, on its stratum: the near rule each store had), and those the
  store's own routine keeps (a callback into the caller's code, from the ROM: ipage's `kept_k`, make.c's `busy`); a
  free one first (`RTF_FREE`).
- **A region's file of chunks' groups** (`RP_GROUPS`): the groups read past, each but the chunk at hand's copied into
  the new version, or to the chunk at hand's: the things' and the traces' `others ()`.

Each store keeps only its own logic: what its records are and where (pages, buckets, a bank of denizens, maps), what
a record replaces, what's kept besides (the regions known to have no things, an actor's region), the records read,
renumbered and written.  `rp_dirty`, `rp_flush` and `rp_save` as ENGINE-CRITIQUE.md drew them aren't calls: what's
dirty and when it's written is each store's (the pages' `dirty`, the traces' `TC_DIRTY`, the things written as a
chunk goes out), and the save is the stamps' commit, already one.

**Where it lives, and why the ROM.**  The paged ROM's library (Â§1.4), not a C source compiled into each task: vfile.inc
was compiled six times (four of the world's parts, the denizens' part 21, the maker's main RAM), and with it the C
library's file calls it took in; the tables' loops a dozen times.  In the ROM they're one copy and no task's room: the
library is assembly, its scratch the caller's block (a part's own data, or the main RAM for a table read in play),
its kernel calls its own (the fds the cc65 library's, the same numbers).  Its bank has room.  The cost is an XCALL
each call (some 120 cycles), nothing beside a file's work.

**Measured.**

- **The chunks' cache** (the one called in play: a reader's `at` the long way, as its square's in another chunk than
  its last): the world's `find`, entry to return, in a game of seed 5's (34 calls): median 1,195 cycles before, 1,326
  after (least 619 and 1,084: the XCALL and the block made), 0.06M and 0.08M cycles of a 461M game.  RP_FIND built
  for it: the key's bytes as offset and value pairs in the call registers first, an entry some 60 cycles to the C
  loop's some 155, so the ROM's quicker on a long search and slower by its call on a short one.  dq0lookway 357.0M
  cycles to 358.0M, dq0travel 2,727.6M to 2,728.8M.
- **The start** (dq0starttime): 234.0M cycles at seed 1 before (310.6M at 123456789), 232.3M after (308.6M); the
  maker's main RAM's code the less.
- **Room** (bytes free):

  | | before | after |
  |---|---|---|
  | dq0w main RAM | 2,182 | 2,236 |
  | dq0w part 17 (ipage.c's table) | 1,222 | 1,529 |
  | dq0w part 18 (wfile.c's files) | 422 | 1,791 |
  | dq0w part 19 | 638 | 654 |
  | dq0w part 20 (ipage.c's files) | 120 | 1,518 |
  | dq0w part 22 (sfile.c) | 1,554 | 2,811 |
  | dq0w part 28 (wtrace.c's store) | 483 | 682 |
  | dq0w part 30 (wtrfile.c) | 329 | 1,713 |
  | dq0w part 38 (store.c's put) | 1,860 | 1,853 |
  | dq0d main RAM | 2,184 | 2,379 |
  | dq0d part 1 / part 21 | 811 / 897 | 907 / 2,369 |
  | dq0m main RAM | 2,737 | 4,635 |
  | dq0p main RAM | 3,122 | 3,176 |

  The world's overlays as the packer placed the parts (7: 130 to 2,897, 10: 328 to 1,712, 21: 48 to 1,725; 17: 119
  to 22, 27: 114 to 23); the UI, the referee and the rest as they were.

**Checked byte for byte** (a scratch script, not committed): every save-folder file each task wrote (each OPEN,
CREATE, WRITE's bytes hashed, SEEK, by file: the kernel's fd table, TA_FD, read at each call to name an fd), in order
per task, over 32 games: 8 seeds (3, 5, 7, 42, 99, 1000, 31337, 70000), each with -C -T -T -T, -C -T, -T -T and -C (the
small caches, so every store makes way; strata below), a save folder, a fixed random player's commands (the soak's
`choose`, its own stream, not the game's words: the same commands whatever the timing), quit, carried on and saved
twice: some 1,200 to 2,500 region files' versions a game.  The baseline itself isn't the same at each power-up (the
tasks' timing: a walk in the dark interrupted by a sound a step sooner or later, so its later commands land
elsewhere), so it was run at four power-ups; each stage's every game is one of the four's byte for byte (life.dat left
out: its date the host's clock).  One game at stages 5 and 6 was another of its timing's outcomes (that build at
another power-up gave the baseline's bytes); at the end, 31 games the first power-up's, one the third's.  The
sentences (the transcripts, by sentence) the same but that one's.

**The suite: 443 of 444; dq0reachstop's race flipped (for review).**  Its game 7 types "stop" as "take all"'s first
"Taken." shows, at the turns pace, and wants the take ended before the next thing (the tinderbox) is reached for.
That's the race "The referee's skeleton" found (whether a line typed as a step's words show lands in that step or the
next turns on some thousands of cycles; the user accepted either step for its game 2, not yet for game 7).  From
stage 6 on (the denizens' regions on the record store) the next thing's reached for first ("You reach for the
tinderbox. Taken. You stop there."), at five power-ups of five; the baseline, and this build with dq0d.c alone put back
as it was, pass at five of five.  Not a change of what the denizens do: their files, every other store's and the
sentences of the 32 games are the same; their work's timing is (the regions' lookup and the part it's in quicker), and
with it how soon the UI goes on to the next thing.  Not made to pass: neither the test loosened nor a delay put in.
Wanted: the user's word on game 7 as on game 2, or a pace that waits on a line typed as words show.

**Left.**  The plans' pages (changes.c's `plt`, its own least-lately-wanted loop over a file that isn't versioned) and
the senses' caches of their own (none paged to a file) aren't on it; the chunks' cache's plain comparison
(RTF_PLAIN) could go round as the others do, a behaviour change (which chunk's made way for after the clock wraps) to
be taken as one; the item store's `items.dat` and the traces' flush at the save stay each store's; an `rp_save`
walking every store's dirty entries would want the stores in one task.

## The life's record and the clues declared, the generators driven from one place, one rumours' reader (2026-11-12, as built, for review: ENGINE-CRITIQUE.md's §9 and step 11's "Left")

What step 11 left: life.dat into records.def, the other files the meta programs share where it's clean, and the
generators into tools/dq0build.js; and the rumours' reader, written twice (dq0acct's quest.c, dq0town).  Every
program's words and every save-folder file the same; the images' bytes as below.

**1. The records.**  records.def (games/dq0/records.def, read by hubgen.js into obj/gen/dq0rec.h and dq0hub.json's
records) now declares, beside struct between:

- **struct life** (life.dat): `struct life_head` (the 32-byte head: magic, version, flags, seed, dseed, dungeon,
  date, born, reputation, clues, thread, fast, room[5]) and `struct life_dungeon` (seed, days, outcome, flags) as
  structs of no file of their own, and `record life "life.dat" 0x464C 1` their record (h, d[LIFE_DUNGEONS]); with
  LIFE_DUNGEONS, the head's flags LF_BETWEEN, LF_OVER, LF_ABROAD, LF_HARD and the dungeon's LD_DIED.  lives.dat (the
  lives over, this record one after another) noted with it.
- **struct clue** (clues.dat): `record clue "clues.dat"`, a file of records one after another with no head; with
  CLUE_SIZE, CLUE_WORDS (its words' 342 bytes: was `CLUE_SIZE - 106` in the struct), CL_MORE and the asked bits CA_*.
  CLUES_FILE is CLUE_FILE now, the record's name's (read.c, dq0life.c, dq0schol.c: three files, seven uses).

records.def's grammar grew three things for them: `struct NAME` (a struct with no file, a field's type for a record
declared after it), a field's type `struct NAME`, and `record NAME "FILE"` with no magic or version (a headless file
of records); a record's head may be its first field's struct (life's h).  hubgen's decoder decodes a struct field as its
own object (an array of them its objects), and ok is the head's magic and version (a headless record's always true).

life.h keeps what isn't a record's layout: the folder's files told, struct save and struct quest (game.dat's head,
the UI's), the outcomes' enum LO_* (game.dat's too), the thing's fuel for a clue (CLUE_HERE, CLUE_COPY), CLUE_NOCOPY,
the grades' enum CL_*, the clue's size check (clue_fits), LIVES_FILE, FAST_STEP and the calendar; it includes dq0rec.h.
No C program had a copy of struct life of its own: they all included life.h, so the hand-written copies were the tests'
and the soak's, reading life.dat by offset (`a[38]`, `readUInt16LE(12)`, `a[26]`, `0x464C`, `& 2`, `544`).  Now:

- tests/lib/dq0.js's `dq0Life (bytes)`: the decoder's record ({ h, d, ok }, null if the bytes are short of it), its
  `size ()` and `C (NAME)` (LF_*, LO_*): dq0carry, dq0account, dq0hardon, dq0lifeend, dq0fastkept, dq0nosave
  and dq0why (the reputation) read their fields by name.  Each check holds the same
  thing it did.
- sim/tools/soak.js's lifeRecord: the same decoder; its flags and outcomes by name (LF_OVER, LO_SOLVED, LO_NONE,
  LIFE_MAGIC).

Left as they are, not clean:

- **kit.dat, kit0.dat, townkit.dat** (dq0w/outfit.h's struct kitrec): the world's.  Its layout is struct item's own
  fields in their order (KIT_ITEM, KIT_STATE: memcpy'd onto a thing as it's made) and its ofit's `in` holds the worn
  places' bits (WP_*: the referee2 work's); declared in records.def it would be a copy of struct item's order, held
  to it by hand.  The tests read kit.dat by offset in one place (dq0account's sword, 23 bytes a record).
- **heard.txt, rumours.txt, quest.txt**: text, the theme format's (themegen's), not records.
- **game.dat's struct save**: the UI's save's head, its versions main.c's (SAVE_VER); not a file between the programs
  but the dungeon's.

**2. The generators driven from tools/dq0build.js.**  What was copied or open-coded across build.js and the make.js
files, now each one function there:

| | before | now |
|---|---|---|
| the headers' numbers (hubgen's load) | dq0, dq0d, dq0s, dq0w make.js and dq0w/outfits.js, each `require('../dq0/hubgen.js').load ()` | `hub ()` |
| hubgen's generate, then saidgen's | build.js's hubNumbers, the two calls | `generate ({ say, cc65 })` (build.js passes cc65 as the C target has it) |
| pack.js's and parts.js's driver | build.js's swappedParts (some 45 lines: compile, route, measure, plan, apply, make) | `swappedParts (dir, objdir, extra, tools)`, build.js's cc65, od65, compileC, csources and flags passed in |
| the overlays' packing (ovlpack.js) | dq0, dq0d, dq0w make.js, packOnly | `pack (objdir, name)` |
| themec in the emulator, its failures thrown | dq0/make.js's after and themegen/make.js, each its own compile and check | `themes (list, opt, size)` |

Each generator's own work stays in its file (hubgen.js, saidgen.js, parts.js, pack.js, ovlpack.js, tools/themec.js):
dq0build.js is where they're called from, not their code merged into one file.  Why not moved: saidgen.js is the
narration work's (narr4's, being edited); hubgen.js's load is the tests' and the sim's (dq0meaning, hubown,
commandwords, the suites: some twenty requires by path); pack.js has its own command line (node games/dq0/pack.js TASK);
and every one's comments name its file (parts.txt, bank.s, the cfgs).  A move is a rename the day those settle.

**3. One rumours' reader.**  meta.c's META_HEARD: `heard (name)` (a rumours' file in the save's folder read into rbuf
after what's there, each block begun by a rumour line noted in rat[], rto[], RUMOURS at most) and `rumours_read ()`
(heard.txt's, then rumours.txt's, afresh).  The two copies' differences are parameters: META_RB_N (dq0town's RB_N, its
words' bank; sizeof rbuf the default), META_MADE (dq0town counts rumours.txt's, nmade: themegen -r -a goes on from
them), META_TEXT_ON/OFF round rbuf's filling (dq0town's WORDS () and KINDS (), as it had).  dq0acct's quest.c joined
its path itself into path (in_'s same bytes) and used its file's fd: the library's in_ and a local fd.

**Images:** dq0acct's (its main RAM's and overlay 4's code: quest.c's reader now the library's, in_
and a local fd) and dq0town's (the same length, 1,322 bytes moved: its reader's code where the library's section puts it,
rumours_read's clearing written apart) changed, and with them the SD card's image and the browser page's; every other
image byte for byte the same: the UI (dq0), dq0life, dq0schol, dq0end, dq0harn, the world, the denizens and the rest
(the records' header made from records.def lays out struct life and struct clue as life.h did), the paged ROM's chips
too.  So the start is as it was (dq0's image the same).

**Room** (bytes free, before → after): dq0acct's main RAM 2,157, 2,157; its overlay 4 250, 279; dq0town's main RAM 3,379, 3,379
and overlay 1 6,443, 6,443; the others the same, their images being so.

**Held to:** every dq0 test's transcript before and after (294), sentence by sentence (the scratch's sent.js,
a hook in sim/test.js not kept): none differs.  Byte for byte, 279 the same; 15 (the life's and the town's, whose card
changed: dq0account, dq0why, dq0life ...) differ only in the progress dots ("The dungeon is being made ready ...", "Nearly
ready") and the status line's clock as a long wait goes: the SD card's reading times, its image changed.  The save folder's files: the programs that write them, but dq0acct, are
byte for byte the same images, and dq0acct's change is in its reading of the rumours' files alone (no write); the
tests' xd of life.dat, kit.dat and the rest in those transcripts the same.  The suite: all 444 tests pass, before and
after.

**For review:**

- **CLUES_FILE renamed CLUE_FILE** (the record's name, as BETWEEN_FILE and LIFE_FILE are): the UI's read.c touched for
  it, two lines.
- **The outcomes' enum (LO_*) and the grades' (CL_*) stay in life.h**, as enums: records.def has no enums, and LO_* is
  game.dat's struct save's too.
- **The generators' files not moved** (above).

## HarnMaster's remnants behind the referee (2026-11-12, as built, for review: ENGINE-CRITIQUE.md's §8 and step 9's "Left:")

The user's decision of 2026-11-11 (§8's option (a)) wants a ruleset to be its answers alone, behind the referee, and
no HarnMaster in the engine.  "The referee's skeleton" left four pieces outside; three are moved, the fourth is a
question.  dq0null, the second referee, is the check: it includes no HarnMaster header and now neither does the world.

- **1. The places on the body a thing's worn at are the engine's.**  items.h's `PB_*` (`PB_HEAD`, `PB_FACE`, `PB_NECK`,
  `PB_SHOULDER`, `PB_UPPERARM`, `PB_ELBOW`, `PB_WRIST`, `PB_HAND`, `PB_BREAST`, `PB_WAIST`, `PB_GROIN`, `PB_HIP`,
  `PB_THIGH`, `PB_KNEE`, `PB_CALF`, `PB_FOOT`, `NPB`) and `PB_SIDED` (a limb's, a left and a right) in place of harn.h's
  `HARN_SKULL ... HARN_FOOT`, `HARN_NLOCATIONS` and wworn.c's `sided`; named for what the game says of them (the
  words were already "head", "wrist", "breast", "waist").  **The numbers are the same**, so an item's `sn` (WP_AT | its
  place | WP_PAIR | WP_RIGHT) and kit.dat's worn entries (WP_KIT's) read as they did: no save file changes.
  kinds.c's `wplaces` and `wroom`, wworn.c and the UI's worn.c (its letters A ... P the places) name them; the world's
  kinds.c and wworn.c include no harn.h (the world now includes no HarnMaster header at all; nor does the UI, the
  denizens, the senses, the maker).  **The mapping to a ruleset's locations is the referee's:** dq0harn.c's
  `PB_HARN` (HarnMaster's strike locations in the same order: the head its skull, the wrist its forearm, the breast its
  thorax, the waist its abdomen; WP_RIGHT its HARN_RIGHT), checked at compile time.  No referee reads a place worn yet
  (the jewelry worn at places covers nothing; armour is by its kind, the referee's own tables); one that does maps it
  there.
- **2. The ruleset's words for its stunned state and its own stance.**  RS_HELLO's text, after its name and its
  character generator, gives three more lines (rs.h): the words for the state C_STUNNED is (HarnMaster's "in shock"),
  the word for its own stance, D_OWN ("counter": "" none), and that stance told ("You will strike back first.").  The
  UI keeps them (main.c's `rs_stun`, `rs_own`, `rs_owns`, the main RAM's 54 bytes) and uses them where it had its own:
  tend.c's refusals while stunned ("You're " and the words, ": you can do no more than rest, or walk slowly away."),
  said.txt's HARM.state (" You are {text}." and "still", the words its text: main.c's state_said), blows.c's stance
  (the word's first letter after dodge's and block's, as 'c' was; o_fight_note's D_OWN sentence the ruleset's), and
  the help (help.txt's "@own " the word and a comma, help.c's).  dq0harn says "in shock", "counter", "You will strike
  back first." (part 4's s_hello_words, after part 1's name and generator); dq0null "stunned" (never set) and no stance of its own.  **dq0null's one change:**
  "defend counter" under it was D_OWN, which it took as D_AUTO, told "You will strike back first."; it's now D_AUTO
  told as such ("You will defend yourself as best you can."), and its help lists no counter.  No test runs it.
- **3. The character's outfits are the ruleset's.**  outfits.txt was HarnMaster's through and through where it
  outfits the character: hmchar's occupations, harn.h's skill names, the book's cultures and classes as tags.  Split:
  games/dq0harn/outfits.txt (the universal kit, the skills' weapons and tools, the bents, the occupations, the
  soldiers, the clerics: the character's, by its tags) and games/dq0/outfits.txt (the denizens' roles, lairs, purses,
  standings, and the format).  **The tags are the ruleset's file's own `tags` line** (its words in RS_OUTFIT's bits'
  order, dq0harn.h's OT_*), not outfits.js's `TAGS` list: the build names no HarnMaster word.  make.js reads each
  referee's outfits.txt in rsmeta.h's RULESETS order (those that have one: dq0null has none, its RS_OUTFIT names no
  occupation and no tags), then the game's, into the one lib/dq0/dq0o.dat: **byte for byte the same file** (the same
  sections in the same order).
- **4. dq0acct: not moved (a question, below).**

**The words.**  Every dq0 test's screen saved before and after (sim/test.js's TRANSCRIPTS hook, not committed) and
compared sentence by sentence (sent.js): 7 of 294 differ, all timing noise: dq0antidote's real-time fight; dq0answer's, dq0jobs' and dq0prompt's denizens met at other moments (a rat 10 or 15 feet off, a squeak ahead or not, "Something stirs."); dq0sway's; dq0reachstop's accepted race (the stop landing a step later); dq0starttime's progress dot landing after "(B) a sconce." (the start's timing).  No sentence of the stunned state, the stance, the help or a place worn changed: "in shock" in the same 10 tests, "You will strike back first." in dq0parse, the places' words as they were.  The save files: nothing written changes (an item's sn and kit.dat's places the same numbers, dq0o.dat the same bytes, RS_HELLO's words not saved).

**Room** (main RAM free, before → after): the UI (dq0) 3,415 → 3,359 (its overlays 1 1,112 → 989, 8 236 → 93, 22
508 → 536, 28 0 → 7, 29 6,577 → 6,595: the packer's placing; parts 1 406 → 263, 11 2,346 → 2,336, 12 2,214 → 2,193);
dq0harn 2,209 → 2,189 (its part 4 4,509 → 4,456, put in bank 5 by the packer; part 1 as it was, 108: its bank, shared
with part 3, was full, so the words are part 4's, asked by rf_answer); dq0null 4,605 → 4,595; dq0w's main RAM as it
was (2,236; part 107 1,481 → 1,471, overlay 3 247 → 237).  **The start** at seed 1: 231.5M → 234.1M cycles (245M
the budget); at 123456789, 308.2M → 308.8M (325M).  **The suite:** 444 of 444, before and after.

**For review:**
- **dq0acct (the question).**  It's the ruleset's own program already, the table's (rsmeta.h's RULESETS: dq0harn's
  "/../acct/dq0acct"), run by dq0life for dq0harn's games alone; dq0null's games have no time between.  What in it is
  HarnMaster's: the healing told day by day (body.c: course.h, watch.h, the library's harnheal.c), the month's
  practice and the wages (skill.c), the sheet read and written (sheet.c, hfile.c: hmchar's format), the physician's
  words (town.c: harn.h's locations), the living by the sheet's social class and the age by its birthday (dq0acct.c).
  What isn't: what solving brings, the standing and the rumours (quest.c), the money's matters (living, selling,
  mending, restocking), the town (dq0town).  Putting more of it "behind the table" means splitting it into a generic
  account every ruleset's games run and the ruleset's pieces (its healing, its practice, its sheet) through rsmeta.h's
  functions: a large change to a program whose words and saves must stay the same, and one that only pays when a
  second ruleset wants a time between dungeons.  **For the user: split dq0acct now (the generic account, rsmeta.h's
  hooks for the healing, the practice and the sheet), or leave it the ruleset's whole program until a second ruleset
  wants the time between?**  Not built.
- One outfits file for the game: a second ruleset with its own `[universal]` and its own tags would clash with
  dq0harn's in the one dq0o.dat (a section twice, the tags twice: the build fails, loudly).  Then the file wants to be
  each referee's (lib/dq0/REFEREE's dq0o.dat, the world opening the save's referee's).
- The bestiary (the creatures' sheets, RULESETS.md) and dq0schol's d100 test are as they were.
- A ruleset's own stance word is matched by its first letter after dodge's d and block's b/p (and before none's n), as
  'c' was: a ruleset's word beginning with d, b or p would never be reached.

## Narration as data, stage 4 (2026-11-12, as built, for review: ENGINE-CRITIQUE.md's §3.2 and step 5, its fourth stage)

Stage 3 left fight_words, sway_words, sky_words and throw_words, the UI's refusals and a command's first words.  This
stage gives the UI kinds of its own, puts those words on the phrase table, and deletes the four functions, and with them
grip_words, time.c's sky[] and refuse.c's said[].  The words the player sees are the same, letter for letter.

- **The UI's own kinds (dq0.h's `UK_*`, declared in msgs.def).**  This is the critique's "kind of the UI's": an outcome
  event's shape (`< E_OUTCOME`, the outcome in w1's high byte) for words that no world's event carries: a reply, a
  refusal, a command's first words.  They are numbered from 0x80, past the hub's events.  They are never sent: the UI
  fills one on the C stack and gives it to outcome_words, so the tests, the denizens and the sound task never see them.
  Each is declared in msgs.def like any kind, its fields named so that said.txt reads them by name, and in dq0.h's
  enum (hubgen.js reads dq0.h).  There are six:
  - `UK_THROW` (THROW rows: ir, how, hard): a throw told.  No mark seen and none carried stay the refusals'.
  - `UK_SKY` (SKY rows: phase, shows): the sky seen, with or without what shows it.  There is one row a phase, and
    `{shows?...}` chooses between the sky's own words and a window's, a skylight's or a shaft's.  Also a shaft's
    refusals and a rope's (own outcomes tore and far).
  - `UK_SWAY` (SWAY rows: res, how, now, order; arg, how the attitude moved): every answer of sway_words'.  The packer
    (sway.c's sway_said) keeps what is the sway's own: the one it's about, as it's said ("the giant rat", "Grub"), and
    how the attitude moved (won over, warmed, swayed, not swayed, badly, a gift spurned).  A row can't work that out,
    since it compares two fields.
  - `UK_FIGHT` (FIGHT rows: out, the record's which, the flags it left them in, the weapon's word, the strikes' lines
    told apart, who isn't seen; arg, the attacker's body): every blow, stray, rise, grapple and dispatch that
    fight_words and grip_words said.  blows.c's fight_said keeps the rest: reading the record, working out who's seen
    ("?": something), cutting the four lines apart, and setting the black-out's once-an-answer flag.  FO_GONE, FO_BLED
    and FO_BOUND are 0xFF, 0xFE and 0xFD, and 0xFE is the table's "any".  So they, and a stray, are own outcomes that
    the packer maps to.  Getting up is one piece, `@rises`, shared by BEAT.risen and FIGHT.rise ("The cave adder coils
    itself up again.").
  - `UK_NO` (NO rows, named by the U_* and A_* enum's own names, as in `NO.A_SEE_NO`): refuse.c's one table of
    refusals.  say_no still says them (it adds the article, the capital and the nameless form), but their words are
    the rows.  `A_LB` (into lb, not said) is now said_q's, with the row's leading new line dropped.
  - `UK_FIRST` (FIRST rows: form FS_*, the verb FV_* as arg): a command's first words (D17), such as "You reach for the
    spent torch.", "You head for the door." and "You look them over."  sway.c's first_word, walk_says and walk_ahead
    still choose (which verb, which thing, its word), but their sayf lines are gone.  **For review:** this is the
    critique's "first words from a *begun* row", as a row keyed by the verb.  It is not yet the world's own begun row
    for the action.  That would need each action's begun row declared once, with the UI rendering it from the
    command's handles; when it comes, the rows are there to share by piece.
- **The table's field index is wider.**  The rows used 31 of 32 fields, and FIGHT alone needs some twenty more (each
  flag is a field).  A field past the 31st is written as its code + 31 followed by a byte (SC_NUM, SC_CHOOSE,
  SC_CHANGED).  A row's condition byte gives 7 bits to the field, so 126 fields at most.  The rows already built are
  the same bytes.  There are 49 fields now.
- **Three more of the UI's words** (said_s[3-5], `{w4}` `{w5}` `{w6}`: SC_STR2 and a byte).  A blow names two actors,
  a weapon and three lines.  This costs 6 bytes of the main RAM.  As before, a word must be in the main RAM (lb, cmd
  or a main RAM buffer).  The renderer runs in overlay 27, so the packers first copy a bank's strings (such as "it", a
  kind's name, the door's word) into lb.
- **What wasn't moved, and why:**
  - **The world's things' words (dq0w's SA_*, kinds.c's said[]).**  These are the world's, not the UI's.  The world
    builds the sentence with the thing's names and puts it in the UI bank's things_text, next to the inventory's words
    and a count's.  The UI reads that text in a dozen places: tt (), the list, the referee's slot, and a strstr for
    "already".  Moving them would change the protocol in three ways:
    - the world would write a code and the names, not the sentence;
    - the UI would expand the code wherever the text is read;
    - the renderer would write into a buffer that isn't lb.

    It would touch dq0w's things.c, things.h and kinds.c, which other work has open (the step planner, the referee's
    worn places).  **Open, for the user:** whether to make things_text coded (the next stage), or to keep the world's
    words the world's.
  - **The sound's mapping (dq0s's world_sound).**  It is not a clean column.  Only E_PORTAL's and E_TRAP's cases come
    from an outcome.  E_NOISE has no rows (it has no words).  dq0s's own listen loop chooses the denizens' and the
    fights' sounds by loudness, square and ranges of outcomes.  A column would hold half of the mapping and leave the
    rest in the code: two places instead of one.  dq0s would also need a generated table of its own, since it can't
    read the UI's banks.  Left as it is.
  - **The look's wording** (seen_words, den_words, stir_words, focus_words, heard_words, felt_words' two, pose_words,
    way_words, facing_words, the time told), fight_note's stances and body_words.  These describe what's perceived or
    a state, not an action's outcome.
- **Room** (the build's table):

  | | before | after |
  |---|---|---|
  | dq0's main RAM | 3,415 | 3,412 (said_s 6 bytes more; sky_words', fight_words' and grip_words' stubs gone) |
  | overlay 27 (the renderer, the words most said) | 695 | 99 (the codes past 31, SC_STR2, more words most said) |
  | overlay 28 (the rows' first bank) | 0 | 3 |
  | overlay 29 (their second bank) | 6,577 | 2,418 (the rows 13,963 bytes: 351 rows of 14 kinds, 59 pieces) |
  | overlay 1 (sky_words gone) | 1,112 | 2,295 |
  | overlay 6 (fight_words gone) | 67 | 2,294 |
  | overlay 10 (throw_words) | 2,761 | 3,065 |
  | overlay 15 (sway_words) | 308 | 736 |
  | overlay 17 (first_word, walk_says; the packer's) | 329 | 2,547 |
  | overlay 20 (walk_ahead) | 2,898 | 3,039 |
  | overlay 24 (refuse.c's table gone; the packer's) | 132 | 4,136 |
  | overlays 16, 19 (the packer placed parts 17 and 3 there) | 1,672, 1,295 | 161, 234 |
  | dq0w, dq0d | 2,236, 2,379 | the same (untouched) |

  The rows' banks have 2,421 bytes left.  When they fill, a third bank can be added the way overlay 29 was.
- **The start** (dq0starttime): seed 1 231.5M → 231.9M cycles (budget 245M), seed 123456789 308.2M → 308.4M.
- **Checked:** every dq0 test's screen was kept before and after (a temporary hook in sim/test.js, not committed), and
  their sentences compared.  8 of 294 tests differ, all of it noise:
  - the real-time fight in dq0antidote (its new sentences all use the rows' right words: "You strike at the cave
    adder with your torch, but stumble.");
  - denizens met at other moments (dq0answer, dq0jobs, dq0prompt, dq0skykin);
  - dq0reachstop's accepted race ("You reach for the tinderbox." before its stop);
  - dq0sway, a key typed ahead echoed a moment later;
  - dq0starttime, a progress dot after the first look's last sentence ("a sconce..").

  No moved sentence is said differently in any test.
- **Tested:** all 444 tests pass.

## The plans' pages on the record store, the private hashes keyed; the styles not moved (2026-11-12, as built, for review: ENGINE-CRITIQUE.md's §5, §6, steps 7 and 8)

Three things were left by "One record store" and "The placers' table and the keyed dice": the maker's plans' pages
off the record store, the private hashes of caves.c, sky.c, plan.c, uses.c and make.c's mixed third, and the styles'
proportions in uses.c rather than the theme.  The first two are built; the third is not, and why is below.  Nothing
the player sees changes.

- **The plans' pages (changes.c's `plt`) on the record store.**  The pages of a plan too big for its bank (plan.c's,
  more regions than PLAN_MAX, with a save folder: plSS0000.dat, 42 regions a page, 32 pages in the maker's bank, 16
  with dq0 -C) had their own lookup and their own least-lately-wanted loop.  Now a page is found by `RP_FIND` (its key
  plt's first three bytes, the stratum and the page; its stamp the clock's next, `plstamp`) and one is made way for by
  `RP_OLDEST`.  The old loop's choice wasn't quite the others': it took each free entry as it met it and went on,
  comparing the entries after it with the free one's stale stamp, so it chose the last free entry, unless a used one
  after it was older than that stale stamp.  `RP_OLDEST`'s `RTF_FREE` takes the *first* free entry, so it would have
  moved the pages about in the bank (and, once the stamps wrap, which page is written back when).  A flag of the
  library's, `RTF_EACH` (rp.h, dq0lib.s), takes a free entry as it's met and holds those after it to its stamp: the
  old loop exactly, in the ROM.  o_pl_put's second search for the page is gone (pl_at's `plk`).  The file stays the
  plan's own, written in place and not versioned (the plan's grown whole each time its stratum is: there's no older
  version to keep).
- **The private hashes, keyed (place.c's `dhash (p, s, a, b, c)`, grow.h's DH_*).**  Thirteen decisions made by
  hashes of their own are purposes now, each row its old formula: a * m1 + b * m2 + c * m3, plus the stratum's seed's
  part (none, its low half, its halves xor'd, or one of its bytes) xor'd with a constant, the sum xor'd with another,
  then mixed (`h ^= h >> 7; h *= 0x2F1B`, its high byte) or left as the sum.

  | purpose | was | decides |
  |---|---|---|
  | DH_CAVERN | caves.c's `ch` (its seed `V.seed`, the halves xor'd ^ 0x3C5D) | a region's great cavern, its size, wander, pillars; a cave's sloping tunnel |
  | DH_AIR | caves.c's air pocket | firedamp, blackdamp and the room it's in |
  | DH_WET | caves.c's `wh` | a room's water's depth, the rubble, a stream's deep stretch |
  | DH_SKY | sky.c's `wh` (c the top stratum's seed's low byte plus k, a byte) | skylights and windows on the surface |
  | DH_PWATER | plan.c's `wh` | where the plan's streams rise |
  | DH_WELLS, DH_SLOPE | plan.c's water under a region, a stairway a long slope | |
  | DH_RING, DH_VAULT, DH_ROT, DH_ROTU | uses.c's ring, a great hall's vault, a room's leavings and an older layer's mark gone to dust | |
  | DH_MIXED | make.c's mixed third | a mixed stratum's chunk left natural |
  | DH_DIAMOND | caves.c's | a stratum's passages on the diagonal (its seed's top byte) |

  **Not a key of `dkey`'s:** `dkey`'s keys are a 32-bit xorshift seeded from the chunk's place; none of these
  formulas can be written as one without changing every number, so each keeps its own derivation as a row (the
  "own derivation" the earlier entry foresaw).  `dhash` is in the main RAM beside `dr`, since its callers are in six
  overlays and the main RAM.  The files keep their little wrappers (caves.c's `ch` and `wh`, sky.c's and plan.c's
  `wh`) where a parameter's type truncates its argument, so the truncation stays where it was.  sky.c's `wh` no longer
  writes `Y.h`, the shafts' stream's state, which it used to clobber; the shafts' function reseeds `Y.h` before
  every roll, so nothing read it.  **Left as they were:** the streams (sky.c's shafts, plan.c's pieces' and details',
  the growers' and the stock's: call order by design) and the hashes outside these five files (fixtures.c's,
  pass.c's).
- **The styles' proportions: not moved (open question).**  themec can carry them only by taking room from the
  theme's words.  struct theme is 1,739 bytes and the record is THEME_SIZE, 4,096: the UI copies it into its overlay
  5's bank at $9000 (4K exactly), and the theme bank's pieces' places sit at $9000 after it.  The words fill the rest
  (2,357 bytes).  The four styles are 4 × 52 = 208 bytes.  Carried in the head, they cut the words' room to 2,149,
  and themegen's linegen (bank/linegen) budgets each clue by `THEME_HEAD + words + its words + 64 > THEME_SIZE`.
  With the head 208 bytes longer, a generated theme within 208 bytes of the room loses a clue it had, and every
  random () after it shifts: another theme, another dungeon.  Keeping linegen's sum at the old head instead would
  leave such a theme too big to compile.  The stand-ins use 1,224 to 1,612 bytes; the generated themes weren't
  measured over enough seeds to say none comes that close.  Ways the user could choose:
  1. the cost taken (the head grows; the seeds whose themes come within 208 bytes of the room change);
  2. a theme's own styles only when its text gives them (a `style` line; a mission bit, TM_STYLES, bits 2-5 being
     free; the table at the record's end, the words' room cut only then).  The house's styles stay the maker's
     default and no generated theme changes;
  3. the styles outside the record (a styles file the build makes, the theme naming a set).
- **Checked.**
  - The plans' pages: every save-folder file's writes (fhash, as "One record store" did: each OPEN, CREATE, WRITE
    hashed, SEEK, by file, in order per task) over 48 games: 8 seeds (3, 5, 7, 42, 99, 1000, 31337, 70000), each with
    -C -T -T -T, -C -T, -T -T, -C, and -T -T -T -T with and without -C.  The last two are the vast stratum's: its plan
    paged, some 100 to 4,000 plan-page operations a game, -C's making way.  plSS0000.dat's writes are byte for byte
    the baseline's in all 16, and every file's in 46 games.  Two games (s5C3, s70000C1) were another of their
    timing's outcomes at the first power-up: the rats met at other moments, a walk in the dark stopped elsewhere.
    Neither game has a big plan.  This build at the second and third power-ups gave the baseline's bytes; the
    baseline at seven power-ups never gave this variant.  life.dat is left out (its date the host's clock).
  - The hashes: a scratch script ran each row against its old formula in 16-bit C arithmetic, 200,000 random inputs
    each; none differed.  Then the maker's every write to the hub's shared banks (each task's runs of bytes, hashed,
    a sorted list per game) over 80 games: 8 seeds (1, 3, 7, 42, 259, 1000, 70000, 123456789), each as it is, -T,
    -T -T -T, -M, -A, -D, -V, -M -T, -A -T -T and -V -T -T -T, a random player's 50 commands and two ways down.  The
    maker's writes were the same in all 80, and the sentences of all 80 the same.  The other tasks' writes move with
    the timing; the baseline is the same as itself at one power-up.
- **Room** (bytes free):

  | | before | after |
  |---|---|---|
  | dq0m main RAM | 4,635 | 4,193 (the pages' block 33, dhash and its rows 409) |
  | dq0m overlays 2, 3, 4, 5 | 947, 527, 51, 845 | 1,175, 596, 62, 915 |
  | dq0m overlays 6, 7, 8, 9 | 508, 165, 223, 477 | 572, 224, 296, 554 |
  | dq0lib (ROM) | 2,507 bytes | 2,529 bytes |

  No other task touched.
- **The start** (dq0starttime): seed 1 231.5M cycles before, 231.7M after; seed 123456789 308.2M before, 309.4M after (budgets 245M and 325M): dhash's generic rows a little slower than the inline formulas, at the maker's first chunks.
- **The suite:** 444 of 444 (440 run from a copy of the tree, whose four machine tests (hwtest, hwtool, fs, mml) want the repository's own folders and fail there for the baseline as well, run again in the worktree and passed).  Every dq0 test's sentences the same as the baseline's but the known timing noise: dq0antidote's real-time fight, dq0answer's and dq0jobs' rats heard at other moments, dq0robrest's rest ending at another moment (a skill improved, hungry and thirsty, not reached).
- **Open questions.**
  - The styles: the three choices above.
  - The chunks' cache's plain stamp comparison (RTF_PLAIN) made round: a behaviour change, not taken.
  - The old keys' omission of the stratum's seed (dkey's purposes before DP_SEEDED, and these rows too: DH_AIR,
    DH_WET, DH_SKY, DH_VAULT and the rots have none): not taken.  It would change the dungeons.
