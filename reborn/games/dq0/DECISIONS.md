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

### The start's time measured, its progress shown -> built/2026-10-30-decisions-archive.md

### The catalogue's room (the margins' pass, 2026-11-03)

The user (2026-11-02): "Stone chisel versus wood chisel might be a good distinction for future items."  Built: a kind's
number a whole byte everywhere, room for 60 more (NKINDS 206), a stone chisel, a wood chisel, a mattock and a spade
(HOUSE.md's digging, for review).

**Design:** items.h's NKINDS, HOUSE.md's digging.

### The answers' first letters (the margins' pass, 2026-11-03) -> built/2026-11-03-decisions-archive.md

### The answers' times held (the kernel's foreground)

The user (2026-10-30): the answer-time checks (D17's, dq0answer's and dq0prompt's) are the user's and never raised;
they swung 100,000-250,000 cycles with any change in how the tasks interleaved.  Their times are to hold whatever the
background is doing.

The rest of this entry, the account of what was built, is in built/2026-10-30-decisions-archive.md.

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

The user asked for a fresh look at the command processor (COMMANDS-CRITIQUE.md and ENGINE-CRITIQUE.md, beside this file), then: "Go ahead and start work on these changes."

The rest of this entry, the account of what was built, is in built/2026-11-06-decisions-archive.md.

## The build places the swapped parts (2026-11-07, as built, for review)

The user: "Go ahead with the auto-packer" (ENGINE-CRITIQUE.md's §1.2, after the parts' stubs and config were made by the build).

The rest of this entry, the account of what was built, is in built/2026-11-07-decisions-archive.md.

### Taking a thing you carry, lighting the torch you hold lit (2026-11-07, as built, for review) -> built/2026-11-07-decisions-archive.md

### The stairs looked at, examined, sniffed toward (2026-11-07, as built, for review: COMMANDS-CRITIQUE.md's step 6, its first slice) -> built/2026-11-07-decisions-archive.md

### The resolver's choice kept (2026-11-07, as built, for review: COMMANDS-CRITIQUE.md's §5.1, step A) -> built/2026-11-07-decisions-archive.md

### The carried thing chosen once (2026-11-07, as built, for review: COMMANDS-CRITIQUE.md's §5.1, step C's first half)

The user approved a staging overlay bank (overlay 24) and step C.

The rest of this entry, the account of what was built, is in built/2026-11-07-decisions-archive.md.

### One "Which one?" for things and denizens (2026-11-07, as built, for review: COMMANDS-CRITIQUE.md's §5.1, step D)

The user: "Do D then finish C, and if you can do them in parallel, do that."

The rest of this entry, the account of what was built, is in built/2026-11-07-decisions-archive.md.

### A state said, the carried thing chosen once too (2026-11-07, as built, for review: COMMANDS-CRITIQUE.md's §5.1, step C) -> built/2026-11-07-decisions-archive.md

### The doors' "Which door?" the same question (2026-11-07, as built, for review: COMMANDS-CRITIQUE.md's §5.1, step E)

The user: "Finish up 5.1, please, on parallel threads."

The rest of this entry, the account of what was built, is in built/2026-11-07-decisions-archive.md.

### One name match (2026-11-07, as built, for review: COMMANDS-CRITIQUE.md's §5.1, step F) -> built/2026-11-07-decisions-archive.md

### "It" and "take all" by the thing, not its word (2026-11-07, as built, for review: COMMANDS-CRITIQUE.md's §5.1, step B) -> built/2026-11-07-decisions-archive.md

### §5.1's last disagreements, and the start's budget (2026-11-07, as built, for review)

The user: "Don't worry too much about the startup time.  If you need to rewrite some of the portions in assembly, do so.  Do whatever cleanup you can, and then start the next piece of the critiques work."

The rest of this entry, the account of what was built, is in built/2026-11-07-decisions-archive.md.

### The second sweep: every command that names a thing answers about it (2026-11-07, as built, for review: COMMANDS-CRITIQUE.md's §5.2, step G) -> built/2026-11-07-decisions-archive.md

### Refusals as data (2026-11-07, as built, for review: COMMANDS-CRITIQUE.md's §5.2, step K) -> built/2026-11-07-decisions-archive.md

### One path for things, one walk over (2026-11-07, as built, for review: COMMANDS-CRITIQUE.md's §5.2, steps H and I) -> built/2026-11-07-decisions-archive.md

### One path for features (2026-11-07, as built, for review: COMMANDS-CRITIQUE.md's §5.2, step J) -> built/2026-11-07-decisions-archive.md

### The dispatch table, stage 1 (2026-11-08, as built, for review: COMMANDS-CRITIQUE.md's §5.2, step L) -> built/2026-11-08-decisions-archive.md

### The dispatch table, stage 2: the refusals in the rows (2026-11-08, as built: COMMANDS-CRITIQUE.md's §5.2, step L) -> built/2026-11-08-decisions-archive.md

### The dispatch table, stage 3: the handlers by meaning (2026-11-08, as built: COMMANDS-CRITIQUE.md's §5.2, step L) -> built/2026-11-08-decisions-archive.md

## One line of sight (2026-11-08, as built, for review: ENGINE-CRITIQUE.md's §4 and step 2, its first part) -> built/2026-11-08-decisions-archive.md

## The game's own library in the paged ROM (2026-11-09, as built, for review: ENGINE-CRITIQUE.md's §1.4, §4 and step 2) -> built/2026-11-09-decisions-archive.md

## One way-finder (2026-11-09, as built, for review: ENGINE-CRITIQUE.md's §4 and step 2, its second part) -> built/2026-11-09-decisions-archive.md

## The hub's numbers, made by the build (2026-11-09, as built, for review: ENGINE-CRITIQUE.md's §2 and step 3, its first stage) -> built/2026-11-09-decisions-archive.md

## The messages' payloads, declared (2026-11-09, as built, for review: ENGINE-CRITIQUE.md's §2 and step 3, its second stage: the intents) -> built/2026-11-09-decisions-archive.md

## The events' payloads, declared (2026-11-09, as built, for review: ENGINE-CRITIQUE.md's §2 and step 3, its second stage: the events) -> built/2026-11-09-decisions-archive.md

## The referee's requests' payloads, declared (2026-11-09, as built, for review: ENGINE-CRITIQUE.md's §2 and step 3, its second stage: the referee's requests) -> built/2026-11-09-decisions-archive.md

## Wear for what's worn (2026-11-09, as built, for review: the user's, "wear" restricted to articles that can be worn)

The rest of this entry, the account of what was built, is in built/2026-11-09-decisions-archive.md.

## The verbs not done yet: the rows, the aliases and the powers' refusals (2026-11-09, as built, for review: the plan for the 50 H_NOT_YET verbs, its batch B0)

Fifty verbs' rows in the dispatch table (actions.c) said `H_NOT_YET`, "You can't ... in this game yet."  The user's rule: each verb is decided on its own, and each should do something in the world rather than say "not yet".

The rest of this entry, the account of what was built, is in built/2026-11-09-decisions-archive.md.

## Grappling (2026-11-09, as built, for review: the verbs plan's B1g, the user's "full HarnMaster grappling")

The user, of the 50 verbs the game answered "not in this game yet": grappling gets HarnMaster's full rules, "grapple attacks, holds, throws, pins, chokes, breaking free", by the Unarmed skill, the referee gaining them.

The rest of this entry, the account of what was built, is in built/2026-11-09-decisions-archive.md.

## Prayer, fasting, swimming and following (2026-11-10, as built, for review: the verbs plan's B5 and B6)

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

The rest of this entry, the account of what was built, is in built/2026-11-10-decisions-archive.md.

## Worn places (2026-11-10, as built, for review: the user's, "Wearing should extend to rings, necklaces, earrings, and other jewelry, as well as scarves, and should keep track of where worn")

The user's words: "Wearing should extend to rings, necklaces, earrings, and other jewelry, as well as scarves, and should keep track of where worn (as should all worn items that can be worn in more than one place)."  The user's decision: rings by the hand only (left or right), up to four a hand, not by the finger.

The rest of this entry, the account of what was built, is in built/2026-11-10-decisions-archive.md.

## Words said, a knock, the minds swayed (2026-11-10, as built, for review: the verbs plan's batches B2 and B3) -> built/2026-11-10-decisions-archive.md

## Cloth, bandages and dressings (2026-11-10, as built, for review: the user's bandages, the worn-places-and-bandages design's parts M and B, its rip and wash)

The user: "A bandage's location should be known, and different material should have different suitability as bandage material.  'rip' and 'tear up' verbs should be allowed to turn a garment, bed sheet, towel, etc. into bandages ... differing effectiveness (for material type and cleanliness)."  Decided with the user: no cotton; dressings soil over days and are changed; wash, not boil (boiling when there are fires).

The rest of this entry, the account of what was built, is in built/2026-11-10-decisions-archive.md.

## The things' verbs: use, push, pull, touch, taste (2026-11-10, as built, for review: the verbs plan's B4)

The user's rule: each verb a meaningful action; push and pull include moving furniture.

The rest of this entry, the account of what was built, is in built/2026-11-10-decisions-archive.md.

## Charge, a throw a way, engage, a split attack, and again (2026-11-10, as built, for review: the verbs plan's B1 and B7) -> built/2026-11-10-decisions-archive.md

## The UI's room won back (2026-11-11, as built, for review: ENGINE-CRITIQUE.md's §1) -> built/2026-11-11-decisions-archive.md

## Narration as data, stage 1 (2026-11-11, as built, for review: ENGINE-CRITIQUE.md's §3.2 and step 5, its first stage) -> built/2026-11-11-decisions-archive.md

## The tests: split by area, scenes, the runner (2026-11-11, as built, for review: ENGINE-CRITIQUE.md's step 10, §9) -> built/2026-11-11-decisions-archive.md

## The placers' table and the keyed dice (2026-11-11, as built, for review: ENGINE-CRITIQUE.md's §5 and step 8) -> built/2026-11-11-decisions-archive.md

## Narration as data, stage 2 (2026-11-11, as built, for review: ENGINE-CRITIQUE.md's §3.2 and step 5, its second stage) -> built/2026-11-11-decisions-archive.md

## Narration as data, stage 3 (2026-11-11, as built, for review: ENGINE-CRITIQUE.md's §3.2 and step 5, its third stage) -> built/2026-11-11-decisions-archive.md

## The user's review of five defaults (2026-11-11, the user's)

The user reviewed defaults this file had marked "for review" and asked for five of them changed; two more were kept.

- **"g" is again's** (Infocom's convention), no longer give's.
- **A vow of fasting is carried in the save.**
- **A sip can bring on the flux.**
- **The sheet names a dressing's cloth.**
- **The days between dungeons soil dressings.**
- **Kept, reviewed:** generated chests brace doors (the maker's own chests and tables beside a door brace it, as a shoved one does: "The things' verbs"); and the looser hmchartongues check (hmchar.hl's native script among its scripts, not first: its dice can't be seeded; the cloth branch's change) is accepted.

The rest of this entry, the account of what was built, is in built/2026-11-11-decisions-archive.md.

## The referee's skeleton (2026-11-11, as built, for review: ENGINE-CRITIQUE.md's §8 and step 9)

**The user's choice, 2026-11-11: option (a), swappable rules are a real goal.**  §8 found the referee interface
halfway: an indirection the game paid for, and HarnMaster everywhere behind it.  (b) would have said HarnMaster is the
house system and dq0null a test double.  The user chose (a): a referee skeleton in a library so a ruleset is its
answers, the interface trimmed, HarnMaster's lists behind the referee, and the meta programs talking to "the ruleset".

- dq0reachstop's race: a test that types as words show at the turns pace depends on the parser's speed; it wants a
  scene that holds the world while the line's parsed, or a pace that waits on a line entered.
  **The user's, 2026-11-11: accept either step.**  Game 2's check passes the stop landing in the step after the one
  it was typed in (the rations taken in hand, then "You stop there."), the pack still at its feet and nothing after it
  done (tests/suites/dq0/gear.js); the game unchanged.
  **The user's, 2026-11-12: game 7 the same.**  Its "stop" typed at "take all"'s first "Taken." may land one thing
  later (the record store's timing flipped it): the tinderbox reached for and taken, then the stop, the pack in hand,
  no third thing taken.

The rest of this entry, the account of what was built, is in built/2026-11-11-decisions-archive.md.

## The step planner and the activity stack (2026-11-11, as built, for review: ENGINE-CRITIQUE.md's §3.3, §7 and step 6) -> built/2026-11-11-decisions-archive.md

## The meta programs' library, their state record, one theme compiler (2026-11-11, as built, for review: ENGINE-CRITIQUE.md's §9 and step 11) -> built/2026-11-11-decisions-archive.md

## One wording for the player and the denizens (the user's, 2026-11-11; the choices as built, for review)

**The user's decision:** an action has one wording, the character's and a denizen's, said with the actor swapped ("You open the door." / "The bandit opens the door.").

The rest of this entry, the account of what was built, is in built/2026-11-11-decisions-archive.md.

## One record store (2026-11-11, as built, for review: ENGINE-CRITIQUE.md's §6 and step 7) -> built/2026-11-11-decisions-archive.md

## The life's record and the clues declared, the generators driven from one place, one rumours' reader (2026-11-12, as built, for review: ENGINE-CRITIQUE.md's §9 and step 11's "Left") -> built/2026-11-12-decisions-archive.md

## HarnMaster's remnants behind the referee (2026-11-12, as built, for review: ENGINE-CRITIQUE.md's §8 and step 9's "Left:") -> built/2026-11-12-decisions-archive.md

## Narration as data, stage 4 (2026-11-12, as built, for review: ENGINE-CRITIQUE.md's §3.2 and step 5, its fourth stage) -> built/2026-11-12-decisions-archive.md

## The plans' pages on the record store, the private hashes keyed; the styles not moved (2026-11-12, as built, for review: ENGINE-CRITIQUE.md's §5, §6, steps 7 and 8) -> built/2026-11-12-decisions-archive.md

## The scheduler table, the continuations' stack, the after-work's queue (2026-11-12, as built, for review: ENGINE-CRITIQUE.md's §3.3, §7 and step 6, stages (d) to (f)) -> built/2026-11-12-decisions-archive.md

## dq0acct split: the account and the ruleset's hooks (the user's, 2026-11-12; as built, for review)

**The user's decision, 2026-11-12:** split dq0acct now.  The time between dungeons is the engine's (the quests and
rumours, the money's matters, the days passing, between.dat, the hand-off to dq0town); what's the ruleset's rules (its
healing, its practice and wages, its sheet, its words for a wound's place, its living by station and its birthday) are its
hooks.  HarnMaster's hooks are HarnMaster's code; dq0null gets minimal ones, and a dq0null game a working time between.

The rest of this entry, the account of what was built, is in built/2026-11-12-decisions-archive.md.

## Narration as data, stage 5: the world's sentences (the user's, 2026-11-12; as built, for review: ENGINE-CRITIQUE.md's §3.2 and step 5, its fifth stage)

Stage 4 left the world's things' words (dq0w's SA_*, kinds.c's said[]) open.  The user's decision (2026-11-12): move them into the phrase table.

The rest of this entry, the account of what was built, is in built/2026-11-12-decisions-archive.md.

## The styles' proportions in a styles file, the theme naming a set (the user's, 2026-11-12; as built, for review: ENGINE-CRITIQUE.md's §5, step 8)

The user's choice of the three ways "The plans' pages on the record store ..." set out: the third, the styles outside the theme's record.

The rest of this entry, the account of what was built, is in built/2026-11-12-decisions-archive.md.

## The chunks' cache made way for round, as the other stores (the user's, 2026-11-12; as built, for review: ENGINE-CRITIQUE.md's §6, step 7)

"One record store" left one store comparing its stamps plainly: the chunks' cache (make.c's `pick`, RP_OLDEST with RTF_PLAIN), kept as it was because changing it would change which chunk is made way for once the clock wraps.  The user accepted that change.

The rest of this entry, the account of what was built, is in built/2026-11-12-decisions-archive.md.

## A command's first letter the sooner: four costs on its way cut (2026-11-12; as built, for review: D17) -> built/2026-11-12-decisions-archive.md

## The theme tools' own floor (the user's, 2026-11-13)

themec (629 bytes of main RAM free) and themegen (705) were under the 2,048 the dq0 tasks keep.  The user's ruling:
exempt them, as tools run alone (by the build, and between dungeons), not beside the game's tasks in play.
build.js's TOOL_ROOM gives each its own floor, 512 bytes; the build warns only under it.

## The town and the scholar's house: the ruleset's hooks (the user's, 2026-11-13; as built, for review)

**The user's decision, 2026-11-13:** give the town and the scholar's house ruleset hooks now, in the account's pattern.

The rest of this entry, the account of what was built, is in built/2026-11-13-decisions-archive.md.

## One wording for an action: the world's (the user's, 2026-11-13; as built, for review)

Stage 5 ("Narration as data, stage 5: the world's sentences") left open three actions worded twice: the world's sentence (an SA row) and the reach's step (TASK.step's `@steps`).  The user's rule: one wording an action, and the world's wins.

The rest of this entry, the account of what was built, is in built/2026-11-13-decisions-archive.md.

## A knockout's bearings, whatever the timing (2026-11-13; as built, for review) -> built/2026-11-13-decisions-archive.md

## Four answers (the user's, 2026-11-13)

- **Turn pace: a denizen's blow lands when it was meant to.**  The world takes a denizen's blow at its intent's time,
  not at its own clock as it is when it gets to it (wfight.c's pc_ready = now + w2), so a fight doesn't depend on the
  passes' timing (dq0engage's last fight, "A knockout's bearings, whatever the timing" above).  Some fights will play out
  differently.  To build.
- **The start's progress dot stops once the first look begins.**  No dot after the first look's words ("(B) a
  sconce.."): dq0starttime's timing noise goes.  To build.
- **The wordings stay as they are.**  "You put the sword in the scabbard." and "You put on the backpack." (the
  world's wording, "One wording for an action" above), not "sheathe" and "put your backpack back on".  Settled.
- **Town prices: set by local availability, loosely on the ruleset's.**  The ruleset gives a price's base (the book's
  figures for HarnMaster: a scribe's day 3d, a sage's three times, a master's month 84d); the town sets the local price
  around it by how much there is to choose from: where there's a lot of local competition the range of prices is wider,
  in a smaller market narrower.  To build.

## The start's dot stops at the first look (the user's, 2026-11-13; as built, for review)

The user's answer ("Four answers" above): no dot after the first look's words.

The rest of this entry, the account of what was built, is in built/2026-11-13-decisions-archive.md.

## Town prices, local (the user's, 2026-11-13; as built, for review)

**The user's words:** "Town prices should be determined by local availability, loosely based on what the ruleset dictates (so
if there is lots of local competition, maybe the range of prices is larger than you would get in a smaller market where the
range is limited)."  ("Four answers" above.)

The rest of this entry, the account of what was built, is in built/2026-11-13-decisions-archive.md.

## A denizen's blow at its time (the user's, 2026-11-13; as built, for review)

The user's answer ("Four answers" above): the world takes a denizen's blow at its intent's time, not at its own clock
as it gets round to it, so a fight doesn't depend on the passes' timing; some fights play out differently.

The rest of this entry, the account of what was built, is in built/2026-11-13-decisions-archive.md.

## Commands while knocked out: not taken (the user's, 2026-11-13)

A blackout can be long ("drifting in the blackness"), so commands queued during it could pile up and all be carried
out on waking.  The user's decision: while the character is senseless, input isn't queued; it's ignored or refused.
Commands already waiting when the character is knocked out go too.  To build (DECISIONS.md's "A denizen's blow at its
time": the last thing that splits dq0engage's fight by timing).

## Commands while knocked out (the user's, 2026-11-13; as built, for review)

The user's decision ("Commands while knocked out: not taken" above): while the character lies senseless no line is
queued to be carried out as it comes round; lines typed meanwhile, and lines waiting as it's knocked out, go.

The rest of this entry, the account of what was built, is in built/2026-11-13-decisions-archive.md.

## Knocked out: the three questions (the user's, 2026-11-14)

- **"You lie senseless." stays** as the answer to a line typed while knocked out.  Settled.
- **Only quit and help are taken while knocked out** (inventory, sheet, map, time and look refused).  Settled.
- **A knocked-out character neither hears nor sees.**  No sounds heard, no denizens seen stirring, nothing sensed
  while senseless; what's around is learnt on coming round (the look on waking).  To build.

## A knocked-out character senses nothing (the user's, 2026-11-14; as built, for review)

The user's decision ("Knocked out: the three questions" above): while the character lies senseless nothing is heard,
seen or otherwise sensed; what's around is learnt as it comes round.

The rest of this entry, the account of what was built, is in built/2026-11-14-decisions-archive.md.

## The line, the way-finder and the chunk lookup in the paged ROM (2026-11-14, as built, for review: ENGINE-CRITIQUE.md's §4 and step 2's next)

The user's go-ahead, 2026-11-14.

The rest of this entry, the account of what was built, is in built/2026-11-14-decisions-archive.md.

## Opening a portcullis (the user's, 2026-11-14; as built, for review)

The user's report: "`open portcullis` doesn't actually raise it...it just says `You set to winding the winch.` but it
doesn't raise. `raise portcullis` does open it, though."

The rest of this entry, the account of what was built, is in built/2026-11-14-decisions-archive.md.

## No look when the light's the same (the user's, 2026-11-14; as built, for review)

The user's report: "When you light a new torch from a burning one, it should not re-look since the light state has not
changed."

The rest of this entry, the account of what was built, is in built/2026-11-14-decisions-archive.md.

## Looking through a door's little window (the user's, 2026-11-14; as built, for review)

The user: "I still can't look through a little window in a door."

The rest of this entry, the account of what was built, is in built/2026-11-14-decisions-archive.md.

## The hold-back's holes (2026-11-14; as built, for review) -> built/2026-11-14-decisions-archive.md

## The dispatch table's rest (2026-11-15; as built, for review: COMMANDS-CRITIQUE.md's §5.2, step L; ENGINE-CRITIQUE.md's row 4) -> built/2026-11-15-decisions-archive.md

## Compartments: containers and surfaces (2026-11-15; as built, for review: COMMANDS-CRITIQUE.md's §4.9, step 6b; ENGINE-CRITIQUE.md's row 4)

The user approved row 4 finished (2026-11-15), step 6b with it.

The rest of this entry, the account of what was built, is in built/2026-11-15-decisions-archive.md.

## Narration as data, the rest (the user's, 2026-11-15; as built, for review: ENGINE-CRITIQUE.md's §3.2 and row 5's Left)

Row 5's last "Left:" note named three things: the world's other free text in things_text, the look's wording with fight_note's stances, and the sound's mapping.  The user approved finishing them on 2026-11-15.

The rest of this entry, the account of what was built, is in built/2026-11-15-decisions-archive.md.

## Compartments and narration: eight answers (the user's, 2026-11-16)

- **Things on a chest's lid slide off when it's opened**, told as they fall to the floor.  To build.
- **A lid's top holds by its chest's size**: each kind with a lid its own room and weight borne, from its footprint,
  not one guess for all.  To build.
- **Furniture is placed in the dungeon** (desks, cabinets, cases, shelves, with their compartments), as the placers'
  rows; every seed's dungeon changes, accepted.  To build.
- **All four compartment features are built next**: a compartment named by its noun ("open the drawer", "look on
  the lid"), locks on compartments other than the first, hidden compartments (a false bottom, a secret drawer) and
  how a search finds them, the backpack's side pocket.  To build.
- **Every reader of a thing's place knows its compartment**: an ally's errand fetch, the letters' "the open chest",
  the choice of where to stow, the nested inventory (a thing on a carried chest's lid "on it", not "in it").  To build.
- **The look's lists get a list renderer** (what's seen, denizens and their stirs, touch, the inventory, a thing
  looked at, a search's finds), the phrases in the table, the words the same.  To build.
- **A turn always says "You turn to face ..."**, whatever's typed after it: the turn's event carries what it needs,
  the command queue not read as it's told.  To build.
- **The well's words get a reader of their own**, so they become rows of the table, the words the same.  To build.

## The turn's words, the well's, the look's lists (the user's, 2026-11-16; as built, for review: ENGINE-CRITIQUE.md's §3.2 and row 5's Left) -> built/2026-11-16-decisions-archive.md

## Compartments: the second stage (the user's, 2026-11-16; as built, for review: COMMANDS-CRITIQUE.md's §4.9, step 6b; ENGINE-CRITIQUE.md's row 4)

The user's answers of 2026-11-16 ("Compartments and narration: eight answers", above): every reader of a thing's place by its compartment, a lid's top by its chest's size, things on a lid sliding off as it's opened, a compartment named by its own name, the backpack's side pocket, locks on compartments past the first, hidden compartments, furniture placed in the dungeon.

The rest of this entry, the account of what was built, is in built/2026-11-16-decisions-archive.md.

## A look in from an overlay, the side pocket named whole; the timing scenes with their furniture (2026-11-16; as built, for review: ENGINE-CRITIQUE.md's rows 1 and 4) -> built/2026-11-16-decisions-archive.md

## The critique's questions: the user's answers (2026-11-17)

GAME-CRITIQUE.md's section 11 questions, and the containers' and narration's open questions, answered.  "To build"
names the phase of GAME-CRITIQUE.md's section 10 the work falls in.

**The player's view**
- **"look around" is a real sweep**: the character turns to look all round, a few game seconds, and ends facing the
  way it started; a plain "look" stays what's ahead.  It keeps the rule "only what the character can see", since the
  character really looks.  To build (B).
- **"l" stays left** (the command grammar's fifteen answers); the help's first steps say so.  To build (A).
- **Plain words on the screen**: wounds and penalties told in words, no "UP" on the status line at 0, a legend for
  the map; the codes and numbers on the character sheet.  To build (9.12).
- **People are "they", beasts "it"** ("The elf unslings their hartbow.").  To build.
- **"heal" without the talent is first aid** on wounds, by skill; with the talent it stays the psionic power.  To
  build.
- **No north-up map**: the map turns with the facing, as now.

**The denizens**
- **Allies fight beside the character and rival factions fight each other**, by morale; the player may start such
  fights.  To build (E).
- **Talk remembers and tells**: repeated words lose force ("It won't listen."); "ask about" is answered from what the
  minds know (the lair, friends and foes, the leader, the hoard, a goal guarded).  In the dungeon only.  To build (E).
- **Denizens find hidden things only by a search of their own**, by skill and time, where they can see or feel the
  place; what they haven't found they don't take.  To build (A).
- **The 7/5 diagonal for every actor**: the denizens' timing and many transcripts change, accepted.  To build (A).
- **A line being typed costs the denizens nothing**: held back while it's typed, they catch up in full once it's
  entered; one give-way rule on one clock for the world, the minds and the senses.  To build.
- **Sight through a door's grille or spy-hole works both ways** (a denizen beyond can see the one watching), and the
  far side is seen by its own light only, not the watcher's torch.  To build (9.9).

**The dungeon and the story**
- **A stratum's size comes from what was built there** (a storm cellar small, catacombs vast, still any size), not
  the dice.  The ways down stay where they fall: not drawn near the way in.  To build (D).
- **Every dungeon changes once, together**: one keyed hash with the game's seed for every die the maker throws,
  containers stocked, room uses varied, sizes by history.  To build (D).
- **Generated goals use the richer forms** (a way out to open, a combine, two solutions), with guardians placed by
  the piece, a prisoner who follows, the cursed dead stilled.  To build (D).
- **The peoples: neither set.**  "Those are just random.  I would like the history, peoples, etc. be part of a world
  plug-in, similar to the referee/game rules.  One such world plug-in should be Harn World.  Another could be the
  World of Greyhawk.  Third could be Troll Lord's Airde/Winter Dark."  And: "The world plugin would also dictate
  deities."  The history, peoples, lands, deities and names become a world plug-in, swappable as the referee is.
  Built before D, today's data moved into it with the dungeons unchanged.  To build.
- **Deeper is poorer**, as intended; rot reaches the caves and chambers too (no fresh rope in a 4,000-year-old cave).
  To build (D).
- **Clues match where they come from**: "A clue from a person matches the scripts/languages they know, unless the
  clue was a scrap or was copied from an older text/inscription."  To build (D).
- **Freeing a hand**: the hand whose thing has a home goes first (a sword to its scabbard, coins to the purse), the
  light last; a light left burning is picked up when a walk starts, if a hand is free; nothing is battered with a
  light.  The rule of 2026-11-03 kept; only the order chosen.  To build (A).

**Engineering and process**
- **Budgets**: the D17 targets, the RAM floors and a silent start fail the suite; the other budgets become trends,
  flagged on a rise past 5%.  To build (A).
- **The headroom rule**: a part or bank under 64 bytes free, or a measure past 90% of its target, has room or time
  won back before the next feature.  A standing rule.
- **The tests move to the game as played**: pinned generated seeds for each goal kind, the switches off where they
  hide behaviour; most transcripts re-baselined once.  To build (C).
- **Game code in the paged ROM**: tried for one module, measured, then decided.  To build.
- **Real-time games reproducible**: each denizen's dice keyed to it, steps applied at their intent's time, a log of
  game time and commands; the clock may stretch slightly when the CPU is busy.  To build (C).
- **The item record gains a byte for compartment state**; old saves converted when loaded.  To build.
- **The docs**: HOUSE.md and DUNGEON.md become references to the game as it is; its history lives in DECISIONS.md
  alone, which keeps decisions only; a README and an OPEN.md.  To build.
- **The grammar records roles** (verb, object, instrument and the rest) in assembly, and becomes the one reading of
  a line: GAME-CRITIQUE.md's 2.8 step 8 chosen over the cheaper way.  To build (B).

**The containers' open questions**
- **A thing put on an open lid is refused.**  To build.
- **Lid tops**: the chest's and the case's alone, as built.
- **Denizens follow the character's compartment rules**: lids opened, locks unlocked or forced, hidden compartments
  found by their own search; their own gear in their packs and purses.  To build.
- **A compartment looked at describes itself** (open or closed, locked, what's in it), not its holder.  To build.
- **The desk's hidden compartment becomes "the secret drawer"** once the parser tells it from "the drawer".  To build
  (B).
- **"case" names both** the furniture case and the bolt case, the one most to hand meant, as built; "the bolt case"
  names it once the parser takes whole phrases.
- **Where to stow offers a pack's side pocket as a place of its own.**  To build.
- **A hidden compartment's difficulty**: as built (its holder's quality, worse by touch).

**The narration's open questions**
- **The renderer's run of plain letters copied in assembly**: the UI's 40 bytes or so of main RAM spent.  The UI
  then sits under the headroom rule's 64 bytes, so its room is won back before its next feature.  To build (A).
- **The inventory's slower finish is accepted** (its first words as quick as before).
- **The rest of the hand-written words go to the phrase table**: the lists' joins and frames, a thing looked at, the
  "Which one?" choices (with B).  To build.

**Next**: phase A, the safety net and quick fixes, begun.

## One path for every actor (the user's, 2026-11-17)

"Please make sure that the player and any agent's interaction with the world are handled the same, in all cases."

- **The rule.** The character and every denizen and ally interact with the world by the same rules and the same code:
  moving and the body, portals and locks, things, hands and compartments, lights and tools, perception, search,
  combat, hazards, costs, noise, the words told and the sounds played.  Actors differ in what they are (skills,
  senses such as darkvision, size, kind), never in the rules applied to them.  An asymmetry is a bug.
- **What a mind chooses** (when to search, whether to rummage in a chest) stays the mind's; what it can do, and how
  the world answers, is the character's.
- **Settled by it** (OPEN.md's N2, N3, N5, and the bodies branch's gaps): a denizen's search seen or heard is told
  as the character's is; a denizen can take from containers and open them as the character can; a robber can pick a
  box's lock as the character can; a denizen opening a lid tips off what lies on it; a denizen sees by any light,
  the character's torch included.
- **To build**: an audit of every interaction for the two paths, then one path for each (GAME-CRITIQUE.md's E4, its
  "later, the character as actor 0", and E6's action table).

## The one-path plan and phase A's questions: the user's answers (2026-11-17)

ONE-PATH.md's decisions, and OPEN.md's questions from phase A's branches (built/2026-11-17-*.md), answered.

**One path for every actor (ONE-PATH.md)**
- **The character becomes actor 0 first**: its record moved into the hub's actor table, then every act rebuilt on
  it (chosen over the staged way inside the world: every task and the save change together).
- **Now, alongside phase B (the parser) and phase C (tests on the game as played).**  It takes in phase E's denizen
  work; the world plug-in and phase D follow.
- **Denizens hear one another's footsteps by a cost along the straight line** (walls and doors counted, rock stops
  them); loud events by a real pulse; exact whenever the character is at either end.
- **The D17 speed targets: "Rethink the D17 speed targets.  Please investigate the assumptions there and reset
  those that do not make much sense."**  To do, before any measure is made to fail again.
- **Searching is anyone's**: "Any actor should be able to search about for things when exploring, or when
  specifically trying to tail prey (or the player), or when on patrol.  There are probably lots of other reasons to
  search about, and each actor should do so based on its own characteristics.  Please remove the specific
  "robbers" type since searching about is not only for them...anyone may want to find/steal something."

**Hands, lights and the pack**
- **A drawn weapon keeps its hand in a fight**; outside one, the home-first order stands.
- **A rest puts a burning light out first** (for every actor).
- **Any move away takes up a light the rule left burning**: a walk, a climb, going up or down; else "You leave the
  ... behind.".  A light the player put down stays.  An unlit light goes home like any thing.
- **A named light is used for battering as told**; it may break or go out.
- **The side pocket is asked about** even when the pack is the only place, as built.
- **"stop" ends everything under way**, a fight's part and the reach it took over.

**The parser and the words**
- **Pillars and a window's bars are tie points** for a rope.
- **"pick" always picks**; "unlock" uses a key if one fits.
- **"take all from" names what's taken in one list** ("You take a chisel and a key from the case."), and empties a
  hidden compartment once it's been found.
- **A thing put or dropped on a part of the room is placed there if it can rest there** (stairs, a ledge, a floor
  within reach); refused only where nothing can rest (a wall, a door).
- **"say" alone shows the quoted form.**
- **The UI's 40 bytes are kept**: the copy loop isn't spent (it measured slower).
- **An unseen fighter stays "it"**; the two rewordings for "they" are accepted.

**The denizens and sounds**
- **A key, a bar and a pick make quiet sounds**, heard close by under the one hearing rule, for every actor.
- **A follow out of reach says so once**, when it starts.

## The D17 targets rethought (the user's, 2026-11-17)

The user: "Rethink the D17 speed targets.  Please investigate the assumptions there and reset those that do not
make much sense."  The investigation and the targets as reset are built/2026-11-17-d17.md's; the user then chose
how they go in and what is built to meet them.

- **The targets now** (tests/lib/trends.js's D17):
  - **a first letter within 720,000 cycles** (a fifth of a second at the board's 3,579,545 Hz) for every command,
    looks and the map included (a look's was 1.79M);
  - **no pause longer than 1.79M** (half a second) between two letters of a reading once its words begin.  A
    reading's whole time is a trend: it grows with the text and the ground seen.
  - Measured at 115,200 baud; the median of three typings; a line typed while the character is busy fails the
    test; no measure excused by a list or by a trend already over.
- **They go in now; the misses are won back next.**  At 8a9b3e42 the suite fails 4 tests on 9 measures: a look
  one way's pauses (3.2M-5.0M), the look across (17.1M), the map's pause (5.0M), "attack C" and "attack A" (2.2M-3.7M
  to a first letter), "drop 1g" (2.7M).
- **Every command says its first words at once**, in the story ("You close with the giant rat.", "You count out a
  farthing."): a status line's change alone isn't the answer.
- **Long readings, all four fixes:** a look tells what's near first and goes on as the rest is worked out; the maker
  builds the map's pieces ahead of the character's eyes in the background; the map draws row by row; the senses'
  look is made quicker as it is.
- **The timing tests also run at the menacing pace, and once without the test switches** (the game as played, where
  each step of a reach costs the world 2-11M cycles today).

## The merged branches' questions: the user's answers (2026-11-18)

Asked after polish, loadguard, looks, pb, buildspeed, maker and fw were merged (OPEN.md's L6-L7, H9-H10, P4-P10,
M3-M5), with the images' commit.

- **A look one way into ground not made yet tells what the made ground shows first** (M5), then the rest once the
  maker has made it. A look no longer waits whole on a grow.
- **The images are committed now, with the misses named** in the commit, though the suite doesn't pass whole:
  the playable build matches the code.
- **The maker gets room of its own for the chunks it makes ahead** (M3): its own banks (2,707 bytes a chunk, three
  to an 8K bank), so a full reader's cache doesn't stop the making ahead and no read chunk is pushed out.
- **The making ahead stands back for the game's own time** (M4), as built: the start, long tasks, death, the
  world's turn at the turns pace, as well as commands and typing.
- **Counted first words only when the UI's own view shows enough carried** (P7): "You count out two backpacks." is
  said at once only when two are carried; otherwise the world's answer comes first. (The user's first choice was to
  always wait for the world; told that "drop 1g" would then miss its target fourfold, the user chose this.)
- **The closing line dropped when the engage line has just been said** (P8): "The cave adder engages you: you stop
  walking." alone.
- **An attack that names its weapon tells the hold's words first** (P9): "You lift the sword. You close with the
  giant rat."
- **A look's first words use the seen name** (P10): "You look at the giant rat.", as an attack does.
- **"empty X" puts its things down here, or on Y when named; a liquid is poured away** (P4); it takes a moment a
  handful, as a take does.
- **A thing put on a ledge, in a corner, the centre, a side or a passage: walk there and put it down** (P5).
- **A named tool wins** (P6): "unlock the door with the lockpicks" picks, even when a key fits; plain "unlock" uses
  a key if one fits.
- **A rest puts out every burning light within reach**, not only those carried (L6).
- **A denizen's stir is held until a look's far part is told** (L7): the look reads as one piece.
- **A thing is put back only where there's room; the room is looked for before the step is told** (H9): "You keep
  the iron key in hand."
- **Things in a pit are told "In the pit ..." and are out of reach from its edge** (H10).

## The D17 pause, a look's rest, pouring: the user's answers (2026-11-19)

- **D17's pause target stays on readings only** (looks and the map): no gap over 1.79M cycles between a reading's
  letters once its words begin. Other answers' gaps are the game's own time between first words and outcome (a step,
  a wait, a reach); fw4's survey (306 of 762 timed answers over 1.79M, nearly all of that kind) stays a trend, to catch
  a real stall.
- **A look's rest gets a lead-in** (L8): told after the prompt, it reads as the look's continuation ("Peering on,
  ...").
- **A look's rest with nothing new says nothing** (L8), as built.
- **Pouring from one container into another is built** (H11): "pour the water into the flask" moves the liquid,
  not away.
- **No tool to reach into a pit** (H12): climbing down stays the only way.

## Each denizen's dice, catching up, liquids: the user's answers (2026-11-19)

- **Each denizen's own dice is dropped from the one-path plan.** Built and measured (gw1), it didn't reduce the runs'
  variation (147 differing sentences against 155) and reshuffled seven test scenes; one shared dice stream stays.
- **A late denizen catches up all at once**, as built (gw1): the user's "catch up in full" stands, though the denizens'
  timing between test runs is noisier (226 differing sentences against 147).
- **Two different liquids don't mix** (H13): the pour is refused ("The vial has wine in it already."), as built.
- **Lamp oil is a liquid of its own** (H14): poured and carried in any container, like water, not only the flask's
  lamp fuel.

## Facing, and the first command: the user's answers (2026-11-20)

- **Facing is accepted as built** (gw3): denizens see only before their eyes (and right beside them), and their minds
  turn them toward a noise, their target or their way, by the character's rules. It moves when and where denizens are
  met in 36 tests.
- **A rest may end with a robbery noticed**, not only an approach seen (dq0lurkrest's widened check): a robber unseen
  behind a resting character can pick the purse before it's seen.
- **The start's work finishes before the first prompt** (the denizens' kits, the map's scan), so the first command
  is answered on time; the start takes a little longer.

## A senseless foe's shock rolls as its time passes (the user's, 2026-11-21)

- **A foe knocked senseless rolls its shock test as its time comes**, each round (or minute) as the game's time
  passes, not an hour of rolls at once at the moment it falls. The same rule and the same odds; the dice fall in
  another order, so a fight's exact outcome can change. Chosen so that a command typed as a foe falls is answered in
  time (D17): an hour of rolls at once cost up to ~730K cycles.

## Knocked out, oil and reach: the user's answers (2026-11-21)

- **A sleeper struck wakes on the first blow** (K11): the first blow lands undefended, and wakes the sleeper; for
  every actor alike, the character and every creature (one path). Not "struck undefended till it dies".
- **A command typed mid-round waits for the round** (K10): the blow's words first, then the command's, as built.
- **A senseless creature nobody asks about catches up its rolls when next asked** (K12), as built.
- **The character reads "senseless" for the whole black-out** (K13), as built.
- **A knockout from a fall or a trap goes the same way as a blow's** (K14), its rolls a round each, as built.
- **A wound or fatigue taken while out makes the later rolls harder** (K15), as built.
- **The shock rolls' counts are saved with the game** (K16): a game loaded mid black-out goes on where it was.
- **Lamp oil poured out lies on the floor a while** (H15): a pool that burns if a flame reaches it, and is slippery
  underfoot, for every actor.
- **The flask of oil is an ordinary container** (H16) holding lamp oil as a liquid, like the vial: emptied, refilled
  with anything.
- **A thing known in a container within reach is to hand at any depth** (H17), each level deeper taking more time.

## The command's window: the user's answers (2026-11-22)

- **A creature's kit being made is not made resumable** (T1): its overshoot, when a kit is under way as a line
  arrives, is left as it is.
- **The band of lines typed mid-fight is judged with a player's pause between lines** (T3), not back to back.
- **The referee's reply carries the blow's own summary fields** (T4: condition, health, limbs), the rest of the
  summary recomputed after it: a change to the referee skeleton.
- **A look asks for the bodies' look itself** (T2).
- **The lag sweeps become a tool** (T5: lineprof --sweep).

## The referee's reply and the pause: the user's answers (2026-11-22)

- **The referee's reply also carries the limp and the steps** (speed11's question 1), with condition, health and
  limbs: a step taken right after a blow sees that blow's limp.
- **The player's pause between lines mid-fight is one second**, as built (speed11's question 4).
- **A line typed mid-round is made faster, not timed from the round's end** (speed11's question 6): its first
  letter still within 720,000 cycles of its Enter.
- **dq0venom joins the known noise** (speed11's question 7): its skylight's light by the minute its look lands.

## After speed12: the user's answers (2026-11-23)

- **dq0felled and dq0robbed join the known noise** (speed12's question 1): their sentences by the tasks' timing.
- **Features now, with a small speed branch alongside** for the leftovers (dq0letdens' "attack A", dq0letwhich's
  "F", the give-way after a walk's first words).
- **All four of one-path stage 2's remaining areas start at once:** things and robbery; the step and its hazards;
  digging; climbing, links and lights.

## Stage 2's questions, first round: the user's answers (2026-11-23)

- **The character builds anywhere, with materials** (dig1's question 1): hanging a door and walling up a way at any
  doorway or passage, not only a wrecked door's frame or a dug way; a hammer and a door's leaf or stones; a test
  (Timbercraft or Masonry); the builders' times.
- **Each faction knows its own traps** (step1's question 2): its own walked round from the start; another's known
  only once found.
- **A winged body passes over floor traps, pits and spilt oil** (step1's question 1); a door's or a wall's trap
  still takes it.
- **Bandits, soldiers and cultists carry a torch and a tinderbox; dwarf miners a tinderbox** (climb1's question 6):
  their outfits changed, every test with their kit with them.
- **The character robs one bound, held or asleep, as well as one senseless** (things1's question 10).
- **A denizen that catches the character filching: its mind decides** (things1's question 9), its attitude moved
  against the character by its own nature, not always to hostile.
- **A robber carries its takings home** (things1's question 2, C10): on its body till its lair, so one killed on the
  way gives them back.
- **The minds use the new errands** (things1's question 1): a denizen wears armour it finds that's better, stows its
  loot, gives a fellow food, a weapon or a light it needs, and drags furniture to brace a door.
- **A filch is the character's Legerdemain against the victim's hearing** (things1's question 8): a new purpose.
- **A wound from a trap or a fall tests a creature's courage as a blow's does** (step1's question 3).
- **"You hear a cry of pain" is told only where it can be heard** (step1's question 4), by the senses' hearing like
  any noise.
- **Climbing out of a pit ends with a step onto its edge for every actor** (step1's question 6): the character pays
  the step's time too.

## Stage 2's second round, its questions: the user's answers (2026-11-24)

- **Every actor's test gets a fresh die of its own** (climb1 9, things2 12, build2 10): no more of the referee's
  quiet dice repeating for a creature's tests when nothing else rolls.
- **Binding, with a rope** (things2 1): "tie up the bandit" on one senseless or held; one bound can't act until it
  slips free or is cut loose; the same rules for every actor.
- **A door's leaf and stones are things to carry** (build2 1): taken from a broken door or rubble, carried, used at
  any doorway or passage; a new door made where one is hung.
- **The leaders carry a torch and a tinderbox too** (build2 8): the bandit lord, the captain, the priest.
- **A torch hangs at the belt** (build2 9), as a lantern does: the pouch for it dropped.
- **A creature turns to a cry of pain it hears** (step2 4), as to any noise, by its own nature.
- **A faction knows its own door traps too** (step2 1): its folk pass them safely; strangers learn of one only by
  finding it.
- **The character learns of a trap it sees a creature find or step round** (step2 7).
- **A winged body crosses an open pit** (step2 8), not only a covered one.
