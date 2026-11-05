# Dingus Quest Zero's house rules

Every rule the game makes where the book (HarnMaster 3.5) is silent, or where the game chooses how to use it: what
each says, its numbers, where the code applies it, and why.  The book's own rules are the referee's (games/dq0harn,
on the C SDK's harn.h) and aren't repeated here; their places are given as the book gives them (Skills 8, Combat 6).
The game's words only: nothing here is the book's text.
The design the rules serve is DUNGEON.md's, DENIZENS.md's and SOUND.md's; when and why each was decided, in the
user's words, is DECISIONS.md's.

Drafted 2026-10-19 (Phase 3A, "rules on record") from the code's comments, DUNGEON.md and DENIZENS.md, for the user
to review.  The last two sections are what's decided but not built yet, and what's still open.

How to read it.  Each rule says whose it is: **the user's** (the user's own decision: the words are DECISIONS.md's, and
they win over anything here); **the book's** (HarnMaster 3.5 or the HarnWorld Bestiary, by chapter and page); **the
house's** (the game's own, where the book is silent or the game chooses how to use it; "the lead's reading" is the
house's reading of the user's words).  A section's heading says when it was drafted and how it stood.

**Accepted, 2026-11-03.**  The user reviewed this file that day (DECISIONS.md's HOUSE.md's review: A1 to A27 are its questions)
and accepted every rule marked for review or drafted as of that date, on its own or by its section's heading, and the first
draft's sections with no date on their headings (a rule marked the user's or the book's for its numbers only), "until a playtest
says otherwise"; but those the review's decisions settle, whose rule says what the user decided or Decided, not yet built (below)
does: Words reach a creature, Their tongues and Characters' second language (A1); Who trades (A2); A skill used is marked (A3, A7)
and The rolls come after a night's rest (A3); Healing, the book's (A4); Death, and the autosave (A5); Sleep and rest as built (A6);
Carrying on (A8); The creatures' sheets (A9); What strikes and A whetstone (A10); Morale's closing note (A11, A14); A purpose names
one skill and Hiding oneself (A12); No trap sprung (A13); Dead, its things on its body (A15); A tool's use takes its reach first and
Climbing a shaft (A16); Keys that fit (A17); A shaft as a way out (A18); Firedamp's burst (A19); Nothing's read in the dark (A20); The
evil's goal won where it's done (A21); An eighth off (A22); The places (A23); Lighting by a fire-maker (A24); A chisel and a hammer
(A25); Every use of a tool (A26); The hour (A27).  So an older *for review*, or "drafted for the user to review", is only a rule's
history.  A *for review* mark from now on is a new rule, not yet seen by the user, and says so: *(new, for review)*.

The words for the game's parts, each the owner of what it does: **the world** (the bodies, the things, the doors and
traps, the clock), **the referee** (a ruleset's rules: HarnMaster's, or the null one's), **the maker** (the dungeon as
it's grown), **the minds** or the denizens' task (what a denizen decides), **the senses** (what's perceived), **the
UI** (the commands and the words told), **the hub** (the memory they share); **a part** or **an overlay** is a piece
of their code.  A **denizen** is any creature but the character: **an actor** while it's about the character (moved a
step at a time), else only **a record** (moved coarsely).  The book's peoples go by the game's words too: the Khuzdul
(Khuzan) are dwarves, the Sindarin elves, the Gargun orcs.  **Phases** and **slices** ("slice 4.7") are the detailed
plan's steps, **D5** or **G2** its and the game plan's decisions, **the user's answer 7** a numbered answer to a plan's
questions (the command grammar's in Commands, the outfits' plans' in Outfits and The peoples), all in DECISIONS.md; the
plans themselves are outside the repository.  **The old design** is what was designed before the plan was realigned
(2026-10-16).  **The turns pace** is play with the game's time run ahead in slices of the clock, **the real paces** the
rest.  Several of dq0's flags (-I, -N, -J, -K ...) are the tests', and **the soak** the test that plays the game through.

One scale each (rs.h's scales and units): every difficulty and every circumstance is in **steps**, 5 points each on a
d100 test (a request's steps, plus easier, minus harder; a thing's, a lock's or a trap's difficulty the steps worse a
test of it goes); every size in pints; every weight in quarter pounds (told, and written in the data, in pounds);
every distance in feet, kept in the map's squares (5 feet each) where the map counts it, a thickness in eighths of
an inch.  The data files (the themes, the bestiary's kinds) are written in steps, pints, pounds and feet, and their
compilers make them the game's, once.  As a guide to the steps: 1 or 2 a little harder (haste, a simple lock), 4
hard (an iron door to force), 6 to 8 very hard (a sound lock, a thing hidden well, a window's bars), 10 or more all
but beyond most (a thing hidden cunningly).

The rules marked *(every ruleset's)* are the game's own whatever the ruleset: their code is games/dq0/rshouse.h,
which each referee includes (RULESETS.md), or the world's, so the null ruleset (dq0 -r null: its own rules are The null ruleset's, below)
plays them as HarnMaster's referee does.

## Tests

- **A purpose names one skill.**  The game asks for a purpose, never a skill (rs.h's P_*); the referee picks the
  skill a GM would call for: sight, sound, smell and searching Awareness; stealth, hiding a thing and hiding oneself Stealth (Hiding oneself, below);
  climbing Climbing; jumping Jumping; swimming Swimming; balance Acrobatics; forcing and lifting Strength (an
  attribute times 5); picking a lock, disarming and making out a mechanism Lockcraft; enduring Condition; resisting
  poison Endurance times 5; resisting the mind Will times 5; dodging a hazard Dodge; first aid Physician; lore Lore;
  tracking Tracking; persuading and intimidating Rhetoric; animals Animalcraft; digging Mining; guile Intrigue;
  throwing Throwing; quickness Initiative; a thing groped for Dexterity times 5; keeping one's feet Agility (the book's
  stumble roll), getting up the same (the Rise, if it's tested: limbs.c's); a map drawn Drawing; keeping one's bearings
  underground Awareness, or Mining if it's the higher (Bearings
  underground, below: bearing.c's); keeping count of the hours Awareness (the time of day believed, below).
  (dq0harn.c's purpose table.)  Why: the game's code stays the same whatever the ruleset; only the referee knows
  skills.
- **A skill never opened is tried at its opening level,** less five a point of its group's penalty, as an opened
  skill is (the physical penalty for a physical or combat skill, the universal for the rest: Skills 6), as if
  opened on first use.  No test is refused for want of training: only a tool gates one (no lockpicks, no picking).
  (dq0harn.c's test_sk.)  Why: a character can try anything a person could (the user's, 2026-10-20).
- **A theme's own purposes** (P_THEME on) are an even chance, 50 and the steps, till themes say what they test.
  (dq0harn.c's test.)
- **Appraising a thing** uses the craft that knows it: weapons and armour Weaponcraft, metal Metalcraft, wood
  Timbercraft, hide Hideworking, cloth Textilecraft, pottery Ceramics, jewels Jewelcraft, books and bones Lore, food
  Cookery, stone Masonry, glass Glassworking.  (dq0harn.c's appraiser table.)  It's asked for by name, and tells only
  what's asked: its value, its quality, or both (Looking at things, below).  **The first answer's kept** *(the user's,
  2026-11-03)*: asked again of the same thing, a door or a wall's rock, the same die and the same words, till the skill
  improves.  *(The house's way:)* nothing's stored: the dice for it are a hash of the thing (an item's own number, a door's
  or a wall's square), the skill's mastery and the game's seed, the game's own dice put back after (rshouse.h's JUDGE_IN,
  way.c's judge_dice), so its other tests roll as they did.  A performance (entertaining) is a new die each time.
- **Words reach a creature only in a tongue the character speaks:** its own, common, and any its file's "language"
  lines give.  Persuading and deceiving in another tongue does nothing (the reply's result 2); a threat wants none.  **A skill used through a tongue is capped at that tongue's mastery** (the book's, Skills 11; the user's, 2026-11-03, A1): its own tongue its Language's, another its "language NAME=ML" line's.
  **Elders, kings, captains, a Sindarin warden, every leader** (kinds.txt's rank, a warden by its kind) **knows common too** (the house's, the user's), so each people has someone to talk to.
  (dq0harn.c's speaks; dq0d.c's leads.)
- **Picking a lock needs lockpicks** to hand, as the book's Lockcraft wants its tools.  (dq0harn.c, RS_TEST.)
- **Searching for, or disarming, what magic hid** is only for magical means; till spells come in, it fails
  (result 1).  (dq0harn.c, RS_TEST.)

## Noticing

- **A sense is 0 to 9** from its attribute: 3 or less none, 18 or more 9, else (attribute - 3) x 3 / 5; an average
  10 or 11 gives 4.  (dq0harn.c's sense.)  Why: the senses' task (dq0p) works in one scale for every actor (a
  sense's keenness, not a difficulty).
- **Listening as it goes** makes hearing two better (at most 9), and a sound test two steps better.  (dq0harn.c's
  status; rshouse.h's house_notice.)  *(Every ruleset's.)*
- **Pace while noticing:** a creep or a slow walk one step better (but not feeling the way in the dark), a jog two
  worse, a run four, a sprint six.  (rshouse.h's by_pace.)  Why: the faster one goes, the less one takes in.
  *(Every ruleset's.)*
- **In shock, four steps worse** on top of the universal penalty already in every test.  (sheet.c's o_noticing.)
- **A hazard underfoot** (a trap stepped onto) is noticed by a passing glance: twice its hiding's steps worse, and
  two more.  Searching as it goes (watching for traps), the character notices it as a search would: its hiding's
  steps worse.  (rshouse.h's house_notice; wtrap.c.)  *(Every ruleset's.)*
- **A thing lying about is made out** (named) within 1 + sight / 2 + seen squares: the character's sight (0 to 9),
  and its kind's seen, how far off it's made out past a speck (the catalogue's, in squares: a key or a coin 1, a
  sword 4, a chest 7; a theme's written in feet); further off it's only "something", small or large.  (seen.c's
  afar; appraise.c.)  Why: the bigger the thing, the further it's made out.
- **The Khuzdul always know north underground** (the summary's northsense), a trait of their species.
  (dq0harn.c's status.)
- **Hearing and being heard:** Sound, the air and moving quietly, below; the floor's traces and smell: Light and traces.

## Moving and tiring

- **Paces and their times a square** (5 feet), from the character's Move: a walk 200 / Move tenths of a second
  (half its Move in squares a round, the book's walking); a jog 100 / Move, a run 50 / Move, a sprint 33 / Move
  (the book's jog, run and sprint); a slow walk twice a walk's time, a creep three times.  None slower than a walk.
  (dq0harn.c's status.)  Why: the book has no creep or slow walk; they trade speed for quiet and noticing.
- **A Move under 4 walks as Move 4 does,** 50 tenths of a second a square.  (dq0harn.c's status.)  Why: a pace's
  time a square is kept in a byte, and a slow walk or a creep at a Move of 1 or 2 wouldn't fit.
- **A step takes a rise or a drop of 2 feet**, steps or stairs 4; more is a ledge, climbed.  (world.h's STEP_MAX,
  STAIR_MAX.)
- **Fatigue builds by the time spent at each pace, added up across moves.**  A level of fatigue is 6,000 units of
  exertion: a sprint adds 10 a tenth of a second (a level a minute), a run 5 (one each two minutes), a jog 2 (one
  each five minutes), a walk one each 144 tenths (a level a day); a slow walk or a creep tires as a walk does.
  Standing still wears exertion off first, a unit a tenth, then takes a level off each ten minutes.  (sheet.c's
  o_spent.)  Why: the book's movement fatigue, made continuous so short dashes add up.
- **Strenuous work tires** (the user's, 2026-10-20: "Tiring work includes swimming, climbing, running, battering down
  a door"; digging too): a climb's rounds, a dig's spells and a battering's blows are strenuous work, their time the
  referee's as the paces' is (RS_EXERT's GA_WORK), added up with them; HarnMaster's at the book's rate for strenuous
  activity, a level each five minutes (Skills 6; a swimmer's the same), 2 units a tenth as a jog's: a ten-minute
  spell of digging two levels, a climb's round a thirtieth of one, a blow a hundredth.  Each is charged as it's done
  (after its own test: a spell's Mining, a round's Climbing, a blow), a spell given up part way for the part of it
  done; a level more is asked of as the work goes on, soft ("You're tiring.  Keep digging?"), as a walk's is (a
  climb's, answered no, goes on: there's no stopping part way up).  Swimming the same (a minute's swim: slice
  4.9's).  The null ruleset tires the character nothing.  (dq0w.c's arrive, stop and s_batter_on; sheet.c's o_spent, WORK.)  Why:
  the book's rate; a run's (a level each two minutes) would leave one spell of digging five levels down, every
  physical test 25 the worse.
- **An incline told** *(the house's, 2026-11-02, slice 4.10's: for review)*: a walk along a passage tells "The way
  rises." or "The way falls away." once its floor's risen or fallen 4 feet or more since the last beat told (a turn, a
  doorway, a quiet stretch, a rise), steps at once.  Before, a step of 2 feet or more was told each time and a gentle
  incline never; now a sloping tunnel's told as it climbs, about every 20 feet at a foot a square, a rough floor's
  ups and downs not.  Uphill is slower for every actor, as it was (half again a step's time a foot of rise).  The
  character's climb told is strenuous work too, half a step's time a foot, 4 feet at each telling (its walk's beat:
  a rise under 4 feet costs no more than the walk); the denizens' walks tire them no more uphill than on the level
  (the house's, the denizens' bodies, below: only their fights are counted).  (dq0w.c's beat.)
- **Ways that climb, grown** *(the house's, 2026-11-02, slice 4.10's: for review)*: one chunk in eight of a natural
  stratum's or a mine's, with no way between strata in it, by its own numbers (none of the grower's dice; dq0 -P, the
  tests', every chunk that has room, hub.h's GE_RAMP): off a passage, a straight way through the rock to a small
  chamber on its own level (5 squares by 4, a room of the chunk's): in natural rock a sloping tunnel, a foot a
  square up or down for 10 to 16 squares; in a mine a raise, two feet a square for 8 to 12 (a walk's steepest step).
  Walked, not climbed, by every actor, told as any incline is, the map's heights showing it.  (dq0m/caves.c's
  incline, overlay 8.)
- **A load borne** *(the house's, 2026-11-03: the user's "if you are awake and standing with the burden, you should still
  get fatigued", which settles standing; for review, what the book doesn't speak to: the numbers, lying, sitting, kneeling
  and senseless)*: the book has no such rule (its encumbrance a penalty alone, Combat 2 and 6; its GM "may" assess fatigue for
  prolonged activity, Skills 6; ten minutes' rest a level whatever's carried).  Heavily burdened or worse (encumbrance 3 or
  more; "a little burdened", 1 or 2, is no tiring burden: about half of ten thieves tried began at 3 or 4, the rest under)
  tires by a level each 120 / (encumbrance - 2) minutes: two hours at 3, an hour at 4, forty minutes at 5, 24 at 7, on top of what's being done: walking,
  crawling, running, at work, fighting, and **awake on its feet standing or crouched**, when it also rests not at all.
  **Asleep, lying down, sitting, kneeling or senseless it bears nothing**: no tiring by it, and it rests as ever,
  overloaded or not (shrugging the pack off works too: the load's what's carried).  Reckoned as the time's spent (a walk's as
  it goes, a wait or a sleep as it goes and as it ends, so "status" tells it true).  The null ruleset tires nothing; a
  denizen's load isn't counted (Characters' free pounds, below), so it never tires by one, and its own tiring is by its
  rounds fought, rested off by the clock whatever it's doing.  (sheet.c's o_spent, GA_STOOD; dq0w.c's rest_go.)
- **A sleeper lies down** *(the house's, 2026-11-03, for review; as the denizens' sleepers do)*: "sleep" or "rest", standing
  or crouched, leaves it prone (status: "Position: lying down."; the status line's PRONE), and it's up by the Rise as it goes
  on.  Sitting or kneeling it sleeps as it is.
- **Want's fatigue, told:** hunger's and thirst's levels aren't rested off, so a long sleep can end more tired than it
  began; the Condition line says how much of it is theirs ("tired (fatigue 3 (2 of it hunger and thirst, not rested off))").
- **Encumbrance** is the book's (the load over Endurance, rounded), told in words: none, a little burdened (1-2), heavily
  burdened (3-4), overloaded (5 or more), on what's carried past its free pounds by Strength (D13's: the character
  carries on, below).  (harn.c's harn_get_ep; skills.c's set_load; dq0harn.c's status.)

## Climbing and falling

- **A ledge is climbed a round (10 seconds) at a time,** a Climbing test each: a critical success 20 feet, a
  marginal success 10, a marginal failure a slip back of 5 feet (the climb goes on), a critical failure a fall from
  as high as it got (climbing down: to the foot).  Holds may catch the fall: then the climb goes on.  (dq0w.c's
  s_climb_on.)  Why: the book gives the test, not the rate; 10 feet a round is a careful climber's pace.  The round
  is the ruleset's (RS_HELLO's: HarnMaster's 10 seconds, the null ruleset's the same).
- **Jumping down a ledge** is a clear fall of its height.  (dq0w.c's start_ledge.)
- **A fall strikes where a middle strike would,** or low if it lands on its feet, with no armour taken off; the
  book's falling dice and grabbing a hold (Skills 8) otherwise.  (sheet.c's o_harm.)  Why: armour doesn't soften
  the ground.
- **Knocked senseless, the character comes round by the book's shock rolls,** a round each, and wakes anyway after
  an hour, the book's last shock roll then deciding whether it's in shock (2026-10-21: it woke coherent however badly
  hurt).  (sheet.c's o_harm, fight.c's senseless.)  Why: no game should stall on a run of failed rolls.  Struck at as
  it lies, it's finished (Fights: a helpless character); a creature struck senseless again as it lies would be out
  till the later of the two, its black-out told once (fight.c's senseless, AF_AGAIN).  A blow fumbled at one
  senseless or in shock is a miss, not the Ignore table's tactical advantage: it can't take one.  (fight.c's
  attack.)  Why: the advantage is an action, and it has none.
- **A window's sill is climbed through** with a Climbing test; a critical failure is a slip and a fall.  (wwin.c.)
- **A shaft is climbed as a ledge is** (What shows the sky, below, 2026-11-03), a thing dropped where one goes down falls down it.
- **A pit is climbed out of** as a ledge is, but a slip ends the climb (D13's: the character carries on, below).
  (wtrap.c, dq0w.c.)
- **Rope and grapples** (the user's A16, "Queue rope rules"; the book gives the help alone, Skills 8's "good climbing gear", 10 to 20 points of EML; all the rest *(new, for review)*; wrope.c, wrope2.c, wrope3.c, dq0w.c's part 97; those parts' overlays a file of their own, dq0w.rop, read as a climb, a dig or a rope first wants them, some 3M cycles once (a dig's own part too, 84, and a journey's, 55), none of the start's):
  - **A rope is 50 feet.**  It reaches what it reaches.  It's tied, thrown and got ready as a tool is, by the hands routine (its reach's steps, hands made free).
  - **Tied off and let down** ("tie the rope", "tie the rope to the ring", "let down the rope", "let the rope down"), half a minute's long task, with a drop to let it down: a shaft on the character's own square, or a ledge or a pit beside it (the way it faces first).  With no drop: "There's no drop here to let a rope down."; a rope tied to nothing's no use, and none's left so.
  - **What holds:** a thing on its square or beside it that's fixed where it stands (an iron ring, a stout post, a driven spike, an altar, a well: the maker's rings at a built stratum's shafts going down, its posts at a mine's) or that weighs 30 lb or more (a chest, a table); not a sconce.  Named, "That won't hold a rope." if it doesn't.  Else its grapple, set over the edge; else an iron spike driven in, with a hammer: a minute's long task, loud (7), and it stays driven for good.  Tied to a thing on another square, 5 feet of the rope are gone to reach the edge.
  - **Climbing aided:** on the rope's length a Climbing test is 3 steps the better (15 points, the book's middle), up or down, a shaft's, a ledge's, a pit's, by anyone's climb: the rest as climbing is (a slip, a fall, holds).  Past its end the climb goes on unaided.  From below it's taken only if its end hangs within 9 feet of the floor ("The rope's end hangs out of reach.").  A rope down a shaft or a ledge lets "d" and "u" go by it as they come; "climb the rope" at its top, with no way up there, goes down it.
  - **A grapple thrown up** ("throw the grapple up", "throw the rope up"; a grapple not tied on is tied on as it's thrown; "tie the grapple to the rope" beforehand a round's), a round's, at a shaft on its own square (one open to the sky: its mouth over the roof, higher than 9 feet), a ledge above beside it, a pit's edge from inside: the book's Throwing test at its missile bands, twice as hard straight up (a short band 5 feet, medium 10, long 20, extreme 40; 20 less a band; 40 feet at most).  Caught: told, with a clang.  Failed: "clatters back down".  A marginal success one time in four will give as a climber's weight comes on it, nobody falling: the lot at the climber's feet.  Each throw's noise is heard by the denizens as work's is (4 caught; 5 fallen back).
  - **Up a shaft to the sky by a rope** reaches its mouth, and out: left, or solved if it's solved (How a dungeon ends, below; the grating of A18 gone with Q1, 2026-11-04).
  - **Untied** ("untie the rope"), at its top, a round's: drawn up and coiled at the character's feet (what's tied to it with it: "take the rope"; what it was tied to as it is, a spike staying driven).  From below, a knot's out of reach; a grapple can be shaken loose, a Throwing test a round.  Carried, "untie the rope" takes its grapple off, at its feet.
  - **Left in place** it's two things lying there, "tied rope" at its top and "hanging rope" at its foot, seen and looked at (what it's tied to, how far it goes, its end), fixed where they stand so nothing takes them up, kept with the item store as anything lying there is, chunk by chunk and in the save.  Anyone's climb uses it.  **The minds climb nowhere else than with the character** (an ally coming along, up or down: as it comes along by anything): none crosses a roped ledge on its own, and none unties or cuts one.  *Not yet:* two ropes tied together; a rope cut; a heavy thing it's tied to carried off later leaving it as it was; a hold other than a grapple giving.  A climber's own gear: an outfit's Climbing skill's grapple comes with two spikes and a hammer (Outfits).

## Digging

The book has no digging rules.  A spell of work is ten minutes, with the tool to hand that suits the material best
(bare hands if none does better; of two as good, the one it wears less).  Earth, soft soil or hard-packed, is dug at
the tool's rate, no test: hard-packed earth half as far as soft soil, and never with bare hands ("You need better
tools: your hands make no mark on the hard-packed earth.").  Anything harder (a floor's flags, masonry, stone) only a pick bites into, or a chisel and a hammer, and each spell is a Mining
test: the rate at a marginal success, twice at a critical, half at a marginal failure, nothing at a critical
failure.  Feet a spell (kept in half-feet), and the wear a spell does the tool:

| Tool             | Soil | Hard-packed earth | Flags | Stone | Masonry | Wear (each)             |
|------------------|------|-------------------|-------|-------|---------|-------------------------|
| bare hands       | 1/2  | none              | none  | none  | none    | none                    |
| a blade, a knife | 1    | 1/2               | none  | none  | none    | soil, earth 40          |
| pick             | 2    | 1                 | 3     | 2     | 1       | flags, stone, masonry 1 |
| shovel           | 3    | 1 1/2             | none  | none  | none    | none                    |
| chisel, a hammer in the other hand | none | none | 1 | 1/2 | 1/2 | flags, stone, masonry 1 (the chisel) |

A spade and a mattock dig as a shovel does (their rows the shovel's, 2026-11-03, back as the kinds came), each a tool of two
hands, a denizen's as a shovel is (its agear's AG_SPADE).

- **The chisels and the new tools' prices** *(the house's, 2026-11-03, for review; the book has no price for them)*: a stone
  chisel is the chisel, as named (6d); a wood chisel (4d) is no use on stone, flags or masonry, not being in the diggers'
  table: "Your wood chisel doesn't seem to be working on the masonry." (the user, DECISIONS.md); with a stone chisel or a
  chisel to hand, that's the one taken.  A mattock 10d, 7 lb; a spade 7d, 4 lb.  None is in an outfit yet.

- **A chisel and a hammer** *(the house's, 2026-11-02, for review)*: a chisel works stone, masonry and flags slowly,
  a Mining test each spell as a pick's; **it takes the hammer in the other hand** (the user's,
  2026-11-03: "hammer and chisel in each hand"): no hammer carried, it's no tool ("You'd need a hammer to drive the chisel."); an arm
  useless, the same ("A chisel and a hammer take both hands."); carried, **both got out by a tool's steps**, whatever's in its hands put away first (Readying with busy hands, below: the user's, 2026-11-03, A25; a lit torch laid down lights the work as it lies).
  A pick to hand is the better tool and is taken.  A marginal failure with one makes nothing
  (half of its half a foot), so a poor miner's chisel soon comes to the question below.  The denizens' tools are their own (Tunnellers).
- **A tool named** *(the user's, 2026-11-03: "Suggest the better tool if one is available, but allow them to continue with
  the less effective tool.")*: "dig down with the knife" digs with the knife, a pick carried or not.  A better one to hand
  for what it's into is told as the dig's begun, once, not each spell ("Your pick would do this better, but you set to it
  with the knife as you said.").  *(The house's, for review:)* what's named no use on it, or no digger at all, no time
  spent, the better told of ("Your knife won't dig stone. Your pick would.").  None named, the best to hand, as ever.  (The
  referee's P_DIG, its text the name, both referees: rshouse.h's RS_HOUSE_DIG_TOOL; the world's part 84.)  **The kind**
  (the user's: any tool named where a better's carried): the others swept 2026-11-03: a dig is the one task the game
  chooses among tools by how well they do.  A blow strikes with what's named already (Fights).  Sharpening has the
  whetstone alone, a lock its key or its lockpicks: another thing named is refused, not passed over for them ("You have
  nothing to sharpen the knife with.", "Your knife won't do for a lock.").  A fire: a flame carried is still taken over a
  tinderbox named (*for review*: the flame's no worse, and quicker).  Nothing yet pries or cuts (a crowbar's only a club).
- **Told why** *(the house's, 2026-11-02, for review; the user's playtest, DECISIONS.md)*: a dig nothing it carries bites
  into is told at once, no spell spent, by what it can judge: its hands ("You need better tools: your hands make no
  mark on the stone."), a blade ("Your dagger only scratches at the masonry: this isn't getting anywhere."), a shovel the same,
  a chisel alone.  A try that's slow says so with its feet
  to go, each spell, as ever.  **Two spells running with nothing to show** (a poor digger's failures): asked, soft ("This
  isn't getting anywhere.  Keep at it?"), once a dig, as a long task's question; yes goes on.  (Each actor's rule stays the
  character's; only the character is told words: a denizen gives up as before.)
- **What isn't there to dig** *(the house's, 2026-11-02, for review)*: a door in sight ("That's a door: open it,
  or break it down."), a window or a slit ("That's a window: there's no digging it."), a portcullis, a gate, a grate ("Those are
  iron bars: they won't dig."), no time spent; open floor as before ("There's nothing there to dig ...").  A wall beside a door is a wall.
- **A hidden door** *(the house's, 2026-11-02, for review; the user's playtest)*: it's dug at as the wall it seems, by its
  wall's stuff and two feet told, with the same refusal if nothing bites: nothing about the dig shows it till the tool's
  at it.  As the tool comes through the facing (half a foot for a wooden door, a foot for an iron-bound one, two for
  iron or stone) it's found, a find as a search's (handled from then on, as a search's find is marked), the door still whole, its lock as it was ("Your pick strikes wood:
  a door, behind the facing."); the dig stops there.  The chunk's change record the door's own state.  One hidden
  by magic turns any tool: no mark, no door told, and the question after two spells (found only by magical means, as
  a search's).  "examine the wall" takes it for a wall too.  (There's no other hidden thing in the walls: a secret panel
  is a hidden door; portcullises and windows are never hidden.)

Each spell is strenuous work: two fatigue levels at HarnMaster's rate (Moving and tiring, above), so a long dig wants
rests between its spells.  A wall is two feet through, a square of the rock five (corner to corner seven), a heap of
rubble two, a shaft the drop to the stratum below.
A natural cave's earthen floors are soft soil; a built, mined or mixed stratum's are trodden hard (dq0m/fixtures.c).
A floor's flags are lifted, not cut (the user: a material of their own).  The user's, 2026-10-20: "a sword digs
earth too, poorly, and dulls"; bare hands "should only work in soft soil.  Hard-packed soil or harder should result
in a 'you make no progress' result"; "Hard-packed earth should be harder to dig for any tool (half the soft soil
rate)"; earth takes no Mining, "Anything harder uses mining skill, and rate depends on material (only a pick will
work for harder materials)"; the user's later words let a chisel and a hammer work too (2026-11-02, 2026-11-03: above).  (rshouse.h's digger table, the tool's choice and DIG_TESTED; dq0harn.c's RS_TEST
P_DIG.)  *(Every ruleset's.)*  A denizen's digging: Tunnellers, below.

### The rock's kinds and hardness *(slice 4.10, 2026-10-31: drafted for the user to review; every ruleset's)*

The book's Mining is geology, prospecting and mineworking (Skills 16), but it gives no rates; these are the house's.

- **Where each rock lies** (dq0/rock.h, from the game's seed alone, nothing kept): beds 25 feet thick down through
  every stratum (shale 4 in 10, sandstone 3, limestone 3; below 200 feet the shale slate and the limestone marble),
  dipping up to 16 feet a region (1,280 feet) each way and swelling up to 7 feet region by region, so the rock
  changes as a stratum's crossed and as a shaft goes down; a granite boss in one region in six (round, 15 to 78
  squares in radius); in seven regions in 16 a line straight across the region: a vein a square wide (quartz, or a
  seam of ironstone or copper ore) or a basalt dyke three squares wide.  A wall of masonry, a floor's flags or earth,
  aren't the rock; the rock is what's under and behind them, and a cave's or a passage's bare walls.
- **Hardness**: soft (shale), middling (sandstone, limestone, marble, the ores), hard (granite, basalt, slate), very
  hard (quartz).  A pick's two feet a spell in stone is three in soft rock, two in middling, one in hard, half a foot
  in very hard (the referee's rate in stone times 6, 4, 2 or 1 quarters, the world's: rock.h's RK_QUARTERS,
  dq0w.c's test_dig, so every ruleset's alike).  The Mining test is the same in each: the hardness is in the rate, not the odds.
- **Told as it's come to** (the UI's): the first spell of a dig in the rock, and a spell that comes into another rock
  (a shaft down through the beds), say how it looks ("You're into speckled grey rock."); a spell that went well (a
  Mining success) names it too ("You're into speckled grey rock: granite.").  Mining's eye is the test the spell
  makes already; a failure tells only the looks, and the first spell that goes well after it names the rock (the user's,
  2026-11-03: it used to be told only on a first spell, so a first failure left the rock unnamed for good).
- **Poor rock comes down** (shale: rotten, flaking): a spell of digging in it that fails critically brings part of the
  roof down into the hole, two feet of what's dug in the rock filled again ("The rotten rock splits, and part of the roof
  falls into the hole."; E_DIG's D_ROOF).  In a tunnel the roof falls on the digger (a shaft's sides fall into the
  hole below it, not on it): dodged (the referee's Dodge, as a trap's
  strike is), else a blunt strike at its head and shoulders, 1d6 impact.  The book has no falling-rock rule but its
  falling one (Skills 8: a fall's impact 1d6 for each ten feet); the house's 1d6 is stones off a roof a few feet up,
  as a fall of up to ten feet.  The dig
  goes on through the knock (the house's exception to a wound's hard interruption: the roof's part of the work going
  badly), unless it's knocked senseless.
- **A mixed stratum** (caves that sentient hands have been at): a third of its chunks left as they were found, grown
  as caves by the natural style's rules (soft soil floors, the rock's walls, no doors or windows), the rest hewn by
  the mixed style's; but a chunk with a way between strata in it is always hewn, where the builders' work began
  (dq0m/make.c's cstyle; dq0 -M, the tests', makes every built stratum mixed so).
- **The rock looked at** ("examine the wall", "look at the wall": the wall ahead, within six squares): masonry ("The wall's masonry:
  dressed stone, set in mortar."), or the bare rock, how it looks ("The wall's the bare rock, buff."), and named after a
  Mining test (the appraisal's AP_ROCK: "...: sandstone."; the first answer kept for that wall, as any appraisal's),
  three seconds.  Too dark, nothing's made out.  A shut door before any wall: "That's a door: shut, and no wall.", nothing told through it.
- **Age's collapse and rubble**: an old stratum's passages partly fallen in, heaps of rubble a square and some beside
  it.  Six tries a chunk (the maker's own dice), each in a stratum two centuries old or more, a chance in 20 of its age
  in centuries (ten at most), twice that where the rock about is poor (shale at the chunk's middle); never by a room or
  a doorway, so no way is shut.  Rubble is climbed over at twice a step's time, cleared by digging (two feet of it,
  shifted as loose soil is: at a tool's soil rate, no test, bare hands too), told as it's seen along a passage ("It goes
  ahead, over rubble."), drawn on the map as %.  The denizens walk over it at their own pace as yet.  (dq0 -U, the tests': none made, the
  passages as they were before it.)

## Portals

- **Picking a lock:** Lockcraft, at the lock's difficulty, a minute a try: 2 to 10 steps worse, two a grade of its
  make (the maker's dice: a simple lock 2 or 4, a sound one 6 to 10; the closer look tells which).  A timed work (the
  long tasks', below): begun with the picks to hand, the test made at the minute's end.  (pass.c; wdoor.c.)
- **Forcing a door:** Strength, by its material: wood as it is, iron-bound two steps worse, iron four; one worse
  locked or barred; a rotten door (quality 0-1) two better, a worn one (2-3) one better, a new one (8-9) one worse.
  Its lock or bar bursts; it's loud (heard as it's begun), and takes 10 seconds, a timed work, its test at their end.
  (wdoor.c.)  Why: the book's Strength test, scaled by what the door is made of and how it's kept.
- **Bending a window's bars:** Strength, six steps worse; loud, 10 seconds.  **A portcullis's bars,** down: six
  worse and one more for each inch of their thickness (a portcullis's two), and it's a way through for good,
  jammed.  **Heaving a
  portcullis up** from the far side: six worse, a round.  A portcullis is never opened: raised by its winch on its
  own side (30 seconds' cranking, loud), lowered by it only (2 seconds, louder).  Bending, heaving and winding each a
  timed work, the test made (or the portcullis up) at its end; wound part way and stopped, it stays down.
  (wwin.c.)
- **Battering a thing** (the user, 2026-10-16: "model it after unarmed combat"): a blow the thing ignores (the
  book's Ignore defence), struck with what strikes (Fights' what strikes: the thing named, else what's in its hands,
  else a fist, a foot, the head; its hands empty and a tool carried, asked first; a soft thing one attempt); its
  impact for the aspect that does it most harm, less the material's resistance, worn off the portal's body (its
  material, size, thickness and state), the harm kept.  A door broken down stays open for good; a portcullis
  broken through is a way through for good, jammed.  A blow every 3 seconds as the clock goes, a dozen at most (four
  that make no mark at all: given up), a long task (below), the harm kept as each lands; each blow heard (the user's,
  2026-10-20: a noise of 8 for the denizens, a window's shutters or bars 7, and a blow's sound), and strenuous work
  (Moving and tiring).  Told as it goes (the user's playtest, 2026-10-22: "nothing is written to the output until
  after it is resolved instead of something like "You start beating on the door..." followed by progress"): its
  first words at once, by what strikes ("You set to battering the door with the bar.", "You set to battering the
  door." bare-handed, "You set to kicking the door.", punching, butting), then a line as the first blow marks it
  ("Your blows begin to tell on it."), as one leaves it giving ("It's starting to give."), and every fourth blow else
  once it's marked ("You keep at it."); how it stands as it ends ("It
  gives way, and crashes open.", "It's giving, but it holds yet.", "It's marked, but it holds.", "Your blows make no
  mark on it.").  (wblow.c; fight.c's thing.)  Why nothing was told before: a battering's blows were only noises
  (the denizens', the sound task's) till its end's words.
- **A part of the body that strikes a thing** (a fist, a foot, the head: Unarmed's) is the house's Unarmed test (the
  user's, 2026-10-22: "you should be able to avoid injury with a critical success or marginal success (or possibly
  even a marginal failure) test against unarmed skill.  MF only results in no damage, and CF results in damage to
  self?": the lead's reading, the user's as it is, 2026-10-22): a critical or a marginal success a blow that tells on the
  thing (the Ignore table's strike, its impact) and no hurt; a marginal failure no harm to either; a critical failure
  no harm to the thing, but the part's: 2d6 and half the thing's blunt resistance, less the part's armour (gauntlets,
  boots' soles, a helm), a blunt hit on that part.  The character stops when it's hurt enough: a serious injury,
  three levels or more to that part, shock or senselessness.  A tool's blow (a pick's) as the book's, never the
  striker hurt.  (fight.c's thing; sheet.c's o_harm, HC_STRUCK.)  Why: the book's H2 for a bare part meeting
  something hard, with the game's numbers; the marginal failure's d6 gone (the user's).
- **Bare, it stops short once a door** (the user's, 2026-10-22: "it stopping short of injury the first time you would
  injure yourself ... After being warned of injury, continuing should only apply for that door, being reset on the
  next door you try to batter down with bare hands/feet"): a bare part (no armour on it: no gloves or gauntlets, no
  boots, no helm) whose blow would hurt it (that critical failure) stops short of it the first time at a door,
  unhurt, the blow struck for nothing, and asks, soft: "You punch the door with your bare hands, but stop short of
  hurting yourself.  Keep at it?" (kicking, "with your bare feet"; a head butt, "your bare head").  Kept at, it isn't
  asked again at that door (its square the world's to keep), and the next such blow hurts; at the next door battered
  it's asked again.  (fight.c's thing, rs.h's RS_SHORT; wblow.c's batter.)
- **Filed elsewhere:** a chest's lock: Light and traces' locks, below; a chest's or a box's key: Keys that fit (Outfits);
  a door's keys, and a denizen at a door or a portcullis: Denizens and doors, below.

## Traps, searching and hiding

- **A trap stepped on, if noticed** (see Noticing), stops the character short.  (wtrap.c.)
- **A search** looks over every square within 3 that the character can see: a hidden trap or thing is found by
  Awareness at its hiding's steps worse, a hidden door at no steps.  (wtrap.c; things.c.)  In the dark, by touch,
  nearby; and its forms, nearby, a wall, the floor, for a thing, on a thing: Walking and searching in the dark, below.
- **A search cut short covers the part done** (the user's, 2026-10-20): the squares nearest first, as far as its time
  went, each at the full chance.  Its reach grows by one each 3.2 seconds of its minute (a square's searched if its
  distance off, squared, is less: about as many squares as that part of the 49), all of them once the minute's done;
  the world's search of the walls and floor, and the things' after it, the same (E_PORTAL's reach, IO_SEARCH's).
  (wtrap.c's search; things.c's s_search.)  Why: the squares as near searched together, the nearest first; a step of
  the reach each 32 tenths puts the whole minute's at 19, every square.
- **What the dungeon hides:** a trap hidden 2 to 8 steps, a covered pit 1 to 5 if not hidden so (fixtures.c); a
  thing lying concealed, by the stratum's style, from 2 steps in twos to 12 in built and mixed strata, from 0 to 6
  in caves, from 2 to 8 in mines (uses.c's styles, stock.c); a theme's piece as its file says (the themes': 2 to 6).
- **Hiding a thing** where the character stands (the user: hiding is a skill, Concealment; the game's Stealth): its
  hiding, in steps, by the test: a critical success 12, a marginal success 6, a marginal failure 2, a critical
  failure 0 (a floor's dust and corners only).  Told: 10 or more cunningly, 4 or more well, less after a fashion.  The
  character knows where it hid it.  A minute's timed work (the long tasks'), the thing put down and the test made at
  its end.  (things.c's s_hide; main.c's hide.)
- **Hiding oneself** *(the user's, A12, "queue it next"; the book's Stealth, Skills 9, where it says; the rest new, for review)*: every
  actor's by the one code (dq0w.c's part 94, dq0d's part 20, dq0p's sense.c and dens.c).
  - **The command**: "hide" with no thing said; "hide behind the table", "hide in the straw": one within reach, walked over to first; "hide in
    the shadows", "in the doorway": the same as "hide", the world finding what's there.  "hide the dagger" still hides a thing.
  - **Where** *(the book: it presumes reasonable cover or poor light)*: a thing big enough (40 pints or more: a cask, a table, a chest, a
    body, a well) on its square or beside it, 2 steps; a heap of 8 pints or more on its square (straw, furs), 2; a doorway it stands in, or
    rubble beside it, 1; its square unlit, 2 more.  Neither cover nor the dark: refused, no time ("There's nothing here to hide behind, and the
    light's too good.").  *The house's: the figures.*
  - **Refused** *(the book: only where an observer wouldn't see it anyway)*: with a light on it ("Not with a light on you: it gives you away.");
    with a denizen not an ally that's noticed it, on a square it sees ("The bandit is watching you: you can't hide from it now."); no time.
  - **Its time** *(the house's)*: ten seconds, a timed work, its words at once ("You set to hiding."), stopped as the others are, not
    hidden then.  At its end it's hidden, told by what ("You crouch behind the cask, out of sight."), crouched if it was standing;
    "status" says so.  Kept with the game (world.dat).
  - **A look at it** *(the book's test; how often the house's)*: a denizen that would see it but for that (within its sight, a line clear: the
    senses' SN_LOOK) tests the hider's Stealth, once for each of how near it's come (over seven squares, 2 steps easier; four to seven, none;
    two or three, 2 harder; beside it, 4 harder), and again as it comes back into sight.  Easier by its cover's steps; 2 easier if the looker is
    about its own business, unaware.  A success, not seen.  A critical failure, found.  A marginal failure, the looker's Awareness tested
    (the book's; a creature's its Eyesight times 5, as its Stealth is its Agility: *the house's*): a success, found.  "The bandit has seen
    you!": its long task stopped, hard.  The tests on the denizens' own stream of dice (the character's and the world's rolls as they were).
  - **What it can't hide from** *(the house's)*: a nose: smelt, as anyone is (Smelt on the air), it's known, and a hostile one comes and strikes.
    An ally always knows where it is.  Heard (creeping, by its grade; still, not at all: its noise 0; asleep, as loud as before), a hostile one
    is drawn to the place (A noise drawing them), looks as it comes, but doesn't know it's there till it sees it.  No hostile engages it (their
    zones aren't for one they haven't found).
  - **What ends it** *(the house's)*: moving at any pace but a creep (a creep keeps it, by the moving quietly test: a stop in the open, lit
    with no cover, ends it, "There's no cover here: you're hiding no longer."); standing up; "stop hiding"; doing a thing (a thing taken, put,
    held, worn, lit; a door worked; a dig, a climb; first aid and the other timed works).  "You come out of hiding."  Words to a denizen tell it
    where it is ("The bandit has seen you!").  Looks (a look in the dark, a feel about it), examining, a search about it, listening,
    sniffing, reading the floor, waiting, sleeping, turning, a lower position, the inventory and the rest of the free commands don't end it.
  - **A blow from hiding** *(the book's Surprise and Ignore, Combat)*: at one that hasn't noticed it, no defence, as a sleeper's; "You strike from
    hiding."  Out of hiding as it's struck, either way; a blow at it, "The bandit has seen you!".
  - **The denizens hiding** *(for review)*: a hostile robber (kinds.txt's ways r) that's just sensed the character, not seen by it, three squares
    off or more, no launcher of its own, lies in wait, by the same cover (its square dark, as its kind with no darkvision feels its way); no cover,
    it comes on as it would.  Hidden, it holds still, left out of what the character sees and engaging no one, two minutes, then comes out; strikes at
    one beside it that it senses, the same no defence ("The bandit leaps out of hiding!").  The character's look at it the same test as anyone's, at
    its Stealth by its sheet, as the character sees its square ("You spot a bandit, hiding."); and a search finds it within sight (the
    referee's P_SEARCH at its cover's steps).  Not yet: other reasons to lurk, a denizen creeping on hidden, denizens looking for each
    other hidden (their factions' fights know where they are).
- **Disarming:** first the mechanism is made out (Lockcraft, half its power in steps worse: never deadly, however
  it goes), then disarmed (Lockcraft, half its power worse, two better if it's a keen one known well); magic only
  by magical means; a pit not at all.  Each a minute's timed work (the long tasks'), its test at its end; one set by
  magic, with no magic to undo it, told at once.  (wtrap.c.)
- **A trap's strike, a burn,** have the armour where they strike taken off them.  (sheet.c's o_harm.)

## Who lives where

A theme's inhabitants (its theme file's faction lines: games/dq0d/pop.h) are placed by popgen (bank/popgen), the
denizens' task's population generator; with none, the stand-in's (kinds.txt and factions.txt) are.  The book is
silent on all of it.

- **A faction's number is spread over its strata,** each weighed by how near its lair's stratum it is: the lair's
  its strata's count and one, each stratum further a step less; each share rounded down, what's left over the lair's.
  (popgen.c's share.)  Why: a people lives most where it makes its home, and fewer the further they go from it.
- **On a stratum, its first kind has twice each other kind's share** (what's left over its too).  (popgen.c's
  pg_stratum.)  Why: a faction's first kind is what it is; the rest go with it (the kobolds' rats).
- **A kind's share is in lairs of its kind's group's size** (kinds.txt's group), drawn by the stratum's seed; eight
  lairs of a kind at most on a stratum, the groups the bigger if there are more.  (popgen.c's groups.)
- **A faction's leader is one, in its home:** the first lair of its first kind on its lair's stratum, the leader
  with them.  (popgen.c; dq0d.c's lair_add.)
- **Where a lair lies is its kind's way,** as the stand-in's: by the stratum's ways in and out if its kind lairs
  there (a theme's lairs in the halls when the ways are all taken), else in a region of halls.  (dq0d.c's lair_add.)
- **A theme's faction speaks its first kind's tongue** (the stand-in faction's of that kind: kobolds kobold, bandits
  and cultists common).  (dq0d.c's start.)
- **How factions stand with one another** is the worse of the two ways it's said; unsaid, indifferent.
  (popgen.c's faction, as make.js does factions.txt.)

## Fights

Filed elsewhere: engagement, the engage initiative test and a grope while engaged (Where you are, going places, and long
tasks); hands, limbs and getting up (Hands and limbs); a weapon's version and Strength (The character carries on); a
denizen's gear, its blow and its shield (Outfits' the denizens' gear); its tiring (The denizens' bodies); a wound's
course, first aid and healing (Wounds over time); courage in a fight (Morale).

- **The creatures' sheets come from the referee's bestiary** (beasts.c's, by the kinds' keys): the HarnWorld Bestiary's
  averages for the like creature (the page each came from beside it), and for folk the
  book's for men (attributes about 10 to 12, skills by their trade), the peoples' by the book's own (The peoples, below); each attribute varied as the book suggests: less
  7, and 2d6, at least 1 and at most twice its own; by a die of its own, the same creature each time.  Why: the game's
  own world (G3), its creatures modelled on the book's (the user: "for denizens, read the harnmaster bestiary").
  **The first six** (the user's A9, 2026-11-03: "the book's figures now"; built 2026-11-04), a beast's Dexterity its Agility where the
  book gives none (as the bat's and the adder's), its natural weapons in the book's order, the first the one it uses most, each one's pace
  (kinds.txt's step) by the book's Move: the ghoul the book's male grave-wight (p48: its grave rot's C4 the book's too), the cave bear
  its black bear (p13: its bite first, its claws the better).  *Where the book has no such creature, its nearest kin's figures (for review)*: the giant rat
  the book's rat, whole (p83: it has no giant one; its weight, 12 lb, the house's, the book giving none); the cave spider the adder's
  (p88: it has no spiders; a small venomous biter, and the adder's venom with it: Wounds over time); the skeleton the gulmorvrus's, as the wight's
  sheet is (p78; it has no skeletons; its bones' 40 lb the house's, and a man's sword at that mastery: its old hide against edges and points gone with its stand-in, no rule of the book's saying
  bones turn a blade); the kobold the small orc's (Gargun 4; it has no kobolds; its spear at the small orc's mastery, folk as before, so no hide but what it wears).
- **What strikes** *(every ruleset's: which thing; the ruleset's: what it does)* (the user's playtest, 2026-10-22: "I
  cannot `hit the door with bar` if I have a `bar` in hand.  It still says I am battering it with my bare hands";
  "A bar should just be treated as a club or stick, no?"; "I think it should try and use whatever you request, but if
  it is something soft (a piece of clothing, a noodle, string, rope, etc.) it should reply with something like `You
  attempt to batter the door with the noodle, but it seems ineffectual`"; "I don't think it should auto-fallback to
  something better unless you have nothing at hand, and then it should ask for confirmation"; drafted for the user to
  review).  At a denizen and at a thing (a door, a window, a portcullis) alike:
  - *A thing named* ("hit the door with the bar", "attack the rat with the dagger", "strike the rat with my torch")
    strikes, and no other, whatever else it carries: taken in hand first (its time; refused, the words why, if it
    isn't to hand or no hand's free for it).  A weapon or a tool of the ruleset's strikes as itself (HarnMaster's: its
    row of the weapons, by its kind's key: the book's for the weapons, the game's for the tools); anything else as the
    book treats a thing swung that isn't a melee weapon (Combat 8: a bow used in melee is a stick, a staff or a club),
    the nearest of the book's own by the thing's data, the house's: *light*, under 2 1/2 pounds, the book's stick
    (Club, attack 5, defence 5, blunt 2); *hefty*, 2 1/2 pounds and more, its club (Club, 15, 5, blunt 4), held in both
    hands if it's big (20 pints or more: the things' rule).  The book has no penalty for such a thing used so, and none
    is taken.  Its words are its name ("You strike at the giant rat with your bar", "You set to battering the door
    with the bar.").
  - *A soft thing*: made of something that gives (cloth, leather, wax, tallow, food and the like) or only breaks
    (glass, pottery); a heap or a lot (sand, pennies, arrows, rations); anything worn (armour, clothes: not made to
    swing); under half a pound; or lighter than its bulk, a quarter pound a pint (hollow or loose: a basket, a pack,
    straw).  It's tried, once: a blow's time (a round at a denizen, 3 seconds at a door), no harm, told ("You attempt
    to batter the door with the rope, but it seems ineffectual.", "You swat at the giant rat with the cloak, but it
    seems ineffectual."); no battering goes on of it, no fight begins of it (a fight it was in ends with it).
  - *Nothing named*: what's in its hands strikes (of two, its main hand's, but a soft thing: the other's then: Hands and
    limbs'), never a better thing it carries, unasked: a torch in hand, the torch.  Its hands empty and something better carried (by the ruleset's
    reckoning: HarnMaster's, at a denizen the best weapon by its attack mastery, but a shield or a torch, carried for its light; at a thing the tool that
    hits it hardest, not a weapon a thing would spoil), it's asked first ("You can try battering it with your hands, but would you rather use your
    pick?", "You can fight it with your fists, but would you rather draw your sword?"): yes takes it in hand and
    strikes with it, as if it were named; no, or any other answer, bare-handed; a command typed instead, the blow let
    go.  Its hands empty and nothing better carried, bare-handed, unasked.  A kick, a punch or a head butt said
    outright is that blow, as it was.  Fighting back unbidden (a hostile engaging it), what's in its hands the same
    way, but never a soft thing (its fists then); fighting on keeps what its attack struck with.
  - *The catalogue's kinds, as they come out* (HarnMaster's): as themselves, a dagger, a sword, a broken sword (a
    sword), a mace, a spear, a round shield (a shield's bash), a torch (the book's stick, its row); the game's tools, a
    pick, a shovel, a chisel; the club, a door's bar, a chair, a table, a chest, an iron pot, a window's iron bars; the
    stick, a lantern, a skull, bones; a theme's, a stone tablet and a totem the club, a chalice, a seal, a brass key
    the stick; soft, a rope, a sack, a cloak, a candle and candle stubs, a backpack, a belt, a pouch, a waterskin, a
    quiver, a basket, rations, pennies, arrows, a tinderbox, lockpicks, an iron key, potsherds, a pane of glass and its
    shards, a flask of oil, a pot of pitch, a heap of sand or of straw, a body, every piece of armour and clothing; the rest by their data (a crowbar the club, a hammer the stick).
    The book's staff (Spear, 20, 15, blunt 4) is for a theme's pole or staff (its key "staff": its data can't tell a
    pole from a club, it has no length).
  - Why: the user's; a ruleset's own weapons as they are, and a thing that isn't one as the book treats one (its
    bows), the nearest of its own; the rest the house's, from the data every thing has (its material, weight, bulk,
    form, whether it's worn).  Its own quality in a blow: not yet.  **A worn edge cuts less** *(the user's, 2026-11-03, A10: "dull edges cut less"; how much the house's, new, for review)*:
    a blade dulled (wear 32 up, "its edge dulled") a point less of its edge's impact, dull (128 up, "its edge dull") two, never under nothing; every actor's, its blows and thrusts as they were.
    **A weapon may break** (the book's Weapon Damage, Combat 12, every actor's): on a Block, weapon against weapon or shield (a bare part struck instead, H2, as ever), the lower Weapon Quality tested first (the attacker's at a tie), the other only if
    that holds: three dice over it and it's destroyed; on a blow at something hard (iron, stone: what its edge can't bite, the door's wood not; the character's, a creature's own is never hurt by it) the same, the weapon alone.  Quality: the book's table (a sword's 12, a dagger's 11, a club's 9, a kite shield's 14)
    *(new, for review: and its make, its quality 0-9 read as the book's table's �4 about 5; a tool, none: no test)*; by their own die, the game's others roll as they did; none on the book's optional partial damage.  **Broken**: "Your sword breaks!", "The bandit's sword breaks!"; it falls as a "broken weapon" (useless: soft to swing, a farthing's scrap), so no blow's made with it; a creature to its fists.  (gear.c's row_of, o_pc_side, o_pc_wield;
    rshouse.h's RS_HOUSE_SWING; choose.c's wield; resolve.c's tool_ok; dq0w.c's door_work, swatted, in_hand.)
- **A creature defends** by dodging, or by counterstriking if it's better at that, as the book advises for animals;
  one holding a shield blocks with it if that's the best of the three, once a round (2026-10-29: the denizens' gear's,
  below; fight.c's defence).
- **The character's defences are chosen for it, a round's at a time** (the user's, 2026-10-20: the game is real
  time): against each blow the better of a block and a dodge it has left that round; each thing it holds that
  blocks blocks one foe a round (a sword one, a shield another), a bare arm if it holds none, and the dodge one
  foe; a foe met again in the round meets the same defence; none left, none.  A bare arm's block against a weapon
  (struck, H2) is the last chosen.  A counterstrike only on command (defend counter), what it strikes with then
  spent for the round as a block would be; defend block or defend dodge puts that first, defend none drops them
  all.  The round runs ten seconds from the first blow at it after the last one ended.  Why: a player can't choose
  a defence each blow in real time, and the book's defender meets each attack with something it has free.
  (gear.c's pc_defence.)
- **Fighting on, as built** (the user's, 2026-10-20: "once engaged in combat, you should continue to attack your
  target as you can, without actively having to say "attack" each time"): a long task of its own (LT_FIGHT: no bar,
  its words on the status line, "Fighting the giant rat"), begun as the character attacks (or a close-in comes beside
  its target) and as a hostile enemy engages it: one that strikes at it, or whose zone stops its walk, on its feet,
  sensing it, not fleeing (not one that isn't hostile, a wary rat keeping its distance; not while the character's
  senseless, but as it comes round, if one engages it still).  A blow each round as it's free, with what it holds (an
  attack's own weapon and aim kept; the middle, else), while its target's within reach and able.  Its target beaten
  (dead, senseless, out of reach), it turns to the next hostile engaging it, the one that struck at it last, else the
  first ("You turn to the giant rat."); none, it stops ("No foe is left beside you.", but after a blow alone, whose
  words say it).  A senseless one's left lying; "attack the rat" finishes it.  Come round in shock, it's in no state
  to fight, and stops ("You're in no state to fight.").  A command that takes time takes over: the round out ("You
  stop fighting."), then the command, and fought on after it if a hostile engages it still ("You turn to the ghoul.");
  commands typed ahead take it over each in turn, a round fought between them.  "stop" (or "no" to its question) ends
  it, those engaging it then left be till they've left it or it attacks.  The free commands don't stop it (a look
  about, what it carries, the sheet, the map, the help, its state, a stance: "defend dodge"); an attack as it fights
  changes its target, struck at the next round.  Its soft question: badly hurt as it fights, then critically ("You're
  badly hurt.  Keep fighting?"), the blows going on as it asks at the real paces, the clock still at the turns pace
  till it's answered; the other soft questions (something seen or heard) don't ask it, nor do a blow, a wound or a
  hostile near stop it.  It tires as strenuous work does (the user's, 2026-10-21: the book's second variant of combat
  fatigue, a level each five minutes of active combat): each round it fights charged so, whatever it does in it (a
  blow, a struggle up from the ground), and a pass while it's engaged too (2026-10-31, for the user's review: the
  book's variant counts every round of active combat, and a pass defends as it's attacked; a disengage's square is
  its walk's, at that pace); a level more asked as a hurt is ("You're tiring.  Keep fighting?"), at every pace (2026-11-04: the turns pace's was turned away, so a fight there asked nothing, however long).  A missile weapon's the same
  task (Missiles, below).  To come, the same task: a setting widening where melee's begins ("autoengage with sword at close range"),
  an ally's.  (dq0w.c's part 24.)
- **Disengaging, the book's** (the user's, 2026-10-21: "try to have it follow the book"; Combat 7): an engaged
  character moves a square, and then, if it's no longer engaged, a half-move more (its walk's squares a round),
  straight on, stopped by a wall, a ledge, a denizen, or another enemy's zone (still engaged after the square, it
  stops there: "You're engaged with the ghoul: you stop walking."), with no blow that round, and the fight's over
  (fought on again if one engages it again).  "disengage", "withdraw", "retreat", "back away", "back off",
  "break off", a way or none: none, away from the foe engaging it (straight away, then half left or right, then
  aside, the first open; a passage's four ways alone).  "flee" ("escape", "run away") the same, facing the way
  it goes, then, free of them, a run on that way (none if the way's shut: the disengage enough); not engaged, a run
  from the nearest foe it sees within a dozen squares.  Neither engaged: told ("You aren't engaged.", "You see nothing to
  flee from.": the same words and no run whether a foe it can't see is near or none is, nothing given away; the words the
  house's, the rule the user's, 2026-11-03).  "pass" a round given up, defending (engaged, a round of the fight's tiring: fighting on's).  A walk as it's engaged goes as a disengage does: its first
  square stopping it if it's still in the zone, then on.  As it fights, either takes the fight over, its round out
  first.  Prone, it gets up first, the move a round the later (the book's: Hands and limbs, below).  No foe's free blow (the book has none).  (dq0w.c's part
  26.)
- **The fatigue rates** (the user's, 2026-10-21: running more than strenuous work, sprinting more again; the book's,
  Combat 6 and Skills 6): Moving and tiring, above.
- **Aiming high or low** is two steps worse (10); **a prone defender** four steps easier to strike (20);
  **outnumbered,** two steps worse for each enemy more.  (fight.c.)  No command says an aim (the user's, 2026-11-03: "Leave unbuilt for now"): a blow or a shot goes at the middle.
- **Armour's quality:** a poor piece a point less protection, a fine one a point more.  (fight.c.)
- **Bones that move** (mindless) feel no shock, bleed nothing, fear nothing, and fall apart when their injuries
  reach their Endurance; so too the death god's mindless dead, the wights (the book's: Bestiary p78).  (fight.c.)
- **A creature's courage** is its Initiative test, its injuries its penalty: a marginal failure holds it back a
  round; a critical failure breaks it, a d100: berserk (1-25), desperate (26-50), broken (51-75), cautious (76-100).
  A wounded creature still standing is tested; a shy one that holds back flees.  Its situation's steps are its heart
  since 2026-10-30 (the field about it and its bonds: morale, below).  (fight.c's courage, left.)
- **Bleeding** loses a point of blood a minute for each bleeding wound; past the Endurance, death.  (heal.c's bleed,
  every actor's: wounds over time, below.)
- **Wounds, as built** (the user's adder playtest, 2026-10-21: "the wounds just kept getting worse and worse", and
  its bites were worded stabs; drafted for the user to review).  Each wound is worded with its own level, the
  book's (M1, S2, S3, G4, G5).  A creature's own part words its wounds by what it is: a bite a bite (minor, serious;
  grievous a deep bite), claws and talons a scratch, a rake, a deep rake, a sting a sting, horns a gore; a weapon's,
  the character's own blows (its fists and feet too), a fall's and a trap's by the aspect as before (bruise,
  fracture, crush; cut, gash; stab; burn).  The character's injuries keep what dealt them, for the sheet and its
  state.  (sheet.c's wound, wound_words; fight.c's dealt.)
- **The record's room:** the sheet keeps twelve injuries (the library's, HARN_INJMAX); past them its two least (by
  severity, then levels) are folded into one, its lesser hurts, and the least of the rest into that as more come:
  every level still counts in the universal penalty and the shock rolls, but the lesser hurts are never named as an
  injury, nor as the worst ("in shock: 11 injuries and lesser hurts, the worst a serious bite to your right elbow
  (S3)"; the sheet: "lesser hurts (7 levels)"); a folded wound's bleeding goes on in them.  (sheet.c's room.)  Why:
  the book keeps every wound on a line of its own; the game's sheet has twelve, and past them the library added each
  wound into the last, its words the running total ("a serious stab (S37)").
- **A shock roll with every injury, in shock too** (the book's, Combat 14; the library rolled only for one
  conscious, so one in shock was struck on and never fell): over its Endurance, it's senseless.  (sheet.c's wound.)
- **In shock, the character can only rest, or walk or crawl away at half its Move** (Skills 22; the book's Combat 14
  asks an Initiative test for the half-move: the game takes Skills 22's, no test): its walk's steps twice as long, no
  faster pace (a run in shock is that walk, told so: "You walk south into the dark.", the status command's "Pace: a run, held to a walk in shock"); it ignores any blow (no defence) and is in no state to fight; nothing
  else but rest, a look, a word called out and the little more wounds over time's in shock lists, below (slice 4.3's).
  (dq0harn.c's status.)
- **A helpless character and its foes, by their nature** (the user's choice, 2026-10-21; helpless: senseless, or in
  shock).  Each kind's way is kinds.txt's (its ways k and r).  A killer (k: the ghoul, the skeleton, the cave bear,
  the basilisk, the lindworm, the wight, the shade, the cultists and their high priest, the small, black and red orcs) strikes on, and one senseless
  it finishes where it lies, as the character finishes a creature (the book's dispatch: a mortal wound to its neck,
  to its skull by a blunt weapon; "The ghoul bites at you: a mortal deep bite to your neck. You are dead.": the game's
  end).  A robber (r: the kobolds and their lord, the bandits and their lord, the brown orcs, the orc king) comes up to it and takes what it holds
  (but a light lit: it leaves it its light) and every coin it has (pennies, farthings, crowns: 2026-11-03), a round's work (and a round a step of its reach:
  Reach's a robber's, below), off to its lair (into its faction's
  hoard, or at its home, hidden if its kind hides its things: Outfits' the robbers' takings, below; where it stands, if that can't be had), and goes ("The bandit goes through
  your things as you lie helpless, and takes what it finds."); it doesn't strike at it.  One that only defended itself
  (neither: the giant rat, the cave spider, the bat, the cave adder, the white orcs, the dwarves, the elves, the soldiers and their captain) stops once the threat is down and goes, fleeing
  a while (24 of its turns) and unfriendly after: it keeps a few squares off, and strikes only if the character,
  itself again, comes beside it.  Nobody engages a helpless character (no engage initiative test): what's done to it
  is each one's mind's; a blow at it senseless from any but a killer is held (one posted as it fell: the referee's,
  RS_KILLER).  The referee is told what's to hand and the load left at once, as it's robbed (the user's, 2026-10-22:
  "Weight should be recounted immediately after a robbery": its encumbrance the less as the robber goes).  (dq0d.c's
  decide, K_HELP; dq0w's wfight.c, s_den_rob; fight.c's attack.)  Not yet: being led away (by an ally, or a captor),
  a captive's fate, a shy creature cornered fighting back (the user's, 2026-10-22: "Rats
  should attack if cornered": defence only for now: it darts past the character; one fleeing, cornered beside a character itself again, does strike).
- **Health in words,** for every ruleset: fine with no injuries; hurt while the injury points are under a third of
  the Endurance; badly hurt under two thirds; critical past that; down when senseless or in shock.  (dq0harn.c's
  health_of.)

## Throwing

- **Anything thrown at a thing or a place goes by Throwing** (P_THROW), whatever it is, at the book's missile ranges (Combat 15): a
  short band, then medium, long and extreme, each twice as far as the last, at 0, 20, 40 and 80 less (0, 4, 8 and 16
  steps).  (dq0harn.c's purpose table; dq0w/things.c's s_throw_at.)
- **A kind's reach is its short band, in squares** (5 feet each): 2 for a thing a hand throws well (a dagger, a
  torch, rations, a key, a skull, bones, pennies, a candle), 1 for a heavy or an awkward one (a pick, a shovel, a
  sword, a mace, a shield, a helm, boots, a basket, a rope, a pane), 4 for a spear (a javelin's) or a taburi, 3 for a shorkana, 0 for what isn't
  thrown (a chest, a table, a chair, a cloak, body armour, a backpack, a belt, a body, a well); a theme's own kinds
  none yet.  (dq0w/kinds.c's throws: the plan's affordances in the kinds' data.)  Why: the book gives thrown
  weapons' ranges; other things are judged by their weight and their shape.
- **Thrown hard: twice as far, and 10 less** (2 steps), at a thing or at a denizen, every round it's fought on.  ("throw the
  dagger hard at the rat": struct cmd's manner; I_ATTACK's AIM_HARD, MS_HARD in the world's request.)
- **Beyond the extreme band it falls short**, at the band's end on the line to the mark; **a miss lands beside the
  mark** (on its square, as yet).  The thing lands out of the thrower's hands, where it falls.  A thing thrown at a
  denizen is a missile's attack (below, slice 4.7's): it strikes, or it misses, by the book (beyond the extreme band it isn't thrown: "It's out of range.").

## Missiles *(slice 4.7, 2026-10-29: drafted for the user to review)*

The book's missile combat (Combat 15-17) for every actor, the character's and a denizen's, by the same code (the world's
dq0w/wshoot.c, the referee's dq0harn/missile.c; the null referee's simple way).  Where the book is explicit it's the
book's; the house's below, each marked.

- **What shoots**: a sling, a shortbow, a longbow, a hartbow, a crossbow, in hand (both hands; a sling the right).
  "shoot the bandit" takes up the launcher it carries if it isn't in hand (slung, on the belt), its round's (the
  book's grope: an unprepared missile weapon prepared, used the next turn), its hands free first ("Your hands are
  full: put something down first.").  A thing thrown at a denizen ("throw the dagger at the rat") goes by Throwing (a
  taburi by Dagger, a shorkana by Axe, a spear by Spear: the book's), its impact the thing's own as a weapon's (a light
  thing the stick's, a hefty one the club's: Combat 8), all of it at the short and medium ranges, half at the long and
  extreme (Combat 16's Melee row); its aspect by the d6 (Combat 17's Random Aspect).
- **Its missile**: the launcher's loaded one, else one of its family carried (a quiver's arrows, a bolt case's bolts,
  a pouch's sling stones), the handiest first; one in the pack its reach owed (Reach, handy tools and things used up, below).  "nock an arrow",
  "load the crossbow" load it ahead (a bow's or a sling's half a round, a crossbow's a round).  *(For review: the
  loading's times.)*
- **Its time**: a round a shot, the book's load and fire (a bow, a sling); a crossbow's load a round more, its shot
  the next (the book's: load, or fire).  Fighting on with missiles is a long task as melee is (Fights' fighting on, above):
  a shot each round at the target while it's in range, seen and the line clear and a missile's to hand, then the
  nearest hostile it can shoot at; an enemy engaging it, the melee takes over, with what it holds (by its weight, a shortbow or a hartbow the book's
  stick, a longbow or a crossbow its club; a sling a soft thing).
- **The auto-target range** *(2026-10-30, the user's of 2026-10-20: "autotarget with bow at medium range"; drafted for
  review)*: "autotarget at medium range", "set autotarget to short", "autotarget off" (also "auto-target", "autoengage",
  "autofire"; the bands short or close, medium, long, extreme; "autotarget" alone tells it).  Set, fighting on with
  missiles turns only to a hostile within it (its target, once chosen, followed to the launcher's own range); and a
  hostile engaging it from further off than beside it (shooting at it), none beside it, is shot back at, unbidden,
  with the launcher it carries, if one's within the range (dq0w.c's fight_on, wshoot.c's PS_AUTO: the hub's pc_auto).
  Under a ceiling a launcher's medium band is its last, whatever's set.  *Not yet: a hostile merely coming into the
  range begins it (the world's main loop has no room for the look); melee's "autoengage with sword at close range"
  (the zone and a square more, closed with); the setting kept with a save.*
- **Ranges** (Combat 16's Missile Data Table, in five-foot squares): a sling 15, a shortbow 20, a longbow 25, a
  hartbow 30, a crossbow 20 the short band, each band twice the last, the medium 20 less, the long 40, the extreme 80.
  *Underground, under a ceiling, a launcher's long and extreme bands none and its medium 15 less* (the book's
  restricted missile ranges, its 10-20 the house's middle).  *(For review.)*  A thing thrown keeps all four bands.
- **The line**: each square between the shooter and its mark must let a missile through (the floor, an open door, a
  portcullis's bars, an arrow slit, a window with no glass and its shutters open or none: DUNGEON.md's what passes);
  one that stands in it stops the shot unless the mark is as far beyond it again (the book's firing over).  The
  mark beside a slit or a window the line comes through is behind cover: a strike below its neck strikes the stone
  (Combat 17's cover). *(For review: the cover up to the neck.)*
- **Aware or not**: a denizen that hasn't sensed the character ignores the shot (the book's Ignore); the character
  ignores one from a denizen it doesn't see.  Aware, the defence is the book's: a shield's block at its full block (a
  denizen's its weapon mastery and the shield's class, as melee's), a low-velocity missile's block with a weapon at
  half (a bare arm's at half its Unarmed, struck by it: H2), none against an arrow, a bolt or a stone; a dodge at full
  against a thing thrown, at half against those.  The character's by its stance (block, dodge; none none; else the
  better), a denizen's the better.
- **The EML**: the skill (Bow for the bows and the crossbow, Sling, Throwing or the thrown weapon's own), less the
  range's band, an aim high or low 10 (no command says one: Fights, above), the mark's size (Combat 16's table: a giant rat 15 less, a bear 10 more; one
  prone, kneeling or sitting a two-foot mark).  *A denizen's skill at a missile weapon its bestiary weapon's mastery
  ten less* (as any other weapon of a skill not its own's).  *(For review.)*
- **What comes of it** (the missile attack table): wild (the shooter's fumble roll for its launcher: failed, let
  fall), a miss, a block (the shield or the weapon struck, no harm), a strike of n dice and the missile's impact at
  its range, the armour at the location off it.
- **A miss's stray** *(2026-10-30; the chances the user's, 2026-11-03: "follow the book")*: a miss (or a dodged one) at a
  mark with others beside it strikes one of them, chosen at random, half the time if the mark's engaged (the book's: a
  missile shot at one engaged in melee, Combat 15) and a fifth of the time if it isn't (the book's nearby target, Combat
  17); *the mark engaged if it, or one beside it, is in a fight (the house's)*; the character too, every shooter's and
  thrower's, never the shooter itself.  *Its strike one die, no test, no defence (the referee's MS_STRAY, both referees: the
  null's a hit; for review)*; told after the miss ("Your shot strikes the giant rat instead: ...", "Its spear strikes
  you instead ..."), the one struck the one hurt (its courage, its fall, its kin's dismay); the missile at its feet.
- **Where it falls**: a hit or a block at the mark's feet; a miss on past it along the line, one to four squares,
  short of what stops it; a wild one somewhere within two squares; known to the character.  *A launcher's missile gone wild is lost one time in two, beyond finding (the user's "wild misses
  should possibly be lost", DECISIONS.md: how often the house's, for review); what's left of it then breaks by its chance as any other; a thing thrown by hand is never
  lost: too big to go missing near by.*  *An arrow breaks one time
  in four (kinds.c's snaps, the user's "arrows can break"), a bolt one in seven (the house's: shorter, thicker), a
  stone never; a thing thrown never.*  *(For review.)*  One of a lot thrown (a taburi of a handful) goes alone.
  "gather the arrows" walks over to each missile it knows of within twelve squares, seen, the nearest first, and
  takes it up into the carrier of its family it has on it (a quiver its arrows, a bolt case its bolts), joined to the
  lot there, if there's room (2026-10-30); else as a thing taken up (the pack, a hand).
- **Denizens**: one with a launcher of its own and its missiles, hostile, sensing the character further off than
  beside it, shoots (its mind's: the world says if it can; no shot to be had from there, out of range or no clear
  line, it closes with the character till the character moves); its launcher taken up from its back or its belt first, its
  round's; its missiles run out, it shoots no more; the character beside it, it lets its launcher fall (no time, the
  book's) and draws its weapon.
- **A denizen's weapon changed for its bow** *(2026-10-30, for review)*: one holding a weapon (and a shield) puts them
  up (slung, hung on it) as it takes up its launcher, all of it its round (the book's grope; the house's one round
  for both); a light in its hand it keeps, and doesn't shoot.  Back to melee, its launcher let fall, the weapon it put
  up is the first it draws (its round), its shield readied the round after, as ever.
- **The denizens throwing** *(2026-10-30, for review)*: one with nothing to shoot (none, or its missiles spent: its
  launcher let fall first) throws, by the character's own path (the same missile attack, Throwing or the weapon's
  own skill): a thing made to be thrown (a taburi, a shorkana: the book's thrown weapons) from its hand or its belt,
  else a dagger, a knife, a spear, a hatchet or an axe in its hand if it keeps another weapon to fight with; one at
  its belt drawn first (its round, what it holds put up); *within the short and medium bands only (the house's: no
  denizen throws its weapon at a far mark)*.  What it threw lies where it fell, anyone's.  *Not yet: one holding
  another weapon (a sword, a mankar) with its taburis or shorkanas at its belt (its belt's looked at, as its things
  are told the referee, only till it's armed: the answers' time); a rock picked up
  (the dungeon has no loose stones to pick: Ways found, positions, below).*
- **Every fall told** *(2026-10-30)*: any actor knocked down in a fight (a stumble, a leg's wound, a missile's), the
  character too, is told as it falls, after the blow's words ("The giant rat goes down.", by its body; "You fall."),
  seen, or heard if it's near and out of sight ("You hear something fall.": within eight squares); its rise told once,
  as it gets up.  Down senseless, or dead, its fall is the blow's own words ("It falls senseless.").
- **The denizens' positions** beyond prone *(not built)*: a shooter crouched behind cover: Ways found, positions, bodies
  found's not yet, below.

## Commands

The words for a walk are Where you are, going places, and long tasks', for a journey Travel's, for a look Looking at
things', for a search Walking and searching in the dark's; a thing of several and how many of it: the last part of Light
and traces.

- **"It" is the last thing named,** of any kind (the user's, the grammar's answer 7): a thing, a denizen, a part of
  the place.
  "Take it" with nothing named yet says so.  (resolve.c.)
- **A verb said alone takes its own thing** (commands.verbs' unsaid): drop, put, wear, wield, hold, read, use the
  last thing named; take the last thing named if it lies near, else the thing near, asking which of several, and
  saying so when there's none (the user's, the grammar's answer 8); attack the target; throw what's in its hand, at
  the target; light and douse what's in its hand; open, close, break ... what's in front; look and examine the place.  (resolve.c; main.c.)
- **"Take all"** takes each thing it sees lying about, eight at most, the nearest first (those in reach before those walked to),
  in turn; one it can't take, or no longer sees, ends it.  (resolve.c's taken_next.)
- **What it's done with** ("hit the door with the bar", "strike the rat using my sword", "dig down with the shovel",
  "light the torch with the tinderbox", "pick the lock of the chest with the lockpicks"; the user's playtest, 2026-10-22:
  "`hit door with bar` hits the door, even if I don't have a bar"): every command done with a thing (a blow, breaking,
  forcing, bending, raising or lowering, disarming, digging, lighting or putting out, opening, closing, locking or
  unlocking, filling, knocking, pushing, pulling, touching, barring, climbing, searching, examining) takes it only if it's
  to hand, carried or lying within reach (the things' IO_FIND: the rule a fill's fuel and a door's bar already had, used
  where it lies); else it's refused, the things' words ("You have no bar to hand.", "The bar is out of reach: go over
  to it first."), never done with bare hands or another thing instead.  A blow's thing is taken in hand first ("You take
  the bar in both hands.", its time), as a named weapon was (names.c's); no hand free for it, refused, the things' words
  ("Your hands are full: stow something, or put it down."); that thing strikes, and no other (Fights' what strikes).  A part after "with"
  is what it's done with, not the door's ("hit the rat with the bar" no longer batters a door).  (resolve.c's tool_ok;
  cmdmean.s.)  A lesser tool named for a dig is the one used, a better carried told of: Digging's a tool named.
- **Which of several:** the closest by squares (and the lead, as yet, the closest); the largest and the smallest by
  a thing's size (how easily it's seen); the second as they lie from the character's left round to its right; any,
  the closest.  Several of a name and none said: the one the world's services choose, as ever (A thing of several,
  below: the one most to hand; the nearest thing, the denizen beside it), but a throw's mark, a target set and a thing gone to must be
  one: then which, asked.  (resolve.c.)
- **A question** lists the choices from the left round to the right, how far off each is where two lie the same
  way, and takes a number, a way ("left"), a selector ("the closest", "the second") or a name; any other line lets
  it go (the user's, the grammar's answer 13).  (resolve.c; the doors' and the stairs' own, choose.c.)
- **The target** is set by "target the rat", or by any "at the rat"; shown by "target"; it's what attack and throw
  take when nothing's said.  It's let go by "clear target"; as it's down while another denizen in sight is up; as
  it's dropped or thrown away; as it's out of sight; as the character's made to turn: a denizen it didn't see before
  comes into sight, a blow comes from one it doesn't see, a trap is set off.  (resolve.c; blows.c, fixt.c:
  untarget.)
- **Shortcuts** (the user's, 2026-10-22: "alias should be for shortcuts, and name for naming things"): "alias nd is
  north then dig down" (or "alias nd = ...") makes a word stand for a line; a line typed that begins with it is that
  line, the words typed after it put after it ("ex the torch": "examine the torch"), made once, as it's entered, before
  its parts, so a shortcut may hold several commands.  A shortcut whose line begins with another's is given that one's
  line as it's made (as the grammar's reference has it).  Its word already a shortcut, the player's asked first ("yes"
  replaces it; anything else keeps it).  Refused: a word the commands need ("look", "north", a number: as for a name),
  the shortcuts' own words, a definition inside one; "alias the sword as Linda" is sent to name.  "alias nd" shows one,
  "alias" lists them and then the game's own words (commands.aliases, the parser's: on the card as aliases.txt);
  "unalias nd" or "forget alias nd" removes one.  **The game's own words can't be removed** ("unalias peer" says so):
  they're the parser's tables, the same for every game; but the player may make one a shortcut ("alias peer is
  status"), which then takes its place as a line's first word, and removing that brings the game's back.  Sixteen
  shortcuts, each a word of 15 letters and a line of 95, kept in the theme bank (hub.h's ALIAS_AT) and the save.
  (alias.c, part 9, a bank of its own; term.c's enter.)

## Holding back while a line is typed *(2026-11-04: the user's "Hold back"; new, for review: the three seconds)*

At the real paces the world's long jobs wait from a line's first key until its answer's first word, so none is under way
as its Enter comes and a command's first letter isn't behind one.  They are a denizen's things drawn (and any intents
behind them in the ring), the traces' journal taken in, the lights' minute with its sconces and the time of day it
believes, the map making's things, and a look about as a body falls: each a tenth of a second of the world's time or
more.  Those that can stop part way do (a denizen's things between sections, the journal, the map making's things, the sconces
between their passes, the senses' sound pulse for an Enter), and the first of them all look at the keys as they fall due.  A denizen's plain intents, its steps and blows, wait for the keys not at all, and for an Enter a quarter second at most,
as before; nor do the character's own doings wait (a step of its walk, its own look), being the answers.  So typing holds
no one still.  A long line typed slowly or a steady typist can't starve them: three seconds at most (600 ticks,
dq0w.c's KEY_HOLD, from the first one held back), then a pass of the world's loop goes free and whatever's due runs through, the keys not
stopping it.  An Enter does, its answer first, but once: the next free pass runs through an Enter too (that one answer behind
it), or a steady typist's Enters would stop a denizen's things every time, drawn again from the start.  Then they hold back again.  A line emptied (Ctrl-U, Backspace) lets them go at once.  At the turns pace nothing holds back: that
game turns on its seed and commands alone.  A line that takes longer than the three seconds to type may still meet a job
as it's entered, as any line could before.

## A command's first words *(2026-11-04: the user's "Word as they begin."; new, for review: the words)*

A command's first letter comes within a fifth of a second (720,000 cycles, D17), or it says a word as it begins and the
world's answer follows.  The word is said before the world's asked, and holds whatever the world then says:

- **A portal worked** (open, close, unlock, force, bar, unbar, raise, lower, bend, go over for traps, disarm, break, climb
  through): "You try the door." to open; "You reach for the door." to close; "You look at the door." for the rest ("You
  feel the door." in the dark); "You head for the door." from across its room.  None if a turn to it comes first (the turn's
  words are its first), or if it's asked which door.
- **A thing handled** (hold, stow, put, take from, wear, take off, light, put out, drop, fill, sharpen, hide, tie, untie,
  open or close a thing, throw): "You reach for the sack."  A thing to take from the floor, "You reach for" only if it's seen in reach; seen only
  out of reach, its walk ("You go over to the bones."); not seen now, "You look for the bones." (then its walk, if it's remembered, or "You see none of those here."; in the dark, none).
- **A thing looked at closely or asked of** (examine, assess, heft, its lock, its traps, look in it): "You look at the sack." if it's
  seen; else "You look for the sack."  In the dark, none: the answer's its first words.
- **A denizen swayed, a gift, a trade** (talk, calm, threaten, name, give, buy, sell ...: the denizens' task's to answer): "You look for the rat." ("You feel for ...").
- **Read, combine**: "You reach for the sack."
- **A look one way**: "You peer ahead." at once, unless it turns first.
- **Listen**: "You listen."  **A dig, the floor or the wall looked at**: "You look at the floor." ("You feel the floor.").  **A journey**:
  "You think back."  **Through a door**: "You head for the door."
- **A wait, a rest, a sleep, as long as it says**: its own words at once ("You wait a minute."), not again as the world starts it.  A round's ("Time passes.") by the world, in time as it is.
- **Told once a line**, not again as its command's asked again after a step of the reach.
- **Short answers go out as they're said**: an answer's first words aren't held for the rest of the world's news
  ("You sit down." at 590,000 cycles, not 860,000).
- **Left as they are, over the target**: a plain walk or step ("n", "go south", "go back": "You set off south." some 650,000 to
  1.4M, its way worked out first; a word before it would be said on every move, and "You set off" can't be said till the
  way's known); a door gone over for traps (about a million: the pace's words looked for first in "search"); a blow's walk over ("You go over to the giant rat.", 1.1M); "name the rat Bob" (2.6M: its names' parts first); "help" (a million: the card); "stop" with nothing to stop, the map, a pace set (to 780,000: the answer itself, no word that reads well).

## Characters *(hmchar: HarnMaster's character generator)*

- **Every occupation the book lists, and one of the player's own** (the user's, 2026-10-20).  hmchar offers the
  Occupational Skills table (Character 15: its 55 occupations, unguilded, guilded and noble; those the character's
  class and sex allow it, below); the soldiers of the character's culture, by unit (Character 27: the Feudal's with
  the fighting orders', the Imperial's, the Vikings', the Khuzdul's, the Sindarin's; the Tribal have none); and the
  clerics, by church (Character 23: a character with a deity its own church's; one with none any of the ten, the
  church making its deity the character's, its ritual skill opened).  Not yet the mages (Character 25): their skills
  come with magic.  A soldier has every soldier's skills too, a cleric every cleric's; a skill both list, the higher.
  The book's specialties are their skills (Ratcraft and Dogcraft Animalcraft, a lance Polearm, a falchion or a
  longknife Sword, a hartbow Bow); more scripts are left out (harn.h has one Script).  **A second language**: the book's Sage/Tutor, Scribe, Innkeeper, Litigant, Mercantyler and Herald (Character 15) have one, its file's "language NAME=ML" line, at the book's opening for an alien family's, its skill base once (Skills 11: a poor grasp, so a poor limit on Rhetoric through it): *(new, for review: which one the house's, by the character's culture among the game's peoples' tongues: feudal or imperial khuzdul or sindarin, viking khuzdul or gargun, tribal gargun or kobold, by its name's first letter)*.  hmchar's C and HyForth (its hylang port not yet).  A second language in a cleric's church list (Character 23) stays left out.  The table is games/hmchar/occupations.txt (on the card in games/lib/hmchar, beside hmchar), read a line at
  a time as it's wanted.  An occupation is chosen by its number or a beginning of its name (a group listed by its
  first letter, ? and one its skills shown); -c NAME picks one and -q draws one, the one draw either way, so the rest
  of the character comes out the same.  (hmchar's bank program, bank/hmocc; hmchar.fs and hmchar.hl the same.)
- **The player's own occupation is 11 points** (the user's: the book's 55 are worth 1 to 21 points, 11 their median
  and their mean, a point opening a skill at its OML and a point for each SB more).  A point opens a skill at its
  OML, or improves an open one (an automatic one too) by its SB; two points in a skill at most, and three in one
  only, its primary (3 + 2 + 2 + 2 + 2 spends them all); the player names it.  Then the usual 5 option points, two
  in a skill at most (an open skill takes one more): the primary's three are the player's own occupation's alone.
  (hmchar.c's spend.)
- **An automatic skill an occupation lists takes the occupation's OML**, not the two added (Character 16); a skill
  listed twice, the higher.  The weapons an occupation gives to choose open at their OML plus its number of SBs (the
  Gladiator's three at OML + 2 SB, the Mercantyler's one at its OML), its crafts at SB times its number (the
  Chandler's three at SB x 3).
- **Occupations by class and sex: the book's limits** (the user's, 2026-10-20; Character 13 and 15).  A slave's, a
  serf's or an unguilded character's choice is the unguilded occupations, a guilded one's those and the guilded, a
  noble's all of them.  The soldiers and the clerics are any class's, as the book lists them among the unguilded (a
  church takes the devout of humble birth too, Character 23), but a knight is a noble (Character 13: knights
  and heralds the high-born's; Character 27's units for nobles: here the Feudal knights, the fighting orders', the
  Imperial Patrician and the Sindarin Knight).  The Lady is a woman's; nothing else is held to a sex.  The player's
  own occupation is open to all.  -q draws within the limits (still one draw, so a fixed -c keeps a test's
  character); -c names any the character's culture has, not held to its class or sex (a game's or a test's choice:
  dq0traps' thief is seed 1's slave).  occupations.txt's headers say it, a group's class or sex beside its culture or
  deity ([noble Female], [soldier Feudal Noble]); hmchar.fs and hmchar.hl read them as C's hmchar does.
- **A human's culture is rolled, mostly Feudal** (the user's, 2026-10-20; Character 3: by birthplace, most of Harn's
  Feudal, the common exceptions Imperial, Viking and Tribal): 01-70 Feudal, 71-80 Imperial, 81-90 Viking, 91-00
  Tribal (our odds: the book leaves birthplaces to the regional modules).  An elf is Sindarin and a dwarf Khuzan, as
  harn_create has them.  A culture not Feudal has its social class rolled on its own column of the book's table (and
  the native tongue's OML follows the class).  hmchar -u CULTURE fixes it (feudal, imperial, viking or tribal, a
  beginning enough: the user's, "fixing culture the same way (with flag)"): the tests make their characters with -u
  feudal, so each is the same every time; -q rolls it (the soak's characters too).  (hmocc's culture: after
  harn_create, which stays as it is, the SDK's and the danlang harn.dl's.)
- **A deity is the player's choice** (the user's, 2026-10-20; Character 12): one of the ten or none, asked once the
  roll is kept, before the occupation, the book's Morality range for each shown as a guide beside the character's
  own (nothing refused; the book's lower Piety for a deity that doesn't suit, 4d6 or 3d6 at the GM's discretion, not
  taken); its Ritual opened at its OML, as harn_create opens a religious character's; -q none.  The profile names it
  after the Piety.  A cleric's church is its deity's: a character with a deity is offered its own church alone, one
  with none any of the ten, the church making its deity the character's (as before).  (hmocc's deity.)
- **The family's wealth is rolled and kept** *(2026-10-27: drafted for the user to review)* (DECISIONS.md's outfits,
  answer 2; Character 17's three levels a class: serf 40, 80, 160d; unguilded 60, 120, 240d; guilded 90, 180, 360d;
  noble 600, 1,200, 2,400d): d100, 01-25 poor, 26-75 average, 76-00 rich, an outcast's or an unpopular child's roll
  ten less, a favourite's ten more (the book's nod to estrangement, made mild); a slave's a serf's poor, 40d (one who
  has come away with something).  Rolled last of all but the timesense (below), so the rest of the character's dice
  fall as they did; written in its file, "wealth 120" (hmchar.c's wealth).  The referee reads it (a file without one, an older one or another
  program's, has it rolled from the character's own numbers, the same each time: dq0harn/outfit.c).  It isn't written
  back into the save's copy: the family's wealth is what the outfit's bought with, at the start.  *(The odds and the
  nudge the user's answer 2, DECISIONS.md's; a slave's the house's.)*
- **A rare trait, absolute timesense:** one character in a hundred: The time of day believed, below.

## The null ruleset (dq0null)

A second referee with no book behind it (Phase 3A: RULESETS.md), to show the game asks a ruleset nothing but rs.h's
requests; the house's rules above marked *(every ruleset's)* it plays as HarnMaster's referee does.  Its own:

- **A test** succeeds on a d100 of at most 60 and 5 a step (5 to 95); the lowest fifth of the success band is a
  critical success, the highest fifth of the failure band a critical failure.  No skills, no attributes, no
  penalties.  Why: flat odds, the steps the only difficulty.  Keeping its feet over a thing in the dark (P_STUMBLE)
  the same, its steps the darkness's, the thing's and the pace's (Walking and searching in the dark): seldom kept.
- **The character takes 10 wounds** (dead at 10), **a creature 6.**  A fall does a wound each 10 feet, a trap's
  strike or a burn 2.  Health: hurt under a third, badly hurt under two thirds, critical past that.
- **A blow** is a flat test (a prone defender four steps easier, two harder for each other foe; a shield held two harder, once a round: a blow that would have hit but for it, blocked); a hit 2 wounds, a
  critical 4.  A prone attacker gets up instead.  A creature wounded that stands holds on a flat test (a shy one four
  steps worse), else it flees.  A blow at a thing does 6, less the thing's least resistance.  A missile the same flat test, two steps worse a band past the short, four behind cover, two easier unaware of it; a stray a hit, no test.  What strikes (Fights')
  the house's: a thing named strikes, its words its name, whatever it is (a soft one the house's single attempt);
  nothing named, the first thing in its hands; never a better thing carried asked about (none's better).
- **No skills to open or develop** (slice 4.4): nothing marked, nothing rolled after a rest; its character carries on into
  a new dungeon as HarnMaster's does (its file, its wounds).
- **No fatigue (but a creature's simple count: the denizens' bodies, below), no bleeding, no shock, no senselessness, no encumbrance.**  Senses all 5 (hearing 7 listening as it
  goes); a square takes 4.5, 3, 1.5, 0.8, 0.4 and 0.3 seconds at a creep, a slow walk, a walk, a jog, a run and a
  sprint.
- **Its character** is any file's name, occupation, pace and language lines (hmchar's will do), with its wounds;
  without one it makes its own, Dingus, a wanderer.  It speaks common and its file's tongues.

## The history and the theme *(every ruleset's)*

- **A game's own theme** (G2): a seed of 65,536 or more has its region's history and its theme made as it begins
  (themegen: HISTORY.md); a seed under it, a stand-in theme's, by its second byte.  (main.c's s_begin.)  Why: the
  clock's seeds are all past it, so every game has a region of its own; the tests' small seeds keep the themes
  they were written against.
- **How a dungeon ends** (the user's Q1 "Every dungeon", Q2 "No, walk out", 2026-11-04: between-dungeons plan, steps 1 and 2; the rest *(new, for review)*).  Three ends, and the words never say you win or lose:
  - **Solved:** a rule with an effect done (theme.h's TE_OPEN, TE_RULE; a take rule's, or a far exit gone out by).  Told at once: the rule's words, then "That's what you came to do.  Now to get out alive.", and the game goes on: the way out opens if it was shut, and what's about is still about.  It's kept whatever follows (Q.solved, in the save).  Out of the dungeon by any way, the theme's solved ending (escaped, or ruled if the place was ruled).
  - **Left:** out by any way, unsolved.  Its own words (the theme's `ending | left`), no loss, and the character carries on.  Walking up a stair out, the first time in a game, asks: "That's the way out, and what you came for undone.  Go up again to leave it so."  A shaft to the sky doesn't ask: twenty feet of climbing says it's meant.
  - **Died:** as ever (D7).  Solved first, it still counts solved (Q.solved stays), and says so in place of the theme's words: "You did what you came to do, and you won't be climbing out to tell of it."  A death isn't kept but in hard mode, so only there does a save hold this one.
  - **The way in:** a theme's reason says if it's shut (`wayin | open` it stands open: the artifact's goal, which came to find a thing and has no reason for a door to shut).  **A shut dungeon has another way out to find** (`wayout | N`: a stair to the daylight, open from the start, on stratum N, as far from that stratum's way in as it's found, the same as a through goal's far exit, which is one).  Themegen's evil and prisoner goals put it on a stratum below the top by the seed's own number, none of its dice.  The stand-ins' are the crypt's Black Crypts, the temple's third, the warren's fourth.  (dq0m/plan.c's links; theme.c's gone.)
  - **The theme checker proves it** (themec's and make.js's, every theme): `wayin` or `wayout` (or a through goal), so a character with nothing but its legs has a way out, and an `ending | left`.  A sky shaft isn't counted: it's by dice, its mouth may be out of reach, and its climb may fail.  What it proves is that the stair is made; that its stratum can be walked to is the plan's own guarantee (its strata all linked, its links in its halls).
  - **Each goal's solving:** an artifact's, taking it up (`rule | take | X | open | words`: the thing come into the character's hands by any taking, the world's); the evil's, the relic laid on the altar; a prisoner's, the shackles opened; a through goal's, going out by its far exit.  A stand-in's rule of `open` or `rule`, the same.  Why: the user's "walk out with what was found, past what's still about".
  - *What's still doubtful, for your review:* the artifact's old door with its wards is gone from its goal (a shut-door kind would be one more block in goals.txt); a freed prisoner doesn't yet walk (she stays a fixture, "I'll come behind you"); a dropped artifact stays solved; the thing itself is still left at the door on a carry-on (the theme's own things: steps 7 and 10); and a curse broken doesn't still its dead (the words say they're slow to learn it).
- **A stratum's age is its history's:** its oldest layer's era's, the centuries since it ended and one more (a
  stand-in's, the world's dice: 1-40 centuries, 1-20 the top's).  (dq0m/plan.c, from the record's sage.)  Why: what's
  rotted, what's flagged and what's earth follow the history.
- **What a history leaves lying about:** a chunk with an older layer under it, each room's use under the older
  purpose its mark (rotted by the older layer's age: a kind that rots gone as often as its centuries in 40); a
  chunk a ruin broke, a room in two its debris (bones where the era was conquered or cursed, potsherds else).
  (dq0m/stock.c, uses.c's US_UNDER.)  Why: exploring tells the story (DesignIdeas.md).
- **A ruin's odds** (stratagen): a chunk of the works standing when it came is broken one in two for a collapse, one
  in three for a conquest, a flood or a curse, one in four for an abandoning; a later people's own part (a reuse:
  a third to two thirds of the way from the way in to the stratum's far side, by its seed) isn't.  Why: a ruin's a
  ruin in places, and those who came after mended what they took.
- **The evil's goal solved where it's done:** the relic laid on the curse's altar breaks the curse (How a dungeon ends, above); its dead aren't stilled, and are still to be got past.  Why: the denizens' answer to it is still to come.  *(The user's Q2, 2026-11-04, in place of ending the game there.)*
- **A faction's others are the theme's own:** themegen writes how a faction stands with the factions the region has
  (the bestiary's factions.txt lists them all), and only a game of its own writes faction lines in the save folder
  (theme.txt: popgen's), a stand-in's game none.  Why: popgen reads every name it's given, and the stand-ins' games
  (the tests') keep the stand-in's denizens.

## Where you are, going places, and long tasks *(slice 4.0: drafted and reviewed by the user, 2026-10-20)*

Every ruleset's.  The user's choices at the slice's start (DECISIONS.md, 2026-10-20) are taken as given; the user kept
the draft's numbers as they are (the sizes, a hostile's 30 feet, a look one way's 3 seconds and twice the reach, a
climb's test to hold on).

- **Where you are is told by the facing alone**, never the compass (the user's).  A room's floor is taken in thirds
  each way by the character's facing: far (ahead), middle, near (behind), and left, middle, right.  The character is
  "in the middle of" a room, "on the right side of" it, "at the far end of" it, "in the near left corner of" it, and
  so on; a thing lying at the room's edge the same ("In the far left corner lies ...", "Against the wall to your left
  lies ..."); a denizen by its bearing and distance ("A giant rat is ahead to your right, 10 feet away": the user's,
  2026-10-20).  Only what's known: with the walls beyond the light, "You are somewhere in a vast cavern, its walls beyond your light."
- **A room's size and shape words**, by its floor: small under 150 square feet, a plain room to 600, large to 2,500,
  vast beyond; long when it's at least twice as long as it's wide, and narrow too if it's 10 feet across or less;
  wide when it's 40 feet across or more ("a long, narrow hall", "a wide hall", "a vast cavern").
- **Going places**: the centre (the middle), the far side (straight ahead to the wall: "walk forward to the other
  side"), the near side, the left or right side, a corner ("the far left corner"), a wall, a feature or a thing seen
  ("the door ahead to my left", the pillar, the body).  The way goes round what's in it through squares the
  character knows; one it doesn't know of is found as it's met.  The small moves stay for fine work.  A journey back to a
  place it knows: Travel, below.
- **Straight walks** *(the user's, 2026-11-02: DECISIONS.md; as built, for review)*: every way word said alone
  (forward, back, left, right, the eight of the compass: one rule for all, the user's) walks straight from a room's
  floor along that heading until something stops it, and says what: "You come to the wall.", "You come to a closed
  door.", a portcullis that's down, the edge of a pit, a trap it knows of, a ledge's words, a denizen in the way.  An
  open door, a doorway or a passage's mouth straight ahead: on through it.  A shut door it has handled (opened or shut
  before): opened as it goes, as a door was by a way word till then.  Refused without moving only with what stops it
  directly in front ("The door is shut: open it first.", "You can't go that way." at a wall), turned to face it.  A
  door off to the side is never walked to by a way word: `go through the door`, `go to the door`, `open the door`,
  `leave` are the way to it.  `go through the door` at a shut door it hasn't handled walks up to it first, and
  says so.  Passages keep their forks and bends; a journey's legs (the door they mean) keep their path.  A walk of a
  square or two isn't "You cross the room."  In the dark it goes the same way, its hands telling what it meets.
- **Going to anything** *(the user's, 2026-11-02; as built, for review)*: approach, go to, walk to, walk over to, head
  for, move to, go near, go beside, and their kin, for a thing, a fixture, a denizen (up to it, no blow; the walk
  follows it as it moves) or a body: anything it sees, or remembers seeing in the room it's in though that's out of
  the light now (a thing it has looked at, on a square it knows).  Of several of the name, asked which ("the closest"
  chooses).  It stops beside it (facing it) by the ordinary walk: its stops, engagement and traps as ever.  Within
  reach already, told where it is.  "You haven't seen a pot here." only for what it hasn't.  A denizen's zone doesn't
  stop a walk up to the one gone to (a close-in's); others' do.  Told as it starts ("You go over to the pot.", "You head
  for the door.", a place in the room "You set off across the room.": the user's D17; a thing to take that's in sight and
  out of reach told before the world's asked, its answer the time).  "Go to the pit", "go to the trap": the nearest trap it knows of in the chunk it's in,
  whatever its kind ("You go over to the trap."; "It's beside you."; else "You haven't seen a trap here.").  "Go to the
  ledge": straight at the part of the room the look told, till its ledge stops it ("You haven't seen a ledge here." with
  none told).  A walk that has got as near as it can ends there, untold; only a walk that couldn't start is refused.
- **Crossing** *(the user's, 2026-11-02)*: "cross the room", "go across the hall", "cross the floor", "cross the
  kitchen": the far side by its facing (the going places' far side); in the dark, straight on, as any place in a room
  it can't see is.
- **Leaving a room** *(2026-11-02, for review)*: "leave", "exit", "leave the room": by the door or the passage's mouth
  it sees, the one ahead, or asked which ("Which exit: 1. the door ... 2. the passage ..."); "exit north": a way word.
- **Inspecting walks over first** *(the user's, 2026-11-02; as built, for review)*: a closer look at a thing (examine,
  check), any aspect of it (assess, judge, appraise, heft, its condition, its lock, its traps), looking in it and
  searching it, out of reach: walked over to first, then done; things remembered in the room too, as is taking one up
  or holding it.  A plain "look at" stays a look from where it stands, as far as it can see (too far to make out, its
  size alone).  Out of the light, what's done beside a remembered thing goes by touch, no worse (a doubt: a close look
  in the dark tells as much as in the light).
- **What can be named** *(the user's, 2026-11-02; as built, for review)*: one rule for "which door?", go to, inspect and
  the rest: what the character sees now, or has seen and remembers. Doors, windows, portcullises, passages' mouths and
  the stairs and shafts of the room it's in, on squares it knows (the senses' record's KNOWN_REL, after those it sees;
  in a passage, doors seen within five squares), things in this room it has looked at (the store's pick), and a denizen
  only where it was last seen as a command was given, in this room.  It walks over and sees it properly as its light
  reaches it.  What it has never perceived: "You haven't seen a door here."  The look still tells only what it sees.
- **The way a thing lies** *(the user's, 2026-11-02; as built, for review)*: "the door on the left", "the door ahead",
  "behind me on my right" mean what the look's words mean, by the facing at that moment: its bearing in the look's
  eighths.  None exactly so, any on that side of the character (half the room; a door along the left wall far ahead is
  "on the left"), of several the question or the one directly ahead; none on that side, "You haven't seen a door that
  way."  "A door" is a door before a portcullis (the look's words), a portcullis only if it sees no door.
- **A walk over for a command** *(the user's, 2026-11-02; as built, for review)*: take, hold, wear, put, light, examine,
  read, and the aspects, a blow's close-in, gathering: the walk told as it starts, at once ("You go over to the pot.",
  "You head for the door.": the user's D17, a command answers at once), what's new since the command began (a thing just come into the light, a denizen new in sight), a beat, then the command's outcome.  No look
  on arrival, no "at your feet" for what it came for.  Stopped short (engaged, a trap), why, and the command dropped.  A
  plain "go to the pot" keeps its look.
- **A step or a walk by a way, told as it begins** *(2026-11-03, for the user's D17, a command's first letter within a
  fifth of a second; as built, for review)*: "step forward", "step back", "step left", "back up" say so at once ("You step
  forward.", "You step back.", "You step to your left."; the small moves by a count too), "go north", "walk west", "go
  through the door" ("You set off north."), once the world has it under way; its look as it gets there, as ever.  What
  can't be begun says only why ("You can't go that way.").  Before, a step's first letter was its look's, a second on.
- **A door's state as it's plain** *(the user's, 2026-11-02; as built, for review)*: the look, "which door?" and a look
  one way say "(closed, barred)" with the bar on the character's side, once it can see the door; a window's shutters
  the same.  A bar on the far side, or a lock, isn't seen: told as an attempt finds it, and remembered ("(closed,
  locked)": PP_FOUND, kept with the door's state).  "unbar the door", "unbolt", "lift the bar", "take the bar off",
  "remove the bar": its bar lifted out, the door left shut; "It isn't barred." with none; the far side's refused.
- **Out of reach, walked to first** (the user's, 2026-10-20): an action that needs its thing within reach (taking it
  up, holding it, wearing it, putting it in or on something, opening a chest; a blow) walks over to it first, a long
  task like any walk, then does it; a blow at a denizen out of reach closes with it first (the walk ends beside it,
  or as anything engages the character), unless the character's engaged already or strikes with a missile weapon.
  A door, a window or a portcullis worked or struck ("close the door", "hit the door", "kick the door", "go through the door")
  that isn't beside it by the four ways, nor on the walls of the room it stands in (round a bend, down a passage beyond the
  room), the same: "You head for the door.", then the command (2026-11-03, the check of this document; dq0's door_go).  A
  denizen standing in the door meant: "A giant rat is in the way.", no walk.
- **Engagement** (the book's engagement zones, Combat 6; the user's reminder, 2026-10-20): an enemy on its feet and
  conscious (a hostile or unfriendly denizen: one that strikes at a character beside it) has a zone, its own square
  and the eight round it, but not through a wall or a shut door, nor round a corner (a diagonal with solid squares on
  both sides).  One lying down has only its own square, one senseless none; a friendly one's or an ally's doesn't
  count; nor one that hasn't noticed the character (roaming, its mind's doing as it doesn't sense it: walked past
  unaware, it engages no one till it does; every actor's, 2026-10-29, for review).  The character in an enemy's zone is engaged with it.  Any walk stops as it comes into an enemy's zone, a
  hard stop ("The giant rat engages you."), and one begun in a zone goes a single square and stops there if it's
  still in one (the book's one square a turn within them): a walk over to a thing, or past a hostile, stops where it's
  engaged.  A blow's close-in ends in its target's zone, beside it, and the blow is struck; engaged by another first,
  no blow, and the player says what next (a hostile one fought: Fights' fighting on).  Engaged already, a blow at
  one out of reach isn't closed with (the user's).  The denizens move by the same rule: one coming at the character
  stops in its zone.  Not used: the book's
  long-reach weapons' two squares (not built) and its reaction zones (the game's 30 feet stands for
  them: a hostile seen that near stops a long task).  As built: a close-in's target
  struck at the character as it comes beside it is no stop (it's the fight it went for: the character's blow follows);
  "attack the rat" with several in sight and none beside the character closes with the nearest; a bare "attack" with
  no target, nothing beside it, is refused, as before.  A target on a square the character's way can't reach is
  walked to as near as it can, then "You can't get near enough to the giant rat." if it's no nearer (2026-10-20:
  it was "You can't go that way.").
- **The engage initiative test, as built** (the user's, 2026-10-20; the book's optional rule): as two are newly
  engaged both test their quickness (the referee's RS_CONTEST, P_INITIATIVE: HarnMaster's Initiative, a creature's its
  bestiary's), and the defender, if it does better, gains a tactical advantage, an action more at once.  Newly
  engaged: the character's walk coming into an enemy's zone (it stops there, as before), a close-in coming beside its
  target, an enemy stepping up beside the character; the one whose move did it the engager.  A denizen's advantage:
  a hostile one strikes first ("Quicker, the giant rat bites at you: ..."), one that isn't (an unfriendly one, a wary
  one closed with) backs a square away, out of reach, if it can ("Quicker, the giant rat backs away from you."), else
  strikes; the close-in's own blow after it.  The character's: its blow first at the one that came up to it, with
  what it holds, at the middle ("Quicker, you strike at the giant rat with your sword: ..."), which stops its long
  task as a blow at it does.  None while the character climbs.  dq0 -I turns it off, and a grope's test with it (the
  user's, for the tests: the runner gives every dq0 test -I but those about it, initiative: true, so their fights'
  dice stay as they were; the soak plays it).
- **A grope while engaged, as built** (the user's, 2026-10-20; the book's grope): taking a thing up (within reach, or
  walked over to), holding one (drawing, wielding, swapping a weapon), stowing it, putting it in or on another, or
  taking it out of one, while the character's in an enemy's zone, is a test of its hands (the referee's P_GROPE:
  HarnMaster's Dexterity times 5); failed, nothing's done and the round's wasted ("You fumble for the sword.").
  Wearing and taking off (but a pack on the back: Reach, handy tools and things used up, below), dropping, lighting from a
  flame to hand and putting out aren't gropes; a fire-maker's steps and a tool's are (the same).
  Its time the reach's (Reach, handy tools and things used up: a round a step, each step its own test).
- **Going places, as built:** the place's square worked out from the room's floor as the character faces (the floor
  in thirds for the centre, its edge rows for a side or a corner, the nearer end of each when a corner's not said);
  where it's rock (a cave's), the nearest square the way reaches; "You're there already." if it is.  The search's
  minute a long task, its tests at its end, cut short only the squares nearest as far as it went (Traps, searching
  and hiding).  Tiring asked of a walk, a dig, a climb and a battering (Moving and tiring: strenuous work).
- **A long task** is anything the character does that takes more than a round (10 seconds): a walk, a dig, a climb, a
  search, battering a door, a wait; and the timed work, however short (below).  Its progress is a bar on the status
  line, ten cells, its part done of its length (a walk's squares, a dig's feet, a climb's feet, a wait's time, a
  battering's blows, a timed work's time), redrawn at most once a real second; and words as it goes (the user's:
  both).
- **The timed work** *(the user's playtest, 2026-10-22: "it would be better if the first half (for example "You study
  the mechanism a while") comes out first, followed by, as the task timer expires, the second part ... Makes it seem
  like there is actually a delay")*: whatever the character does that spends time and whose outcome is a test, or
  comes of the work, is a long task of its own kind: its first words at once, its bar its time, and its outcome told
  as its time's done, the test made then (the referee's, the same request made later) and the world changed then (a
  lock picked, a trap disarmed, a door forced open, a portcullis up).  It's interrupted as any long task is (hard,
  soft, the player's "stop"), and stopped first there's no outcome and nothing changed, the stop told ("You stop
  studying the mechanism."), the time spent spent.  What can't be tried is told at once, in no time: no lockpicks to
  hand (the referee's, asked as it's begun), a trap set by magic with no magic to undo it, a pit, no lock, a door open
  already.  As built (its first words / its outcomes):
  - a trap's mechanism made out, a minute: "You study the mechanism." / "You make out how it works: now to disarm
    it.", "You can't make out how it works.";
  - a trap disarmed, a minute: "You set to disarming the trap." / "You have it: the trap is disarmed.", "The trap
    defeats you, for now.", "Your hand slips: the trap goes off!";
  - a lock picked (a door's, a window's shutters', a chest's), a minute a try: "You set to picking the lock." / "The
    lock gives with a click.", "The lock defeats you, for now." (a chest's: "You pick the lock of the chest.", "You
    fail to pick the lock of the chest.");
  - forced (a door, shutters, a chest), 10 seconds, loud as it's begun: "You set to forcing the door." / "With a crack
    the door bursts open.", "The door holds." ("The chest holds: its lock doesn't give.");
  - bars bent (a portcullis's, a window's), 10 seconds, loud: "You set to bending the bars." / "The bars bend apart:
    there's room to get through.", "The bars won't give."; a portcullis heaved up from the far side, a round: "You set
    to heaving at the portcullis." / "The portcullis rises, and catches, raised.", "The portcullis won't rise."; wound
    up by its winch, 30 seconds, loud: "You set to winding the winch." / "The portcullis grinds up, and catches,
    raised." (stopped part way, it stays down);
  - a window's pane worked free, 30 seconds: "You set to working the pane free." / "The pane comes free of its frame,
    and you lift it out whole."; a window climbed through, a round: "You set to climbing through the window." / "You
    get up onto the sill, and through.", "You can't get up onto the sill.", "You slip from the sill, and fall." (a
    climb's: no stopping it part way);
  - the traces read: a track, a minute: "You cast about for tracks." / what it reads; a sniff, 3 seconds: "You sniff
    the air." / what it smells (too dark to see the floor, no nose: told at once);
  - a light filled, 20 seconds (a torch dipped, 10): "You set to filling the lantern." / "You fill the lantern from
    the flask of oil.";
  - a thing hidden, a minute: "You set to hiding the dagger." / "You hide it well enough." (cunningly, after a
    fashion);
  - battering (Portals): its first words at once, its beats as it goes, how it stands at its end.
  The outcomes worded to read alone after the first words.  Left as they were, told at once: what takes a few seconds
  and has no test (a door opened or closed, a bar lifted out or set in its brackets, a portcullis loosed, a lid, a
  tie, a look in a thing); a look one way and a listen (their sights and sounds told as they come); a search (a long
  task already, its finds at its end); a throw (its round: the throw's flight is the moment it's told); words to a
  denizen (its answer is the denizens', as it's said: a conversation answers at once); a close look at a thing (its
  appraisal).  (dq0w.c's door_work, work_done, the parts' wk.plan; the things' IO_DONE and the UI's reach, words.c's
  worked.)
- **A walk's words**: a beat at each change (the way turns, a side passage or a door is passed, it widens or opens
  into a room, the light changes, a sound is heard), and on a quiet stretch a short line every 6 squares (30 feet) of
  none ("You go on down the passage."; the user's: about every 30 feet).
- **Interruptions, hard** (the task stops at once, and says why): any wound taken (the user's: all wounds), a blow at
  the character, a trap sprung, a fall, its light going out, a hostile denizen coming into sight within 30 feet.
  **Soft** (it asks, "Something stirs. A giant rat is ahead, 40 feet away."  "Keep digging?"; yes carries on, anything else stops it, and a new
  command typed instead stops it and runs): something new seen, heard or smelled (a denizen not hostile, a hostile
  one farther off than 30 feet, a sound), a fatigue level more.  What the bodies' time brings besides: a new level of
  hunger or thirst soft; a new hurt, a sickness shown, bad air hard (their sections, below).
- **Stopping**: "stop" stops what can be stopped ("stop working" too, whatever the work is); "stop digging" (dipping, sharpening, freeing, battering, kicking, punching, climbing, searching,
  walking, waiting, and the timed work's: studying, disarming, picking, forcing, heaving, bending, winding, tracking,
  sniffing, filling, hiding) names it, and a name that isn't the task's is refused ("You aren't digging: you're walking."; none under way, "You aren't doing anything to stop.").  What's
  imposed can't be stopped: senseless, a fall under way.  A question's answered yes by "y", "yes", "go on", "carry
  on", "continue", or "keep" and anything ("keep at it", "keep kicking").
- **What stopping costs**: a walk stops where the character is; a dig keeps the feet dug (the change record: a hole
  half dug stays half dug), and the part of its spell done tires; battering keeps the harm done; a search cut short
  covers the squares nearest first as far as it went, each at the full chance, and a wait rests for the part done; a
  climb, disturbed hard, is a Climbing test to keep the grip (the plan's holding on), a fall from where it is if it
  fails; a timed work, nothing: no test made, no outcome, the time spent (a lock not picked, a mechanism not made
  out, a winch's portcullis still down).
- **Looking one way** ("look north", "look left", "look at the door"): a focused look that way, 3 seconds, twice as far
  as a glance about but 31 squares at most (155 feet: the senses' grid, 63 squares across; so a sight of 6 or more, a
  glance's 16 squares or more, looks less than twice as far: Open's sight's detail), as far as the light and the eyes
  allow (the light layer, distant lights, the dark's adapting);
  through what lets sight through (a doorway, a window, a grating, bars, a slit, a hole), what lies beyond.  Listening
  or sniffing one way ("listen at the door", "sniff at the grating") the same, through what lets sound or air through.
- **Things told as found**: whatever a test finds (a search, a trap noticed underfoot, a hidden door, a sound heard by
  listening) is told after what's plain at a glance, as its own line, saying how it was found: "You come to a
  closed door.", "Searching about...", "You find a trap: a gas trap, set to go off as its door opens."

## Light and traces *(slice 4.1: drafted 2026-10-21; reviewed by the user, 2026-10-22: as it is for now)*

Every ruleset's.  The book gives no burning times: these are the house's (the old design's hour for a torch).  Nor
any numbers for the traces (its Tracking and a notice test of smell are its tests): the house's too.  The section's last
part is here by its date: the 2026-10-21 playtest's things looked into and opened, a chest's lock, which of several, how
many, walking words.

The lights (lights burning down, renewed; the kit's spare; the status line's light left):

- **Every light lit burns down by the game clock, wherever it is**: carried, put down, set in a sconce; but not one
  its keepers tend (a kept room's sconce: the keepers renew it), nor one that never burns down (a glow, the theme's
  lights).  Its fuel left is the thing's own (minutes of burning), taken off a game minute at a time.
- **How long a whole one burns:** a torch an hour, whatever its make; a lantern six hours on a fill (a pint of oil);
  a candle four hours at middling make, of tallow, three-quarters of an inch thick: longer or shorter by its
  thickness (half an inch to an inch and a quarter, in proportion), half as long again of beeswax (a fine one,
  make 6 or better, is beeswax), and by its make, (15 + make) / 19 (a crude one four-fifths as long, a master's a
  quarter longer); candle stubs half an hour on the same terms.
- **Burning low:** its last ten minutes, told once for a light the character carries ("Your torch is burning low."),
  when no other light it carries lit has more left (two torches lit: told once, as the last of them burns low);
  nothing's asked.
- **Going out:** at none left, told ("Your torch gutters and goes out."), and for one lying where the character sees
  it ("The candle stubs gutter and go out."); a candle burns away, gone; a lantern's dry, a torch spent, kept to be
  renewed, and neither lights till it is ("The torch is burnt out.", "The lantern is out of oil.").  The character
  left in the dark by it (its only light, and no light of the room's): its long task stops hard ("...: you stop
  waiting."), but a climb part way and a fight go on; in a lit room, or with another light, nothing stops.
- **Renewing:** a lantern filled from a flask of oil ("fill the lantern", "refill the lantern"), as much as it has
  room for, the flask the less by it (20 seconds); a torch dipped in oil or pitch ("dip the torch in the oil", "soak
  the torch in pitch"), back to its hour, the oil the less by the minutes it gained (a minute's burning a minute's
  oil: 10 seconds); a candle takes nothing.  Without a fuel named, the first carried that the light takes.  A timed
  work (Where you are ...'s long tasks), the fuel moved at its end.
- **Fuels:** a flask of oil holds a pint, a lantern's fill (six hours' burning; a torch's hour six times over); a pot
  of pitch four hours' (a torch renewed four times).  Neither is in the maker's stock yet: the kit's flask, an occupation's outfit (a chandler's: outfits.txt), and
  dq0 -k's.
- **The kit** (D5, and the user's, 2026-11-03: by the money): Outfits, below, the null ruleset's too.  dq0 -K's as it was
  (a torch lit, a spare, a flask, its waterskin empty: the tests').
- **Found lights have some life left:** part of each used before it was left, 2 to 13 sixteenths of a whole one (by the
  thing), a sixteenth less for each century of its stratum's age; a candle has always some left (wax keeps); a
  lantern's oil has dried, or a torch's pitch crumbled, when that leaves none.
- **The status line** shows the minutes left of the light the character carries, the brightest's ("torch 40m",
  "lantern 5h59m"), when there's room for it beside the referee's words and the facing (and dq0 -m's minutes: the tests').
- **Far off, lights burn on:** a chunk's things paged out to the save folder are caught up as they're read in again,
  by the minutes since (the clock's 25.6 seconds a unit; after fourteen hours, all of it).

**The traces on the floor** (the user's, 2026-10-16: dust and dirt disturbed or not, by whom, how many, how recently;
rough stone shows little, earth and smooth stone more; swept floors hide it; smells fade faster than dust settles;
and 2026-10-21: keen noses follow a scent far longer than an hour, a hound's especially):

- **Every passing is kept**, the world's: each square the character walks off, each step an actor takes (a coarse
  move's too, out of sight), each coarse move of a denizen away from the character to the square it comes to: whose
  (its kind), how many passings, when the last went (in fours of minutes), and the way it went from there.  A square
  keeps its own records among the four of its bucket (a hash of its chunk and place, 2,048 records in all); a new
  one goes over the bucket's oldest, so traces are trodden out where the place is busy.  The store holds 32 chunks'
  (the nine round the character's always); one put out goes to its region's file in the save folder, read back as
  it's wanted, and the save keeps them all.
- **What the floor shows**, by the maker's material where each square is: **earth** holds a print well (6; soft soil
  7) for three days; **rough stone** little (2, and a point for each three of dust), a scrape lasting an hour;
  **flags and paving** nothing of themselves, their **dust** every mark (3 and a point for each of dust; none, 1):
  dust two deep or more keeps them three months, a skin a day.  The dust is the stratum's age's: a level for each of its
  first two centuries ("a skin of dust"; the user's, 2026-11-03: a place of no age has little or no dust; none but a stratum
  of none), 4 from three centuries, a level more at 6 and 12 (6 the most: "deep in dust").  **Swept** (a chunk that's a lair of a kind that keeps its
  floors swept, kinds.txt's t: the cultists and their priest, the dwarves but the miners, the soldiers and their captain, as
  their lair is placed): no dust at all ("swept
  clean"), and every hold a point less, the marks scattered.
- **How clearly a trail shows**: the floor's hold, a third of its kind's tread, a point more for 4 passings, another for
  12 (the scents' rule too, below: hub.h's TR_MORE); less a point for each quarter of the print's life gone, none at its
  end; a tread of 0 (a bat, a shade) leaves no print.  Each kind's tread, marks, smell, scent and nose are kinds.txt's
  (the character's own: its sweat's smell 3, its prints by what it wears on its feet, the user's, 2026-11-03, "build it":
  bare feet 2, sandals or shoes 3, knee boots 4, hobnailed boots 6, *the house's numbers, for review: a third of a tread
  shows, so bare feet leave the fainter prints and hobnails the clearer (5 would show no clearer than boots)*; as it's
  shod when they're read, not when they were made, as the words have it: wtrword.c's feet).
- **Examine the floor** ("look at the ground"): sight alone, no test, 5 seconds, the floor where it stands and the
  squares two each way: its material, its dust, and "It's been trodden" if any trail there shows 4 or more.  In the
  dark it can't be done.
- **Track** ("look for tracks", "follow the tracks"): the Tracking test, a minute's study of the same squares, at steps
  the clearest trail's clearness less 4 (-8 to +6).  A critical success tells all (how many: one set, a few to 3, several
  to 11, many; how long since: only just under eight minutes, recently under an hour, some hours under ten, a day or so
  under two days, long ago; the way it led, by the facing; whose, "plainly"); a marginal success the same, "by the look
  of them"; a marginal failure only that the floor's been trodden; a critical failure a misreading, told as if it were
  read: the way reversed, the time two steps off.  Three trails at most, the clearest first.  A timed work ("You cast
  about for tracks.", the reading at its minute's end: the long tasks').
- **A scent fades by its reader's nose**: a scent of strength s (kinds.txt's smell, and its passings' and stays' points:
  below) read by a nose n (kinds.txt's nose; the character's a quarter of its smell sense, the referee's: 1 for most)
  lasts 2^(s + n - 2) fours of minutes, its strength s + n - 1 losing a point each time its age doubles, and half a
  point a square away.  So a hound's nose (9) follows the character's scent (3) some three days old, a rat's or a
  ghoul's (7) the most of a day, a bear's (8) a day and a half; the character (1) smells a rat's (4) for half an hour,
  a bear's (8) for eight hours.
- **Smell** ("sniff", "smell the air", "sniff at the grating"): a notice test of smell, 3 seconds, at steps twice the
  strongest scent less 6; about the character it reaches two squares each of the eight ways, one way six that way and
  three the two ways beside it, through what the air passes (an open or broken door, a portcullis, a grating, an arrow
  slit, a window open with no glass), not rock, a wall, a shut door.  A success tells the two strongest scents and
  whose ("by the smell of it"); a marginal failure the scent but not whose ("something passed not long ago"); a
  critical failure nothing.  A scent told hangs here fresh (under an hour old), faint (ten hours) or lingers stale, but
  one of strength 6 or more as it's smelt hangs heavy ("A musky smell hangs heavy here: giant rats, by the smell of
  it.": a lair, a bear just gone by).  The character doesn't smell its own sweat.
  A timed work ("You sniff the air.", the smell told at its 3 seconds' end: the long tasks'), the draught told first
  (Sound, the air and moving quietly, below).
- **Smoke**: a flame the character carries leaves smoke on the squares it walks (a torch's strength 5, a lantern's or
  a candle's 2, by its name); a light lying lit (a sconce's torch, a fire) smokes 4 where the air reaches; a light of
  its own lit, the character smells no other smoke.  Smoke fades as a scent does, with no nose to read it longer.
- **Denizens by smell**: a kind with a keen nose (6 or more: giant rats, ghouls, cave bears, cave adders, lindworms, small orcs)
  that's hostile and doesn't sense the character follows its trail: its last 64 squares (the world's journal) as long
  as its nose can still smell them, then the store's older trail (sniffed once each ten seconds, at once again while
  it leads on).  Any kind with a nose smells the character in still air within a third of its nose (squares: a
  hound's 3, a rat's 2, a man's none), along a line the air passes: sensed, as seen or heard; and the further as the
  character stays where it is (its stay's points, below, added to the nose: a rat's 3 squares once the character's been
  on its square some three quarters of an hour, a kobold's 2 after ten minutes or so, a man's 1).
- **Strengthened by passing and by staying** (the user's, 2026-10-22: "repeated passage or occupancy should enhance
  prior smells"; built 2026-10-22, these numbers the user's as they are for now, 2026-10-22): a scent is the stronger by the
  passings over its square and the stays on it, as a print's clearness is by its passings: a point more at 4, another
  at 12, each point doubling how long it lasts.  So the character smells a rat that passed once for half an hour, one
  that passed four times for an hour, twelve times for two; a rat follows the character's scent the most of a day
  after one passing, nearly three days after twelve.  A stay counts a passing for each four minutes spent on the
  square: the character's (waiting, resting, searching, digging, fighting where it stands: its time there, from the
  step that brought it to the one that takes it off, counted as it leaves; the square it begins on, none), a
  denizen's standing, resting or fighting where it is (each four minutes as they go by), and a lair's keepers' where
  they live (each keeper a passing at the lair's home as the lair is first placed, and the most stays there are: so
  a lair smells strongly of its keepers from the first, and their comings and goings about it keep it fresh).  One
  record a square and kind, its counts rising (255 the most), so a busy square or a long wait floods nothing.  Smoke
  the same (a torch kept lit where the character waits the most of an hour leaves smoke there four hours, not one).
  A stay makes no prints: the Tracking test's how many and how clearly go by the passings alone.
- **Still to come**: the air layer's rest: a denizen downwind smelling the character from far off is built (dq0p/sense.c:
  Sound, the air and moving quietly, below), but the character's own sniff reaching further upwind and a trail followed
  downwind aren't (wtrace.c's sniff, the trail's way, wtrscent.c); and rain and running water washing scents and prints
  away faster (wtrace.c's faded, hub.h's SCENT_LIFE: the one place it comes in).

**Kept light** (DUNGEON.md's light kept; the user's, 2026-10-16: lights are somebody's):

- **Who keeps lights:** the kinds with kinds.txt's f (kobolds, the kobold lord, bandits, the bandit lord, cultists,
  the high priest, the dwarves, the elves, the soldiers and their captain).  The dead, the beasts, the vermin and the things that see in the dark keep none.
- **How far:** a lair keeps lit every room whose sconce stands within its kind's range (kinds.txt's: a kobold's 150
  feet, 30 squares) either way of the middle of the lair's chunk (its home isn't known till it's placed).  Passages
  have no sconces, so they stay dark.
- **Sconces:** the style's old odds of a room lit by its own light are now the odds of a room built with sconces:
  built 5 in 8, mixed 4, mine 2 (a natural stratum's caves have none), one fewer each stratum down; and every room its
  keepers keep lit has one, their own.  A glowing cave chamber never has one (its light is nobody's).  One sconce a
  room, standing for its sconces, on a square of its floor beside a wall.  An unkept room's holds a cold torch, part
  burnt by its stratum's age as a torch found lying is, but never to nothing (its last sixteenth at least: 2026-11-03), three times in four; else nothing (burnt away, or taken, long ago).
- **The sconce:** iron, a pound and a half, worth 4 pennies, fixed to the wall (never taken up); it holds 3 pints
  and bears 2 pounds (a torch, a candle; not a lantern); made out 12 squares past a speck.
- **A lit thing in a sconce lights its room** throughout, as a room's own light does (level 6), whoever lit it: its
  keepers, or the character lighting the cold torch where it is or putting a lit one in.  Put out, taken, or burnt
  away, the room is dark again.  It is never a lamp of its own besides.  (The design's first thought was that a torch
  the character lights in a sconce lights only round it, as a torch put down does; one item stands for the room's
  sconces, so it lights the room as the keepers' does, the same rule for everyone.)
- **Tended:** a kept room's torch is tended (renewed by its keepers: it never burns down while they live).  Taken
  from its sconce it is the character's, an ordinary torch (the item store's moves end its tending: the lights').
- **Keepers gone:** as a lair's last keeper dies, its torches are tended no longer and burn down from then; seen to
  each game minute round the character, so a torch whose chunk was away from the character begins to burn as it comes
  near again.  Keepers that leave a stratum: none do yet.
- **Taken from a fixture** *(the user's, 2026-11-03: DECISIONS.md's)*: "take the torch from the sconce", or "take the torch" for
  that one: to a free hand, the pack only if both hands are full (a second, no pack off); any thing taken from a fixture alike
  (s_carry's, part 48's HF_FIXT).
- **Commands:** take the torch from the sconce, light the torch in the sconce, put out the torch in the sconce (each
  walked over to first, if it's out of reach); put the torch in the sconce (within reach: "go over to it first").
- **The same dungeon each game:** a stratum's populations are decided before its first chunk is made (the world waits
  for them, ten seconds at most), so the same seed stocks the same sconces and lights the same rooms.

What the user's playtest's faults made (2026-10-21: things looked into and opened, things of several and how many, walking
words):

- **Looking into a thing:** "look in", "into", "inside" a thing, "look on" one (VB_OBSERVE_IN): what's in a container,
  on a surface, hanging from a belt, alike told together ("In the sack lie a candle and two chisels.", "On the table
  lie two chisels."); empty ("The sack is empty.", "There's nothing on the table."); closed ("The chest is closed.");
  a well, its shaft ("The well's shaft drops away into the dark."); a thing that holds nothing ("You can't see into
  the sword."); carried, or lying within reach and seen (out of reach, walked over to first); 3 seconds.  Into a way
  ("look into the passage") a look that way, as "look through" is.
- **Looking at a thing** ("look at the table"): what anyone sees of it, no test, its appraisal taken out (the user's,
  2026-10-22: Looking at things, below).  At a denizen, a look about as before; at a way, a look that way, as before.
- **What closes:** a lid (a chest, an iron pot, a pot of pitch, a backpack's flap), a tie (a sack, a pouch), a stopper
  (a waterskin, a flask of oil); a basket, a quiver, nothing.  Found closed; the character's kit open, but the
  waterskin and the flask, stoppered.  Opened or closed in 2 seconds ("You open the chest.", "You untie the sack.",
  "You unstopper the flask of oil.").  Closed: nothing put in it or taken out, nor out of anything in it ("The sack is
  closed."); what's in it not seen, nor listed in what's carried ("a sack (closed)"), but weighed with it.  A lantern
  filled from a stoppered flask: unstoppered as it's poured and stoppered again, nothing asked (the fill's own time).
- **Locks:** a chest has one (its lid's); a found chest is locked one time in three (by its number, no dice).  Picked
  with lockpicks to hand (P_PICKLOCK, as a door's lock: a minute a try, 1 step harder and 1 more for every 3 of its
  make; no picks, refused), or forced (P_FORCE: a wooden one at no steps, another 2 harder, and 1 more for every 3 of
  its make; 10 seconds, loud, a noise of 6 the denizens hear): its lock burst, it's open and never locks again; held,
  "The chest holds".  Picking and forcing each a timed work (the long tasks'), the test made at its end.  Forcing one
  that isn't locked opens it.  Locked and unlocked with its own key (Keys that fit, below); no key: "The chest has a lock, but you have no key to
  it."
- **A thing of several** (the one a command means, the world's to choose, never asked which; a throw's mark, a target set
  and a thing gone to are asked: Which of several, Commands): the one most to hand:
  held, then worn or hanging from the belt, then in what's carried (the shallower first), then lying within reach
  (the nearer first, a surface's too), then further off.  A command naming two of a name takes different ones: the
  other never the first, nor one in it ("put the sack in the sack").  The ordinals count that order, for both things
  ("put the first sack in the second sack"; "the second" of them all, so it may be the first itself: refused, it won't
  go inside itself).  "The other": one not the first; alone, the second.  By state: lit, unlit ("burning" is lit),
  open, closed ("shut"), empty.  By where ("the sack in my hand"): not built: the grammar reads "in", "on" as the
  other thing's.  Among those lying about in sight the UI's choosing (closest, largest) is as before; an ordinal or a
  state among things alone is the world's.
- **How many:** a number ("drop two sacks", "take 4 rations", "two of the sacks", "both sacks"), all ("drop all the
  sacks"), half ("drop half the rations": half of all there are, at least one): each thing moved in turn, each its
  own record; a lot counted (rations, pennies, arrows) split for part of it, the part a lot of its own (its make the
  lot's), and a lot moved joined to one alike where it goes (the same kind, the same place, held or worn alike,
  neither named: "put back with others alike").  Fewer than asked: as many as there are, told ("There are only 2
  rations.", "You have only a sack."); out of reach, walked over to first, then as many as are within reach.  Words by
  the count ("You take 4 rations.", "You put down two sacks.", "You put the 2 rations in the sack."); the time each
  handling's own, added (taking up and putting down: as one's).  A counted lot weighs each one's times its count (a
  day's ration a pound: the kit's three, 3 lb, where they were 1 lb all told).
- **Alike told together:** in what's carried ("two sacks"; but each thing in a hand, and anything with something in
  it, told each), in a look in, and on a surface ("On the table lie two chisels."); things on one square in one
  sentence ("At your feet lie a table, a sack and a chest."), those too far off to make out each on its own.
- **Crossing a room on a walk:** a walk across a room to a door or a passage's mouth (n, s ... from a room) tells the
  crossing as it leaves the room ("You cross the room.", after two squares or more of it), then the door's beat
  ("You go through a doorway.").
- **A wide passage is one passage:** a square beside the way that's only the passage's breadth (it and the square
  walked going on side by side, nothing out of it the square walked hasn't, nothing past it: two squares across, or
  three) is no way on, for a walk and for the look
  ("You are in a passage.", not "The passage forks here."); the openings into one room beside a passage are one way
  into it (a walk along a cave's side); a walk set out from a fork (a wide one's squares, each a fork of its own)
  goes three squares the way said before a fork stops it again.

## Looking at things, and what opening shows *(2026-10-22: the aspects' words the user's, as asked, DECISIONS.md's; the times the house's)*

Every ruleset's.  The user's, 2026-10-22: "When examining an item, I want to know about its state, and for containers
if they are open or closed, and what is on/in them.  `look [at] item` instead of examine should give these things.
Examine gives too many unimportant details (like value)"; the aspects asked by name, as the user chose them; and opening
something tells what it reveals.

- **Looking at a thing** ("look at the chest", "look the torch over"; LM_AT, the world's IO_LOOK): a second, no test.
  What it is and where: "Your torch is in your left hand.", "The chest is beside you.", "Your dagger is on your belt.", "The
  torch is in the sconce.", "The sword is some 20 feet off."; its state, as anyone sees it: lit or unlit, burning low,
  burnt out (a lantern out of oil); open or closed, tied shut or untied, stoppered or unstoppered; a fuel empty;
  battered, nearly broken, dulled, dull, worn; with a lock (its lock broken, once forced); a container or a surface
  empty, bare ("It's lit, burning low.", "It's closed, with a lock.", "It's untied, empty."); and what's in it or on
  it, within reach, look in's words, alike told together ("In it lie a tinderbox, a bar and two torches.").  Whether a
  lock is locked doesn't show: trying it tells.  Too far off to make out (1 + sight / 2 + its kind's seen squares, as
  a glance's), only its size ("Something small is some 40 feet off."); more than a square off, nothing in
  it.  Of several of a name, the one most to hand (A thing of several, above).  A thing of a kind with no words of its
  state says where it is only ("The bones are beside you.").
- **Examining a thing** ("examine the sword", "x", "inspect", "check", "study", "look closely at"; LM_CLOSE): a closer
  look, three seconds, no test, no value nor quality: what looking at it tells, and what a closer look shows besides:
  what it's made of, how thick (a candle's), its wear (as good as new, little worn, well worn, battered, close to
  breaking: its durability left, in tenths, 10, 7, 4, 2 and less), a blade's edge dulled or dull, how much of a light
  or a fuel is left (a torch's or a candle's hardly burnt ... half burnt down ... burnt out; a lantern's full of oil ...
  out of oil; a flask's full ... empty: by quarters of a whole one's life, a candle's of its own, its thickness and its wax), and a lock's make ("Its lock is a simple
  one.": a chest's 1 + make / 3 steps to pick, 2 or less simple, 3 sound, 4 good).  ("It's made of wood; it's little
  worn, half burnt down.")  Out of reach, walked over to first (this and the aspects below).  A portal examined
  (a door, a window, a portcullis) tells its plain features as before (its kind, size, material, thickness, a grille,
  a spy-hole, studs, its hinges, a bar's brackets, its lock's make: simple 4 steps or less, sound to 10, intricate),
  and the blows' marks, but no judgement of its state: that's its quality, asked for.
- **The aspects asked for** (the user's choices, 2026-10-22), each a word of the command's (commands.words' [aspects],
  the verb's own: struct cmd's aux, EX_*):
  - **Value:** `assess X`, "value X", "price X", "what is X worth", "value of X", "assess / determine / examine
    the value (the worth, the price) of X": the referee's appraisal test as before (its craft's), its value alone ("You
    judge it worth about 115 pennies."), no time.
  - **Quality** (how well made): `judge X`, "how well made is X", "quality (make) of X", "judge the quality
    of X": the same test, its make alone ("You judge it a fine chest."); a portal's, its state and how stout it is
    judged, as the old examine did ("You judge it rotting, and stout: ...").
  - **Both:** `appraise X`: the whole appraisal as it was (its make, its material and what it stands up to, its size,
    its life left, its worth).
  - **Weight:** `heft X`, "weigh X", "weight of X": in hand, or lifted where it lies (out of reach, walked over to
    first), a second, no test: "The chest weighs about 30 pounds, with what's in it.", less than a pound, about a pound,
    else to the nearest pound; a thing fixed where it stands, not lifted; a portal, part of the place.
  - **A lock:** "examine the lock of the chest", "study the lock of the door", "examine the lock" (nothing named: the
    last thing named, else the door in front): its make and how hard it looks, a second, no test (the picking is the
    test): "The chest has a simple lock: easy to pick, by the look of it.", "... a sound lock: a fair test of a
    lockpick.", "... a good lock: a hard one to pick."; broken, once forced; none.
  - **Traps:** "check the chest for traps", "examine the door for traps", "study the door for traps": the
    search's test on that one thing, ten seconds (a search's minute is the 49 squares about it; one
    thing, a sixth of it).  A door (or a window, a portcullis), walked up to: its trap, if it has one hidden, found by
    Awareness at its hiding's steps worse (by magic, never: told as none); found, told as a search's find ("You find a
    trap: a gas trap, set to go off as its door opens."); known already, said so; none, or missed, "You go over it
    carefully, and find no trap."  A thing (a chest), within reach (walked over to first): things have no traps yet,
    so none is found ("The chest shows no sign of a trap: you go over it carefully, and find none.").  The search's
    own forms are the search's (Walking and searching in the dark, below): "search the chest for traps" is this in the
    light; "search for traps", a search about it.  (wtrap.c's PA_CHECK; things.c's LM_TRAPS.)
  - **Condition:** "check its condition", "examine the condition (the damage) of X", "condition of X": its wear and
    damage alone, the closer look's words ("Your sword is battered, its edge dulled.", "Your torch is half burnt down."),
    a second, no test; a portal's, the blows' marks ("It's splintered with blows.", "It's been broken down.", "It shows
    no damage.").
  - The general form "examine / determine / assess / judge / check / study the ASPECT of X" for every aspect, the
    aspect said overriding the verb's own ("assess the quality of the sword" is its quality): a later aspect is a word
    in commands.words' [aspects] and an EX_ bit.  "Its" and nothing named: the last thing named ("check its
    condition"; the resolver's).  A denizen judged ("judge the goblin"): how it seems, a look at it, as an examine of a
    denizen was.  "Sense a lie from the goblin" is the old "judge" (Diplomacy), not yet built.
- **What opening shows** (the user's: "if I open a chest, I should get a 'You open the chest, revealing a pile of gold
  coins'"): told in the open's own sentence.  A thing that closes: its lid lifted, its tie untied, its lock forced open:
  what's in it, alike told together ("You open the chest, revealing a candle and two chisels.", "You untie the sack,
  revealing 3 rations.", "You force the chest, and its lock bursts open, revealing ..."); nothing in it, ": it's empty."; a
  stopper drawn, how full it is, by quarters (a flask of oil: ": it's half full."; a waterskin, what's in it).  A door
  opened, unbarred, forced or battered down, a window's shutters opened, burst or smashed, a portcullis raised or
  heaved up: what the look after it sees beyond it, the look one way's words ("You open the door, revealing a passage,
  30 feet of it.", "..., revealing a kitchen, lit by torches.", "..., revealing only darkness beyond."): from the
  portal on, the way through it, along what the eye sees past, straight on through the dark, round a bend only where
  it sees the square, a room seen (not its own) ending it; as far as a look one way reaches (31 squares).  The look
  after the open (the senses', its view changed) tells nothing of it again.  An opening that sets off a trap, or hurts
  the one battering, is told at once as before, nothing revealed.  A walk's own opening of a door it knows ("You open
  the door." as it goes) is a walk's beat, nothing revealed: the walk goes through.  (things.c's reveal; the world's
  tail and revealed, dq0w.c; fixt.c's shows.)

## Walking and searching in the dark *(2026-10-22: drafted; the user's as it is for now, the same day)*

Every ruleset's (the stumble's roll and the harm the referee's).  The user's playtest (DECISIONS.md's walking and
searching in the dark): no walk turned toward an opening the character can't see; a cautious half-pace in the dark,
anything faster said outright and dangerous; a search in the dark by touch, nearby; the search's forms.  The numbers
are drafts, the user's as they are for now (2026-10-22).

- **Feeling your way tells only what the hands and feet could tell** *(the user's, 2026-11-02: DECISIONS.md; as built, for
  review)*: every look in the dark, and every dark walk's end: "It is dark. You feel a wall ahead and to your left;
  nothing to your right."  Touch within arm's reach, ahead and to each side (a wall, a door, iron bars); "nothing" only
  means nothing within reach, never a way.  Underfoot what its feet have felt ("The way rises."), as it walks.  No name
  for the place, no ways told, no slope ahead: "The way turns left.", not "The passage turns left."; a straight stretch
  quiet but for "You feel your way on.", but what changes either side as it goes (its hands on the wall as it walks, a
  beat, WB_SIDES): "The wall to your left ends.", "A wall begins to your right.", "You feel a door to your left."  Where
  it has been before and hasn't forgotten, memory adds: "This should be the kitchen.", "You've been along this
  passage."  A blind character the same, "You can't see." for "It is dark."
- **Nothing's read in the dark** *(2026-11-02; as built)*: "You can't see to read.", in the dark or blind,
  a tablet's cut letters too (the user, 2026-11-03: "I wouldn't worry about reading tablets in the dark"), before any walk over.
  In the light anyone reads anything, no skill tested: *left open for now, the user's, 2026-11-03 (A20)*; clues taken to a library or a scholar, each perhaps in a script of its own, are for the plan for the game between dungeons (Decided, not yet built, below).
- **In the dark** is where the character sees nothing by: its own square unseen (no light of its own, no room's own
  light, no lamp lying near lighting it), or blind (the referee's summary, C_BLINDED: the one place a blindness, however
  it comes, will take it).  (dq0w.c's dark.)
- **The walk goes by what it knows** (the squares seen now or before, and those walked in the dark, felt: the senses'
  T_SEEN, the map's): from a room, by a door or a passage's mouth that way only if it knows of it, else straight across
  the way said till something stops it; along a passage, straight on, stopped by a way off it it knows of (a fork it
  knows), round a bend only where it knows the way on, else meeting the wall ahead.  It never turns toward an opening
  it hasn't seen or felt.  A walk to a place or a thing it names, or to a door it knows of, goes there as in the light
  (the way's the room's; what's in it found as it's met).  (dq0w.c's walk, go; the senses' felt goes the same way.)
- **Its pace is a cautious walk** (the slow walk's: twice a walk's time a square, half its speed), feeling ahead, arms
  out (or what it holds probing), whatever pace was set in the light; the status line shows it "by feel".  It finds
  what stops it before it blunders into it, and stops there, told by touch: "Your hands meet a wall.", "You feel a door
  before you.", "Your hands meet iron bars.", "The floor falls away before you." (a drop, a ledge down; a pit's edge),
  "The floor rises steeply before you." (a ledge up).  A thing in its way that it doesn't know of, of 20 pints' bulk or
  more (a chair, a backpack, a table, a chest, a body, a well's curb), stops it a square short: "Your foot finds
  something on the floor: a chest."; known from then on, it's gone round as in the light.  Smaller things are stepped
  over unremarked.  "You feel your way north." as it sets out.  (Parts 43, 44, 45: dark_way, dark_end, ahead.)
- **Faster, said outright:** a pace said in the dark faster than the cautious walk (a jog, a run, a sprint: "run
  north", "set pace to run"; "walk quickly", "walk normally": a walk) goes at that pace there, till a walk begins in
  the light (a pace set in the light doesn't carry into the dark: there the walk's cautious again).  "You run north into
  the dark.", "blind" on the status line.  Its dangers, by what's really there (the world's squares, the item store's
  things, the traps' records), the greater the faster:
  - **a thing in the way** (20 pints or more, not known of): the book's stumble roll (Skills 22: the referee's
    P_STUMBLE; HarnMaster's 3d6, the physical penalty and the circumstances against Agility, two steps a point), its
    steps the darkness's 4 worse, a light obstruction's 2 (under 60 pints: a chair, a backpack), a heavy one's 4 (60
    or more: a table, a chest, a body), a run's 2 more, a sprint's 4: so the book's darkness +2, heavy obstruction +2,
    running +1.  Kept its feet: "You stumble over something on the floor (a table), but keep your feet.", and on.
    Fallen: "You trip over something on the floor (a table), and go sprawling; you pick yourself up.": the walk
    stopped short of it, a round lost rising.  The book gives a stumble no harm, and nor does the game.  The thing's
    known after, either way.
  - **a wall, a door, bars, a rise run into** (where the walk ends against it): a walk's a bump, no harm ("You walk
    into a wall."); a jog's a blunt blow of a die, a run's two, a sprint's three, high on the body (HC_STRIKE's high
    zone, the book's: the head, the shoulders or the arms, and the armour there counts), "You run headlong into a wall!"; a door or bars a thump the denizens hear (as loud
    as 2 and the pace's number: a walk 4, a jog 5, a run 6, a sprint 7).
  - **an edge** (a drop, a pit's): a dodge (the referee's P_DODGE_HAZARD) to stop short, a walk's 4 steps the easier, a
    jog's at no steps, a run's 4 the harder, a sprint's 8: "You stop short, teetering, at the edge of a drop."; failed,
    over it: "You blunder over the edge of a drop!", a fall of its height (not a clear fall: no landing on its feet),
    or into the pit ("The floor drops away under your feet: a pit!"), a fall of its depth (a spiked one's spikes too,
    as its trap's) and the climb out, as a pit's.
  A small move ("step forward", "move north 2 squares") in the dark is the same walk, unannounced.  (Parts 43, 44.)
- **Every actor:** a denizen's kind with no darkvision (kinds.txt's sight, its second figure 0), stepping onto a square
  that's dark (not seen lit by the character, nor in a room's own light), feels its way, its step twice its time.  All
  of today's kinds see a little with no light (10 feet at the least: bandits, cultists), so none is slowed yet; a theme's
  kind may be.  (dq0d.c's STEP_FEELS, wbody.c.)
- **Looking in the dark is feeling about** *(the user's, 2026-11-03: DECISIONS.md's)*: in the dark, or blind, "look", "look around"
  and their like are "feel around": what its hands and feet tell at once (the look's words as they were, what it sees of any
  light far off with them), then the search nearby, its minute, its find; what it finds can be taken next.  In the light they
  stay as they are.  *(The house's, for review)*: "look for the key" the same way is "feel around for the key"; as the
  character's busy, or in shock (which can't search), a look is only the look, no time.  (main.c's chores, search.c's.)
- **Searching in the dark is by touch** (SF_TOUCH): "search" there searches nearby, its own square and the eight round
  it (a reach of 3, a square's distance off squared less than it), whether it sees them or not.  Everything lying there
  is felt: what's in plain sight to the hands found and known with no test ("Your hands find a sack, and a chisel."),
  what's hidden found by the search's test (the referee's P_SEARCH, at its hiding's steps worse) 2 steps the harder (the
  eye not there to help the hands: a draft), and the same for the hidden doors in the walls within reach and the traps
  there ("Your fingers find a wire stretched across the floor: a dart trap, set off by a tripwire.", "Your fingers find
  the edge of a cover in the floor: a covered pit."); nothing: "You fumble around in the dark and find nothing."  Its
  time a minute, as a search's, its reach growing as it goes (cut short, the nearest first).  Searching as it goes in
  the dark searches by touch where it stops.  (wtrap.c's search, things.c's part 46.)
- **The search's forms** (light or dark; search.c, its words as it begins: "You feel along the east wall in the
  dark..."):
  - **"search"**: in the light, about the character as far as it sees within 3 (as before); in the dark, nearby.
  - **"search nearby", "search around", "feel around", "grope about"**: within reach (a reach of 3), seen or felt.
  - **"search the east wall", "search the wall ahead", "search the wall to my left", "search the left wall"**: that
    side's walls and doors (the squares past the character that way that are wall, rock, a door or a window) within
    the search's reach (3 in the light, as far as it sees; by touch, those within reach: none there, "Your hands find no
    wall within reach that way.", no search), for what's hidden in them: a hidden door, a door's trap; 2 steps the
    easier (the whole search on one wall: a draft).  No floor's traps.
  - **"search the floor", "search around on the floor"**: the floor nearby: the things lying there and the traps in the
    floor (plates, wires, pits), no doors.
  - **"search for the key", "search around for a dagger", "feel about for the key"**: that thing, the nearest of the
    name it would find within the search's reach (in plain sight; or hidden, and found by the referee's test, made as
    it begins): the search lasts till it comes to it (3 seconds, and 3.2 more for each of its distance off, squared: at
    its feet 3 seconds, beside it 6.2, a corner 9.4, two squares off 15.8), "You find a dagger.", "Your hands find an
    iron key."; none, a minute, "You find no dagger here."  Cut short before it comes to it, not found.
  - **"search the table", "search on the table", "search the chest", "search the straw", "search the body"**: a thing
    within reach (in the light, out of reach, walked over to first; in the dark, one within reach, felt: "You feel about
    you, but there's no table within reach."), a minute, for what's in it or under it, never traps unless they're asked
    for (the user's playtest, 2026-11-02: "it says 'no sign of a trap', but I am not looking for traps, but for anything
    hidden in the straw").  Worded as it begins by what it is (the catalogue's, no names): a heap, "You poke through the
    straw..."; a container or a surface, "You search the chest..."; a body, "You go through the body..."; anything else,
    "You look the dagger over...".  What came of it:
    - **A heap** (a kind that's no container nor surface but has a size it hides, its cap_bulk: straw, sand, a blanket,
      furs, potsherds, a cloak lying): what's hidden on its square is in it, each by the search's test at its
      hiding's steps, **2 steps the easier** (all the search on it: *for review*): "You find an iron key in the straw.",
      "You find nothing in the straw.", "Your hands find nothing in the straw."
    - **A container or a surface**: "On the table you find two chisels.", "Your hands find two chisels on the table.",
      "There's nothing in the chest."; closed, not looked into: "It's closed: you go over the outside of it."  A body:
      what's worn beneath its clothes found (as before).
    - **Anything else**: "You find nothing hidden about the dagger."
  - **"search the chest for traps"**: in the light, examine's check alone (ten seconds: Looking at things, above); in
    the dark, the search on it by touch, and its trap's words after what it found (things have no traps yet: "The chest
    shows no sign of a trap ...").  **"search the door"**, "search the door for traps": the door gone over for
    traps (the world's PA_CHECK, examine's).  **"search for traps", "search for hidden things"**: a search, as "search"
    is.
  - **"search the room"** (the chamber, the hall, the cave, the cell, the vault): the whole of it: below.  In a passage, a
    search about it, as "search" is.  "search" alone stays the quick minute about the character: a look round from where
    it stands, for one who has no time for more (the user's words asked for the whole room's search under its own name).
- **A room searched whole** *(the user's playtest, 2026-11-02: every number the house's, for review)*: a long task
  (LT_SEARCH: its bar its whole time; stopped, asked and interrupted as any; "stop searching"), the character on each
  square in its turn at a slow walk at most (as one searching as it goes: a hidden trap underfoot noticed at the search's
  chance, else sprung, and the search stops there), so the denizens see it there; told as it goes, its finds told as
  each is found.  (dq0w's wroom.inc, wroom.c; things.c's part 46; search.c's words.)
  - **Its stops:** the floor cut into blocks of 3 squares by 3 from its northwest corner (the last of a row or a column
    what's left), a stop each: the ring of blocks along the walls first, clockwise from the corner nearest it (so a wall
    on its left hand, "You work along the wall to your left.", "You turn the corner, and work along the east wall."), then
    the rings inside them ("You work back and forth over the floor in the middle of the room."); a room of one
    block, "You go over the walls and the floor about you."  A block it can't reach
    any of (a cave's rock in the room's rectangle) isn't walked to; what's there is tested all the same.
  - **At a stop, each thing lying in the block**, by its time: a heap poked through **20 seconds** ("You poke through
    the straw.", then "You find a silver ring in the straw." or "You find nothing in the straw."); a body gone through
    **30 seconds**; an open container **10 seconds** ("You go through the sack.", what's in it); anything else, a surface
    or a closed container too, looked over **5 seconds** ("You look over the iron pot.", "It's closed: you go over the
    outside of it.": its lid not lifted, not forced; a closer look, examine's, no appraisal: what it's made of, its wear, a
    lock's make; what's on a surface).  Each told with what's in it, as its time's up.
  - **Then the block itself, 10 seconds** (the search's own rate: a minute the 49 squares about it, a block's 9 a sixth of
    that): its tests as its time's up: each hidden trap in the block's squares, each hidden door and door's trap in
    the walls beside them, and each thing hidden there (a second chance at what's in a heap), by the referee's
    P_SEARCH at the steps a search's are, told as found.  A hidden door found in a wall it's working along is a door from
    then on.
  - **By touch** (its square dark, or the character blind, as it comes to each stop): "You feel along the wall to your
    left.", "You feel through the straw.", "Your hands find ..."; each thing's time twice, the block's **30 seconds**, the
    tests 2 steps the harder, as a search by touch's.  Its pace the dark's, feeling its way.
  - **Stopped part way, what's searched stays searched**: the stops done are kept (a change record on the wall square at
    the room's northwest corner, the DUG kind's, its tile the count, its elev the corner it set out from: one record a
    room, whatever its size; in the save with the rest), a stop not finished not counted.  "search the room" again goes
    on from there ("You go on searching the room."), the same way round; all done, it says so at once, no time ("You've
    been all over this room already: search about you, a wall, or a thing, for a closer look.").  What's been missed
    there is missed: the other forms are for a second try.
  - **A great cavern** (a room across the chunks, as much of it as it's seen vast) is refused kindly: "This cavern is too
    vast to go over from end to end: search about you, or search for traps as you go."
  - **Every actor's rule where it applies:** a denizen's search isn't asked for yet; the same stops would serve it, wk.who
    its own.
- **What the maker hides in heaps** (the same dungeon for the same seed but for these): one heap of straw in four of those
  a room's former use left, by its square, no die, has one of the style's hidden kinds in it (hiding 1: found by a
  search of it, or of the room).  A lair's beds were so already: a denizen's stash lies hidden at its bed, in its straw
  (outfits.txt's @stash and @bed).

## Hands and limbs *(slice 4.1b, drafted 2026-10-22; reviewed by the user, 2026-10-23: they stand)*

The user's (DECISIONS.md's hands and limbs): two hands, handedness from the character's profile, the off hand's
penalty unless ambidextrous, the book's fumbles for arm wounds dropping what that hand holds, a leg's wounds slowing
the walk and its stumbles laying it prone, a limb made useless.  The book's own rules are the referee's: the medical
traits (Character 9: left-handed, ambidextrous, an arm missing or deformed), the one-handed penalties and the off
hand's (Combat 3), the fumble and stumble rolls (Skills 21-22) and a grievous limb wound (Combat 14), the Rise
(Combat 8).  What the house adds:

- **Each thing held is in the left hand, the right, or both** *(every ruleset's)*: the item's own (its sn, the
  hand: items.h's IH_*).  A thing taken up goes to the hand that suits it: a weapon or a tool the main hand, a shield
  (its key "shield") or a light the other, a two-handed thing (the things' rule: two-handed, or 20 pints or more) both;
  the other hand if that one's taken or useless; none free, the pack, else refused ("Your hands are full: stow
  something, or put it down.").  The kit's torch in the off hand.  Said outright: "hold the sword in my left hand",
  "take the torch in my right hand", "put the dagger in my other hand", "hold the spear in both hands" (any thing
  that isn't two-handed may be held in both: the book has no bonus for it, and none's taken), "in my off hand", "in
  my main hand"; a thing held already moved to it ("You shift the sword to your left hand."); a hand that holds
  something else refused ("Your left hand holds the torch."), a two-handed thing in one ("The pick takes both
  hands.").  "swap hands", "switch hands", "change hands": both things change hands ("You change hands: the sword
  in your left, the torch in your right."), refused if a thing's in both, or a hand to go to is useless.  A second's
  work each (10 tenths).  What's carried, a look and an examine say which hand ("a torch (lit), in your left hand",
  "Your spear is in both hands.").  (things.c's part 48; main.c's hand_said, limbs_cmd.)
- **The main hand** is the referee's (its summary's limbs: HarnMaster's by the medical traits): the right, the left if
  it's left-handed; ambidextrous, the right, with no off hand.  The book's arm missing or deformed (Character 9: no
  work that needs both hands) makes its off arm the useless one, the house's choice of which.  (gear.c's limbs_of.)
- **What strikes, nothing named, is the main hand's** *(every ruleset's)*: the thing in the main hand (or both), else
  the other's; a soft thing only if that's all it holds (the user's rule of the soft, as before).  This replaces "the
  better of two" (an earlier draft): a sword in the off hand and a torch in the main, the torch strikes, till
  the player says "with the sword".  A thing named strikes in the hand that holds it (the main hand's first, if both
  hold one of its kind), or the hand said ("attack the rat with the dagger in my left hand": taken into that hand
  first, if it isn't there).  (choose.c's wield, RS_WIELD's x[0] the main hand's; the hub's pc_shand.)
- **A thing in one hand, the book's one-handed penalty** (Combat 3's HM: a spear's and a staff's 10; the game's tools
  none, a pick or a shovel always in both); **a weapon in the off hand, the book's 10 more**, but a shield's, and none
  if it's ambidextrous.  (gear.c's hand_pen.)
- **An arm's hurt hinders what its hand uses, the house's: each level of its injuries (shoulder, upper arm, elbow,
  forearm, hand, on that side) a step** (5) worse, for its attack and its block; a thing in both hands the worse arm's.
  Why: the book's universal penalty already counts every injury once; the arm's own levels count again for what that
  hand does, as the user asked ("arm wounds and using a shield or weapon in that hand"), in the book's scale of a
  level's worth.  The numbers the user's to judge.  (gear.c's hand_pen.)
- **An arm's wound lets go of what its hand holds, the book's** (Combat 14, Skills 21): a wound to the arm (shoulder to
  hand) its fumble roll, 3d6 and the physical penalty over Dexterity (the Combat Tables' u, on every limb's row), a
  grievous one at once; and the melee table's fumbles (AF, DF, BF) what struck or blocked.  The house's: a harm's (a
  trap's, a fall's) roll is made only if that hand holds something (the world tells the referee which do, RS_EQUIP's
  w2); a fight's is made as before, whatever it holds.  What's let go of falls where the book's die puts it (a d10: 1-6
  the square that way, north, northeast, southeast, south, southwest, northwest, if it's open floor; 7-10 its own),
  seen lying there, told after the blow's words ("Your sword falls from your hand."); the referee told what's to hand
  at once.  A creature's own parts (a bite, claws) are never let go of.  (fight.c's strike, mishap; gear.c's fumbled,
  wound_drops; sheet.c's harm; wlimbs.c's drop_held, part 49's.)
- **A useless arm** (a grievous wound, till it heals to M1; an amputation for good): its hand holds nothing ("Your left
  arm is useless."), no two-handed thing ("You can't hold the pick in one hand."), and a shield or a weapon on it
  does nothing (no block, no blow).  (things.c's hand_for; gear.c's pc_side.)
- **A leg hurt, a limp, the house's: each level of the injuries to a leg it walks on (hip, thigh, knee, calf, foot)
  a fifth more time a square** for every pace (a serious thigh wound of 3 levels: 24 tenths a square for 15), told
  once as it walks ("You limp.").  Why: the book slows only by the universal penalty (its Move, Combat 6, is the
  attributes' alone); the user asked that a serious fracture make walking harder.  The numbers the user's to judge.
  A leg missing or deformed (the medical trait, Character 9: the book's half move) is a limp of five levels, twice
  the time a square; the book's pegleg, its full move, left out for now (the user's, 2026-10-23).  (dq0harn.c's status; gear.c's
  limbs_of; the summary's limp.)
- **One leg useless** (a grievous wound, the book's Combat 14: a crutch or a staff to lean on, else a crawl): it hops,
  at a crawl's pace, the house's three times a walk's time a square, tiring as strenuous work does (the user's,
  2026-10-22); told once ("You hop along on your good leg.").  A thing long and sturdy held (its kind's form KF_LONG,
  the catalogue's: a spear, a shovel, a door's bar) is a crutch, a walk's pace at most ("You lean on the spear and
  hobble along.").  No faster than a walk on one leg ("You can't run on one leg.").  "crawl north" goes down and
  crawls, "hop north" hops, "walk north" the best it can (a crutch, else a hop), each kept as a pace is till another's
  said.  (dq0w.c's part 50, limbs_pace.)  In the dark its words and its dangers go by the pace it's really going at, a limit's
  (s_limbs_pace, 2026-11-04, *new, for review*): on a crutch "You walk south into the dark."; hopping or crawling, feeling its way;
  to the waist in water or swimming, its hands first ("Your hands meet a wall."), no run into anything.
- **Crawling** is at the same pace, three times a walk's time a square (the house's), tiring as a walk does: on its
  belly, prone ("You crawl."; PRONE on the status line).  It needs a good arm and a good leg, or both arms with no
  good leg (the user's, 2026-10-22); both legs useless, it falls at its first move ("You fall.") and crawls; neither
  way, it can't move at all ("You can't move.": the referee's C_STUCK).  On two good legs "crawl" is a creep, as
  before.  (gear.c's cond_of: the book's two legs, C_CRAWLS or C_STUCK.)
- **Prone, and getting up** (the book's Rise, Combat 8: a round): fallen in a fight (a stumble), from a harm (a
  trap's), or over a thing in its way in the dark, it's prone.  "get up", "stand", "rise" take a round; a walk, a
  step, a climb, a disengage gets up first (a round the later) unless it's crawling; prone in a fight, its round is
  getting up, no blow.  The test: the book's Rise is free but for one seriously or grievously wounded (the book leaves
  a test to the GM: the house takes it, Agility times 5); and the house's, with no good arm (the user's, 2026-10-22:
  "getting up with no good arms ... should probably require an agility check"), the same test four steps the harder;
  failed, the round's spent, still down ("You struggle, but can't get up.").  No good leg, it can't ("Your legs won't
  bear you."): in a fight its round goes on finding that out, the fight's over and the prompt back, no blow ever to come
  from the ground *(new, for review, 2026-11-04: it struggled on each round before, for ever if nothing killed it; struck
  again, it's fought on again, a round, and stops again; a crawl or a throw the player's to say)*.  The hub's pc_pos is the character's position (POS_PRONE; crouching, kneeling and sitting are slice
  4.6's).  (dq0w.c's part 50, rise_pc, limbs_go; in a fight the referee's, fight.c's attack, O_RISE; gear.c's
  rise_of, P_RISE's.)
- **The denizens' limbs are the character's** *(every actor's)*: a creature's weapon fumbled (the melee table's
  fumbles, an arm's wound: the book's, as the character's) falls where the book's die puts it (a d10, as above), seen
  lying there, told ("The kobold drops its spear."), its kind the catalogue's of the weapon's name (a kobold lord's
  axe, the book's handaxe, a kind of its own); its own parts (a bite, claws) never.  Disarmed, it fights with its
  fists, the house's: its weapon's mastery less 10 (the book has no number for a creature's punch).  Its weapon still
  lying in its reach (its square or beside it), its next blow's turn is spent taking it up ("The kobold snatches up its
  spear."); taken by the character first, it fights on with its fists.  (fight.c's effect, beast_side, RS_REARM;
  wlimbs.c's part 51.)
- **A creature's legs** (the referee's, by its body: the bestiary's two legs or four): on two, one leg useless, it
  hops or crawls at a third of its pace (as the character's: three times the time a square); on four, one useless
  (its forelegs the strike table's arms), at half its pace (the book's: on three good legs, half its move), two, a
  third, more, not at all; on two, both legs useless, a crawl if both its arms are whole, else not at all.  A
  creature on neither (a spider, a snake, a bat) none of it.  (gear.c's cond_of; wbody.c's step, its time.)
- **A creature prone gets up as the character does**: in a fight its turn (the book's Rise, with the house's test,
  above: seriously or grievously wounded, an Agility test, failed still down, "The ghoul struggles, but can't get
  up.", told once as it lies there, 2026-11-02, the round spent at each try, told again as it rises, falls or is
  wounded; no leg to stand on, never); out of a fight, at its next move, a round (no test: a creature's moves are its
  mind's, unfought), unless it crawls on (its legs won't bear it as they should).  ("The kobold gets to its feet.":
  wlimbs.c's part 51, den_legs.)
- **Knocked senseless, an actor falls, and does nothing till it comes round** *(every actor's; 2026-10-29, drafted
  for the user to review)*: it lies prone where it fell.  Whatever its mind meant to do (a step, a blow, a robbery,
  posted as it fell), and whatever would rouse it (a noise, words), it does nothing till its time out is over; talked
  to, it hears nothing ("The bandit lies senseless: it can't hear you.": no test, no rouse); left behind and met again
  still out, it's lying there still.  Come round, its first turn is the book's Rise ("The bandit gets to its feet.",
  the test above), then its weapon, if it let it fall and it lies in its reach, taken up (a turn, as before), then it
  fights.  The character knocked senseless falls too: up again as it comes round (a round: "get up"; a walk, a fight's
  first round, gets up first).  Why: the book's unconscious have no engagement zone (Combat 6) and a fallen body rises
  by the Rise (Combat 8); before, a bandit came round where it stood and struck at once, or snatched up its sword from
  where it lay, with no word of its coming round, and an intent its mind had posted as it fell, or a rouse, was carried
  out while it lay there (it was shown fighting, not lying senseless).  (wfight.c's blow; wbody.c's den_intents;
  wlimbs.c's den_enter; dq0d.c's enlist and sway, SR_DOWN; fight.c's attack: the Rise before the weapon taken up;
  dq0w.c's main, the character's.)  *Not yet* (siblings found, left): a denizen come round out of a fight walks off
  without its weapon let fall (only its next blow's turn takes it up).  (A creature come round in shock, and the
  character in shock held to rest, a look and a half move: built with shock's recovery, slice 4.3's, wounds over time,
  below.)

## Bearings underground *(slice 4.2, drafted 2026-10-23; its plan seen by the user: "Those look really good", DECISIONS.md's; its numbers the house's)*

The user's (DECISIONS.md's bearings and the map, 2026-10-16: "traveling underground, for many species, is
disorienting ... Dwarves or other denizens that live underground don't have this problem, but humans would, so
instead of direction heading, a check would need to be made"; 2026-10-23: a start below, waking from a fall,
disoriented, and anything else that disorients the same way; familiar ways easier).  The world keeps the true facing
for every task that works the place; the character's north is a belief, the hub's pc_skew (the heading it believes it
faces the true one and the skew, in eighths of a turn) and pc_lost (it knows it's lost them), the world's, kept with
the game (world.dat).  Every ruleset's but the test itself (the referee's).  Each number here is a draft for the user.

- **Who knows north:** the Khuzdul, who live below (the referee's summary's northsense: HarnMaster's species
  attribute); every other kind keeps its bearings by attention (the Sindarin are forest folk).  One that knows north is
  never tested and never astray, whatever befalls it.  The denizens keep the true north: they're the underground's
  own.  (dq0harn.c's status, bearing.c; dq0w.c's part 52.)
- **The tests keep north** *(the house's choice, for the user to review)*: dq0 -N has the character keep its bearings
  as one that knows north does; the runner gives every dq0 test -N but those about the bearings (bearings: true, as
  -I is given but to those about the engage initiative test), so their compass words go where they went before the
  bearings came; the soak plays them.  (main.c's args, WQ_START's WY_NO_BEAR; sim/test.js.)
- **The test** (rs.h's P_BEARING): Awareness, or Mining if the character's is the higher (a miner knows tunnels).  The
  book has no skill of direction: an Intelligence roll was the other candidate, but keeping count of the turns is
  attention more than reasoning; Survival is the outdoors' (the sun, the stars), of no use below.  Its dice the
  bearings' own (rshouse.h's RS_HOUSE_BEARING), so the game's other tests roll as they would without it.  (dq0harn's
  bearing.c; dq0null's flat odds.)
- **When it's asked:** at a move's end, after a walk along a passage that bent (each turn between two of the
  passage's squares a bend: the walk's beats' WB_TURN), or across a room in the dark (felt across, not seen: two of
  its squares or more, and on out of it or to its far side; a small step, or a walk to a place in it, isn't a
  crossing).  A straight passage and a lit room ask nothing.  (dq0w.c's beat, part 52's s_bearings.)
- **Its steps:** one bend **four steps the easier**, each one more **two the harder** (two bends 2 easier, three none,
  four 2 harder, twelve or more 18 harder); a room crossed in the dark counts as a bend; a walk in the dark **four
  the harder** (so a dark room crossed, alone, is a test at no steps).
- **Familiar ways the easier** (the user's, 2026-10-23): by the map's count of passings (the map kept, below: the
  times the character's come to see that part of the place again, four minutes or more after it last saw it), of the
  chunk the walk ends in, and the trace's rule for them (TR_MORE: a point at 4, another at 12): **4 passings or more
  four steps the easier, 12 or more no test at all**.  One count for the map's memory, the traces and the bearings,
  not a second.  (Part 52's familiar, world.h's KEPT_KNOWN.)
- **What comes of it:** a success changes nothing (a belief already wrong stays wrong: only a landmark puts it
  right); a marginal failure drifts the believed north **an eighth**, and the character doesn't know it; a critical
  failure **a quarter**, and it knows: "You've lost your bearings: you're no longer sure which way is north.", the
  status line's facing with a ? after it ("facing NE?").  Which way it drifts is the bearings' own d8 (odd the other
  way).
- **An eighth off, the compass misleads in a passage:** the four ways the character believes in are truly
  diagonals, and a passage goes by the four but where a diagonal way goes on (2026-10-31: Diagonal ways, below), so
  "north" there is "You can't go that way."; the ways as it faces
  (forward, back, left, right) still go where it means.  The game's choice, kept (the user's, 2026-11-03): the believed way is taken as said, not
  nudged to the nearest passage (at an eighth off both of the nearest are as near).
- **Landmarks:** coming back into a room it has been in before (the senses' R_VISITED: come into, not only seen;
  the room by the way in, a stairway's room it came down by, any it has crossed), from elsewhere (a passage walked,
  another room, another stratum), sets its bearings right, told if they were wrong or it knew itself lost ("You know
  this place: your bearings come back to you."), and tests nothing more.
- **The start** (theme.h's TM_FELL, a theme file's "start | walked" or "start | fell"): walked in by the way in from
  the surface (the default) it knows north; come round below from a fall, its bearings are tested **ten steps the
  harder**, a failure leaving its north **anywhere** (the d8: any of the eight, the true one by chance), not known to
  it but on a critical failure.  The kobold warren's stand-in starts so (its story: fallen through a ruin's floor);
  themegen's own themes walk in, and pass a hand-written theme's start line on.
- **What disorients it** (the user's, 2026-10-23: any moment the game moves the character without its walking):
  knocked senseless (by anything: a blow, the referee's summary's C_UNCONSCIOUS as the world sees it; a harm, a fall's,
  a trap's, a wall run into in the dark, by its time out; a trap's gas choked on; its own blow at a door or a thing
  gone wrong), its bearings tested as the start's from a fall (ten harder, a failure anywhere); a fall of **ten feet or more** (a pit, a ledge jumped or fallen from, a shaft, over an edge in
  the dark, a window's sill; a hold caught, none), **a step the harder each five feet** (ten at most), a failure the
  walk's drift.  Nothing moves the character while it's senseless yet (a robber takes things, it doesn't drag); if
  one comes, its being knocked out has done it already.  A climb, a window climbed through, stairs and ladders don't
  disorient.  (main's watch of the summary, harm_fx's, part 49's: part 52's bearings_off.)
- **What goes by the belief:** the status line's facing, "You turn to face ..." ("You turn right, to face west." said by its way:
  the user's, 2026-11-03, all the turning words: half left and right, left, right, around), "You face ... already.", the dark's
  "You feel your way north." and "You run north into the dark.", "You search the north wall...", and every compass
  word typed: "go north", "turn east", "face north", "the door to the north", "north and to my left", "search the north
  wall", "look north" mean the north it believes in (the UI makes them the true ways it means, main.c's cq_cmd).  The
  ways as it faces (ahead, left, behind) are always true, the body's own, as the senses' words are (all of them as it
  faces).  The map, its compass and its panel's facing are the map's (The map kept, below).

## The map kept *(slice 4.2's map: drafted 2026-10-23 for the user to review)*

The user's (2026-10-16 and 2026-10-23, DECISIONS.md's bearings and the map): the map plain only where the character
sees now, the rest obscured; a mental map by its Intelligence, fading by its Intelligence and Will, the better kept
where it's often been; mapping as it goes keeping it the better, on paper with the things to draw with handy and
equipped; a drawn map as good as its maker's drawing (Cartography first, then Drawing), and only as good as what was
seen; a dwarf's map north up, anyone else's the way it faces up; what's seen now told apart from what's remembered,
in colour.  The book's own rules are the referee's: Drawing and its specialty Cartography (Skills 14: a poetic map's
quality limited by what its maker knew), a specialty chosen at 40 and opened at the skill's mastery (Skills 2), a
skill's opening (its OML).  Every number here is the house's.  *(Every ruleset's, but the referee's numbers, marked
dq0harn's, and the drawing's test.)*

- **How well it keeps what it sees** (the referee's summary's mapmind): five times its Intelligence, of a hundred (an
  Intelligence of 20 or more, all of it), mapping as it goes; half that, not.  The null referee's 50, a middling mind's.
  (dq0harn.c's status; dq0null.c's.)  Why: the user's Intelligence; mapping is attention, twice as well kept.
- **How fast it fades** (the summary's mapfade): a tenth of it (ten points) every twice its Intelligence and Will
  together, minutes: some forty-five minutes for most (Intelligence 10, Will 13: 46); the null referee's 40.  A place
  seen once without mapping (a quality of 25) is gone in some two and a quarter hours, mapped (50) in four and a half.
- **A place often come to** (the user's, 2026-10-23, a trace's rule: hub.h's TR_MORE): each time the character comes to
  see a place again, four minutes or more after it last saw it, is a passing (a stay isn't, but a long wait with no
  look about, then one, counts once).  Four passings make its memory a quarter of mapmind the better as it's seen
  again and fade half as fast; twelve another quarter, and a quarter as fast.  So a human's (Intelligence 10) place
  come to four times is still kept, put off a little, after three hours in the dark, where one seen once is gone.  The
  count is the bearings' too (world.h's KEPT_KNOWN: Bearings underground's familiar ways, above).
- **A place is a chunk** (160 feet square): what's remembered, when, how well and how often, is each chunk's, not each
  room's.  Why: it's kept with the seen squares the maker already keeps by chunk, in the same bytes' room.
- **What the map shows of a memory**: each square of it by its quality and forty more, of a hundred, a square at a
  time by its place (gone at nothing); faintly, a floor a comma, no heights.  For one that doesn't know north
  underground (anyone but a dwarf) a chunk's squares are put off where they truly are, the more the more it's faded
  since it was seen: six squares at the most, as it's gone, its own way (the user's "fuzzy or off": directions and
  distances off).  A dwarf's are never put off, only fewer.
- **A walk's squares**, felt or seen as it goes, are remembered as their chunk's memory goes (a walk's look is at its
  end: a chunk only walked through, never looked at, is kept as it was).
- **A disorienting moment** (a fall, a blow or a trap's gas that knocks it senseless, coming round) leaves what it kept
  as it was: the bearings take it (the believed north, Bearings underground's, which the map's compass shows), not the
  memory.
- **The things to draw a map with** (the catalogue's, by their keys, things.c's chart_now): a sheet to draw on (some
  paper, some parchment) or its map, and a point that draws (some charcoal, some chalk, a pencil) or a quill and a horn
  of ink.  "Handy and equipped" (the user's): handy (the reach's rule, 2026-10-26, below: in a hand, or on the body, in
  nothing that isn't open to the hand: a pouch on its belt yes, its pack worn or a sack no), and a hand to draw with,
  free or holding one of them (its other may hold its light, not a sword in both); its arm not useless; and light to
  draw by, its own square lit.  Without them, mapping as it goes keeps the map in mind, twice as well; carried but not
  handy, told once as it looks about ("Your drawing things are in your backpack."), till they're handy or it stops
  mapping.  A map carried anywhere is shown.  *(For review: the hand and the light.)*
- **A map begun**: mapping as it goes with them and no map, its first sheet is made the map ("You begin a map."): an
  item, "map", dropped, lost or stolen like anything else.  What it draws is what it sees (each square marked as it's
  seen now, a walk's squares in the light too), each chunk's drawing one map's (a newer map drawn over it there wipes
  the older's).  Its drawing is shown only while it carries that map (the first it carries, if it has more), and only
  where it was drawn: a map found or carried from another stratum shows nothing of this one.
- **How true it's drawn** (the referee's P_DRAW: the book's Drawing, its Cartography specialty first, dq0harn's): a
  test as each room's come into and each way's walked, mapping on paper (and the first look it draws at), its grade
  kept with each chunk as it's first drawn on the map, with the north it believed in then.  A critical success true;
  a marginal success one square in eight left out; a marginal failure one in four, and put off a square; a critical
  failure one in three, and put off three; and two squares more for each eighth of a turn its believed north was off
  (a lost character draws a wrong map without knowing it).  The book's limit holds: only what it saw is drawn.  The
  map's own make (its quality, 0-9) the mean of the last and each test's: 9, 6, 3, 0.  Drawn squares shown brown,
  faintly, never as seen now.
- **Drawing opened by a map** (dq0harn's, over the book's month of teaching, its [1 Month]): neither Drawing nor
  Cartography open, the first map's test opens Drawing at its opening level, told once ("You start to learn
  drawing.").  **Cartography** opens only by teaching (a teacher, a book: none in the dungeon yet; the hook,
  dq0harn.c's draw_learn: the book's rule, Drawing 40 or more, opened at Drawing's mastery); a character's sheet may
  carry it (the referee's own skill, its file's "ml 91 N").  hmchar's Cartographer/Artist (the book's Drawing/4) could
  take it as its specialty at Drawing's mastery: not built (hmchar's main RAM).  *(Drawing opened at the first map, and Cartography only by instruction, the user's: DECISIONS.md's bearings and the map; the specialty the house's.)*
- **What the materials cost to use**: built 2026-10-26, the reach's things used up (below): each drawing two minutes
  of the point's life, or the quill's and the ink's; a sheet holds a stratum's map whole (the map's sheets its own).
- **The universal kit carries none of them** (the user's, 2026-10-23: each occupation its own): an outfit may, by its
  skills (Drawing's in the pouch, handy; a thief's always; Script's, Inkcraft's and Law's in the pack: Outfits, below);
  dq0 -k paper,charcoal gives them, the tests'.  None are in the dungeon's stock yet (a solar's, a chapel's would be
  the places: dq0m/uses.c).
- **The map turned**: one that knows north (the summary's northsense, a dwarf) north up, the window kept in the
  stratum, its square in the panel; the rest the way it faces up, to the quarter turn (its body's: facing north or
  northeast north up, east or southeast east up: what it faces up, or up and to its right), the character in the
  middle, the rows turned, a compass in the top right corner, its N where the north it believes in lies (a ? when it
  knows it's lost its bearings).  The panel's facing and the words above the map the one it believes in ("you face
  northeast, or so you think", lost).  No square given in the panel: its coordinates would give the true north away.
  Why turned by the body's facing: what's ahead is always up, whatever it believes; the belief only names the way.
- **The colours** (the user's, 2026-10-23): what it sees now bright white, everything remembered dim grey however
  well kept (how well decides whether a square's shown, and how true its place, never its brightness), what's drawn
  on its map brown, the @ bright cyan, the compass cyan; the glyphs as without colour, so the map reads either way.
  "colour off" (or "no colours") plain letters, "colour on" again, kept with the game (game.dat).  (look.c's s_map.)
- **The catalogue's** (kinds.c's): paper and parchment a sheet's (weight nothing and a quarter pound, each worth 2 pennies), charcoal
  (a farthing), a pencil (3 pennies), a quill (a penny), a horn of ink (a quarter pound, a penny), the map (a sheet's);
  chalk (a farthing, 2026-10-26).

## Reach, handy tools and things used up *(2026-10-26: drafted for the user to review)*

The user's (DECISIONS.md's outfits, reach and things used up, 2026-10-23): handy is readily available, a worn pack less
so than a belt pouch; each container deeper longer; a pack on the ground or in hand reached into without taking it off;
a worn pack searched taken off and put back on, each its time; a pack shrugged off or dropped before a fight at no
time; charcoal and chalk used up soon, a pencil and a pot of ink long.  The book's own rule is the referee's: the
Grope (Combat 7: drawing or changing a weapon, retrieving an item from the ground, a turn; its Dexterity test,
P_GROPE, Skills 21).  What the house adds, every number the house's:

- **Reach, one rule for every taking out, putting away, look inside, search and tool's use** *(every ruleset's, every
  actor's; what isn't by it as yet, Every use of a tool's "Not by it", below)*: a thing's reach is by where it is, its levels out to whoever carries it.  **In hand**: none.  **On the
  body** (worn, hung from a belt, in what's open to the hand: anything that isn't a container, a belt or a surface; a
  container that hangs from a belt, a pouch, a quiver, a sheath; anything worn that isn't a pack on the back) **or
  lying within reach** (on the floor, on a table): a quick draw, no time more than the handling's own (a second to take
  in hand, two to put away).  **A container's level** (held, set down, in another: a sack, a chest, a pack in hand or on
  the ground): two seconds more each, so a thing in a sack in a chest four.  **A pack worn on the back** (the
  catalogue's: a kind worn on the back, KF_BACK, the backpack's; not names in code): taken off first and put on again
  after, five seconds each, and its level's two, once a handling however often it's reached into.  So a thing from a
  belt pouch a second, from the worn pack thirteen, from a sack in the worn pack fifteen, from a pack on the ground
  four (its level's two, and two for a thing taken from what isn't carried, things.c's take_from: from what's carried, a second); a thing taken up into the worn pack fourteen.  *(For review: every number.)*
- **A reach is a sequence of steps** *(2026-11-02: the user's words, DECISIONS.md's; drafted for review: the words, the
  hands, the tools' order)*: each told as it begins, each its own time, then the next; the long tasks' own (a bar for
  each, "stop", hard and soft interruptions), the world changed at each's end.  In the user's order:
  1. "You take off your backpack." (5 seconds), into a hand; or, no hand left to reach with, "You take off your
     backpack, and set it down at your feet."  A thing to get out wants two hands free (the pack in one, the thing in
     the other); a thing held already, or a look in, one; a two-handed thing, or fewer, its feet.
  2. A level each, 2 seconds, the outermost first: "You reach into the backpack." on the way, then "You get out the rope."
     (a thing got out) or "You hold the sack open." (a thing put in, a look in, a search, a lid of what's in it).
  3. The handling, its own words and time as they were ("You take the rope in your right hand.", a second; "You put
     the rope in the sack.", two; "In the backpack lie ...", three; a search's minute).
  4. "You put your backpack back on." (5 seconds), as the character's free; after a timed work's end, its outcome first.
  A pack on the ground or in hand has steps 2 and 3 alone; a thing on the body, or lying within reach, 3 alone.  One
  taking off and one putting on a handling, however many things it moves ("take all from the pack", "drop 3 torches":
  the first thing's steps, each thing after it its level's two seconds, untold); a run of commands each its own.  What
  it can't do is refused first, in no time (no hand for it, no room, closed: the pack stays on).  For everything the
  rule covers: a thing taken out, put away, taken up into the pack, worn, let fall from a pack, thrown, filled from, a
  lid worked, a look in, a search of it.  (things.c's parts 82, 83, 84: reach_step, called by each handling just before
  it changes anything; the reply's IO_STEP, the UI's first words and wait, the command asked again as it's up: main.c's
  reach 5, part 19's step_go.)  The time out of a fight is the house's seconds: the book gives a grope no time but in
  combat, a turn.
- **Lists** *(2026-11-04: new, for review; the words the player may use, none of the book's)*: "take the rope and the torch", "drop the sack, the knife and the rope", each in turn, as lines typed one after another, each by its own steps.  After "and" or a comma a part that begins with "the", "my" ... is a thing: the verb before it's put to it.  One with no such word ("take rope and torch") is a command if it's one ("take the sack and look"); no verb in it, a thing again.  What's said after the last goes for all that want it: "put the rope and the torch in the sack", "give the rope and the knife to the dwarf", "throw the knife and the rock at the rat"; "take and drop the pick", a verb alone with the next one's thing (take, get, drop, open, close, hold, stow, wear, light).  A failure doesn't end the list ("You see none of those here.", then the next).  **Stopped, the rest of its line goes with it**: "stop" typed, taken at once, not after the rest; the long task stopped by a blow, a hostile near, or asked about ("Keep walking?"): what's on the same line (with "then" too) is let go, lines typed since are kept.  So the rest of a line never answers a question meant for the player.  Nothing within quotes is split.  At most eight parts a line.
- **A kind's word said before a name** *(2026-11-04: new, for review)*: the parser keeps a thing's last word alone ("black orc": "orc"), so the character went at a small orc for "attack the black orc".  Now the word said just before the name, among those that make one kind another (black, brown, red, white, small, great, cave, giant, dwarf, kobold, iron, brass, broken), must be in the name of what it sees: "attack the black orc" with only small orcs there, "There's nothing like that here to fight."; a word that isn't one of them ("the sleeping rat") is passed over.  Of what it sees, not of what it carries (the world's chooser has the last word alone, as yet: "drop the iron key" drops the first key), nor of the words to a denizen ("talk to the black orc" is any orc).
- **A run of handlings on one line** *(the soak's first play, 2026-11-03; for review)*: "take all" takes its pack off once and puts it on once,
  after the last thing, each thing "Taken." between.  The next thing out of reach: the pack on first (never left behind at the feet), then
  the walk, and the rest of the line by the lump, untold, its time as ever (told once; the character doesn't shrug it off and on again in words).
  A count's later things went by the lump already.
- **Two torches dipped** *(the same: the house's, each step about a named one (credited to the user till the user said on
  2026-11-03 those weren't the user's words); for review; the dip itself: A torch redipped, below)*: which one is chosen as the sequence begins (the most
  room for more, as ever) and kept to its end: "the old torch"; another of its kind in play is "the new torch".  Two lit: "You blow out the old
  torch.", "You set the new torch down at your feet.", the flask, "You set to dipping the old torch.", the flask away, "You put away the old
  torch." (only if the pack's off for the flask: else it stays in its hand), "You take up the new torch."  The old one already spent: nothing to
  blow out, the same after.  One torch alone keeps its plain words.  Taken up, the burning one before a sword.
- **Kinds past 127** named as they're stopped as well as begun: the long task's what has seven bits, the UI keeps the top one (what_hi).
- **For the tests, dq0 -J** (as -I and -N are): the reach in one lump, untold, as it was: the runner gives every dq0 test -J but those
  about it (reach: true: dq0handy, dq0packsteps, dq0packgrope, dq0packasks), so their games stay as they were; the soak plays
  the steps.
- **Stopped part way the world stays true**: a step not finished hasn't happened; those before it have.  Stopped with the
  pack off (hard: a blow, a wound, a hostile near; soft answered no; "stop"), it's off, told ("You stop getting out the
  rope. Your backpack is in your hand.", "... lies at your feet."), the thing out if it got that far; "put on the pack"
  finishes it, the same command again goes on from where it stands (no taking off).  A pack at its feet can be left, or
  taken by a denizen (its take rule): then it's not put on, and nothing's said.  Soft questions read "Keep taking off
  the backpack?".
- **Every use of a tool, one routine** *(2026-11-02: the user's words, DECISIONS.md's)*: **its hands made free first, if they're busy** (below); the pack off,
  at its feet, if the tool's in it; its levels ("You reach into the backpack."); "You get out the pick." (a second; in hand already, none; from a belt pouch, this and no pack): **now truly in its hand, or both for a two-handed one**;
  the use, its own long task and words; "You put away the pick." (two seconds), back where it came from, or the pack; "You put your backpack back on."  Each a grope engaged.
  **Stopped at any point the world stays true** (the user's, 2026-11-03, A26): the pack off told, **the tool out stays in the hand, told** ("Your pick is in your hand."), and a later use
  finds it there: not got out, and not put away after it, as anything held before.
  As built: a dig's pick or shovel (the first spell's), lockpicks for a door or a chest, a chest's own key, a flask for a light filled or a torch dipped, a bandage for first aid, a fire-maker below, a whetstone.
  And *(2026-11-04, the sweep, for review)*: a door's key, the one that fits, a timed work of three seconds ("You set to turning a key.", "You lock it with its key."; a denizen's its own, no reach); rations for a meal and a waterskin for a drink (eating and drinking: a ration begun no reach, a well or the water it stands in none); a blow's weapon named from the pack ("attack the rat with the pick": the same steps, a round each engaged, then the blow).
  Not by it, and why: **a load** ("nock an arrow" from the pack, no quiver: its bow's in the hands that would reach, and readying with busy hands would put the bow away; nothing's left to put away after, the arrow's on the string: the lump, untold, its time as the steps'); **the drawing things** (handy or not: no drawing from a pack, the map kept's rule); **a robber's rifling** (the character helpless: the lump).
- **Readying with busy hands** *(the user's, 2026-11-03, A25: "If your hands are busy and your want to ready tools or other things, it should probably put the other
  things away first (in their appropriate place) then take the tools in hand."; what goes where the house's, new, for review)*: any tool's use, and "hold the X", with no hand for it, puts away what's held, a thing at
  a time, each told and each stopping as a step does: **a light burning in the hand** (not a lantern's, that hangs) into an empty sconce within reach, "You set the torch in the sconce."; else laid at its feet still lit, "You lay the torch on the ground, still burning."
  (it lights the work as it lies; a look says "a lit torch"; taken up lit); **what's kept worn** (a shield), "You sling the round shield." (five seconds); **what a carrier of its family on the body holds** (a sword, a scabbard; a knife, a sheath), "You sheathe the sword." (two);
  **anything else into the pack**, by the reach's steps; no pack, or no room, set down at its feet.  A chisel takes both hands cleared, and its hammer got out by the same steps after it.  In a fight the same, no refusal: its time is the danger.
  **Never what's being readied**: the tool, a tool out already, the thing it's for (the torch to light), a chisel's hammer; the pack off for this, if it's in a hand, set down, not slung; six things at most a sequence, then no hand, the handler's words (the soak, 2026-11-04: nothing goes round for ever).  A second thing put away told "too".
  **As the job ends what was put away isn't brought back** (it's where it was put): the one exception's the routines that did so already: a torch's dipping and a new light set down what's in the way ("You set the sword down at your feet.") and take it up again after.  dq0 -J's lump has none of it, the hands untouched (the tests').
- **A whetstone** *(2026-11-02: the user's "like sharpening your sword (if you have a whetstone)"; the book has none: all
  the house's, for review)*: "sharpen the sword", "hone", "whet" (the fill's verb, the rules' bank spared): a metal
  weapon, the whetstone carried (6d, a quarter pound; one in four with Sword's skill have one): that routine, its use a
  minute's timed work, "You set to sharpening the sword.", at its end half its wear gone, "You put a fresh edge on the
  sword."; no wear on it, "The sword is as sharp as it will go."  None carried, told.
  That wear reaches the blow (What strikes, above): a dull sword cuts two points less, so the whetstone's halving brings it a point or two back.
- **Which is lit** (2026-11-04): "light the torch" means one in a hand first, whichever way it's lit, none put away for another; a dipping's spare the one in the other hand first.
- **Lighting by a fire-maker** *(2026-11-02: the user's words, DECISIONS.md's; for review: the numbers, and the test)*: for
  anything lit with no flame to hand (a lit thing carried or within reach lights it, as ever, no fire-maker).  That routine,
  its use the tries: "You strike a spark.", 5 seconds, its test at its end, whether engaged or not: the referee's grope
  (P_GROPE: Dexterity times 5 in HarnMaster, the null referee's own, *not a skill of its own: one more purpose takes room in
  the referee's tables, its main RAM at the floor*), 20 less in the dark; "It doesn't catch.", and again, till "It
  catches." or it's stopped or hurt; then "You light the torch.", put away, the pack back on.  **Empty** (a tinderbox thirty
  lightings, one off as it catches: the catalogue's burns, FU_DRAW, a closer look's "half used"): told as it comes to the
  tries, "The tinderbox is empty.", put away, the pack back on, and known from then on (the thing's IS_SPENT): refused at
  once, no steps, the next time; another tinderbox carried tried first.  The fire-maker takes a hand, its hands made free for it first as any tool's (the user's, 2026-11-03, A24).
- **"take off the pack" leaves it off** (in both hands, by its bulk: either holding something, refused) till "put on the
  pack" (five seconds each, as any worn thing's).  "the pack" names the backpack (the chooser's: a name of one word's end, four
  letters or more: "pack", "skin" a waterskin, 2026-10-26); "drop the pack" lets it fall, and "shrug off the pack", "shrug the pack off", the user's
  word (2026-11-04).
- **Dropping takes no time** (the book's: discarded freely, no grope), a lit one too: a thing in hand let fall, a pack on the back
  shrugged off; a light's change (a lit torch dropped, put out, taken up lit) its handling's own time, the look in the new
  light with it (not a step's, as it was: I_LIGHT's w1; lit, three steps as before); a thing in a pack or a pouch got out first (its reach); worn armour taken off first (a doff's five
  seconds).
- **Looking in a thing, or searching it, is its reach**: a look in the worn pack takes it off and puts it on again
  (fifteen seconds, the look's three with it), searching it a minute more than those (the search's minute itself
  whole: *for review*); a look in a sack held five seconds.  "look at" a thing isn't (the plain look).
- **A torch redipped** *(2026-11-02: the user's words, DECISIONS.md's; drafted for review: the order, the hands, every
  number)*: "refresh the torch", "redip the torch", "dip the torch" (and "dip the torch in the oil", "fill" a lantern as
  ever): refused first, in no time, if it has all it can take, or there's no fuel.  **Never dipped burning.**  By kinds of
  case: (1) *Another light* (another lit thing carried or within reach; the room's own): "You blow out the torch.", the
  flask, the dip, by that light; put away after if the other's carried.  (2) *Still lit, a spare, no other light: the
  preferred order, chosen for the player*: off with the pack, "You get out the new torch.", "You light the new torch from
  the burning one." (3 seconds), "You blow out the old torch." (a second), "You set the new torch down at your feet." (the
  lit one: the hands' rule), "You get out the flask of oil.", "You set to dipping the old torch." (its time, the oil moved
  at its end: "You soak the torch in the flask of oil."), "You put away the flask of oil.", "You put away the old torch.",
  "You take up the new torch.", the pack back on.  (3) *No spare, no other light*: "You go to blow out the torch: that will leave you in the dark.", asked
  ("Keep blowing out the torch?": no leaves it lit, as any soft stop); then the same by touch: the dip's end tested, the
  dark's (the referee's grope, 20 less): failed, "Your hands slip in the dark: oil spills, and the torch takes none of
  it.", half the flask's oil gone, nothing dipped; the flask put away; then "light the torch" by the fire-maker's own
  steps, in the dark, tries at 20 less; the pack back on.
  **The hands** *(for review, as above)*: a torch dipped asks both hands, the torch in one and the flask in
  the other; so whatever's in the other hand, a burning spare, a shield, a sword, is set down at its feet first, told, no
  time and no grope (the book's discarding), and taken up again at the end, a second, a grope engaged ("You take up the
  sword."): stopped first it lies there, as the look says.  A spare lit wants a hand as well: the other hand's set down
  for that.  One arm useless: refused first, "You need two good hands to dip the torch."  The torch not in hand is taken
  in hand first.  Interrupted part way the world stays true: oil moved only at the dip's end, a torch left unlit is unlit,
  what's down is down.
- **A new light** *(2026-11-02, 2026-11-03: the user's words, DECISIONS.md's; drafted for review)*: "light a new torch", "light a
  fresh torch", "light another torch", any light of that name.  **The old one burning in a hand: lit from it before it's
  put out** ("Of course, light the new torch from the old one before it is extinguished"), the redip's own steps: what's in
  the other hand set down if the spare wants the hand ("You set the sword down at your feet.", no time, a shield or a weapon by
  the dip's rule), the pack off and "You get out the new torch." by the reach's steps, "You light the new torch from the
  burning one." (3 seconds), "You blow out the old torch." (a second), "You let the old torch fall." (free, the book's
  discarding), what was set down taken up ("You take up the sword.", a second), the pack back on.  Engaged, each step a round
  and a grope as ever; stopped part way the world stays true (two lit; the old one out and still in hand).  **The old one
  spent, or none in hand**, as it was: the old let fall, told ("You let the torch fall."), then a spare got out by the
  reach's steps from wherever it is, into the hand (the one that suits it; a useless arm's never), then lit by the
  fire-maker's steps or from whatever flame is to hand (a lantern, a torch lying within reach: any lit thing lights any light,
  a candle from a candle, a lantern from a torch, by "light the ..."), then the pack back on.  The new one ends in the old one's
  hand, what was set down back in its own.  *(For review, 2026-11-03)*: one unlit and unspent in its other hand is the new one, first, lit there from the old with no pack off; one lying within reach only if it has more left
  than the one burning, so saying it again doesn't take up the old one at its feet.  No spare (none carried or within reach, nor burning, nor spent):
  "You have no other torch.", nothing dropped.  No flame to light it with: told first, nothing dropped (the old one
  still lit is a flame for it, as it lies within reach).  Hands full with no old one: refused first, as a hold's.
  Interrupted part way: the old one where it fell, the pack as it stands.  **"New", "fresh", "another"** are a selector
  for any handling: the thing of that name not in a hand and not burning ("take a fresh torch", "hold another torch":
  into a hand, from wherever it is).  (commands.words' SL_NEW; the catalogue's bank's light, part 4's pick; part 82's
  RS_GO.)
- **Engaged, a round a step, each a grope** (the book's grope, Combat 7, a turn; the user's, 2026-11-02): the readiest
  (on the body, lying within reach) a round; each step above a round in place of its seconds (so a thing stowed in the
  worn pack in a fight three rounds: drop the pack first, at no time).  Each step its own test (the referee's P_GROPE);
  failed, "You fumble for the rope.", the round wasted, the step not done, and the sequence over: the world as it stands,
  the pack off told, and the player chooses again (it isn't tried again by itself: the fight may have changed).  Engaged a
  step is the round's action, no long task: a blow at it doesn't stop it, or nothing in a fight would ever get done (a
  blow a round is what a fight is); a command typed waits for it.  *(For review: a wound's not stopping it either, as it
  stands.)*
- **A pack dropped in a fight is automatic, taking it in hand is a grope** (the user's, 2026-11-02; every kind worn on
  the back, KF_BACK): "drop the pack" no test and no time, as the book's discarding; taking it off the back into a hand
  ("take off the pack", a reach's first step), up from the ground ("take the pack", "hold the pack") and putting it on
  ("put on the pack") its test, failed the round wasted.
- **A robber's** the same (every actor's): its rifling the helpless character a round a step, its round's own the
  first, a pack taken off and not put on again (wfight.c's den_rob).  A denizen's weapon taken up again, its turn, as
  before.
- **A tool's use takes its reach first** (handy tools): Every use of a tool, above; by the reach alone, a thing thrown, a
  thing opened or closed, a blow's weapon taken in hand first (its hold).  By the catalogue's tool keys, the first
  carried (part 4's tool_reach).  A rope's the same: readied by the hands routine, then tied or thrown (Rope and grapples, under Climbing and falling).
- **Handy** is in hand or on the body (the reach's rule: in nothing that isn't open to the hand); mapping as it goes
  wants its things handy (the map kept, above).
- **Things used up, one way for every kind, by its data** (the catalogue's burns, as a light's fuel: the item's fuel its
  minutes left, its whole life the kind's, FU_DRAW's): drawing with them, a stick of charcoal four hours, chalk three, a
  pencil twenty, a horn of ink ten, a quill thirty; each drawing (a room come into, a way walked, mapping on paper) two
  minutes off the point it draws with, else off its quill and its ink.  Spent, gone, told ("Your charcoal is worn to
  nothing.", "Your horn of ink is dry.", "Your quill is worn out."); how much is left a closer look's ("half used").
  A sheet's room no limit (a stratum's map on one).  (dq0w.c's part 7: s_use_up, drawn_down.)  *(For review: every
  number.)*
- **Missiles' use** (slice 4.7's: Missiles, above): a lot's one used up (s_use_up by none: its qty), and what breaks as
  it's used, its chance in a hundred, the catalogue's (kinds.c's snaps: arrows 25); their taking up again, Missiles' (the user's,
  DECISIONS.md).  A wild miss may be lost: Missiles' where it falls.

## Travel to a place it knows *(slice 4.2's travel: drafted 2026-10-26 for the user to review)*

The user's (2026-10-16, DECISIONS.md's names for rooms and bearings and the map: "name this room "The Mausoleum" and
then you could say "I want to go back to the mausoleum" and you would be on your way, and the game would move you at
your current rate of speed back toward that named place, as long as your direction sense/orientation checks held up.
Orientation checks would be aided if good maps are used"); the old design's going back (2026-10-16: a leg at a time,
each bend tested, the way out nearest where it believes the place to be, not back the way it came, refused under a
tenth, stopped at a barred way or after forty legs).  The book has no rule for it: every number here is the house's,
for the user.  *(Every ruleset's: the bearings' tests the referee's.)*

- **The places**: a room named ("go back to the Mausoleum", "go to", "return to", "head back to", "walk
  back to", "travel to", "make my way back to", "find my way back to"); the way in ("the entrance", "the way in", "where
  I started", "the start"; "the way out" said going back): the top stratum's way out, where the character came in; and
  a kind of room the theme names ("go back to the kitchen"), the last of them it was in that it still remembers (the one
  it's in only if there's no other: there already).  Why the last: "the kitchen" said after leaving one means that one.
  *(The same level, for now (the user's, 2026-11-03): a journey to another stratum refused, "The Mausoleum isn't on this level."  Not yet: a fork named, a thing.)*
- **Remembered under a tenth no longer** (the old design's): the place's chunk's memory as it is now (the map's rule:
  The map kept, above) under 10 of 100 (KEPT_GONE), and no map carried that shows it: "You don't remember the way to
  the Mausoleum."  One rule wherever a square seen before is gone by (below).  Why a tenth: the old design's, and at a
  tenth the map shows a square in two; the user's to set.
- **A map shows the way** *(for review)*: the place's room drawn on the map it carries, and light to read it by (its own
  square lit, not blinded): the place where the map has it, off as it was drawn (its drawing's grade, the north
  believed in as it was drawn: the map kept's rule).  In the dark the map's no help, as it can't be read; its memory
  still can be.
- **Where it believes the place lies**: seen now, where it is (straight there); else where its map or its mind has it,
  put off as each is (a mind's as it's faded, for one that doesn't know north).  The way to it by the north it believes
  in: *(the house's, for review)* its mind's by how far its belief's gone since it last saw that part of the dungeon
  (each chunk keeps the north believed in as it was last seen), so a belief gone wrong since takes it astray, and one
  as wrong then and no worse takes it there (its sense of where things lie went wrong with it); its map's by the north
  it believes in now (a map's read by its compass, its own errors drawn in).  The old design's "the true way off by its
  bearings' skew" was the whole skew; this is the part that's gone wrong since.
- **The way out of each place** *(for review)*: from a room its doors and its passages' mouths (in the dark only those
  it knows), from a passage its ways on and its doors; the one whose first square is nearest where it believes the place
  is (as the crow flies, near enough: the further way and half the other); a way it hasn't taken on this journey before
  one it has (the house's: else it would go back and forth between a fork and a dead end that points the place's way),
  and not back the way it came if there's another (the old design's).
- **Each leg a walk** (the old design's): at its pace, each bend tested for its bearings as any walk's (and the dark's,
  the traps', the denizens' rules as they are); a landmark sets its bearings right as it passes one, and the journey
  goes on by them.  A door it has opened or shut before is opened as it goes (go's, the user's); one it hasn't, or one
  locked or barred, and a passage's shut door, ends the journey there: "The door is shut: open it first."  *(Settled,
  not for review: the user's, 2026-10-28, "travel should stop at a closed door", DECISIONS.md's.)*
- **A map aids the bearings** (the user's): a walk ending in a part of the dungeon drawn on the map it carries, not in
  the dark, four steps the easier (the old design's "four steps better"); *(for review)* two if that part was drawn at a
  marginal failure, none at a critical failure (a bad map no help).  Every walk's, not only a journey's.
- **Its end**: there ("You're back in the Mausoleum."); its memory of the place gone on the way ("You no longer remember
  the way to the Mausoleum."); its bearings lost on the way, and known (a critical failure: "You've lost your bearings:
  you can't tell which way to go."); forty legs (the old design's), or no way on it knows of: "You've lost your way: you
  can't find the Mausoleum."; a leg's walk ended short (a denizen in its way, a trap, a fall, a ledge: told as they
  are).  A long task (Where you are's long tasks: LT_TRAVEL, "travelling"): stopped hard as a walk is, asked soft ("Keep
  travelling?"), "stop", "stop walking", "stop travelling"; *(the house's, for review)* any command typed as it goes
  stops it at once and is carried out after (the old design's "any command ends it"), but those taken at once (the
  map, what's carried, the status, a look about).
- **Its words** *(for review)*: "You head back toward the Mausoleum." as it sets out; each leg's beats and the place it
  comes to, told whole (a look of its own each leg, as a walk's is); "You're back in the Mausoleum." (the way in:
  "You're back where you came in."), "You're in the Mausoleum already."
- **What's kept now** (slice 4.2's map work's list): the dark's walks go only by squares of parts of the dungeon
  it still remembers (a tenth or more); a part seen again once it's forgotten is remembered afresh, what it saw of it
  before gone (its seen squares, its rooms visited: so a room's size, a landmark, a first look go by what's kept; its
  drawing on paper kept); *(for review)* a walk felt in the dark keeps the parts it goes through as a look keeps what
  it sees, as well and as fresh (they were remembered as felt, but their memory never made fresh).
- **Every actor** *(not shared: listed)*: the denizens go to a place by a way found (a field about it and its mark: Ways
  found, positions, bodies found on the rounds, below), and away from the character coarsely about their lairs; none
  goes by this journey's ways out of each place, or by a belief: the underground's own know north, and know their way.
- **Found on the way** *(settled: the user's, 2026-10-28)*: a wait longer than 100 minutes was a wait of 100 ("wait 2
  hours", "wait 3 hours": the world's busy took 60,000 tenths at most, the UI's wait_of held it to that).  Now the
  long durations are in minutes, a night's sleep and more (Long waits, sleep and rest, below).

## Long waits, sleep and rest *(2026-10-28: the user's 16-bit minutes; the rest drafted for the user to review)*

The user (2026-10-28, DECISIONS.md's travel's stop and waits a night long): "waits should be able to last a full night's
sleep, so 8 hours at least.  Some spells may take longer than that to cast, so 16-bit minutes would be better."  Every
ruleset's (the fatigue rested off the referee's).

- **Long durations in minutes, as built** (the user's): a duration past 100 minutes (the 60,000 tenths that sixteen bits
  were given) is carried in minutes, to 65,535 of them (some 45 days): a wait's intent (I_WAIT's WA_MIN), the long
  task's length and part done (its mins: the bar's ten cells as before), the time told (E_TIME's).  Shorter ones keep
  their tenths, so no short action's time changed.  The clock and the ready times were 32-bit tenths already (65,535
  minutes is 39 million of them), so the world counts a long wait as any other; the general kind (the world's task_for)
  is ready for the timed work that runs longer, and a spell's casting when it comes.  The referee's fatigue rest is told
  the whole of it, in pieces (RS_EXERT's sixteen bits; it was held to 100 minutes too).
- **What's said** *(the parser's; the house's words for review)*: "wait 10 minutes", "wait an hour", "wait half an
  hour", "wait 8 hours", "wait eight hours" (the parser's numbers: figures, and one to ten), "wait 3 hours 20 minutes",
  "wait 3 hours and 20 minutes" (one duration, not two commands: the line's parts don't split at an "and" between a unit
  and a number), "wait for an hour and 5 minutes"; "sleep" and "rest" (one time, one recovery; what each perceives: Sleep and rest as built, below), a night's, eight
  hours, when no length is said *(the house's: the book's night of sleep, Skills 6)*; "short rest", "short sleep" half
  an hour *(the house's: the grammar's 30 to 60 minutes)*; "sleep 8 hours", "rest for an hour", "sleep for 2 hours" as
  said; "sleep until dawn" by the hour it believes (The time of day believed, below).
- **Told back** *(for review)*: "You wait eight hours.", "You wait three hours and twenty minutes.", "You wait half an
  hour.", "You wait an hour and five minutes."; "You settle down to sleep for eight hours." ("to rest" when rest was
  said); a round's wait, or less than a minute, "Time passes." as before.  As it goes: "Keep waiting?", "Keep
  sleeping?"; stopped, "You stop waiting.", "You stop sleeping."; "stop sleeping" and "stop resting" name a sleep, "stop
  waiting" a wait.
- **A light seen going out** (2026-11-04): one lying about or in a sconce, told once an answer however many go in a long rest.
- **Sleep and rest as built** *(the house's, but for the senses, the book's and the user's, 2026-11-03: A6)*: the same as a wait in the world but for
  their words and their lying down: the book's fatigue recovery for the time of it, a fatigue level each ten minutes of rest (Skills 6's
  Fatigue Recovery, the referee's already; a night's sleep, the book says, clears most or all of it: eight hours of it
  does), the wait broken off as any long task is (hard: a wound, a blow; soft, asked: a sound heard; its light going out no stop of a sleep, a rest's yes).
  **Asleep (LT_SLEEP) the character sees nothing and hears at half** (Skills 11's Awareness at half; the denizens' own the same: The denizens' day,
  below): the senses' look is blind as it sleeps, so nothing comes into sight, no hostile within 30 feet stops it till it's heard or a blow lands;
  its hearing in the summary at half (the UI's reach of a sound its hearing plus three: that halved); its eyes open as it wakes, what's there told.
  **A rest ("rest", "rest for an hour") stays awake**, lying down all the same (LT_WAIT's): it sees and hears as ever, a hostile in sight within 30 feet
  stopping it, another seen asked.  It lies down to sleep or rest (A sleeper lies down, above).  Hunger and thirst as it rests: Water, hunger and thirst, below (slice 4.9's).  Healing as it rests: wounds
  over time, below (slice 4.3's).
- **A long wait's speed** *(the house's, for review)*: at the real paces a wait or a sleep longer than a round runs as
  the turns pace runs it, as fast as the tasks go (an hour's wait no hour of the player's, nor a night's sleep a night),
  the pace's own back as it ends, is stopped or asks.
- **Coarser ticks while the character rests** *(the house's, for review: the game's rules allow it, the book's silent)*:
  the world's slices of the clock go on from one actor's moment to the next (none acts between: the soonest of their
  ready times), ten minutes at most (the book's rest unit), a minute at most while the light the character carries is
  within ten minutes of burning low; the lights burn the minutes of a slice at once, looked at every ten minutes (every
  minute near burning low, and at once as the rest begins or ends); the traces' clock counts each four minutes gone.
  The denizens about it, idle while it rests: a watcher's or an idler's while thirty times as long (a watcher's three to
  six minutes, an idler's one or two: roused all the same as the character changes, and every idle one as its rest
  ends), no idle wandering in its sight, one unseen and unaware a coarse move ten or twenty minutes apart; a hostile
  one, one on its scent, one alerted by a noise, one fleeing, as ever.  Those away from it (not actors) a coarse move
  five or ten minutes apart (ten times their while).
- **Those it can't reach, as it rests** *(new, for review; 2026-11-05, the delays' round)*: a hostile one standing still as
  the character rests (lying in wait, alerted by a noise) looks again no sooner than every thirty seconds, and one whose
  step was refused twice from the same square (a shut door on the scent, the way taken) tries again thirty seconds on:
  a second at a time, three robbers lying in wait or a hunter stuck on a scent kept an eight-hour rest to ten-second
  slices, 4.9G cycles, 23 minutes of the machine's, and more (the fight soak's seeds 326 and 136).  Still in the same
  scene as dq0restbusy's bandits, 1.6M to 2.9M cycles a game minute, 3.6 to 6.4 minutes of the machine's for the night (more than 37 before).  A step that
  isn't refused and a blow go as ever; the character changing rouses them as ever.
- **The figures** *(2026-10-28, the emulator, 3.6 MHz)*: "wait 8 hours" with nothing near, 20.9M cycles (some 6 seconds
  of the machine's; it was 0.6M a minute, some 81 seconds for eight hours, and held to 100 minutes); at the normal pace
  the same; in the dark by the giant rats' lair (seed 3, three rats about), 66M (18 seconds; 782M, three and a half
  minutes, before the coarser ticks); in the lit cave below with rats about (seed 21), 123M (34 seconds; 3,200M, a
  quarter of an hour, before).

## Wounds over time *(slice 4.3, 2026-10-29: the book's, and the house's where it's silent, drafted for the user to review)*

The user's (DECISIONS.md's D6: "bleeding over time, as HarnMaster has it, with first aid and bandages that stop it, and
healing over days of rest"; a helpless character, and poison, 2026-10-21: the adder's venom, resisting it, its effects
over time, antidotes).  The book's rules are the referee's (dq0harn/heal.c, on the library's harnheal.c: the Treatment
table, the healing roll, the infection's and the shock's rolls); what the house adds, and how it's built:

- **Time told the referee** *(every ruleset's)*: the world tells it the minutes gone (RS_TIME) a minute at a time while
  any body bleeds or the character's poisoned, else as the referee's next mark comes (HarnMaster's: a watch, four hours
  on from the game's start or its carrying on, not the hour of the day, which the denizens' own watches go by: The
  denizens' day, below), with what the
  character was about: at rest (standing still, waiting, asleep), about, or at vigorous work (at a run, a fight,
  strenuous work).  What came of it for the character is told as it happens ("You feel faint from the loss of
  blood.", "It spreads to your left knee."), a hurt anew or a death a hard stop of its long task; while it sleeps,
  held, and told as it wakes.  (dq0w/wtend.c's body_ask, with the lights' minute; dq0/tend.c's body_words.)
- **Bleeding, the book's** (Combat 14, Physician 3): a grievous blow's bleeder a point of blood a minute, dead past the
  Endurance, every actor alike (it was the denizens' alone, and a minute's however long a wait had been: now the
  minutes gone, and the character's too).  Told once, past half its Endurance ("You feel faint from the loss of
  blood."); the status's "weak from the loss of blood", past half "faint".  Its blood got back the book's way, a
  five-day roll.
- **First aid, the book's treatment** (Physician 2-3), the character the healer: "tend my wounds", "first aid",
  "bandage my left arm", "treat the bandit" (tend, tend to, treat, bandage, dress, bind, staunch, first aid, first aid
  to or for; a part said, its wound there; none, the most urgent: a bleeder first, then the worst not yet treated).  A
  timed work (LT_TEND: "stop tending"), its outcome at its end: a bleeder's tourniquet a minute, tried again each
  minute till it holds (the book's), +50; a dressing five minutes a level; a compress or a splint 5d6 minutes; surgery
  10d6.  The Treatment roll the referee's (Physician, unopened at its opening level as every skill is), the table's
  bonus with the treatment to hand.  *The house's*: bandages for a dressing; a knife or a dagger and bandages for
  surgery (no needle and thread in the catalogue yet: the blade and the bandages stand for them); a compress's cold
  water and a splint's stick taken as found; the tourniquet anything to hand (a strip of cloth, a belt), its +50
  always.  A dressing and surgery use a bandage up if it has one (the things' IO_USE).  The wound's healing rate after
  it (the sheet's "tended"), told plainly ("It's well tended.", "It's tended, roughly.", "It's badly done."); the book's
  one treatment roll a wound.
- **Treating itself** (Physician 2: one not in shock may treat its own minor and serious wounds, not its grievous
  ones): *the house's*, a grievous wound's bleeder it may staunch itself, the wound itself not ("You can't treat a
  wound so grave yourself.").  Not yet: the book's 5 less a day past a day's delay, an attendant's half Physician on the
  healing and shock rolls, diagnosis, cauterising.
- **Another's wounds**: one beside it, an ally (friendly or allied) or one lying senseless; any other "won't let you
  near its wounds".  Its sheet's wounds as the character's.
- **Healing, the book's** (Physician 4): a healing roll each wound every five days (the Sindarin two), its rate times
  the Endurance, its levels gone, a wound healed told ("The minor cut to your left forearm has healed."; a roll that fails told too, "... is no better.", the house's); an EE
  treatment healed at the day's end *(the house's: the book's a day, the next day's mark)*; an open wound infected
  on a critical failure, then the infection's daily roll, gone at H6, dead at H0 (the Sindarin never).  *The house's*:
  the character's five-day rolls want sixty hours' rest of the five days (the book's twelve hours a day: its waits, its
  sleep and its standing still, the fatigue's rest), else forfeit, as the book has it.  So a night's sleep heals
  little: the book's rolls come five days apart.  *(Kept: the user's, 2026-11-03.)*
- **The creatures heal the same**, every sheet at each mark, in their lairs or wherever they are, every five-day roll
  made (*the house's*: as if they rest), but never infected (*the house's*: a creature away from the character dies of
  nothing unseen).  A folk creature binds its own bleeding (*the house's*: a minute's tourniquet each minute it isn't
  fighting, a layman's Physician 30 and the +50, "The bandit binds its bleeding wound."); a beast bleeds on.  A hurt one goes home to rest: The denizens' bodies, below.  Not yet: one tending another.
- **Shock's recovery, the book's** (Physician 4): one in shock rolls each watch (four hours), its rate (H5 at first)
  times its Endurance, out of it at H6 ("You come out of shock."), dead at H0.
- **In shock, the book's** (Combat 14: it may rest, be led away, or make a half move), as built at last (Hands
  and limbs left it till shock's recovery came): *the house's*, it may look, wait, sleep, walk (at half its move,
  as before), turn, let a thing drop, call out, give an ally a short order, and the game's own commands; anything else
  refused ("You're in shock: you can do no more than rest, or walk slowly away."; to talk, "you can't string the words
  together"), first aid too (the book's: one not in shock).  (dq0/tend.c's early, before a command's things are looked
  for.)  A blow at it in shock from any but a killer held, as one at it senseless is (the referee's, RS_KILLER: nobody
  engages one helpless; a killer strikes on).
- **A creature's shock, the book's** (every character's): one knocked senseless comes round in shock if its last shock
  roll failed (the referee had dropped it), its only defence none and striking nothing, till its watch's roll brings
  it out.  *The house's*: a creature's shock never kills it (H0 held at H1).  Not yet: the minds' way with one in
  shock (it stands, or lies, doing nothing).
- **Venom, the bestiary's** (HarnWorld Bestiary 1, Snakes 2-3: an infection's course): the cave adder's bite, and the cave spider's, its kin's *(the house's, for review: the creatures' sheets)* (the heath
  adder's H3, a roll each hour at rest, a B1 bleeder at the bite, an ache, an M1, at each place it spreads to, healing
  at H6 once the venom's gone), the basilisk's (Dragons 13: H2, ten minutes), the lindworm's (Dragons 14: H3, ten
  minutes, each place it reaches numb, a limb useless, the chest or the belly reached its breath stopped).  Delivered
  by a bite that wounds (a weak one may not: its rate a point more for each point of impact under 5, past H5 none, the
  bestiary's option); its first roll at once ("The bite burns: there's venom in it."), then one each interval, at rest
  the whole, about half, vigorous a quarter; the worse spread a place nearer the heart (a critical failure two, the
  bestiary's option), the better its rate up; gone at H6 ("The venom's fire dies away: you've thrown it off."), dead at
  H0 ("The venom reaches your heart.").  The status's "poisoned, the venom in your right calf".  *The house's*: bitten
  again, the worse venom kept; the places it spreads to, the next nearer the heart, then up the body to the skull; no
  senselessness of an ache (the book's shock roll for a new injury not made of it).  An antidote: Poison, bad air and disease, below.  Not yet: stings (no stinging kind in the game yet), creatures envenomed (none fights another, no blade's
  poisoned).
- **The body's own dice** *(the house's)*: the time's work (the bleeding, a folk creature's binding, the watches' rolls)
  and a venom's rolls roll the referee's dice of their own, swapped in about them as the bearings' are (rshouse.h's
  way), so a fight's and a walk's dice roll as they did whether a body bleeds, heals or is poisoned meanwhile.
- **The null ruleset's simple way** (dq0null.c): no bleeding, no shock, no venom; first aid five minutes a wound, each
  tended once ("You clean and bind a wound."); a wound healed each seven hours' rest if it's tended, each twenty-one
  hours' if not ("A wound has healed.").

## Outfits *(2026-10-27: drafted for the user to review)*

The user's (DECISIONS.md's "Outfits, reach and things used up", and the eleven answers to the plans' questions; those
plans were working papers outside the repository, and "the plan" below in Outfits is theirs, not DENIZENS.md): each
occupation its own starting kit, the universal kit for everyone, weapons with their carriers and missiles, a shield
with Shield, bought at the book's prices with the family's wealth; random, but suiting the occupation and the skills
chosen.  The data is games/dq0/outfits.txt (its head says how it's written), the draw the world's (dq0w/outfit.c,
outdraw.c, outmake.c), the referee's the makings (RS_OUTFIT: RULESETS.md).  The book's own is Character 17 (what a
character begins with), Combat 2-4 (weapons, armour), Character 27-29 (the units), Religion 9 (a new cleric's things),
HarnWorld's price list.  What the house adds, every number the house's (and outfits.txt's every chance and weight):

- **Its light by the family's money** *(the user's, 2026-11-03: "Light source by available money"; the thresholds the house's,
  for review)*: a rich family of any class (Character 17's: a serf's 160, an unguilded one's 240), and every guilded and noble family
  (90 pennies at the least, 600) a lantern lit in the off hand and two flasks of oil, some 20 pennies; the rest (a serf's 40 or 80, a
  slave's 40, an unguilded family's 60 or 120; the null ruleset's 49) a torch lit, two spare and a flask of oil, some 3 pennies (the
  light's bought first, so at 120 a lantern, or a third spare, left no money for a bow and its arrows).  All within their money, the rest for what comes after; every family can pay for the poor's light, so none starts in the dark.  An occupation's chance of a
  lantern isn't drawn on top of the kit's.  Words as a light runs low: its last ten minutes, a lantern's and a torch's alike
  (Light and traces).
- **What starts full:** every vessel in an outfit starts with what it's for: a waterskin its four pints of water (a lair's cask
  too, "full" in outfits.txt), a flask of oil a pint, a lantern a fill, a pot of pitch, a horn of ink, a tinderbox's lightings
  (each by its burn: outmake.c, every actor's).  Not: a vial (the herbalists' to fill: nothing's made to go in one), what's found on
  a skeleton in a lair (found empty, the outfit's meaning), dq0 -K's old kit.  **Told by its heft:** a vessel stoppered shows
  no contents, so it says how full it is: "a waterskin (full)", "(empty)" in the list; "it's little worn, full." on a closer
  look; "Your waterskin is full already." to a fill, no time.  Water's counted in pints ("4 pints of water").
- **The order an outfit's bought in:** the universal kit (in the user's order: its light, as above, a tinderbox,
  three days' rations, a full waterskin, a knife in its sheath on the belt; and free, the
  book's: clothes, sandals for a serf or a slave and shoes for the rest, a backpack, a belt and its pouch, a guilded or
  noble character's papers); the occupation's needs; its first weapon (its highest weapon skill's); a tenth of what's
  left kept as money (a mercantyler's fifth: merchants carry silver); its other weapons, three in all (Combat 2: a main
  one, a reserve and a shield), each its skill's own draw; a shield with Shield; its bent's armour (none, light,
  medium: each piece by its chance); its skills' tools (a tool it has already not bought again); its likely and rare
  things; what's left joins the money, in the pouch.
- **An outfit that fits** *(the user's brief, 2026-11-03: "An outfit should fit the character's strength: leave out the heavy things first"; the numbers the house's, for review)*:
  what it starts with weighs at most what leaves it a little burdened (encumbrance 2: free pounds and just under two and a half
  times its Endurance, the referee's RS_OUTFIT, x[3]).  Past that the heaviest thing's left out, then the next, a lot's one at a time, till it fits:
  armour and weapons, a pick, a rope, spare torches, a flask.  Never its clothes and footwear, its pack, belt and what hangs from it, a container with something in
  it, its lit light, its fire-maker, its food, its last weapon (the lightest it was drawn).  Not refunded: left at home.  A carried-on kit isn't touched.  (dq0w/wtake.c's fit, part 81; dq0 -F, the tests', leaves the outfit as it's drawn: the runner gives it to every test but those with fit: true.)
- **Strictly within its money** (answer 1): a thing's price is what the appraiser would value it at, its kind's value
  times (2 plus its quality), over 6 (appraise.c's rule); a draw is among the choices it could pay for at the best make
  it might get, so the price paid never passes what's left; a poor character may go without some of the kit (the book's
  free things it always has).  A thing of its own make (a hideworker's cap, a weaponcrafter's weapons) costs half, a
  point lower.  *(For review: the free things.)*
- **The make:** a poor family's things 2 + 1d3, an average one's 3 + 1d3, a rich one's 4 + 1d3 (the game's 0-9, about
  4 middling); a unit's issue 3 + 1d3 (the Khuzdul's 4 + 1d3, the Sindarin's too), a knight's from its lord 5 + 1d2, a
  high guard's 5 + 1d3.  A blade's metal by its make (steel at 7 on, bronze at 1 or less: make_own's rule).
- **Weapons by skill** (outfits.txt's [skill ...]: the book's Treasure table's weights, narrowed to the catalogue's
  kinds): Dagger the kit's knife made a dagger, and three to five taburi in a bandolier one time in seven; Sword a
  shortsword, a broadsword, a falchion, a bastard sword or a longknife in a scabbard (a Sindarin's always the
  longknife); Club a mace or a club; Axe a handaxe, a hatchet, a battleaxe, two shorkana (a Viking's mostly) or a
  sickle; Spear a spear or a staff, in hand; Polearm a poleaxe; Bow a shortbow, a longbow, a crossbow (a Khuzan's
  mostly) or a hartbow (a Sindarin's always), slung, with a quiver (a bolt case) of 12 to 24 arrows (bolts); Sling a
  sling and a pouch of 10 to 20 sling stones (free: the book's plentiful); Net, Whip theirs; Shield a round shield, a
  buckler or a kite shield (a noble's a knight shield or a kite shield as often, an Imperial's a tower shield), slung
  on the back.  A weapon that won't hang from the belt is held, or in the pack.
- **Armour by bent:** none, a leather cap one time in ten; light, a leather cap or a kurbul helm, a leather vest,
  gauntlets, knee boots in the shoes' place, each by its chance; medium, a helm, a gambeson or a leather tunic, a ring
  byrnie (a rich family's a mail byrnie), knee boots, gauntlets.  An occupation's bent is outfits.txt's (most none; the
  gladiator, the mercantyler, the pilot, the thief, the weaponcrafter light; the herald medium).
- **The skills' tools:** each open skill's by its chance (Drawing paper and charcoal or a pencil, in the pouch, handy,
  and chalk; Script, Inkcraft and Law parchment, a quill and a horn of ink; Lockcraft lockpicks in the pouch; Climbing
  and Seamanship rope, a grapple; Mining a pick, a shovel, candles; Masonry a chisel and a hammer; the smiths a hammer;
  Woodcraft and Timbercraft a hatchet; Physician bandages and herbs; the herbalists herbs and a vial; Survival and the
  others a sack, rope).  *(Not drawn by whether the skill's the occupation's own or an option point's, as the plan
  had it: the referee's makings don't say which.)*
- **Soldiers** (answer 3): the unit's issue free, its weapons and its troop type's armour (Character 29), no weapon
  skill drawn besides; the family's wealth for the kit and the rest.  **Knights** (answer 4): the lance and the horse
  left at the dungeon's mouth; its lord's gift, the sword, the axe or mace, the dagger, the shield, the armour; a
  surcoat, a ring, a lantern by chance.  **Clerics** (answer 5): the book's things free (a robe, a ring of its rank,
  boots two times in five, a dagger or a staff), the universal kit its temple's, the fighting churches' weapons and
  shields (Agrik's mace, sickle and round shield; Larani's sword and round shield; Sarajin's handaxe or battleaxe and
  round shield); its money 6d6 pennies (Religion 9), its skills' tools bought with it; each church's marks
  (Morgath's mask, Naveh's two daggers, Larani's surcoat for a robe, Peoni's staff).
- **The money:** what's left, all of it, silver pennies in the pouch (piles of 250 at most, a lot's count a byte,
  told as one: "600 pennies"), its odd farthings farthings (cut pennies: Money, below).  *(For review.)*
- **Placing:** worn; in a hand (the main one; the torch the off one); hung from the belt, in its carrier (a sword in
  its scabbard, a knife in its sheath); in the pouch (mapping things and lockpicks: handy: Reach, above); in the
  pack; what won't go where it's meant in the pack, what won't go in the pack not made.
- **Its own die:** the referee's seed for the draw (the character's attributes and its name hashed), so the same
  character in the same game draws the same outfit; the world's dice untouched (each thing made its own with its make
  already drawn).
- **The null ruleset's** the universal kit only, with 49 pennies to buy it (some money left).
- **dq0 -K** (the tests'): the kit as it was before the outfits, no outfit drawn (and the denizens with none of
  theirs): the tests' dice as they were.
- **A carrier keeps its own family's things alone** (the plans' rule: a kind's family, a scabbard a sword's, a
  sheath a knife's, a quiver arrows', a bolt case bolts', a bandolier throwing knives'), **missiles weigh by the ten**
  (arrows, bolts and stones a tenth of a pound each, the book's), **a belt has six slots**.  *(For review.)*
- **The book's prices**, in farthings, for every kind it has (Money, below: HarnWorld's price list and Combat 3-4),
  the house's by its nearest for the rest (kinds.c marks them).

### The denizens' gear *(2026-10-27: drafted for the user to review)*

The denizens' plan's phase 1 (the user's answers 6, 7 and 8): each denizen's weapons, armour and clothes things
of the item store, drawn by the same code as the character's outfit, from outfits.txt's [role KEY] sections (the
kinds' keys: kobold, kobold lord, bandit, bandit lord, cultist, high priest, skeleton, wight, shade; and the Gargun,
the Khuzdul, the Sindarin and men's, for the kinds the later phases bring).  What the house adds:

- **Made as its lair's placed** (answer 6: weapons and armour at its creation; the purses and the trinkets later):
  every line of its role given, on a die of its own (its stratum's seed and its number: the same things each time,
  the world's dice untouched); filed with its lair's home (the store's chunk there: items.h's IW_DEN), not seen
  lying there; that chunk kept in the store while the denizen's an actor.  Folk and the dead only (their kinds'
  natures); a beast has none.
- **Its blow by the weapon it holds** (gear.c's rows, the character's): its kind's own weapon (the bestiary's) at the
  bestiary's mastery; another the better or the worse by their attack classes (a bandit's shortsword five less than
  its sword), another skill's ten less; nothing in its hands its fists, ten less (as a weapon let fall).  A beast's
  as before.
- **Its armour the pieces it wears** (answer 7: a folk denizen's fixed numbers gone): each piece's protection where it
  covers (a poor one a point less, a fine one a point more), as the character's; the dead their own (bones, dead
  flesh) or their pieces, the better; a beast's hide as before.
- **Unready** (answer 8): a denizen at its ease keeps its weapon sheathed or hung from its belt (a spear, a staff in
  hand); about to strike with its hands empty it draws the best on its belt first, its round's (the book's grope),
  told ("The bandit draws its shortsword.").
- **A weapon let fall the real one**: its own weapon, fumbled, the same thing on the floor, taken up again the same
  (a sword the character takes up is the bandit's own).
- **Dead, its things on its body**: what it wore and carried on its body (a body and bones are surfaces: "look at the
  body", "take the vest from the body"), what it held let fall where it lies.  **A thing that's worn (the catalogue's K_WEAR) taken off a body or bones
  takes twice its taking-off time more** (ten seconds: the user's, 2026-11-03, A15), so stripping a body is a risk with others near; an ally's
  errand there is a round's work, ten seconds, as it stands.
- **A shield held blocks** *(2026-10-29, drafted for the user to review)*: a denizen with a shield in hand blocks with
  it (the book's Block, Combat 9) when that's the best of its three defences, once a round as the character's
  defences are; else it counterstrikes or dodges as before (a weapon alone: as before).  Its block's mastery is its
  kind's own weapon mastery (the bestiary's: the book gives a creature one figure for its fighting) and the shield's
  defence class (Combat 3: a buckler 15, a round or a knight shield 20, a kite or a tower shield 25), a shield none the
  worse in the off hand.  Told: "You strike at the bandit with your sword; it fends off the blow with its buckler."  A
  fist that meets it is struck by the shield (its blunt impact), not by the weapon the denizen holds.  A shield's block
  fumbled lets nothing go (strapped on: the book's fumble roll is the easier for it, Skills 21; the house's, no roll).
  A shield slung on its back is taken in hand as it fights (its weapon in one hand, the other free), its round's grope,
  as a weapon's drawn from its belt ("The bandit unslings its round shield.").  The character's block says what blocked:
  "you fend it off with your shield" (it said the weapon it strikes with).  The null ruleset's simple way: a shield
  held, either side's, makes a blow at its holder two steps the harder, once a round; a blow that would have hit but for
  it is blocked, told so.  (fight.c's defence; gear.c's den_side, pc_defence; wlimbs.c's den_draw; dq0null.c's attack.)
  Against missiles: Missiles, above (a shield's block at its full block).
- **dq0 -K** (the tests'): no denizen's things, the bestiary's as before.

### The denizens' belongings and valuables *(2026-10-27: the plan's phases 2 and 3, drafted for the user to review)*

The denizens' plan's phases 2 and 3 (the user's answers 6, 9 and 10): what a denizen owns but
doesn't carry, where it sleeps, its community's stores, its leader's hoard, its purse and its valuables, all things of
the item store, drawn by the same code from outfits.txt.  What the house adds:

- **Where they sleep:** a lair's home is the room of its chunk its kind likes best (kinds.txt's home: bandits quarters,
  barracks or a guardroom; their lord a solar, quarters or a hall; cultists a cell, quarters or a chapel; their priest
  a chapel; the ghouls, the skeletons, the wights and the shade a crypt; beasts a cave; kobolds anywhere), one of them
  by the dice, else any room as before.  Each denizen's bed is a square of open floor in that room, its own (a
  blanket, a heap of straw, a lord's furs); what it keeps there lies on it, or hidden in it (a kobold's stash one time
  in four, a few pennies or a bright pebble: found only by searching).
- **Kept by trust** (kinds.txt's keeps): all on the body (beasts, the dead), open at its bed (the cultists: a candle),
  hidden at its bed (a kobold's stash; their takings their lord's), in a locked box at its bed (a bandit one time in
  three; the bandit lord's and the high priest's always: its savings, as many pennies again as its purse's dice, the
  house's), given to its leader.
- **Keys that fit** (answer 10): a key made with a box locks it, and opens its lock alone, with no test, in three
  seconds ("You unlock the chest with its key."; "lock the chest" locks it again); another key doesn't fit ("Your key
  doesn't fit its lock."), and a lock without its key is picked or forced as before.  A box's key is in its owner's
  pouch, a hoard's on its leader.  The rule is every making's: a key made
  after a box locks it (dq0 -k's "chest,iron key", the tests').  **A found key** (the user's A17, "fit something nearby"; *(new, for review)* how): one the maker lays in a chunk fits a lock in that same chunk, so nothing past it need be made and the same chunk's always the same (no dice: dq0m/sky.c's list_over, as its things are listed): the chunk's next locked door that has a mechanical lock (that door's own number, a door's key's), else the next chest in its list, locked to fit it; with no lock in the chunk, **no key's put down**: none made a lock for it.  A locked door whose lair's key is on a denizen has two keys.  A key an outfit gives with no box (a locksmith's stock) fits nothing still.  A door's keys:
  Denizens and doors, below.
- **The shared stores** (each lair's, with its first denizen's: outfits.txt's [lair KEY]): a larder (a sack or a basket
  of 2 to 12 days' rations), the bandits' water (a cask, full, one time in two; their lord's always), spare arms (a sack
  with a spear and a club, one bandits' lair in three), the cultists' candles.  A thing in another in a line (@in) is
  in the line's first thing that isn't (a chest's pennies, its crowns, its goblet all in the chest: 2026-10-29; each
  was in the one just before it, the goblet in the jewels in the crowns in the pennies).
- **The beasts' dens and the dead's lairs** *(2026-10-29, drafted for the user to review: the denizens' plan's)*: made with the lair's first denizen as it's placed (as the character first comes near), then kept, as a folk lair's things are; lying
  on its home room's floor.  A giant rats' nest a heap of straw and bones; a cave spider's bones; a cave bear's den
  straw, bones, a skull one time in two; a basilisk's bones; a lindworm's bones and a skull; and a past victim's remains
  now and then (a spider's or a bear's one time in four, a basilisk's one in three, a lindworm's one in two): bones with
  an explorer's things on them (a backpack, a torch, a rope, a waterskin, a few pennies; a tunic, a sword, a pouch; a
  cloak, a pick, a lantern, a sack; a lindworm's victim in mail with a sword and a shield).  The dead's: a ghoul's nest
  bones and one to three skulls, the arms it plundered (a sword, broken or whole, a spear, a mail byrnie, a helm, a
  shield: one seven times in ten, another four in ten) and its grave goods (one to four jewels, Treasure 5's, three
  times in five); a skeleton's crypt a few pennies (2d6, two times in five), candle stubs (three in ten), a jewel (one
  in five); a wight's bones, pennies (3d6, one in two), a jewel or two (four in ten); a shade's tomb its grave goods
  unrotted (one to three jewels, 20 to 100 pennies, a parchment one time in three: no book in the catalogue).  What's
  found there is in the condition Treasure 1's Found column gives (lost or left: broken 01-20, poor 21-60, fair 61-80,
  good 81-95, as new 96-00), silver and gold 30 the better (as the maker's found things), and the dead's of old 5 the
  worse a century of their stratum's age; a shade's as new (the Shadow keeps them).  The dead's coins are old: struck by
  the people of their stratum's oldest era, that mint whole (HISTORY.md's mints: "Their stamps: all of ..., old"); none
  such (no era, or one still held), today's.  (outfits.txt's [lair rat] ...; its found and old; outdraw.c, outfit.c,
  outmake.c; dq0d.c's place: a beast's lair's things now too.)  *Not yet:* a bat's roost (guano: no kind), an adder's; a
  victim's things aged by how long ago (Phase 4's explorers); zombies (no kind yet); the leavings owned by the lair's
  faction as a folk lair's stores are (ownership's not used yet).
- **The hoard:** a faction's leader's lair keeps it (the kobold lord's, the bandit lord's, the high priest's; of the peoples
  the orc king's, the dwarf elder's, the captain's): a chest of 20 to 200 pennies, a few crowns (the bandit lord's, the
  high priest's, the elder's, the captain's) and jewels (Treasure 5's, drawn), locked (the kobold lord's one time in two,
  the orc king's never), its key in the leader's pouch.  (The book has no hoard tables: its possessions
  tables serve as guides, Treasure 1.)
- **The robbers' takings:** what a robber takes goes into its faction's hoard if its leader's lair is placed on the
  stratum (the robber's kind's faction, the stand-in's: its takings in the chest, the chest as it was); else hidden at
  its own home if its kind hides its things; else on its home's square, as before.
- **Personal things made when first met** (answer 6): as a denizen's first made an actor, its purse and its valuables
  by the book's Personal Possessions (Treasure 2-3: Money, below; outfits.txt's [purse KEY], else its standing's
  [standing N]), then kept.  A box at its bed is a personal thing too (made then, with its key).
- **Coins for all folk** (answer 9): silver pennies everyone's, by its category's chance and dice (a bandit's 1d6 three
  times in four, a lord's 7d6); a gold crown only by its gold chance (the user's, 2026-10-28: not every dwarf's).
- **Valuables:** the new kinds (crowns, a brooch, a necklace, a pendant, a goblet, a gem, a talisman, dice) and the
  ring; a jewel's item and material drawn by Treasure 5's tables (outfits.txt's jewel), worth by its material (Money,
  below).
- **Worn beneath its clothes** (DUNGEON.md's concealment): a purse under a tunic, a ring on a cord (a bandit's one time
  in seven, a lord's purse more often): not told by a look at the body, nor taken; found by searching it (its minute),
  then as any other thing on it.
- **The kinds past seven bits:** the belongings' and valuables' kinds are numbered 128 on (items.h's KI_HIGH, NKINDS
  206), and since 2026-11-03 a kind's number is a whole byte everywhere: lit, struck with and named as any other.
- *For review:* every chance and number in outfits.txt's [lair] sections, the boxes' savings, the talismans and dice
  (the book's own the purses' silver, gold and jewels: Money, below); the home rooms by
  kind; the bed, a blanket for every folk denizen (the plan's straw for kobolds one in two); a box's contents made as
  it's first met rather than as its lair's placed.

### What's seen of an actor *(2026-10-27: the user's three levels, drafted for the user to review)*

The user's (DECISIONS.md's, 2026-10-27): "the game say general things when looking, more detail when examining,
including basic quality of visible equipment when assessing ... If assessing at a distance, eyesight should dictate
quality/granularity of any assessment/examination based on viewing distance".  One rule for every viewer (the
character's side built):

- **A look** says how it seems in general: "A bandit, armed and armoured, is ahead to your left, 10 feet away."
  (armed: a weapon in hand or at its belt; armoured: armour worn, not clothes), within one square more than the eyes'
  sight (the senses' eye, 0-9: its name's made out within three more).
- **Examining** ("examine the bandit") tells what it holds, wears and has at its belt, each thing made out within its
  kind's seen squares and half the eyes' sight more (a thing's look from afar, as appraise.c's): "The bandit holds a
  spear. It wears clothes, shoes and a leather vest. At its belt: a pouch, a knife in a sheath and a shortsword in a
  scabbard."; past that, "There's more you can't make out from here."
- **Assessing** ("assess the bandit") adds each one's make and condition, within half that distance: "a fine
  shortsword", "a sound leather vest, cracked" (by its material: iron and steel rusted, leather cracked, cloth frayed,
  wood rotten, silver and bronze tarnished; badly, under four tenths of its durability left).  By eye alone, no test
  (the book's appraisal tests are for a thing in hand: assessing a thing itself still is).
- **Never told by looking:** what's beneath its clothes, in its pouch, in its pack.
- *For review:* the distances; assessing by eye with no test; the words.

## Money *(2026-10-28: the book's, and the house's where it's silent, drafted for the user to review)*

The user (DECISIONS.md's money): "Is the money system modeled after the one in the HM rules?  If not, it should be.",
then the dwarves' crowns, each coin's provenance, a pile of many mintings.  The book's: HarnWorld 23-26 (the coins,
the reckoning, the price list, the incomes), HarnMaster's Treasure 1-7 (the condition table, the Personal
Possessions, the jewelry, the gems), Combat 3-4 (the weapons' and armour's prices), Character 17 (the family wealth).
What the house adds, for review:

- **Worth in farthings** (kinds.c's value; the outfits' prices and budgets; the family wealth four times its pennies);
  the appraisal's words: under a penny "a farthing", "half a penny", "three farthings"; under a pound "about N
  pennies" (from 20 on to the nearest five); a pound or more "about 2 pounds 6 shillings" (to the nearest shilling).
- **Farthings** (the user's, 2026-10-28: "Please keep farthings as they might be useful for denizens"): a coin of its
  own, a silver penny cut in four (HarnWorld 23), worth a farthing, too light to weigh (a quarter dram, under the
  weight's 64th), in piles with mints as pennies are; the character's odd farthings its purse's; the poor's purses
  some besides the book's pence (the house's: a tribe's, a kobold lord's, a Gargun's, a Khuzan miner's 1d4 three times
  in ten, a vagrant's 1d6 three in ten, a bandit's 1d4 one in five; a kobold's stash its pennies or its farthings).
  "count X" an examining ("count the pennies": their number, their stamps); "count the money", "the coins" the
  pennies'.
- **A coin's weight by the dram** (items.h's LOT_WT: a pile's count over 64 quarter pounds, to the nearest: under 32
  pennies nothing; a crown an ounce).
- **The house's prices** (the book has no such thing; by its nearest): a torch 1d, a backpack 24d, rations 1.5d a
  day (priced as two cold meals; eaten as three: Water, hunger and thirst, below), a shovel 8d (a pick's), lockpicks
  40d, a chisel 6d (a hammer's), rope 12d, a chest 20d, a
  broken sword 10d, a cloak 24d, a basket 2d, a door's bar 4d, a pane of glass 30d, iron bars 10d, hobnail boots 120d,
  a flask of oil 2d (a pint of the book's lamp oil and its flask), a pot of pitch 2d, a sconce 4d, paper 2d, a stick
  of charcoal and of chalk a farthing, a pencil 3d, a horn of ink 1d (the book's 3d a quart), a map sheet 2d, a sheath
  6d, a bandolier 12d, a bolt case 8d (a quiver's), bolts as arrows (1.25d), a hide jerkin 60d, a shroud 4d, a mask
  6d, bandages 1d, furs 30d (an ermine's or a sable's hide), a cask 12d (a hogshead 15d), a talisman 1d, dice 2d.
  The book's, read from its own table: a candle 1f, a table 36d, a chair 12d, a blanket 48d, a waterskin 10d (its
  wineskin), parchment 2d, a quill 1d, an iron pot 10d (its cauldron), arrows 15d a dozen.
- **Jewels** (Treasure 5): a kind written in silver, its worth the book's middle weight at 60d an ounce (a ring 28d, a
  pendant 47d, a brooch 34d, a necklace 540d, a goblet, half a pound of plate, 480d); by its material, Treasure 5's
  values: twenty times in gold, eight in stone (jade), a tenth in glass, a sixtieth in the base ones (bronze, pewter,
  bone and horn).  The items narrowed to the catalogue's (a bracelet or an anklet a necklace, a comb or earrings a
  brooch), the materials too (ivory with bone, amber with silver, mithril with gold); no adornments, inlays or
  engravings yet.  A gem (Treasure 7) a cut common one of middling size, 400d.
- **The categories** (Treasure 2-3: the book's own for each, but): the kobolds the Gargun's Hunters/Tribesmen, their
  lord the same; a bandit lord both a Brigand/Outlaw and a Warrior/Noble, the higher chance and dice (the book's rule
  for two); a cultist, an outlawed cult's lay folk, a Brigand/Outlaw; a Khuzan miner Hunters/Tribesmen (working folk:
  the book has no craftsman's); a kind's standing for one with no purse of its own: 1 Hunters/Tribesmen, 2
  Vagrant/Beggar, 3 Mercantyler, 4 Warrior/Noble.  Its notes, trade goods, written works, potions and artifacts left
  out; a bandit's box at its bed keeping as many pennies again as its purse's dice; a talisman, dice.
- **Condition** (Treasure 1's table, its life's tenths): broken or decayed 0-1, poor 2-3, fair 4-6, good 7-9,
  excellent 10 (as new); a denizen's things by the Person column, what lies lost or left by the Found column, silver and
  gold 30 more on the roll (slow to decay, the book's word); a portal's part set down as before (up to six tenths
  worn); the character's own new.  The appraisal's life left now its tenths of the whole (as good as new at its whole:
  it was a tenth short before).
- **Mints** (theme.h's struct tmint): themegen's today's four, from the seed (the region's own realm and two
  neighbours named in the men's tongue, one of them hard to pass one game in two; the dwarves' hold in theirs, gold,
  at par), and an era's people's of old for each but the kobolds and the hill-men; each one's word its name with an,
  or n after a vowel ("Velmoran"), an era's its people's name.  A hand-written theme gives them with its region, or
  has none (its coins of no mint told).
- **A pile's mix:** four groups at most, each a mint and its share in fifteenths (a byte each of the pile's size,
  thick and fuel); a part taken the same mix (proportional); two piles merged by their coins counted, the four
  greatest kept, the rest folded into the greatest.  Today's coins as they're first wanted: a fifth of the pile, ten
  to thirty in a hundred by its numbers (one coin one time in five), another of today's mints' (one, by its numbers),
  the rest the region's own; a crown the gold mint's.  Coins lying in an old era's works its people's, whole, a
  gold-striking people's one time in four crowns, an eighth as many (the maker's, by the square: no die).
- **Their worth as a local takes it:** the region's own and the dwarves' at par, another realm's 85 in a hundred (the
  book's 10-20% off), a hard one's half, an old era's three quarters (its silver's: the book's coins 75% metal).
- **Told:** a closer look at a pile within reach, its stamps by how much of it each group is ("all", "most", "many",
  "some", "a few"), an era's "(old)"; the appraisal its worth so.

## Sound, the air and moving quietly *(slices 4.5 and 4.6, 2026-10-29: drafted for the user to review)*

The plan's slices (DUNGEON.md's still to come; D15; the user's 2026-10-21 on scents, the air and hunters downwind):
sounds that go round corners and through doors as a pulse over the rooms and openings, a background that masks, the
character's steps heard by its pace and its armour, the denizens hearing by it; a draught carrying smells downwind;
the body's positions; moving quietly by Stealth, the denizens' stealth too.  The book's own: Listening (Skills 22: a
hearing test at a multiple of Hearing by the noise's loudness, background noise interfering), Stealth (Skills 9:
tested to move without being detected, its EML by the distance and the cover, any failure detection), Awareness
(Skills 11), a prone man's and a crouching man's height (Combat 16's target sizes).  The game hears by reach, not by a
roll (SOUND.md's rule, D9: tuned by ear later), and tests Stealth where the book does.  What the house adds:

- **How far a sound goes: the sound layer** *(every ruleset's)*: the senses work out a pulse from the character's
  square over the squares round it, 20 each way (dq0p/sound.c, pulse.s: hub.h's SOUND_AT), the way a sound goes to
  each: round corners (the eight ways; a corner cut only past a square that isn't rock), through an open door, a
  doorway, a portcullis, a grating, an arrow slit; a shut door or a closed window two squares more (SOUND.md's
  figure, as before), a room's wall four a square of it (SOUND.md's), never the rock a passage is cut in; nothing
  further than 58 squares of it.  The same way both ways: what the denizens hear of the character, and what it hears
  of them (the world's E_DEN, what lies between it and the character, from it: wbody.c's between).  Past the 20
  squares, or not reached, not heard (no step is heard so far).  Why: the line through the rock it was let a rat hear
  a man through thirty feet of stone; a pulse is the plan's, and the senses' to keep as the light layer is.
- **How loud** *(every ruleset's, for review)*: the character's steps at a slow walk or a walk 2, a jog 3, a run 4, a
  sprint 5, hopping 3; metal armour worn (six pounds or more of iron, steel or bronze: mail, plate) a level more as
  it moves; asleep 1; anything else it does 2 (a wait too: its gear, its breath, as the denizens heard it before); a
  dig's and a battering's noises as they were (the world's E_NOISE).  The denizens' steps 1, a dash 2, a noise of
  their own 3 (every eighth step), as they were.  (The world's as a move starts creeping, crawling, hopping, not
  standing or its legs not whole: dq0w.c's part 50, step_loud, the hub's pc_noise; else the senses' by its pace,
  pc_going; the armour weighed as what's worn changes, the hub's pc_clank.)  Why: the book's Listening table has
  breathing soft and footsteps two levels louder; the walk's 2 keeps the reach the denizens heard the character at
  before (their kind's hearing).
- **Heard** *(every ruleset's)*: a sound of loudness L, less the background where the hearer is (below), heard within
  L times the hearer's hearing over two squares the way it goes (a denizen's its kind's, kinds.txt's; the
  character's (hearing + 3) / 2, as before), a little further downwind and less upwind ((32 + w) / 32, w the
  draught's figure below: a fifth at most).  The denizens hear the character as it's busy (sense.c's), and its
  noises as they come (dq0d.c's hear: the senses asked of each, its loudness); the character hears them as it did
  (main.c's heard, the sound task's), the way the layer says.
- **A noise drawing them** *(for review)*: a hostile denizen, not a shy one, that heard the character (or saw it, or
  smelt it) and has lost it goes on towards where it last sensed it, a step at a time, for its next eight decisions
  (DRAWN_N: dq0d.c's draw_on), unless it senses it again, as far as it would follow a scent (one that
  keeps to its lair a step or two from it); a shy one keeps away as ever.  So a bandit that hears a door
  opened comes to see, by its way round walls and doors if it needs one (ways found, below).
- **The background** *(every ruleset's, for review)*: water in a well or a cistern 2, the wind at a shaft or the way
  out 1 (2 in a draught of 2 or more; none in still air), each a level less every four squares off; a crowd about
  the character (three or more within three squares of it, awake: the character's alone, the denizens hear past
  their own) 1; the loudest of them where the hearer stands, taken off every sound's
  loudness (gone at none).  A stream's running water is no source yet.  Told as a listen ends: "You hear nothing but water
  trickling somewhere near.", or after what it heard, "Behind it all, the wind in a shaft near by."  Not yet: a
  fight's din as a background that masks (its blows are heard by the denizens near, as loud as 5: Morale's a fall known).
- **Moving quietly** *(the book's Stealth, D15; the rest for review)*: creeping ("creep", "sneak", "sneak north",
  "tiptoe", "quietly"; a crawl, on two good legs the same, prone the lower; a creep crouched) tests Stealth as it
  starts (the referee's, the world's slot), again
  as its body's position or its stratum changes or five minutes on (QUIET_T), and again whenever it starts after moving at any other pace; the grade sets its steps' loudness: a
  critical success 0 (no one hears it), a marginal one 1, a marginal failure 2 (as a walk), a critical failure 3.
  Low the easier: crouched a step, prone two (the book's cover).  The grade is secret, as the GM's roll; nothing's
  told of it.  The null ruleset's flat odds the same way.  The test has dice of its own (rshouse.h's
  RS_HOUSE_QUIET, as the
  bearings' have), so the game's other tests roll as they did whether anyone creeps or stalks.
- **The denizens' stealth** *(for review)*: a hostile hunter by smell (its nose 6 or more, kinds.txt's: K_NOSE)
  coming at the character stalks it: its Stealth tested as it begins (the denizens' slot, RS_TEST with the actor:
  HarnMaster's referee by its sheet, the bestiary having no Stealth of its own, its Agility times 5; the book's
  Stealth is Agility, Hearing and Will, Skills 9), its grade its steps' loudness, a success 0, a marginal failure 1, a
  critical failure 2, and no noise of its own while it stalks (dq0d.c's decide).
- **The draught** *(for review)*: a stratum's air goes from its way up (the way out, on the top stratum) towards its
  way down, the heading between them in plan, strength 1 with both, a level more for the way out to the open air,
  another for a shaft (3 at most); with no way down, or none up, still air (the senses', the hub's air_h, air_s).
  Its figure for a way from the character: straight downwind twice its strength, within 45 degrees of it once, across
  it none, upwind the same less.  Why: the plan's draughts through the dungeon, made one figure a stratum till the
  surface's wind comes.
- **Smelt on the air** *(for review)*: a denizen smells the character within a third of its nose in still air (slice
  4.1's, as built), times (2 + w) / 2 with the draught: downwind in a draught of 2, three times as far, a keen nose's
  some 7 or 8 squares where a walk's heard 8 and a creep's 0 to 4; upwind less, or nothing; along the way the air goes
  (no shut door, no closed window nor wall on it: the sound layer's).  So a hunter downwind knows of a creeping
  character before it hears or sees it (the user's, 2026-10-21).  "sniff" tells the draught first: "You feel a faint
  draught from behind you to your left.", "The air is still."  Not yet: the character's own sniff reaching further
  upwind (wtrace.c's sniff: its overlay has no room), a trail followed downwind (wtrscent.c's).
- **The body's positions** *(for review)*: "crouch" (or "squat"), "kneel", "sit", "lie down", "stand" or "get up":
  a second each (POSE_T), up from prone the book's Rise (a round); there already, told so, no time ("You are
  crouching already."); no leg to stand on, none but prone.  Crouched it walks low, a slow walk's pace at most (twice
  a walk's time a square); kneeling or sitting, a move gets up first (a second, "You get to your feet."); prone it
  crawls (slice 4.1b's).  Its eyes 5 feet above its floor standing, 3 crouched, kneeling or sitting, 1 prone (the
  look's, what's seen over a ledge or a rise: the book's prone man some 2 feet tall, a crouching man 4, Combat 16).
  Seen low with no light of its own three quarters as far (prone half) by a denizen.  In a fight, kneeling or sitting
  is as prone (the easier struck, its turn spent getting up); crouched as standing.  The status line says it
  (CROUCHED, KNEELING, SITTING; PRONE as before).  A missile's mark the same: low, a two-foot mark (Missiles, above);
  crouched, as standing.  Not yet: a crouched mark's own height; the denizens' own positions but prone (Ways found, positions, bodies
  found's not yet, below).
- **What it costs** *(2026-10-29, the emulator)*: the pulse some 0.2-0.35M cycles (its fill a chunk's rectangle at a
  time in assembly; its search 0.04-0.12M), worked out only as a denizen's senses or a wait's end find it out of date
  (the character moved, a door or a square changed): once a denizens' turn at most, nothing for a turn or a look in
  place; the world's what lies between from a layer a step or two old (from where it was worked out, and those squares
  more).  The senses' answers to the denizens their part 6, the pulse their part 5 (both in their overlay 4); the
  world's step loudness at once by the senses for a plain walk (no part swapped as a move starts: dq0answer's times as
  they were).

## The peoples *(2026-10-29: Phase 4's, the user's answer 11; drafted for the user to review)*

The user's (DECISIONS.md's outfits, answer 11): "Please use the denizens from the HarnWorld Bestiary, and include
Khuzdul (dwarves), the Sindarin (elves), Humans, and Gargun (or all types)."  Twelve kinds more (dq0d/kinds.txt, 29 in
all): the five Gargun sub-species and their king, the Khuzdul's warrior, miner and elder, a Sindarin warden, men-at-arms
and their captain.  The books' where they speak: HarnMaster 3.5's Gargun 1-8 (each sub-species' attributes, skills,
natural armour, equipment, group, habitat, ways), Character 7-8 (the species' attribute modifiers), HarnWorld's
Azadmere, Evael and Gargun (where they live, their ways).  What the house adds, for review:

- **The names:** the Gargun by the book's English names, "small orc" (Gargu-arak), "brown orc" (hyeka), "black orc"
  (khanu), "white orc" (kyani), "red orc" (viasal), and an "orc king"; "dwarf", "dwarf miner", "dwarf elder"; "elf";
  "soldier", "captain".  More than one: "dwarves", "elves" (dens.c's plural, an f made ves).
- **Their sheets** (dq0harn/beasts.c): the Gargun's the book's (attributes, Initiative, Dodge, the usual weapon's
  mastery); their fur and hide the book's natural armour (B4 E3 P1 F3), the better of it and what they wear at each
  place (not folk's: the book has their armour over it, the house takes the better, as the dead's); the black orc and
  the king sooner berserk than fled (the book's: a critical failure of
  courage berserk six in ten, desperate a quarter, cautious the rest, never broken); the king the house's, a black orc's
  best (Strength 15, Initiative 80, its mang 70).  The Khuzdul and the Sindarin a man's 11 with the species' modifiers;
  their trade's mastery the house's (a warrior's 65, a miner's 50, an elder's 70, a warden's 65); men-at-arms and a
  captain as the bandit and its lord.  Their weights by their build (a dwarf some 150 lb).
- **Their senses and ways** (kinds.txt): the Gargun nocturnal, a brown orc keeping watches (they keep to the dark),
  their night vision seeing nearly as far in the dark as by a light (40 feet to 50; the book's: no darkness penalty),
  the small orc's hearing and nose keen (a hunter by
  smell, following a trail); the dwarves', the elf's and the soldiers' sight a man's, by their own light (they keep
  their rooms lit, as the bandits do), the elf's the keener (Eyesight 2 more: 60 feet).  Their paces: a small orc's
  quick (the book's Move 13: 8 tenths a square), a dwarf's short-legged (11 tenths a square), an elf's light (9).
- **Their minds** (DENIZENS.md's factions and attitudes, what they allow now): the Gargun hostile to everyone, the
  dwarves above all (the book's, a hatred past reason); a brown orc a robber that hides its takings at its bed (its mates not trusted: the book's squabbling, thieving tribes), a black and a red orc killers (the book's: they eat their
  prisoners), their king a robber, the tribe's spoils its hoard's, a small orc a shy hunter, a white orc neither (the book's least violent, its honour: a foe brought
  down left be), and the easiest swayed (one step), a black orc the hardest (three against).  The dwarves wary, keeping
  to their halls (a lair intruded on, the attitude a step down), cunning (hard to fool), stubborn (a step against);
  the elf wary and aloof, keeping its distance (shy), two steps against swaying; the soldiers unfriendly, keeping to
  their posts.  A faction's kin dismayed as one falls (each of its kind and, since this, of its faction among the
  actors, its courage tested two steps worse), and four worse as its leader falls (the king above all: the book's
  Gargun submit only to a strength they've seen); since 2026-10-30 the field and the bonds instead (morale, below: a
  friend's fall the book's bad situation, a leader's its influence and bond removed).
- **Their tongues** (factions.txt): the dwarves' khuzdul, the elves' sindarin, the soldiers' common; the Gargun one,
  "gargun" (the book's: five tongues and a sign language every Gargun knows; the house's one for them all, the sign
  language's).  A word to one wants its tongue (a character's file's "language khuzdul=30"), or a leader's common (Words reach a creature, above).
- **Where they live** (themegen's histgen, its peoples): the dwarves, the elves and the men's soldiers where an era of
  their people holds its works still (the last era's "it is theirs still"), on that era's strata; the Gargun in
  mountains, hills or forest where an era of the dwarves lost its works and no dwarves hold theirs (the book's colonies
  in abandoned Khuzdul halls), a tribe of white orcs in cold mountains, else by the dice brown orcs ruled by black ones,
  red orcs alone (too fierce for the black orcs' rule), or brown and small orcs; their king their leader, the dwarves'
  elder theirs, the soldiers' captain theirs; the peoples' factions before the others (a theme places seven). 
  The story's who has it now: a people that holds it first.
- **Their gear** (outfits.txt): the Gargun's the book's Equipment lists, their own make (crude to middling; a white
  orc's finer, the book's: they make finer things), the cloth tunic the catalogue's clothes, a cowl a leather cap, a
  plate halfhelm an iron helm; the mankar and the mang their own weapons (Combat 3's (G): the catalogue's, with their rows
  in the referee's weapons; named in "strike with the mang", it's what strikes, as any thing named); a white orc's
  silver now and then (the book's: they treasure it above all); their nests' straw, a small, brown or red orc's stash
  at its
  bed; their lairs the bones of what they've eaten, their king's hoard.  The dwarves' fine (a warrior's mail, an
  axe or a warhammer, a shield; a miner's pick, hammer, chisel and lantern), their stores and their elder's hoard (the
  dwarves' gold in it, crowns, in a purse only by the purse's own chance: the user's), a locked box at a warrior's or
  a miner's bed; an elf's
  longknife and hartbow (Combat 3's (S)), its stores; the soldiers' a unit's issue, their stores and spare arms, their
  captain's strongbox.
- **A leader's things in a theme's home** (all the themes', 2026-10-29: found building the hoard's test): popgen puts a
  faction's leader in its home, the first lair of its first kind; its own lair things (its [lair KEY]: a king's, a lord's,
  an elder's, a captain's hoard) are made there too as the home's placed, and a robber of its faction (the theme's)
  takes its takings to that hoard (they were made for the stand-ins' leaders only, whose lairs are their own).
- **Seen:** a look's "in mail" for a body in mail or other metal body armour ("A dwarf in mail, armed, is ahead"),
  else "armoured" as before.
- **Left for later** (written down, not built): the black orcs' slaves (the book's hybrid colonies: brown and small
  orcs ruled, a faction within a faction), the red and black orcs' feuds within a tribe, a swarm and a civil war; the
  white orcs' trade in silver and the dwarves' own trade (the market, below, has no merchant kind yet); the Gargun
  cowed by a strength they see (an
  attitude by the character's power: DENIZENS.md's more of the attitudes); the elves' barter (the user's answer 9
  gives them coins); a captor leading the character away (the soldiers'); the Gargun's sub-species' own tongues.

## Morale *(2026-10-30: the user's of 2026-10-29, DECISIONS.md's; its numbers and the rest drafted for the user to review)*

The book's morale is the Initiative test (Combat 18), its modifiers the GM's reading of the situation, a bad one ten
worse, a desperate one twenty, a good one ten better, an excellent one twenty.  The house reckons that situation from
the field about the one tested and its bonds to its leaders (the user's).  It is reckoned only as a test is made, or as
one may be (no standing updates: the 65C02's time).  *(Every number here is for review.)*  (dq0d.c's part 6.)

- **Who sways, who's swayed.**  Every intelligent creature sways the minds about it: a mind low, average or cunning
  (kinds.txt's); no beast, nothing mindless.  A beast isn't swayed by rank or words: it feels only a foe's might over
  its own and its fellows' falls (the book's situations); the mindless fear nothing (the referee's).  *(For review: a
  pack's lead beast swaying its pack left out.)*
- **Influence** (points, 5 to a step): its rank's weight, the rank and file 2, a captain 5 (a band's head: a black orc,
  the soldiers' captain), a lord or a spiritual leader 10 (a chieftain, a king, a high priest, a dwarf elder, a shade
  over the dead: kinds.txt's new rank), and a sixteenth of its influence skill (the book's Oratory or Rhetoric, the
  better).  A creature's skill is the house's, three times its Will (Rhetoric's base, Skills 12's three attributes, the
  bestiary keeping only Will of them); the null ruleset gives none.  The character is the rank and file, its own skill
  on top.
- **The field:** every actor within eight squares on its stratum, and the character, its influence the less with
  distance (all of it beside one, half at five squares, a third at eight: four over the distance and three).  A
  friend (its faction, or its kind not of a faction hostile to it) for it, a foe (a faction hostile to its own; the
  character to one hostile or unfriendly to it, not while it lies helpless) against it.  A friend's counts half to
  whole by how familiar one's own band is (its time served and contact, the bonds' measure below).  How each is doing:
  wounded, half; berserk, twice; fleeing, a friend's turned against one; fallen
  (senseless or dead), its influence gone and the book's bad situation (ten worse, once, however many), desperate if
  none of one's friends is left standing about one (twenty).  A foe's weight is its influence and an eighth of its might
  over one's own (the game's power, the referee's: a fearsome foe the heavier), wounded half, fleeing or fallen none.
  The character's allies are its friends, swayed by it for them.
- **Bonds upward:** each subordinate's to its band's head (the nearest rank above its own in its lair: popgen's leader
  with its home) and to its faction's highest above that (in any lair, wherever it is: the records kept).  A bond is its
  leader's rank's weight by the time they've had together and their contact, half each: the time the lesser of its
  weeks served and the leader's own (a new leader's the less), whole at 128 weeks; the contact 0-255.  As the
  populations are made each band has an age, 8 to 135 weeks by its lair's number (not the dice, so the dice are as
  they were), each of it about that; the game's weeks are added as they pass.  Contact: a band living with its leader
  starts at 200 and is whole each day; others start at 64 and lose an eighth a day (eight days at most at a time: the
  clock's, a long wait's too), and gain 16 each time one's reckoned near its leader.  Only the minds have bonds.
- **The band:** the field held to twenty points either way, the field and the bonds together held the same, the
  situation beyond (a desperate one lower); the steps the points over five, towards nothing.
- **When:** an actor's heart is reckoned as it engages (the steps the world's blow carries to the referee: its courage
  tested with them as it's wounded, both rulesets) and anew for every actor as one near falls.  A morale test is made
  as before: one wounded and standing (the referee's, its heart its steps), and the kin of one fallen (below).
- **A fall known** (replacing the peoples' fixed two steps and four): each of the fallen's kin (its kind, its faction)
  that hears it fall tests its courage at once: within its hearing of a fight's din (the world's blows' loudness, 5: its
  hearing's squares times that, over two, as it hears another's noise; within its sight the din always reaches it, so
  sight isn't reckoned apart); not one senseless (it hears nothing); not one fleeing already (it knows, and flees on).
  Its friend's fall is the book's bad situation (above).  If the fallen was its leader (its band's head or its
  faction's), that leader's influence and its bond are removed for the test; dead, it's known lost from then (no bond to a dead
  leader after).  One that doesn't hear it (out of hearing, on another stratum, not an actor, senseless) keeps its bond
  to the dead leader till it's told (dq0d.c's s_told, the hook): a leader's body found on the rounds does it (the
  denizens' day, below); a runner's word or a survivor's, not yet.
- **Succession** *(the user's, 2026-10-30, DECISIONS.md's; the rest for review)*: a leader's death leaves its post (a
  band's head, a faction's leader: any rank above the rank and file) to the most influential of those directly below
  it: if it was its faction's highest (none left as high), of its faction's next rank down, wherever they are (a king's,
  its captains); else of its own lair's (a band's head's, its band).  The next rank down from that if none's left;
  minds only.  Influence as the field's (rank and a sixteenth of the skill; a record not met yet, its rank alone: the
  ones about the character are known best); one fleeing or lying senseless only if none else is fit; ties by might (an
  actor's power, and two for its arms, four for armour, eight for mail), then the dice.  The successor takes the
  post's rank (its
  record's, beside its kind's) and its weight; its tenure begins then, so the bonds to it are a new leader's, but those
  of its own lair (its band) keep what they had with it; its own service counts whole (the house's: its record keeps the
  week it took the post, its tenure, in place of its service).  Those who know of the death (it's heard, or told) know of the
  successor too; those who don't, not till they do.  The band's test on the loss is made without the dead leader, as
  before, the new leader counting from then.  An actor at its post, not fleeing, rallies its fellows: a shout where it
  stands (a moment's), told as the character sees it ("The small orc rallies its fellows.") or hears it ("You hear a
  rallying shout ahead.").  Only a death leaves a post for now: a leader gone off the level for good (the world knows
  of none yet) would too.  The post a successor leaves (a captain's made king) is filled the same way, from its own
  band, and so on down the chain (the lead's, 2026-10-30: the user's words' general form).  A band's head is the
  nearest rank above one in its lair (a new captain under a king who lives among them), its faction's leader the
  highest above that, wherever it is.
- **The character's own morale** is the player's: never tested.  (D14 makes it an option, like hard mode, off by
  default, and magical or psionic fear always able to test it: neither built yet.)

- **The character's might** *(the user's, 2026-11-03: A11; new, for review: "its weapon skill" read as its best)*: as a creature's, three times its size
  squared (a man's 4) and a tenth of its best weapon skill (in hand or not), so a master frightens a rat as a beggar doesn't: an eighth of what
  it has over the creature's own weighs against it in the field, at most a step or so; its heart carried to its tests.  A beast feels it too (as it engages, below).
  The null ruleset's stays 10.
- **A test on first facing it** *(the user's, 2026-11-03: A14; new, for review: its worth once played)*: besides a wound and a fellow's fall, one more test of
  courage as a creature comes to strike or shoot at the character not already fighting it (a beast's too; not the mindless; once each time it
  engages), its heart the steps: broken, or shaken if it's a shy kind, it flees with no blow struck, its first moment gone in breaking; so a weak band can break before a blow.  Fleeing with nowhere to go, cornered beside the character, it fights at bay, its courage not tested again (2026-11-04: it tested it over and over with no time gone, and struck no blow).  (RS_MORALE's MR_FACED.  dq0 -I, the tests' with no engage initiative test, has none of it, nor a weapon broken: their games as they were; the soak plays them.)

*(A fall seen but not heard isn't known apart; morale tests not before every action as the book has it: the user's, 2026-11-03, these and no more.)*

## Fetching, gifts and reputation *(slice 4.8, 2026-10-30: drafted for the user to review)*

The book has an ally fetch nothing and says little of gifts: Rhetoric "in the target's best interest" is the easier
(Skills 12, Rhetoric), the GM's to judge.  The rest is the house's.  *(Every number here is for review.)*
(dq0d.c's part 5: s_ally, s_aside, s_regard; part 16: s_gift; dq0w/wfetch.c, part 72; things.c's fetch_find.)

- **Who fetches:** an ally, by its own turn: it walks to the thing, takes it up beside it, brings it back to where it
  last sensed the character and hands it over beside it.  Each a round's work (the world's I_ROB, as a robbery is).
  An errand isn't kept with its record: one that stops being an actor drops it, and follows after.
- **What it can carry:** one with hands (kinds.txt's body two-legged: folk) up to 50 lb, if a hand's free (it holds
  fewer than two things); any other in its mouth up to 3 lb (a rat a ration or two, a key, not a sword).
- **Handed over:** into the character's hand if one's free, else its pack (as a thing taken up is); neither, set down
  at its feet.
- **What can be fetched:** a thing the character sees lying about (or on or in an open thing lying there), on its
  stratum.  Not one in a thing closed, carried by the character or a denizen: told so, and the ally isn't asked.
- **Errands at a door** *(2026-11-02)*: "tell Grub to shut the door", "ask the dwarf to open the door", "ask the dwarf
  to unlock the door": an ally with hands walks to beside it, works it by the character's code (its key, if it carries
  one: denizens and doors, below), and follows again; no way to it, it gives up and follows.  A beast doesn't know how.
- **Following at a distance:** "tell Grub to follow me at 3 squares": it keeps that far behind (8 at most), not beside.
- **Moving aside** (D10): a friendly one or an ally a walk stops short of moves a square off the way, at once, unless
  it was told to wait (guarding), is fighting, fleeing or senseless, or has nowhere to go.  The walk stays stopped: the
  player walks on.
- **A gift's worth:** the thing's worth as a local takes it (the appraisal's: coins by their mints), times the count
  given.  Measured against the receiver's means by its kind's standing (kinds.txt's): none (beasts) a penny, tribal 4d,
  common folk 12d, people of standing 48d, lords 240d.  Under a quarter of its means a paltry gift, a step against; a
  quarter none; a half one step for; its means two; twice three; four times or more four.  Food two steps more, and
  two or four more again to the hungry and the very hungry (the denizens' bodies, below).
- **The test:** Rhetoric (a beast's Animalcraft) at those steps and its attitude's (hostile -3 ... friendly +1, as
  words') and its kind's sway; no tongue wanted (a gift speaks for itself).  A critical success two steps, a success
  one, a failure none, as far as friendly (a gift doesn't win an ally: persuasion does); a critical failure an insult:
  a step back, the gift refused.  Two seconds of the character's time.
- **Who takes one:** anyone with a mind within reach (a square); the mindless nothing; a beast only food ("The giant
  rat has no use for it."); one senseless nothing.  A gift taken is the receiver's own (its things, as its kit is).
- **Reputation:** a faction's regard is its attitude now (as harm done lowers it), raised by deeds it knows of: a
  gift of its means' worth or more taken by one of it (its steps less one, in points), harm done to a foe of its before
  one of its eyes (a point a blow).  Four points a step, to friendly at most; its newcomers and those of it not met
  yet in the cache take the step with it.  Not lowered by a paltry gift, nor raised past friendly.

## Water, hunger and thirst *(slice 4.9, 2026-10-30: the book's where it speaks, the house's else; drafted for the user to review)*

- **The water in the dungeon** *(every ruleset's)*: traced over a stratum's plan as it's grown (the plan's own
  numbers: its other dice as they were): water under two regions in three (the regions a built stratum's wells and
  cisterns were in before: they're the plan's now); in a natural stratum a stream for each 16 regions (one at least,
  four at most), from a region of its halls downhill to the lowest neighbour as long as there's one lower, ending in a
  lake, or sinking away where the next lower is rock.
  A chunk's water as it's grown: a cistern full, 5 to 8 feet over its floor's lowest; a cave's chambers in a
  lake's region flooded, 4 to 11 feet; in a stream's region its passages running a foot deep (*the house's, for review:*
  one chunk in eight of it, by the seed, six feet deep, over a man's head, so the current can happen: the user's,
  2026-11-03, that some streams run deep enough to swim) and its chambers' hollows
  pooled a foot or two; where there's only water under it, one chamber in three a puddle a foot deep in its hollow;
  but a chamber with a way between strata in it (a stair's, a ladder's landing) kept dry, the house's.
  A square under a still pool's surface is as deep as the surface is over its floor.
- **Wading** *(every ruleset's)*: a foot or two of water makes each step half as long again; waist-deep (3 or 4
  feet), twice as long.  Told as the character comes into it, deeper, shallower and out ("You wade in up to your
  waist.").
- **Swimming** *(the book's, Skills 9, where it speaks)*: water five feet deep or more is over the character's head
  (the book's, deeper than one is tall: the house's five feet), and it swims.  The referee tests Swimming as it
  begins and each minute after (the book's once a minute; calm water, a stream's current four steps harder); its
  distance a minute the book's yards for the grade (critical success 9, marginal success 7, marginal failure 2) times
  its skill index, in squares (HarnMaster's; the null ruleset's a middling swimmer's 27, 21, 6); a critical failure
  sinks it, under for the minute.  A minute's swimming is strenuous work, a minute under three times that (the
  book's: a level each five minutes, triple under water).  Its flames go out as it starts to swim ("The water puts out
  your light.").
- **Drowning** *(the book's)*: the breath held Endurance times 10 seconds (the null ruleset's two minutes); under
  longer than that, it drowns: dead (the bodies' time, RS_TIME's x[2] 3: "You drown.").  The house's: there's no one
  to revive it, so it's death at once rather than the book's Endurance times 20 seconds.  Coming up again is told
  ("You come up, gasping.").
- **Hunger and thirst** *(every ruleset's: the book has none; the user's of 2026-10-21, DECISIONS.md's)*: counted in
  minutes of want as the bodies' time goes (the referee's, every one's the same rules, rswant.h: the world tells it the
  minutes gone, a minute at a time while anything's to tell, else as its next level could come; twice as fast at
  vigorous work: running, fighting, swimming (whatever its pace: wtend.c's pc_swim), digging, climbing).  Thirst: thirsty at 8 hours, very thirsty at 16, parched at 24, dying of it
  at 48, dead at 72.  Hunger: hungry at 8 hours, very hungry at 24, starving at 3 days, wasting away at 10, dead at
  30.  Each told as it comes ("You're getting thirsty."), a long task (a wait, a sleep, a walk) asked about then, soft
  ("Keep waiting?"), as another soft interruption is, shown on the status line once it matters ("thirsty",
  "hungry"; the referee's news, RS_DESCRIBE b 4's).  The want's fatigue (HarnMaster's): no level at the first, then 1, 2 and 4 fatigue
  levels for each of hunger and thirst, not rested off until it eats or drinks (HarnMaster's: a fatigue level the
  universal penalty's, in every test; the null ruleset none).
- **Eating** *(every ruleset's)*: a day's rations is three meals; "eat" takes a meal (five minutes), eight hours of
  hunger off; a new day's ration begun (one of the lot used up) when the last one's meals are eaten.  Not hungry
  (under two hours of want), refused.
- **Drinking** *(every ruleset's)*: a pint takes six hours of thirst off; "drink" drinks as many pints as it wants
  (half a minute each), from the water where it stands (its fill) or a well within reach, else from its waterskin a
  pint at a time (four pints; "drink from the waterskin" from it whatever's about).  "fill my waterskin" in the water or by a well, a minute.  Not thirsty (under an hour), refused.
- **Not yet** *(DUNGEON.md's still to come)*: the user's standing order to eat and drink when needed (DECISIONS.md's
  hunger and thirst); running water and rain washing scents and prints away; a stream's current carrying a swimmer;
  a swim's test as it treads water standing still; things soaked.
- **The denizens** *(2026-10-31, every actor's)*: their wading, their hunger and thirst: The denizens' bodies, below.

## The denizens' bodies *(2026-10-31: the character's rules for every actor, the house's where it's cheaper; drafted for the user to review)*

- **Tiring in a fight** *(both referees)*: every actor tires as the character does, by the same rule (Moving and
  tiring, above; the book's second variant of combat fatigue, the user's of 2026-10-21): each round of a fight it's in
  (its own blow, a shot, or a blow at it) strenuous work, a level each five minutes of it; the time since its last round
  rest, its exertion worn off first, then a level each ten minutes, its want's levels never (HarnMaster's: the referee's
  bank of the creatures' bodies beside their sheets, dq0harn/tire.c, reckoned by the clock as it's wanted, never
  ticked; a round charged once, however many blows are in it; kept with the sheets in the save's beasts.dat).  Its
  fatigue levels are in the universal penalty of every test of its (its blows, its defences, its courage, its stealth),
  and its summary's tired (C_FATIGUED).  A beast (on four legs or none: not folk, nor mindless, nor one on two legs, so no Gargun, ghoul or shade, as their sheets have it: 2026-11-03) that its round leaves a level more tired
  tests its courage, its fatigue in it: short of steady (or gone berserk), it breaks off and flees, as one broken does.
  The null ruleset's simple way: its rounds fought counted, a level each 30 (a step the worse in its tests, attacking
  or defending), 30 rested off each ten minutes since its last; a level more, its courage (the flat odds, its heart less
  its levels) or it breaks off, any creature.  *The house's, for review*: a walk tires a denizen a level a day, no more
  than its rest takes off, and its hurried jog isn't charged (a chase is short, and it would want a request a step), so
  only its fights are counted.
- **Their paces** (the user's, 2026-11-03: "walk unless hurried, and closing in to attack should be hurried unless they
  are cautious"; every actor's, wbody.c's): a kind's walk is its step (kinds.txt's, tenths a square: 200 / its Move, as the
  character's walk: a man or a bandit 20, a dwarf 22, a giant rat 11, a bat 6, a cave bear 7, a ghoul 29); hurried, its jog, half that, one level
  louder (its step 2, not 1).  Hurried: one fleeing; one coming at its foe (hostile, drawn on or sensing it), but a
  cautious one.  *Cautious, the house's reading, for review*: a hunter by smell (it stalks: its steps are quiet, at a
  walk), a shy kind, one hurt, one whose heart is against it (the morale's steps as last reckoned, under 0).  One only coming
  to threaten, or wary, walks.  *The house's too*: an ally more than three squares from the character hurries to keep up;
  a dart past the character is its two squares at a jog; a wade's extra time is by its walk.
- **Water** *(every ruleset's)*: each kind's way with it (kinds.txt's water): the deepest it wades, in feet (by its
  size: a rat 2, a kobold or a small orc 2, the other Gargun and the dwarves 3, men and the dead 4), whether it swims
  deeper (the bestiary's swimmers: the giant rat, the cave bear, the cave adder, the lindworm, the elf), keeps out of
  water altogether (the cave spider: 0), or flies over it (the bat, the shade).  It goes onto no square deeper than it
  wades unless it swims or flies (its steps, its coarse moves, its dart past the character, its placing in its lair,
  its roaming): so it goes round, or stops at the water's edge.  Its step into water takes half again as long to half
  its depth, twice as long deeper, a swim twice (the character's wading, its depth by its size).  *Not yet*: a swim
  tiring it (strenuous work), a denizen drowning (one knocked senseless in water, or forced in over its depth: none is
  yet, as none goes in), a stream's current.
- **Hunger and thirst** *(every ruleset's: the user's of 2026-10-21 for every actor; the house's numbers)*: those that
  eat (not the dead), kept cheaply in each denizen's record: the clock it last ate and drank (in units of the clock's
  high word, near enough 1.8 hours), reckoned as it's wanted, never ticked.  Hungry about 9 hours on, very hungry a
  day, starving three days, wasting away ten; thirsty about 9 hours, very 16, parched a day, dying two (the
  character's, a little coarser).  Away from the character, as it goes about its lair: thirsty, it drinks (its lair's
  water, or what it finds: *the house's*, there's always some within its range); hungry, folk eat at their larder (and at
  the day's meals, a slot in the day: the denizens' day, below), a beast ranges twice as far for prey and finds some now
  and then (one time in eight, by its number and the clock, not the dice).  Near the character, the same: it drinks
  standing in water, or at its lair; folk eat at their lair; a hungry beast that doesn't sense the character finds
  prey now and then, else wanders twice as far; a very hungry hunter (a nose for prey, or a killer) that senses the
  character hunts it (hostile).  Food (what's eaten, by its key "food": rations; no candle, herb or remedy, though the appraisal's craft takes them in) given it is the more welcome the hungrier it is (a gift's two steps more hungry,
  four very hungry, beside food's own two), and it eats it.  The want's fatigue (HarnMaster's, as the character's: 1,
  2 and 4 levels for each of hunger and thirst past the first) the referee's, told as it's made an actor, never rested
  off; the null ruleset none.  None dies of it (*the house's*: one unseen a long while has fended for itself, its want
  held at about fifteen days).
- **A hurt denizen rests** *(every ruleset's; slice 4.3's leftover)*: one wounded (its mood's hurt, as its fellows see
  it), not an ally, with nothing after it (not hostile and sensing the character), goes home to its lair and rests
  there, a minute at a time.  *Not yet*: one tending another's wounds (a folk creature binds its own bleeding only);
  stings, and creatures envenomed: Wounds over time's not yet.

## The denizens' day *(2026-10-31: slice 4.5's daily life; drafted for the user to review)*

The book gives the creatures' habits (the Bestiary: nocturnal, diurnal ...), a watch of four hours, and a sleeper's
Awareness at half (Skills 11); the rest is the house's.  DENIZENS.md's daily life has how it's built (dq0d.c's part 9).

- **The hour** *(the user's, 2026-11-03, A27: "vary by game"; new, for review: how)*: the game begins at the hour its seed gives (the seed modulo 24), its clock's hour from then; the
  character knows it as it walks in; the underground has no day the character sees, but its denizens keep one.  dq0 -E, the tests', dusk, six in the evening (the runner gives it every dq0 test but those with hours: true; the soak plays the seed's), and the environment's dq0hour any hour, a scene's.  (What hour the character thinks it is: the
  time of day believed, below.)
- **The days** *(the house's, by the bestiary's habits; kinds.txt's day, for review)*: diurnal, up 6-22; nocturnal, up
  16-8; crepuscular, up 4-10 and 16-22; the dead never asleep.  Each denizen up to half an hour later than its
  fellows (its number's), so a lair wakes one by one.
- **Watches** *(the house's, on the book's four-hour watch)*: the folk that keep guard (kinds.txt's day w) by thirds,
  each third four hours on watch, four asleep, four about, in turn; the watch changing at midnight, four, eight, noon,
  sixteen and twenty o'clock.  So a third of a garrison is always awake on its rounds, a third always asleep.
- **Sleep** *(the book's Awareness at half asleep; the rest the house's)*: a sleeper's hearing halved and its sight
  none; woken by a noise it hears, harm or sensing the character, up ten minutes, then back to bed if it's still its
  time.  A snore now and then, as loud as two (heard within the character's hearing plus three squares).
- **Meals** *(the house's)*: folk's, an hour as it wakes and an hour halfway through a long day, at its lair's home,
  fed as it sits down (the bodies' want: hunger and thirst, above); a beast's meal its prey, no slot in the day.
- **Work** *(the house's; kinds.txt's work, for review)*: about but not on watch, in its work's room if its lair's chunk
  has one (the cultists' chapel, the dwarves' and kobolds' workshop, the lords' solars, the captain's guardroom).
- **Rounds** *(the house's)*: on watch, the rooms of its lair's chunk in turn, a word with a fellow on watch near it
  (as loud as three).  A body of its own faction's within four squares of a room of its rounds found (once): the alarm
  raised, a shout as loud as a rally's; its faction's attitude to the character a step worse (the intruder suspected,
  though nobody saw it), and its fellows near's; each woken and alerted; a leader's body, the leader's loss known
  (the morale's hook for a loss told).
- **Unaware** *(the house's)*: asleep, eating, at work or on watch, it hasn't noticed the character: it engages no one,
  and one asleep breaks no long wait.

## Leisure *(2026-10-31: the user's words, DECISIONS.md's; drafted for the user to review)*

- **When** *(the house's)*: folk with pastimes (kinds.txt's) have leisure in their time about: a watch-keeper two of its
  free watch's three hours after its meal (its third's two hours about before its next watch on); a diurnal or nocturnal
  kind the last four hours of its day before it sleeps (some 4 of its 14 waking hours besides its meals: about a
  quarter).  The beasts, the dead and folk without pastimes none.
- **Pastimes** *(the house's, by the peoples)*: reading (the dwarves' elders, the elves, the priests, the lords), dice,
  cards and knucklebones (bandits, kobolds, orcs, soldiers), a board game (dwarves, soldiers, captains), talking and
  telling stories, singing (bandits, cultists' hymns, dwarves, elves, soldiers), drinking (kobolds, bandits, orcs,
  dwarves, soldiers, lords), mending gear, whittling and carving (kobolds, small and white orcs, dwarf miners, elves).
  Each one's by its number and the day's; a social one (dice to drink) its lair's, the same for all of it at leisure
  then, by its lair's number and the day's, so they're at it together; reading, mending and whittling alone, at its bed.
  Not the dice.
- **Where** *(the house's)*: the social ones gather in a room of their lair's chunk that suits, the first it has of a
  hall, a guardroom, a barracks, quarters; else at its home.
- **Less watchful** *(the book's half Awareness for one not paying attention, Skills 9's Legerdemain; the house's
  reading of it as the senses' layers)*: its sight and hearing halved; at its cups, quartered.
- **In drink** *(the house's)*: one at its cups is in drink an hour after its last (the hub's AM_DRUNK): its blows two
  steps worse (10), and blows at it two steps easier, both referees' steps.
- **Let go** *(the house's)*: risen to act (it senses the character, a foe, harm), its pastime's let go, told as it's
  seen: "The soldier lets the dice fall.", "The dwarf shuts its book.", "The bandit sets down its cup."
- **Heard** *(the house's)*: now and then, a third of its moments at it: dice rattling and counters clicking as loud as
  a snore (2), laughter, singing and cups clinking as loud as talk (3); reading, mending and whittling silent.

## The time of day believed *(2026-10-31: the user's words, DECISIONS.md's; drafted for the user to review)*

The user's (DECISIONS.md's the time of day believed, 2026-10-31: timekeeping fuzzy unless the sky can be seen; after
sleeping quite arbitrary; drifting further from known as play goes on; corrected whenever the sun can be seen and the
direction is known, or by watching it a while).  The world keeps the true clock (the denizens' days and all that happens
go by it); the character's time is a belief, as its north is: the hub's tb, its error from the true time (skew, minutes)
and how sure it is (unc, minutes either way), the world's (wtime.c, part 76), kept with the game (world.dat).  Every
number here is a draft for the user.

- **The start**: walked in, it knows the time (the game's hour, The hour above; a test's `dq0hour`).  Come round
  below from a fall (the theme's start | fell), as a black-out of an hour (below).
- **Absolute timesense** (the book has none: the house's rare trait): **one character in a hundred** (hmchar's roll,
  after all the rest, so every other die falls as it did; its file's `timesense` line, kept as the game's saved: C's,
  HyForth's and hylang's hmchar alike).  It always knows the hour; nothing below touches it.
- **The test** (the bearings' P_BEARING, its steps from rs.h's TIME_STEPS on): **Awareness** (the hours noticed going by, as the turns of a way are: attention, as the
  bearings'), its dice the game's own as they stand, mixed and put back (moving quietly's way), so no other test rolls
  differently for it.  The null ruleset: its flat odds, the same way.
- **The hours awake**: a test **each hour** (hours gone by together, one test for them all), its drift and doubt each
  hour: a critical success none and **3** minutes' doubt; a marginal success **3** off and **5**; a marginal failure
  **8** and **10**; a critical failure **15** and **20**; its way the referee's d8 (odd the earlier).  The longer since
  it last knew the time, the more: **times one for the first four hours, two for the next four, three, then four**.
  Not asleep or senseless.
- **A sleep or a black-out**, guessed as it wakes or comes round: the test **two steps the harder each hour asleep**
  (twenty at most), a black-out's **ten** the harder.  A critical success: no error, doubt an eighth of the length; a
  marginal success off by **a sixth** and doubt **a quarter**; a marginal failure **a third** and **a half**; a critical
  failure **two thirds**, and **no idea at all** (doubt six hours: any hour).  A black-out's failure half an hour more
  off; a quarter of an hour's doubt at least.  (So eight hours' sleep: a marginal success off 80 minutes, doubting two
  hours.)
- **Clues that narrow it** (each a belief of its own; taken when it's surer than the one it has):
  - **its own light burning down** (it sees its torch: a light's minutes left stay true on the status line): the
    minutes burned since its belief was set by that light (as it was lit, or a reckoning found it lit), **read to a
    twelfth**; a light burnt out to the end, the whole of it.  A light changed, or put out part burned, starts afresh.
  - **its hunger** since its last meal (the referee's want: minutes of hunger), **read to a quarter**.
  - Noted, not built: thirst, tiredness, the denizens' routines learnt (the watch changing, meals), a chain of torches
    counted.
- **The sky in sight** (an open way out within **two squares**: the top stratum's way out, opened, up its stairs, shows
  **its light alone**; a far exit below the top, out into the open, **the sun or the stars**; a window on the surface, a
  skylight or a shaft open to the sky from the square under it or beside it: what shows the sky, below): its light narrows the belief to its part of the day
  (dawn 5-7, day 7-17, dusk 17-19, night 19-5: the house's even day); **the sun or the stars read by the north it
  believes in**: the time to **half an hour** (the stars an hour), **wrong by three hours each eighth its north is
  off**, unknown to it; its bearings known lost, the sun **high** (10-14) or **low** (morning or afternoon by what it
  believes), the stars night alone; **watched** (in its sight through a wait, or "watch the sun": **ten minutes**, the
  stars **twenty**), their way across the sky: the time to a quarter of an hour (the stars half an hour), and **north
  known** (its bearings set right), and kept so while it stays in sight.  Told as what's seen changes ("Daylight shows above: it's day.", "You see the sun in
  the sky: it's four in the afternoon, you judge."; by a skylight or the rest, by what shows it: what shows the sky,
  below).  Weather (overcast: its light alone) not yet.
- **How it's told** ("time", "what time is it"): doubt ten minutes or less, plainly to the quarter hour ("It's half past
  six in the evening."); 45 or less, "about" the hour, "you think"; two and a half hours or less, "some time around";
  more, the part of the day, "you'd guess, but you can't be sure"; six hours or more, no idea ("You've no idea how long
  you slept: it could be morning.").  No hour on the status line, and no clock (the user's, 2026-11-03: "Hide the minutes":
  it used to show the minutes gone, and a game starting at a known 18:00 gave away the true hour; players see what the
  character believes).  The minutes stay for the tests and debugging: dq0 -m puts them back on the status line.
- **Waits by it**: "sleep until dawn", "wait until morning" (midnight 0, dawn 6, morning 7, noon 12, afternoon 14,
  dusk and evening 18, nightfall 19, night 21) last till that hour **as it believes the time now**; what happens goes by
  the true clock.  "watch the sun" (the sky, the stars) is a wait of a quarter of an hour (the stars half an hour).
- **When it's reckoned** (never polled: the lights' burning's look, part 27's): an hour on; every five minutes on a
  stratum the sky may be seen from; at once as a light changes, a wait begins or ends, it's knocked senseless or falls,
  goes to another stratum, or the way out opens, or it comes by an opening that shows the sky; a minute at a time
  while it's gone.

## What shows the sky *(2026-11-03: the user's "All three together", DECISIONS.md's the time of day believed; the rest the house's, drafted for the user to review)*

The user's (2026-10-31): the time's known where there's "some place where you can actually see outside through a window,
skylight, vertical shaft, or the like"; (2026-11-03) all three, together.  Everything else in this section is the house's:
every number and every word for review.  The maker places them by the seed alone (dq0m/sky.c, its own dice, after the
plan's and the links': a chunk made again is byte-identical, dq0pure's); a test's scene, the environment's dq0sky (rc's): "0" none at all (a test about other things, in the dark, say), anything else every room both and a shaft always.

- **A skylight**: on the **top stratum** alone, in its hewn rooms (a natural cave's none, nor a mixed stratum's caves):
  **one room in six**, over the middle of its floor.  No way up, too high and too small (u there: no way); nothing's
  stocked under it.  On the map a **^**.  Its square's kept as a way up's is (T_UP, marked T_HOLE).
- **A window on the surface**: the **top stratum**'s alone, **one room in six** of its hewn rooms, in a wall with solid rock
  two squares deep behind it: a light well, up to the day.  Any of glass, shutters and bars, by its dice; 2 feet wide, 3
  high.  **Which windows show the sky**: these, marked as they're told ("There is a glazed window ahead, looking on the
  sky."; "It looks on the sky." to examine it; "Through the window ahead you see the sky, up a light well."), and
  only with their shutters open.  **Every other window** (fixtures.c's, on every stratum: a passage running beside the
  wall) **looks on what it did: a passage, the dark, and shows no sky.**  Not a way out: its bars can be bent and
  its glass broken, as any window's, but the sill goes nowhere ("You can't get up onto the sill.").
- **A shaft open to the sky**: **one game in two** the top stratum has one, on a square of its halls, in a chunk with no
  other way between strata in it; **one in two of them goes on down**, and on from there the same, **three strata at most**, while
  the stratum it's on has a way down of its own.  Its foot on a stratum below is there if the square lies inside that
  stratum's bounds, off their edges, in its halls, in a chunk with no other way in it; if not, it's **choked** ("The shaft is choked with fallen rock
  a little way down.").  So no stratum below is laid out any differently for it (its bounds aren't stretched
  round it).  It's told as what it is ("a shaft open to the sky"; "a shaft in the floor" where it goes on down), the
  map's < and >.  Two of a stratum's eight links are kept free for shafts dug.
- **What's seen**: from the square **under it or beside it** (a square each way; a window's, the floor squares beside it),
  the belief's set as at the way out (The time of day believed, above): **a window, a skylight, or a shaft on the top
  stratum** the open sky (the sun or the stars read by the north it believes in; its bearings lost, high or low; watched
  a while, the time and north both); **a shaft from below the top stratum** a patch too small and far to read: **its
  light alone**, the part of the day.  At once as it comes by one, even on a walk (its looks every third square find it, by the squares' mark, T_HOLE: part 17's, part 89's sky_near; at the
  turns pace, at the next minute's look; on a way up's own square beside one, at the next reckoning).  Words by the hour and by what shows it ("Pale daylight falls from the shaft
  above: it's day.", "Grey light comes in at the window: it's dawn, or near it.", "Fading light falls from the skylight
  above: it's dusk.", "Stars show far up the shaft above: it's night.", "Stars show in the skylight above: it's nine at
  night, you judge.", "The sun shows at the window: it's noon, you judge.").
- **Daylight falling in** (Light and traces' levels): by the true clock, by day (7 to 17) each opening's a lamp of **reach 4** on the
  square under it (9 there, 7, 5, 3, 1, as a torch), at dawn and dusk (5 to 7, 17 to 19) **reach 2**, by night **none**:
  the light layer's, so every actor's sight goes by the same squares (a denizen's in its light, as the character is; a
  window's with its shutters open).  The look says so ("You see a skylight in the roof ahead, daylight falling from
  it."); and it's noticed from a little way off as any light is ("A light shows far off ahead.").  The emitters near the
  character the world's list (the hub's tb.skx: four at most, made again as it comes into a chunk and at each
  reckoning), the reach tb.sun, set at the day's parts' edges (wtime.c's).  Far off, in a chunk the cache hadn't made as the list was made: dark, till then.
- **Climbing a shaft, any shaft** (Climbing and falling's rules, as DUNGEON.md always said they were; a dug one's only went by
  time till now): a round at a time, a Climbing test each, its height the drop between the strata (a shaft to the surface
  **20 feet**); a slip back, a fall from as high as it got (down: to its foot, the stratum below).  **Too high**: a shaft
  open to the sky whose room's roof is more than **9 feet** up (a jump and a pull) can't be reached from the floor ("The
  shaft's mouth is too high to reach."), but by a rope hanging there, its end in reach (Rope and grapples, under Climbing and falling; a ladder, no rule yet).  A dug shaft's foot is reached, as the digger left it.
  Jumped down: a fall of its height, as any shaft's.
- **A shaft as a way out**: at the **top** of a shaft on the top stratum, climbed, the character's out (E_LEFT, EL_SKY): left, or solved if it's solved (How a dungeon ends, above).
  *The user's, 2026-11-03 (A18): "It is not about winning or losing.  It is about solving a puzzle."  Climbing out unsolved is "not a loss, just a disappointment"; Q1, 2026-11-04, took the grating off.*
- **A thing dropped down one** (any shaft's): a thing put down on the square where a shaft goes down falls to its foot, on the stratum
  below ("It falls away down the shaft, and you hear it land far below."); a choked one, it lies where it's put.  The
  denizens don't hear it yet.  A thing thrown down, or dropped by a denizen, not yet.
- **Kin**: a shaft's **draught** is the air layer's as any shaft's (The draught, Sound, the air and moving quietly): the
  stronger on a stratum with one; its **drip**: under a shaft open to the sky a little noise of water (the background's,
  2). **Rain and weather**: not yet; their place is what's here (a square under or beside one of these is where rain would
  reach, the same check; overcast its light alone); sounds from above, not yet.

## Poison, bad air and disease *(2026-10-31: the user's words, DECISIONS.md's; the book's where it speaks, drafted for the user to review)*

The book's (Physician 4): a disease has a Contagion Index (C1 highly contagious ... C7 barely) and a healing rate (H1
the plague ... H5 influenza); one exposed tests CI times Endurance (95 at most), failing it catches it, then a recovery
roll each day on the Infection table (a critical failure two worse, a marginal one one; a marginal success one better,
a critical two), recovered at H6, dead at H0; its symptoms a special penalty of 10 to 30 to physical skills.  A poison
is an infection whose intervals are shortened to the toxin (a large dose of a deadly one H1, a small dose of a mild one
H5).  The HarnWorld Bestiary's venoms (wounds over time, above) and its grave-wight's grave rot (H4, 2d6 hours'
incubation, a universal penalty of 20 till healed).  What the house adds, every ruleset's where it says so:

- **One course for each** *(HarnMaster's: dq0harn/afflict.c, overlay 8)*: the character carries a toxin (a venom or a
  poison) and a disease at once, each its kind, its rate, its interval's minutes; the venoms one case of it, their tests
  and dice as they were.  Its symptoms the book's special penalty as fatigue levels (5 points each), held, not rested
  off till it's over (as the want's are): a dart's poison 2, a contact smear's 2, spoiled food's 3, the flux 3, rat-bite
  fever 2, grave rot 4, a level more for each step its rate is worse than it began.  Poisoned (C_POISONED, the status's
  "poisoned"), the bodies' time is asked a minute at a time; a disease shown, its status "feverish".
- **The poisons** *(for review)*: a dart's or a needle's (H4 at a middling dose, ten minutes), a contact smear's (H4,
  twenty), spoiled food's (H4, an hour, never deadly: held at H1); a dose a step the stronger or the weaker its rate a
  point worse or better.  Taken, its first roll at once ("The dart's poison burns in the wound."), then one an interval
  (at rest the whole, about half, vigorous a quarter, the bestiary's), worse "You feel sick and dizzy.", gone at H6
  ("The poison's hold on you passes."), dead at H0 ("The poison stops your heart.").  Poisoned again, the worse kept.
- **A dart trap's poison** *(for review)*: one dart trap in four (by its square) is poisoned, its dart's wound drawn
  (wtrap.c's, the world's: the referee told with the bodies' time, at its next minute).  *Not yet*: a door's needle's
  contact poison (the doors' part has no room: overlay 2's), a chest's; a denizen's poisoned blade (the outfits'
  factions to say who uses them); food that's spoiled (the book's critical failure tasting it: food found, the market's).
- **The diseases** *(for review)*: the flux (still water drunk where it stands: a pool, a puddle, a cistern; or from a
  waterskin filled from it, the same risk at each drinking: the user's, 2026-11-03, the skin's water marked, and a skin
  topped up with it foul all through, *the house's*; a stream's running water and a well's clean; C5/H4, a day's incubation), rat-bite fever (a giant rat's bite that wounds: C5/H4,
  two days), grave rot (a ghoul's bite or claws that wound, the bestiary's grave-wight's: C4/H4, seven hours).  Caught,
  nothing's told (the character doesn't know it's caught something) till its incubation's over: "Your belly gripes and
  cramps: you have the flux.", "A fever's on you, and the old bite throbs.", "A wound the ghoul gave you festers: a
  fever's on you." (a hard stop of a long task); then its daily roll ("The fever burns higher.", "... eases a little.",
  "... holds."), "The sickness leaves you." at H6, "The sickness takes you." at H0.  Its dice the body's own (rshouse.h's
  way), so a fight's roll as they did whether a bite carries anything.  *Not yet*: corpses handled (no handling of a body
  but a search), the undead's touch but a ghoul's (the wight's drains), a lair's sickness spreading among its folk.
- **A remedy** *(for review; built 2026-11-02: the market, entertaining and remedies, below)*: an antidote taken makes
  the toxin's rate the better by its power (a theriac 2), the lindworm's numbing venom only by a strong one (afflict.c's
  o_aff_cure, by remedy: "drink the antidote"); a remedy the disease's.
- **The air** *(every ruleset's, rsair.h; its figures for review)*: blackdamp (stale air, too little in it to breathe:
  choke-damp, a mine's CO2), whitedamp (a fire's or a blast's smoke: carbon monoxide, poison that gives no warning),
  firedamp (marsh gas, in the coal and shale beds: it burns).  Strength 1 a little to 3 much.  Blackdamp felt at once:
  1 short of breath (a fatigue level; stale air never kills), 2 gasping (2 levels), dizzy at ten minutes (4), the legs
  failing at thirty (6), dead at an hour; 3 the head swimming at once (3), the legs failing at two minutes (6), dead at
  five.  Whitedamp builds up in the blood, its strength squared a minute (1, 4, 9): the head aching at 30 (a level),
  dizzy and sick at 60 (3), the legs failing at 120 (6), dead at 180; in good air it clears two a minute ("Your head
  clears.").  Firedamp only chokes when it's thick (3: as blackdamp 2).  Out of it, "You breathe freely again."  Each
  worsening told and a hard stop of a long task.  The null ruleset the same, its levels none.
- **A flame's warnings** *(the miners' old ones, for review)*: told as the character comes into the air with a flame
  lit: in blackdamp it burns low (1), shrinks to a blue-edged bead (2), and in thick blackdamp goes out (3: the world
  puts its lights out, "The air's too foul to keep your light burning."); in firedamp a pale blue cap trembles over it
  (1), leaps up tall (2, a lantern's); in whitedamp nothing at all.  Out of blackdamp, "Your flame brightens again."
- **Firedamp's burst**: firedamp of 2 or more at a naked flame (a torch, a candle), or of 3 at a
  lantern's: **its first step in, from air that isn't firedamp, the flame's warning and no burst** (the pale blue cap, as thin firedamp's: the user's,
  2026-11-03, A19: "warn before bursting"); **the next step in, it bursts into flame** ("The air ignites with a roar: a sheet of blue fire rolls along the roof!"): a burn
  (3d6, 4d6 at 3: HC_FIRE), the lights out, a noise as loud as any (9: the denizens hear it), the pocket burnt off,
  its afterdamp left (whitedamp 2, about seven hours: the clock's high word, four of them).  A step back out instead: nothing.  *Not yet*: stones falling
  from the roof where the rock's poor (a second harm's words want the UI's room), the denizens near harmed, rubble
  left in the passage.
- **Where the air's bad** *(the maker's, every ruleset's; for review)*: a pocket in one room of a chunk, round its
  middle, its reach the room's half and a square, from the chunk's place alone (no dice: the layouts as they were):
  firedamp where the rock at the chunk's middle is shale (one chunk in four); blackdamp 100 feet down or more (one in
  eight; 200 feet one in four, its strength 2); a step stronger one time in four; none where there's a way between
  strata in the chunk (the air moves there).  A draught of 2 or more thins it a step where the character breathes.  A
  dig into a pocket's rock comes into it as it steps there.  *Not yet*: old mines' own (their works still to come), the
  angles' diagonal ways and the draughts meeting.
- **The denizens** *(for review)*: they know where the air's bad and keep out of it (the minds' steps, their placing,
  their roaming: dq0d.c's wades), the dead too.  *Not yet*: one that blunders in (hunting the character into it) choking
  or dying of it; their own poisons and diseases (none bites another; a lair's sickness spreading among its folk, by
  the clock, at need, the plan's); the explosion's harm to those near.
- **The null ruleset's simple way** (dq0null.c): a poison sickens it a while (a dart's or a smear's half an hour a point
  of its dose, spoiled food's two hours: "You feel sick: there was poison in it.", "The sickness passes."), the flux
  caught on a flat test shows a day on and lasts three; neither kills; the air rsair.h's, its levels none.
- **What it costs** *(the emulator)*: the air's looked at only as the character steps in a chunk that has a pocket, or
  out of one (part 73's, the water's, swapped in as wading is); the minds' steps a chunk's pocket's test each.

## Diagonal ways *(2026-10-31: the user's words, DECISIONS.md's; step 1 of three, drafted for the user to review)*

- **What makes a diagonal a way** *(the house's)*: a step between two squares that share only a corner, where neither
  corner is a square to stand on and one of them is cut (world.h's T_CUT: rock, or a wall, a diagonal way's corner cuts
  through, half of it open); never squeezed between two solid corners (the engagement zone's rule, "not round a
  corner").  Where a corner can be stood on, the way's the four's round it, as ever: so a room's eight ways, and every
  passage made before, are as they were.
- **Its length** *(the house's)*: corner to corner, seven feet: a walk's square on a diagonal way takes its time and
  two fifths more, the denizens' too, the ways across a room's search the same.
- **Every actor** *(the house's)*: the character's walks along a passage (its ways on, a fork, a bend: the eight, none
  behind), its ways across a room and a journey's legs; the denizens' steps (the four first, then the diagonal ways,
  the same dice); a hunter's scent (a diagonal it follows only as a way, else the four's as before); engagement round
  a cut corner (a cut's not solid).
- **Seen and told** *(the house's)*: a cut is seen past (the eye's line along the way), a passage's ways on told with
  the diagonal ones ("ahead to your right"), the map's stroke along the way ("/" or "\", turned with the map).
- **The caves** *(the house's)*: a tunnel's zigzag, one square wide, its corner square's two ways on a quarter apart,
  the rock about it, the zigzag going on past it, is the corner its diagonal cuts: the tunnel walked corner to corner.
  A bend alone stays a bend; a wider tunnel stays as grown.  No dice: the same caves for the same seed, cut.
- **Digging** *(the house's)*: "dig northeast" (any diagonal): the square corner to corner, seven feet of rock (14
  half-feet: a square's five feet, 10), its two corners cut as it's through, so it's walked; a cut dug as rock.
- **The believed compass an eighth off** *(the house's, as before)*: "north" a true diagonal: in a passage a way only
  where there's a diagonal way.
- **Built since** *(the user's "queue up 1-3")*: a passage's heading, built passages on the diagonal and rooms on the
  diagonal, below.  The tunnellers (slice 4.11) dig by the same rules (Tunnellers, below; 2026-11-01).  *Not yet*: one
  complex alone (a temple, a keep) at 45 degrees in a stratum otherwise square to the compass.

## A passage's heading *(2026-11-01: the user's words of 2026-10-31, DECISIONS.md's ways off the compass; step 2 of three, drafted for the user to review)*

- **A run** *(the house's)*: a passage's squares from where the character is, as a walk goes along them (its one way on,
  no fork), within sight (the squares it sees now), eight steps at most: the stretch of them that keeps one shape.  Two
  headings an eighth or a quarter apart, the one that comes twice running the main one, the other never twice running
  (a staircase of short steps, a zigzag): a slant, once the other has come twice; its bearing the steps' sum, or, the
  two by turns with no main one, halfway between them.  Anything else is straight on its first heading as far as that
  goes, so a bend stays a bend (its corner one step the other way, then on that way).  Its bearing in sixteenths of a
  turn.  (dq0/run.h, run.inc.)
- **Told by its shape** *(the house's)*: a passage's ways on worded by their runs' bearings as the character faces, in
  sixteenths: the eight's words where a run lies on one of them ("ahead to your right"), else the quarter it's nearer
  and a little the other way ("ahead and a little to your right", "behind you and a little to your left"); a straight
  passage as before.  So a staircase of steps is told as the slant it is, not by the step it starts with.  (The
  senses' runs, dq0p/view.c, part 4's; sight.c; words.c.)
- **A walk along it** *(the house's)*: the walk's passage cut into runs from its start; a turn told where the run's
  bearing changes, by more than an eighth "The passage turns left." as before, by an eighth or less "The passage bends
  away north-northeast." (the new run's bearing, by the north the character believes in); none at a staircase's every
  step.  The walk's last step or two, of the run before's two headings, are that run's (too few to show a shape of
  their own).  A bend of an eighth or more counts for the bearings as a bend did (Bearings underground, above); a
  staircase, once a bend at every step, is none.  (dq0w.c's run_at and beat, part 23's.)
- **The map** *(the house's)*: as drawn, a passage's square that's an elbow (two ways out of it a quarter apart, only
  walls and rock else beside it) with an elbow turned the same way beside it along one of its two, a staircase's step,
  drawn "/" or "\" along the slant; a bend's corner alone stays "#".  On the map as it's shown (turned, faded), so what
  the character's map shows of it is what's drawn so.  (The UI's map, look.c's slants.)
- **Left** *(for later)*: a wide tunnel's staircase (two or three squares across: the walk's ways there a fork); the
  facing after a walk along a slant (its last step's, as before).
- **Built since** *(2026-11-01, the house's)*: a look one way goes on along a diagonal way (way.c's seen_on: the eight,
  a diagonal only past its cut corner: "Down the passage ahead you see into the cell"); a denizen's coarse wander (out
  of the character's sight, the turns pace's catching up) takes the diagonal way an eighth round from the four's step
  it rolled where that step's closed and a corner's cut (the same dice: dq0d.c's s_coarse).

## Built passages on the diagonal *(2026-11-01: step 3 of the user's three, in part; drafted for the user to review)*

- **Which strata** *(the house's)*: a fifth of the built strata (and the built parts of the mixed ones) lay their
  passages on the diagonal, by the stratum's own seed (a byte of it no other die reads: the same dungeon for the same
  seed but in those strata); dq0 -A, the tests', all of them (hub.h's GE_SLANT).  (dq0m/pass.c's leg, uses.c's
  US_STYLE.)
- **How** *(the house's)*: a passage between two doors (or from a chunk's edge) sets out from its door a square, then
  goes a step each way by turns while both are still to go, then straight on: a staircase of squares, cut once the
  chunk's grown as the caves' zigzags are (caves.c's slants: each other square of it the corner the diagonal cuts,
  T_CUT), so it's walked corner to corner (Diagonal ways, above).  A cut in a built stratum is the passage's
  dressed corner, masonry as its walls are; its rooms: Rooms on the diagonal, below.  The chunk's reach (built.c's
  connected) is tried before the cut, by the four.
- **Its words**: as any diagonal passage's (a passage's heading, above: "The passage bends away northeast.").

## Great caverns *(slice 4.10's rest, 2026-11-02: DUNGEON.md's caverns across many chunks; drafted for the user to review)*

- **Where** *(the house's)*: a natural stratum's region (1,280 feet each way) holds a great cavern one time in three,
  by its own numbers (a hash of the stratum's seed and the region's place: no die of the grower's, so the chunks
  round it are as they were); none in a region a way between strata comes into (a stairway's foot, a ladder's), so
  each stratum's way in stays a cave's chamber; none in a mixed stratum's natural part (its hewn chunks would cut it).
  dq0 -V, the tests', gives every region of a natural stratum one, its middle as near the region's first way's square
  as the region lets it (hub.h's GE_CAVERN).  (dq0m/caves.c's cavern, overlay 8.)
- **Its shape** *(the house's)*: a box in its region, 10 squares or more off the region's edges (so no two meet), its
  half-breadths 24 to 63 squares east and west and 20 to 55 north and south (240 to 630 feet across, 200 to 550
  feet deep), cut down to the region; its floor an ellipse in the box, its edge wandering in and out by up to a
  quarter of the way (a lattice of numbers 40 feet apart, evenly between), here and there a pillar of rock 10 feet
  square well inside it (one lattice point in six); its floor a hollow 2 to 8 feet deep at its middle, nothing at its
  edge, so a walk across it rises and falls gently; its roof rough stone 40 to 99 feet up at the middle, half that at
  the walls.  Dry: no lake or pool in it yet.
- **One room across the chunks** *(the house's)*: each chunk it touches grows its part of it from the seed and the
  square alone (the walls at a chunk's edge too, beside the next chunk's part), so the parts meet at the edges and a
  chunk made again is as it was; the part is a room of its own in its chunk (world.h's R_CAVERN), the chunk's other
  chambers kept clear of it and its tunnels meeting in it.  Walking across it ("go north", "walk forward") goes on
  over the chunks' edges to its far wall; a place in it ("go to the centre") is the part's, in the chunk the
  character stands in (a cavern's own places are still to come).  The map draws it as it's seen, whole.
- **Its words** *(the house's: Where you are, above)*: "You are somewhere in a vast cavern, its walls beyond your
  light." while fewer than six squares of its walls are in sight (a pillar or two doesn't count); "You are in a vast
  cavern, by the light of your torch." once more are; where in it never told (the thirds have no meaning there), nor
  its figures on a close look ("Its far side is lost in the dark."), nor where things lie in it; its roof told as any
  ceiling, "Above, the dark hides the ceiling." while the light doesn't reach it.  (dq0p/sight.c's S_VAST, S_VASTW;
  dq0/words.c's room.)

## Long slopes between strata *(slice 4.10's rest, 2026-11-02: raises and inclines; drafted for the user to review)*

- **Where** *(the house's)*: a stairway down from a natural stratum or a mine is a long slope instead one time in
  three (a sloping tunnel worn or cut down to the stratum below), by its own numbers (a hash of the stratum's seed and
  the way's square: no die of the plan's, so the strata are laid out as they were); a ladder stays a ladder.  dq0 -P,
  the tests', makes every stairway down a long slope, any style (hub.h's GE_RAMP).  Its foot below is a long slope
  up, on the same square, as a stairway's is.  (dq0m/plan.c's links_down; level.h's O_RAMP_DOWN, O_RAMP_UP.)
- **Walked, not climbed** *(the house's)*: "d" and "u" take it as they take a stairway, from anywhere in its room;
  it's four feet along for each foot of the drop between the strata (a grade of one in four), so it takes longer
  than a stairway: down, eight tenths of a step's time a foot of the drop (a stairway half a step's); up, thirteen
  tenths (a stairway one): the walk's squares at the pace, and uphill half again a step's time a foot of rise, as
  any walk's (dq0w.c's cost).  Going up it is strenuous work for the time over a level walk's (half a step's time
  a foot), tiring as a climb does (Moving and tiring, above); going down isn't.  (dq0w.c's s_climb.)
- **Its words**: "a long slope leading down" (or up) where a stairway's would be; "You walk down the long slope."
  and "You trudge up the long slope." as the stairway's "You go down the stairway, a long flight."; on the map its
  square drawn as a stairway's.  An ally following comes along as it does on a stairway.

## Rooms on the diagonal *(2026-11-02: step 3's rest of the user's three, ways off the compass; drafted for the user to review)*

- **Which** *(the house's)*: the built strata whose passages run on the diagonal (a fifth of them, by the stratum's
  seed: built passages on the diagonal, above) have their rooms' walls on the diagonal too, so the whole complex lies
  at 45 degrees to the compass; dq0 -D, the tests', every built stratum's (hub.h's GE_DIAMOND).  A room under four
  squares either way, and a room a way between strata comes out in, stays square.  (dq0m/caves.c's diamonds, overlay
  8, called by built.c as the chunk's grown.)
- **How** *(the house's)*: each room cut to the diamond its box holds: a square of its floor kept while its distances
  from the box's middle, east-west and north-south, add to no more than half its longer side (in a square box a
  diamond, its points at the walls' middles; in a long one, its four corners cut off on the slant); the squares cut
  become its walls (masonry, as its others), a staircase of them along each slant.  A door's, a doorway's or a
  window's square in its old walls is kept joined to the floor by the squares straight in from it (a short alcove),
  its steps kept, so it's walked as it was; its things, traps and sconces are put on its floor as ever.  Its words and
  its size are its box's (the thirds of "the far left corner" by the box; a diamond's own words are still to come).
  The passages to it are the slanted strata's.

## Tunnellers *(slice 4.11, 2026-11-01: the book's where it speaks, the house's else; drafted for the user to review)*

The HarnWorld Bestiary calls rats efficient diggers that excavate burrow systems, and has its grave-wights dig rapidly
through most soil; the Khuzdul and the Gargun dig their halls.  It gives no rates.  These are the house's.

- **Who digs** *(the house's)*: kinds.txt's ways **d**, digging by nature with its claws (a burrower: the giant rat, the
  ghoul, after the bestiary's rats and grave-wights), and **m**, digging with the tools it carries (a miner: the dwarf
  miner, the kobold, the brown orc).  A miner digs only with a pick, or with a shovel, a spade or a mattock, among its
  things (the world's agear, AG_PICK and AG_SPADE).  A dwarf miner always carries a pick, a brown orc sometimes does,
  and a kobold always has one at its belt (2026-11-02, outfits.txt's [role kobold]: the folk tales' mine-kobolds).
- **A burrower's claws** (the user's, 2026-11-03, "Soft rock only": "Shale is too hard for claws; the same soft-rock rule
  at work and in a chase"; the numbers *the house's*): two feet a spell in soft soil, one in hard-packed earth (two thirds
  of a shovel's), rubble as soil; nothing in masonry, flags or any rock, shale too.  The one rule at its work and coming
  at prey, a bare wall by its stuff.  The rock's softest is shale, and the maker's walls are stone or masonry, so as the
  game stands claws bite rubble and nothing else.
  No rock softer than shale is added (the user, 2026-11-03: "we are fine without generally allowing claws to dig rock"; clay or softer wouldn't stand unshored): a kind that tunnels through rock has it said on its sheet.
- **The character's rules** *(every actor's: DUNGEON.md's digging)*: a spell of ten minutes at a time; the tool's rate
  on what it's into (rshouse.h's digger table, the character's); earth and rubble at the tool's rate, untested;
  anything harder a test each spell, its rate at a marginal success, twice at a critical, half at a marginal failure,
  none at a critical failure; the rock's hardness the rate's (rock.h's); a wall two feet, a square of rock five, corner
  to corner seven, rubble two; the hole kept as it goes (the change record DUG, the character's: the character can
  go on with a hole a denizen began, and the other way about); through, the square a passage (a diagonal's corners
  cut, as the character's).
- **A creature's test** *(the house's, both referees)*: the bestiary has no Mining, so a creature's dig is its
  Strength times five (HarnMaster's, its sheet's: as its Stealth is its Agility times five), its injuries' penalty off
  it, on dice of its own (moving quietly's: the game's other rolls as they were); the null ruleset's flat odds.
- **Its spell reckoned as it ends** *(the house's)*: the feet a spell makes are reckoned when its mind asks again at
  its end, the same way; a spell cut short by anything else it does (a step, a blow) is lost, as the character's part
  spell is; roused meanwhile (a noise), it goes back to it for the rest of the spell.  Nothing it has bites into what's
  there: it gives up for five minutes.
- **Where it digs** *(the house's)*: a hostile tunneller kept from the character by rock, a wall or rubble (no step
  nearer: the character sensed beyond it, or where it last sensed it, drawn on) digs the square beside it that brings
  it nearest, the four first, a diagonal where it's nearer as the crow flies (round a shut door or a portcullis it
  can't open: a way round it), within its reach of its home (twice its range and eight squares); the one it's digging
  kept while it's nearer.  Digging spends none of its being drawn on.  A step on a diagonal way nearer as the crow
  flies, as near across and down, is a step nearer too (every denizen's: so it comes through what it dug).
- **Heard and seen** *(the house's)*: a spell's noise as loud as the character's (6), the way through louder (7, its
  spoil falling); the rock carries a pick's knock where the air doesn't (each four squares between them as a square of
  wall to the sound).  The player's told its first spell ("The dwarf miner digs into the wall.", or "You hear a
  scraping in rock ahead."), not each spell after (the denizens hear every one); the way through seen ("A fresh tunnel
  opens ahead. Spoil lies heaped by it.") or heard ("You hear rock falling to your left.").
- **Its work's dig** *(the house's, 2026-11-02)*: a tunneller's other reason to dig is its work, as its day says (the
  day's work hours, DENIZENS.md's daily life: decided by the clock, never polled).  A miner (ways m, with a pick) digs
  its faction's mine: from its lair's home straight across to the vein of its region (rock.h's veins: a seam of ore
  or of quartz; a dyke of basalt isn't mined), its middle on the home's row no more than a chunk's breadth away, then
  along the vein (north or south by its lair's number) row by row, keeping to its middle (a diagonal step where the
  vein slants a square a row, across it where it slants more), 64 squares at most.  A burrower (ways d) digs its
  lair's burrow: from its home one of the four ways (by its lair's number), eight squares, as far as its claws bite
  (A burrower's claws, above: rubble, earth, soil, and no rock; the one rule coming at prey, settled by the user, 2026-11-03).  A kind that has its own work (a smith's forge) digs
  when there's a vein; one with none (a rat, a brown orc) half of them dig through their waking hours, the other half
  go about as before.  Nothing of the mine is kept but the dungeon itself: its way is worked out again each time
  (the squares dug already, the open ones and the doorways on its line walked through), and it digs the first square
  that's a wall, the rock or rubble, from the square before it, a spell at a time by the character's rules (above);
  a shut door on its way to it opened as one following the character opens one (its key if it's locked; never
  forced).
  Within its home's chunk only, and only where that chunk's made; it stops at water, a pit, a portcullis let down
  (its work raises none), the chunk's edge.  None near: its work's room, as ever.
- **Away from the character** *(the house's, 2026-11-02)*: a denizen out of the character's reach (not an actor: its
  record moved coarsely) digs its work's spell all at once as it's due, untested (a marginal success's rate: no dice
  of its own), the hole kept and the way through made as an actor's are; it's never told (the character isn't near).
  Its tools aren't looked at away from the character: a kind whose miners work (the dwarf miner, the kobold) digs as
  if with its pick; a brown orc digs only when near.
- **Poor rock on a tunneller** *(the house's, 2026-11-02)*: a tunneller's spell gone badly (a critical failure) in
  shale brings the roof down as the character's does: two feet of the hole filled again, the rock showering it ("The
  dwarf miner is showered with rock from the roof.", heard: rock falling).  No harm comes to it: a denizen's wounds are the fights' only, as yet.
- **Its sound played** *(2026-11-02)*: a tunneller's spell (begun, each one after) the sound task's dig, its way
  through and rock coming down a smash, by the rock's carrying (the noise's x[3]) as any denizen's sound.
- **Builders** *(the house's, 2026-11-02, for review)*: a faction's miners are its builders too.  At work, a miner
  first puts up again a door broken down in its home's chunk (a door's leaf burst from its frame, the wrecked door):
  from a square beside it, a spell's work, the door shut, its lock and bar whole again, its harm gone (the portal's
  change records, as any door's: "The dwarf miner hangs a new door.", heard: hammering).  Away from the character, at
  once as it's due.  Then it walls up a way dug into a room of its home's chunk by others (a passage a dig made, the
  character's or a tunneller's, a room's floor beside it, not of its own mine): from the floor beside it, an hour's
  work, the square a wall again ("The dwarf miner walls up the way.", heard: stones being laid).  One square, at the
  room: the way behind it stays; its wall can be dug again (two feet).  *Asked, and left as it is (the user's, 2026-11-03):*
  it can wall up a room the character dug into, with the character inside and no tool left.
- **Not yet** *(the plan's)*: builders shoring a passage (timbering in the soft rock: nothing yet comes down on a
  passage, only on a dig's face, so it would change nothing), the materials as things (timber, a door's leaf, stone
  carried), the spoil as a thing (rubble beside the way: no such thing in the catalogue yet), harm from rock falling
  on a denizen, a mine past its home's chunk.

## Denizens and doors *(2026-11-02: Phase 3C's one path; drafted for the user to review)*

The book has creatures open doors as anyone does; it says nothing of who carries which key, or of beasts and doors.
These are the house's.

- **The character's rules, every actor's:** a denizen opens, shuts, bars and unbars a door, forces it (its Strength,
  its own sheet's, against the door as the character's is) and batters it (each blow the character's attack at a thing
  the door ignores, by its own weapon or body, off the door's body), by the character's code; its time its own.  The
  code would let it set a bar and lock a door with its key too; no mind asks that yet.
- **No trap sprung** *(the house's; left for now, the user's, 2026-11-03)*: a denizen never sets off a door's trap as it opens it, as it never sets
  off one underfoot.  A faction knowing its own traps (and being caught by another's) is a planned item; till then
  none is.
- **A lock picked** *(2026-11-02, the house's)*: a robber's kind (kinds.txt's ways r: a bandit, a kobold, a brown orc,
  their leaders) with no key to a
  locked door picks its lock, by the character's code (a minute a try, its test the lock's steps worse), two tries,
  before it forces it; its tools its own (none in its things: the house's), its test by its sheet (the bestiary has no
  Lockcraft: its Dexterity times five, as a creature's other tests; the null ruleset's flat odds).  Heard working at
  the lock 1 ("a faint scratching"), picked 1 ("a lock clicking").  Other kinds pick none.
- **Hands** *(the house's)*: only a denizen with hands (a two-legged body, kinds.txt's) opens, shuts, bars or unlocks
  a door.  A beast opens none; a large hostile beast (its size 5 or more: a bear) batters one that keeps it from its
  prey.  So a rat or a snake stops at a shut door, and a bear breaks it down.
- **How far it tries** *(the house's)*: its key first (if the door's locked), a robber's picking next, then, hostile,
  two forcings (ten seconds each), then a dozen blows; then it gives up and goes round as its steps can (its way found:
  Ways found, below).  One not hostile (an ally, one going about its day) only opens and unlocks.  A hostile one
  drawn on loses none of its while for working at a door, as digging loses none (2026-11-02).
- **A portcullis** *(2026-11-02, the house's; the character's code, wwin.c's)*: one with hands, its way through a
  portcullis that's down, raises it by its winch on its side (half a minute's winding, loud) or heaves it up from the
  far side (a round, its test by its sheet, P_LIFT six steps worse), two tries; then, hostile, it bends the bars
  apart (ten seconds' straining each, the bars' thickness the harder), two tries; then it gives up (and a tunneller
  digs round).  Against the character (fleeing, shy, or watching it, beyond the portcullis), from its winch's side it
  lets it fall (no one under it).  Beasts work none.  Heard: winding and heaving 7, raised or let fall 8, bars bent 8.
  Windows are left alone (to come: shutters, glass, a climb through).
- **Shut against the character** *(the house's)*: one fleeing, shy, or watching the character shuts an open door
  between them (the character beyond it, within eight squares, no one in the doorway); fleeing, it slams it, louder
  (5, a door shut 3).
- **A door left open** *(the house's: DENIZENS.md's daily life)*: one on watch, walking its rounds, that finds open a
  door the character has handled (the world's PP_KNOWN) shuts it and stands alert a minute.  A door the denizens open
  or shut themselves isn't noticed so.
- **Keys** *(the house's: keys that fit, every actor's)*: each locked door of a lair's chunk has a key, two at most,
  made with its lair's own things and carried by its first denizen (a theme's leader, with its home, one too); a
  door's key opens its lock in three seconds, no test, and locks it again, shut; another key doesn't fit.
- **Heard** *(the house's)*: a door opened 2, shut 3, slammed 5, a key turned 1, a bar lifted or set 3, forcing and a
  battering's blows 7, a door broken down 8.

## Denizens handling things *(2026-11-04: Phase 3C's one path, the rest; new, for review: when each does it the house's, how the character's rules)*

- **It eats what it holds:** hungry, with food in hand: one less of the lot, five minutes (a meal's, as the character's); it's fed.
- **It hands food to a fellow:** not hungry, holding food, a hungry one of its faction beside it, with a hand free: two seconds; the fellow eats it at its next look.
- **It lets fall:** fleeing, broken: what it holds that's no weapon nor light, at no time, where it stands, as the character's let fall (a shaft's head takes it).  Wanting a weapon, its hands full of other things: puts one down first.
- **It throws:** an ally with a thing it fetched, no way nearer the character, in its sight: by the character's own throw (the catalogue's reach, the four bands, Throwing by its sheet), two seconds; lands by the character's feet, or short of it.  Only that as yet: a mark but the character, and a rock picked up (no loose stones in the dungeon), to come.
- **Seen, or not told:** what a denizen does with things is told only if its square's in sight.
- **Not by these:** an ally told to drop a thing, or to give it to another: no words for it as yet.

## Denizens taking things *(2026-11-02: Phase 3C's one path; drafted for the user to review)*

- **The take rule, every actor's:** a thing lying on the taker's square or beside it (or on what lies there), not a
  door's part (lifted out first), not fixed where it stands (a theme's altar); then the taker's room for it: the
  character's pack or hands, as ever; a denizen's hands (a two-legged body's), two things held at most, or its mouth
  (a beast's), one.
- **What it lifts** *(the house's, the fetching numbers as they were)*: in its hands 50 lb, in its mouth 3 lb.  A lift
  by size (a larger beast lifting more) is planned, not built (DUNGEON.md's still to come).
- **What it wants** *(the house's)*: only folk take things up (a beast hunts, and takes nothing); one with no weapon
  to hand takes up one lying near (a sword, an axe, a club: the catalogue's weapons held in a hand); a hungry one food
  (eaten at its next look: Denizens handling things, above; what's eaten, by the catalogue's key "food", not the appraisal's craft, that takes in a candle); a robber (kinds.txt's ways r) coins of any kind and jewels; nothing lying at its own home (its lair's own things, a robber's takings put down there).  It
  looks once in a hundred seconds, as it isn't fighting or digging, going on with what it was doing; one it sees within
  four squares, idle, it goes to (its steps), and takes up beside it.  An ally takes nothing of its own accord.
- **Two seconds** *(the house's)*: stooped, taken up, as the character's handling of a thing in reach.

## Ways found, positions, bodies found on the rounds *(2026-11-02: the minds' leftovers; drafted for the user to review)*

- **A way found** *(the house's: no book rule)*: an actor that can't step nearer to where it's going (its foe, where
  it last sensed it, its bed, its rounds, the character an ally follows) finds a way there, once: a field over the 32
  squares each way about it and its mark (the chunks made only), each square's step toward the mark, searched outward
  from it the four ways and the diagonal ways (past their corners as the walk's rule), no rise or fall more than a step
  takes (on steps, a stair's), a shut door a way for hands (worked as it comes to it), not for a beast; and follows it
  a step at a time till it's there.  Four fields kept, shared by every actor bound for the same mark (the character: within four squares of it);
  another found ten seconds after the last at most; a mark further than the field reaches, its edge nearest the mark.
  None to be had, the old greedy step (none looked for again for five minutes), and a tunneller digs.  Water and bad
  air are the step's own tests as it goes, not the field's.
- **Lying down to sleep** *(the house's)*: a denizen asleep at its bed lies prone (a moment's), and gets up as it
  next acts (a step's or a blow's Rise, the referee's or the world's round, as one knocked down; else a Rise alone).
- **A sleeper struck at** *(the book's Ignore, Combat 9; the null ruleset's two steps easier)*: a creature asleep
  doesn't defend the blow that wakes it; prone, it's the easier struck as any prone.
- **A body found on a record's rounds** *(the house's)*: one away from the character (not an actor), on watch at its
  rounds' place, that finds a body of its faction's not found yet within four squares: its faction's attitude to the
  character a step worse (as an actor's finding); the alarm raised out of the character's hearing: each of its
  faction's others within 24 squares of the body gathered to it (a square of its own about it), each a step less well
  disposed, staying a while (ten minutes), their day after; a leader's body (its rank), each known lost to them all
  (D_LOST: their bonds to it gone as they next reckon), no test of courage till they're actors.
- **Not yet**: a shooter crouched behind cover, a parapet or a slit's wall (the world gives cover only by the slits and
  windows, and the bodies' positions are every actor's already, slice 4.6's: a cover map, a position the minds choose,
  and a crouched mark's own height for a missile, to come); a rock picked up to throw (the dungeon has no loose
  stones; the spoil as a
  thing, slice 4.11's, would give them); windows worked by a denizen.

## The market, entertaining and remedies *(2026-11-02: the book's where it speaks, the house's else; drafted for the user to review)*

The book (HarnWorld's prices): its listed prices are retail at their source; a mercantyler buys at as little as a tenth
of them and sells at about half; a seller sets a price by the buyer's purse, and haggling moves it by a quarter or more;
a jeweller or a mercantyler buying a jewel pays about half (Treasure 5).  Rhetoric is the mercantyler's first skill
(Character 17).  The rest is
the house's.  *(Every number here is for review.)*  (dq0d.c's part 16: s_gift's trade and entertain; dq0w's things.c
part 62, goods, and wfetch.c part 72; the UI's errand.c, part 15, in overlay 17's bank.)

- **Who trades:** any denizen with a mind that isn't a beast (folk), friendly to the character or an ally (the user's, 2026-11-03,
  DECISIONS.md's A2: "Friendly folk only"): "The bandit won't trade with you." if it's indifferent or worse, its price, its offer and the
  deal alike, so trading has to be earned first (a gift, entertaining, words).  An antidote or a remedy is sold by this rule like anything
  else; an errand asks an ally already, so one that isn't refuses as ever.  No merchant kind yet (the peoples have none): every friendly
  folk denizen trades what it carries.
- **What it sells:** what it carries but what it wears and what it holds itself (its own gear in use): its pouch's, its
  pack's, what hangs at its belt (the world's goods, IO_GEAR's GR_GOODS, the nearest named or any).  What it buys:
  anything the character carries.
- **The price:** the thing's worth as a local takes it (the appraisal's: by its material, coins by their mints), times
  the count, by its condition (its life left in tenths).  It sells at the whole of that friendly, three quarters to
  an ally; it buys at a half, three fifths.  Rounded to the penny (buying up,
  selling down); **under a penny it's in farthings** (the user's, 2026-11-03: a farthing candle sells for a farthing), a
  farthing at least.
- **Haggling** ("haggle for the rope", "bargain over my dagger": selling if the character carries it, else buying):
  the book's Rhetoric in the trader's tongue (none in common, no haggling), at steps by its attitude (hostile -3 ...
  friendly +1), its kind's sway and its mind (the cunning one the harder, the low one the easier); a critical success
  a quarter better, a success a tenth, a failure as asked, a critical failure an offence (a step back, no deal).  **Buying
  never comes below selling for the same thing, whatever the critical** (the user's, 2026-11-03): *the house's way, for
  review:* no one buys under two thirds of its worth, nor sells over them, an ally's haggling being the only case that
  reached them (56 and 75 hundredths).  The deal done at once if it's paid.  Half a minute's work.
- **Asking** ("how much for the lantern", "what will you give for this"): within earshot (12 squares), no time but a
  moment's.  **The deal** ("buy the rope from the dwarf", "sell my dagger"): within reach (a square).
- **The money:** the payer's one pile of pennies, at face, for a price in pennies; of farthings for a price in farthings
  (*the house's, for review:* a character with no farthings pays a whole penny, no change given; a trader with none can't
  pay: no pump either way); the character without enough: "You haven't the money: the dwarf asks 12 pennies."; the
  trader: "The dwarf hasn't the money".  *Not yet* (DECISIONS.md's, the user's of 2026-10-28): paying with crowns, a price
  over a penny in farthings, change given, the lowest-worth coins first, the seller asking for the local or the dwarves' coin, "pay with Kanday pennies", the coins'
  worth abroad in the trade itself (it's at face now).
- **The exchange** (the one path): the trader's own intents, the character's way with things for every actor (the
  world's wfetch.c): it takes the payer's coins or the goods sold (FE_GIFT, a count split off, kept in its things, not
  held), and hands over its own (FE_GIVE, a count of a lot split off, from its pouch or its pack: into the character's
  pack first), each told ("The dwarf takes the pennies.", "The dwarf hands you the rope.").
- **Entertaining** ("entertain the dwarf", "sing to the bandit", "play for", "dance for"): within earshot, no tongue
  wanted; the referee's performing skill (HarnMaster's Singing, unsaid: the book's automatic skill, everyone's;
  Musician; Dancing), at steps by its attitude and its kind's sway; a critical success two steps, a success one, a
  critical failure a step back (it's bored), as far as friendly (a song doesn't win an ally).  A minute.  Not a beast,
  nor the mindless.  The null ruleset a flat test.
- **Remedies** (the afflictions' cure: Poison, bad air and disease, above): an **antidote** (a theriac: the house's 48d, a
  vial's dose) against a toxin (a venom, a poison), its power 2; a **remedy** (a physician's draught: 12d) against a
  disease, its power 1.  Drunk ("drink the antidote", "eat the remedy": the things' IO_USE by its key, used up), the
  referee told (RS_TIME's RT_TAKEN): dosed by the taker's Physician (the book's: medicine combats an infection; the
  house's: a critical success a point the more, a critical failure a point the less), the toxin's or the disease's
  healing rate the better by its power (at H6 gone: "The remedy works"); nothing on it to treat, "It does nothing for
  you."; the lindworm's numbing venom only by a strong one (power 2).  Given to another, a gift (the denizens have no
  afflictions of their own yet).  Found: an apothecary's kit, a Khuzan elder's purse, a high priest's (outfits.txt);
  for sale by whoever carries them.  The null ruleset: a poison's sickness or a disease's gone at once.

## The character carries on *(slice 4.4, 2026-11-02: the book's where it speaks, the house's else; drafted for the user to review)*

The plan's slice 4.4 (referee: improvement rolls; UI: save; "skill improvement by use and rest"; a save that keeps the
character) and the user's D7 and D8 (DECISIONS.md: death permanent, with autosaves before danger as a setting, off
hard mode; the character carries on into a new dungeon with its skills, wounds and gear), and the user's of 2026-10-23
([First Use] skills open on first use and are told).  The referee's (dq0harn/skills.c, overlay 7), the world's asking
for its words (wtend.c), the UI's start and end (main.c).  D13's rules, built with it, are filed here too: free pounds, a
weapon's version and Strength, a wrecked door, a slip out of a pit.

- **A skill used earns its development rolls** (the book's Skills 7: a stressful use earns one, success or failure alike: one learns
  as much from failing; the user's, 2026-11-03, DECISIONS.md's A3: "Stress, or 5 routine uses").  Used: any test of a skill the referee
  makes for the character (a purpose's skill, test_sk's: climbing, searching, picking a lock, first aid ...; the first use of a [First Use]
  skill opens it, as ever); in a fight, the skill of the weapon it strikes with; **what blocked marks its own skill** (A7: a shield's block
  Shield, a weapon's its weapon's, a bare arm's Unarmed), and Dodge when it dodges, a blow or a shot at it; a missile's skill as it shoots
  or throws.  An attribute's test (Strength, Will) marks nothing: the book develops skills.
- **What's stress** *(new, for review: the book leaves it to the GM)*: a point each for **a foe near or a fight** (a hostile awake within 30 feet, the
  reach of a long task's hard stop; any blow, shot or engaging); **a trap's danger** (disarming one, dodging one); **wounded or in shock** (any
  injury); **in the dark without a light**.  One point, a roll earned.  **Two or more at once is a very dangerous use: two rolls** (the book
  allows 2 or 3).  None, it's routine: counted, and **the fifth routine use a roll**, the count back to none.  A glance, a sound or a smell the
  senses test unasked counts as routine, whatever's about (they're many; no foe sought for them).  **Three rolls a skill at most** till they're
  made (the book's 2 or 3; else a fight's every blow a roll): the rest of a long fight earns no more.  A byte a skill (its count, its rolls, in the referee's
  bank past the bodies), kept in the character's file: its "body u" lines, "body u<its rest> " and each skill's number and byte, two hex
  digits each, those that have any.
- **The rolls come after a night's rest** *(the house's)*: eight hours of rest since the last rolls (sleep, a wait,
  standing still: the time the world tells the referee of, RS_TIME's at rest; every minute while it's hurt, else at the
  clock's four-hour watches, so a rest is counted in those pieces), each roll earned a roll, the book's: d100 and its
  skill base over its mastery, a point more, never past 100 and the base.  Then the rolls are spent, each skill's routine count kept; the rest
  past the night isn't carried.  Told ("Your Climbing skill has improved (46).") as the referee's other news is, as it wakes
  (held while it sleeps).  The rolls by their own dice (QUIET's: the game's other dice roll as they would).  Why rest:
  the plan's "by use and rest"; the book's stress rolls are the GM's to grant at once and its practice rolls monthly,
  and a night's sleep is a natural time between, out of the player's way.
- **Not yet:** the book's monthly practice (SMPs) and teachers (no study in the dungeon); its skill decline (optional:
  not used).  Its limits on weapons (past 70 only by combat) and Initiative (only by combat) hold of themselves, all
  the game's marks of them being a fight's.
- **Every actor:** a denizen's sheet (fight.c's struct beast) has no skills of its own, its kind's figures the
  bestiary's, so it neither marks nor develops; the hook (skill_used) is the character's sheet's.
- **[First Use] skills open as they're first used** (the user's, 2026-10-23; the book's skill table: those with no
  months of teaching, Religion's rituals excepted): opened at their opening level as the test is made (its figure the
  same: a skill not opened was tried at its opening level already) and told, "You've begun to learn Swimming by doing it
  (12)."  The skills the game tests that open so: Swimming (P_SWIM), Acrobatics (P_BALANCE), Physician (first aid),
  Cookery (appraising food); and a weapon's skill the first time it's used in a fight (the book's, Combat 19).  Those
  learnt by study stay unopened, tried at their opening level as before: Lockcraft, Lore, Tracking, Animalcraft,
  Mining, appraising's crafts; Drawing the house's exception as before (the map kept).
- **Told as soon as the world asks** *(the house's)*: a skill opened mid-test is news the referee holds (RS_DESCRIBE b
  4's), flagged in the character's conditions (rs.h's C_NEWS) so the world asks for the time's news within the minute
  (wtend.c), as it does for what time did to the body.
- **Free pounds** (D13, the user's): the first pounds carried are free of encumbrance, ten at Strength 10, doubling each
  four points (tenths of a pound: 100, 119, 141 and 168 at Strength 10 to 13, twice those each four more: 4 lb at 5,
  14 at 12, 24 at 15, 40 at 18, 57 at 20, 134 at 25).  The book's encumbrance (the load over Endurance) is on the rest.
  The sheet tells the whole load and the free part: "load 68 lb (14 free), encumbrance 5".  (skills.c's set_load.)
  Every actor where it applies: a denizen's load isn't counted (its sheet has none), so it bears its things freely as
  before.  The null ruleset has no encumbrance.
- **A weapon's version, and Strength** (D13, the user's: Strength doesn't add to impact; it sets how heavy a weapon can
  be wielded without penalty; heavier versions hit harder; strong characters are made with heavier weapons).  The
  book's custom weapons (Combat 3): a weapon light (a quarter lighter), average, or heavy (a quarter heavier), its
  item's own (items.h's WV_*, in its size; its weight the world's, so its load).  Its impacts by its version, the book's
  table at Strength 9-12 for every Strength (the user's: Strength adds nothing): light 80%, average 100%, heavy 120%
  of each aspect's, rounded *(the house's reading)*.  The heaviest version wielded without penalty *(the house's
  numbers, for review)*: light at any Strength, average from 6, heavy from 13 (the book's table's row boundaries);
  each version heavier two steps (10) worse to strike and to block with it.  The character made with its weapons the
  heaviest its Strength wields freely: heavy at 13 or more, light at 5 or less (the referee's RS_OUTFIT x[2], the
  world's kit).  Appraised: "It's a heavy one." (plain to the hand).  The referee told by RS_EQUIP's marks (L or H after
  a weapon's quality).  Every actor: a denizen's weapon is average (the outfits' gear's versions to come), its Strength
  its sheet's, rolled about its kind's (the book's less 7 and 2d6), so one under 6 is two steps the worse with it: one
  kobold in six (its kind's 8), one in 36 of the kinds at 10, none of those at 11 or more.  A blow at a thing, a
  missile, are as before.
  (skills.c's heft, fight.c; things.c's tools, s_weight_of, s_kit.)
- **A wrecked door, window or portcullis appraises by its state** (D13): its quality asked, "You judge it ruined, past
  mending.", its make no matter now; a closer look and its condition tell it broken down as before.  (fixt.c's
  o_portal_look.)
- **A slip climbing out of a pit ends the climb** (D13): a marginal failure off its floor slides it back to the floor
  ("You slip, and slide back down to the pit's floor: 10 feet to climb (climb up to try again)."), the climb over, the
  player's to say what next; "climb up" (u) in a pit climbs out again, to where it fell in from (or the square it faces,
  a game carried on since).  A critical failure is a fall, as before; a ledge's slip slides 5 feet back and goes on, as
  before.  (dq0w.c's climb_round_on, pit_climb.)
- **Death, and the autosave** (D7): the game is kept as the character comes into a stratum, and as it's quit (the
  autosaves); a death is not kept.  What's next is said last, after the life's account, by what's in the folder (dq0end's: the same tests the next start makes; the user's, 2026-11-04: "Change the words", and no save on arriving):
  - A save in this dungeon: it stays, to go back to.  "Your last save is kept: start the game again with its folder to go back to it."
  - Nothing saved yet, the first dungeon: the folder's next game is a new one, a new character, and the life's told as a life, not a life so far.  "Nothing was saved before you died, so there's no save to go back to: start the game again with its folder, and a new character begins."  *(The words new, for review.)*
  - Nothing saved yet, a later dungeon: that dungeon begins again, by its own seed, the character as it came to it (the sheet and things the time between left).  "Nothing was saved in this dungeon before you died: start the game again with its folder to begin the dungeon again, as you came to it."  *(The words new, for review.)*

  **Hard mode** (dq0 -H, kept in the
  save: the user's "turned off, it's hard mode"): a death ends the save, saved or not before it, "Hard mode: there's no save to go back to.",
  and the folder begins a new game, a new character.  Said once, it goes on with the life, each next dungeon in hard mode too (the record's LF_HARD).  *(The user's, 2026-11-03: they stay as they are, at a stratum and a
  quit, with no save before danger and no pause as a fight starts, till a playtest shows the lost play hurts.)*
- **The ending** (death, or out of the dungeon, solved or left): the theme's words, what's next if it's alive ("Your
  character carries on: start the game again with this save's folder, and go on into a new dungeon."), the character's
  sheet as it ends; dead, the life's account, then what's next (above).
- **Carrying on** (D8): the game begun again with the folder of one ended alive (solved or left: How a dungeon ends) begins a new dungeon (its seed -s's, or the clock's: a new history and theme) with the same character: its
  sheet as it ended (its skills, its wounds as they are, healing on, its fatigue, its skills used since their rolls),
  and its gear: the things it carried out, its money among them, kept with the save (the world's kit.dat, written
  with every save: what's worn and held, then what's in each) and made as its outfit in the new dungeon in place of one
  drawn.  **Each thing as it was** (the plan's step 3, 2026-11-04: "made new" stops): its life and wear (a dulled blade stays dull), its
  fuel, its version, what's shut.  *(New, for review:)* **coins change at the money-changer's 15%** (each pile, rounded to the nearest, so
  a pile of three or fewer pays nothing; all the new region's then).  Left behind, a key, its lock the old dungeon's,
  and the old theme's own things, its puzzle's pieces, with what's in them; 64 things at most.  A new life empties kit.dat first.
  Its ruleset the one dq0 -r names (harn if none).
- **A life's record** *(new, for review: the plan's step 3; DUNGEON.md's a character's life; life.h)*: a life begins in the year 720, on a day
  of it by its seed; the character is then 16 to 25, by its seed, the year it was born kept in the record (the book leaves the year to
  the GM, Character 3); its age on a date is that and its sheet's birthday.  Nothing reads the age yet (the book's aging begins far
  later).  The next dungeon begins the day after the last ended at the soonest; what passes between them is below (between dungeons).  Its hour
  is its seed's, as ever.  Each dungeon's folder (the user's Q3) is kept: two dungeons hardly played came to 156K of the card's free room (df; 44K each by du), so a long life is nothing on any real card.
  *The user's, 2026-11-03 (A8): a random time between dungeons, with healing, gear's maintenance at a cost, and skills by the book past 30 days: the plan for the game between dungeons (Decided, not yet built, below).  The rule stands till then.*

## Between dungeons *(new, for review: the plan's steps 4 to 6, 2026-11-04; DUNGEON.md's a character's life; the user's Q4, Q5, Q6, Q7)*

The game rolls it all between the ending and the next dungeon, and tells it in a dozen lines (games/dq0acct).  **Every number here is the house's and for review unless it's marked the book's.**  No town yet: a made-up town and inn name by the seed, nothing to do in them.  No rumours: the plan's step 7.

- **How long:** the days until no wound worse than minor is left, no infection, no shock, no toxin or disease (by the real rolls), at most 90; the road, 2d6 days; a lull, d100: 01-50 1 to 7 days; 51-80 14 to 21; 81-95 30 to 60; 96-00 90 to 180.  All at rest, every wound rolling all the while; work's days after them.
- **Healing** (the book's, Physician 2-4; the referee's own code told the days): each wound's five-day roll, blood back on the same days, infection's daily roll, shock's watch rolls, venom and disease running their course.  **A character can die of it** (the user's Q5: the roll as it falls): told, its outcome died, the life's account follows; there's no save to go back to.
- **Bleeding** as it comes out: bound at once, its own Physician (or a layman's 30) +50 a minute's try, a point of blood a minute each (the referee's folk's way); it can bleed to death on the road.
- **A physician** for each wound not yet treated, if there's money (or spare goods to pay with): the book's one treatment roll, at Physician 60; 3d a visit, 9d for a grievous wound.  With no money, it tends its own minor and serious wounds if its Physician's open (the book: not a grievous one); else the book's untreated rate.
- **Hunger, thirst and fatigue** clear (a sickness still on it keeps its levels); exertion gone.
- **Living** by its class's habit: slave and serf 1d a day (a hayloft, market bread), unguilded 2d (the common room), guilded 3d (a shared room), noble 8d (a room of its own): lower if what it holds won't last.
- **The money, in the user's order (Q6, Q7: it just happens, no debts):** (1) spare goods sold till all it wants is paid for (what it wants asked again as each goes: below, nothing to hand), at the market's half, by its condition, the cheapest first, valuables (a jeweller's kinds, gold and silver things) last.  Never sold: what the player named (in the dungeon just left: names don't carry on yet), armour, what's worn or in its hand, its dearest weapon if none's in its hand, what's used up (it'd buy it back), a key, a tinderbox, what holds anything.  (2) Down to rough.  (3) Work for what's still short: its occupation's wage (the book's, Character 13; a class's if the book has none: 24d, 24d, 42d, 60d, 72d) at half (the book's journeyman), a calendar day's a thirtieth of the month's, its room and board found (the book's), at most 60 days, those days added to the time; what's short past that is found for nothing.  The book's bonus practice (Skills 7): two points a working day (eight hours), 24 days in 30, ten a roll, on the first three of its occupation's skills that are open, each under a master's 80.  (4) It goes on with what it has.
- **Mending** (Q7: mending, then restocking): a tenth of the thing's price for each tenth of its life lost, at most half; past that a new one at its whole price (the same record made whole).  **Whetting and oiling** a hundredth of its price a month, at least a farthing, and only what shows wear (only what wears today wears; the book's weapon damage and armour holes would plug in here).  In the kit's order, each as the money holds.
- **Restocking:** torches, candles, oil, pitch, chalk and every counted lot (food, bandages, arrows ...) back to what it set out with in this life (kit0.dat: the world's first save of a life), at their whole price, if the money holds; what's burnt out or empty thrown away.  Waterskins filled, free.
- **Nothing goes on what isn't to hand** *(new, for review: the user's "Skip it", 2026-11-04, for the kind)*.  What the account sees is what it carried out (kit.dat), so a thing left in the dungeon was never in it; what goes is what it's sold here.  No selling and no day's work for mending or whetting a thing it's sold, nor for arrows, bolts or sling stones with no bow, crossbow or sling left to shoot them (none bought, either).  A thing to be mended isn't counted as whetted too (mending does both).  Tending is its own body: nothing to reach.
- **The money-changer's 15%** stays as the next dungeon begins (carrying on, above); the account's last line says what it'll come to.
- **Practice** (the book's, Skills 7): three development rolls for each whole 30 days, odd days dropped, one after another on the three skills.  **The question (Q4):** the skills filled in, those it used most (what's on its sheet since its last night's rest: rolls earned and routine uses), then its occupation's, then its best; Enter takes them, or name your three (a word each, the start of the skill's name; fewer named, the rest filled in).  None past 70 in a weapon (the book's, Skills 18), no Initiative, Dodge or Ritual (the book's), no Script without something to write with (the book's, Skills 11).  *(New, for review, the user's "Skip it":)* by what's left after the selling, no weapon skill without a weapon of its kind to practise with (a tool, a torch, a broken sword won't do), no Drawing without something to draw with, no Lockcraft without lockpicks: the filled-in answer passes over them, and one named is told ("You have nothing to practise sword with.").  Other skills ask for no tools: study, or an hour's work with what's about.  Rolls on a weapon that's reached 70 are lost.  The rolls for skills used in the dungeon stay as they are.
- **Date and age** told: the record's date on by the days.

## Decided, not yet built

What the user has decided and no section's rule holds yet.  Most that's decided and waits is in its own section's *Not
yet*: the character's morale as an option (D14: Morale); a
standing order to eat and drink, and rain washing traces away (Water, hunger and thirst; Light and traces); a cornered
rat fighting back (Fights' a helpless character); paying by the coin (The market); the mages' occupations (Characters);
rain at what shows the sky (What shows the sky).  The powers (D11: spells, rituals, psionics: Phase 5)
have no rules here yet.

**The user's review, 2026-11-03: to build** (DECISIONS.md's HOUSE.md's review, their words).  Each rule stands as it is till it's
built, then its line here goes:


**The game between dungeons** (the user's, 2026-11-03: A8, A18, A20; the plan's first two steps built: How a dungeon ends, above; the plan outside the repository): what takes a character
to a dungeon in the first place; a random time between dungeons, with healing auto-rolled, weapons and gear maintained at a cost, food
and housing paid for, and skills by the book past 30 days; clues taken to a library or a scholar, languages and scripts known, scribes
and scholars as characters.

- **A theme's own tests name a purpose** (the user's, 2026-10-20), as the rest of the game's do, which each
  ruleset's referee maps to its own skill.  Today they're an even chance (P_THEME on).
- **Climbing's rate by the face's grade** (the user's, 2026-10-20): faster up an easy face, slower up a sheer one,
  once the maker gives faces a grade.  Today it's 10 feet a round at a marginal success and 20 at a critical,
  everywhere.

- **Auto-engaging at a range the player sets** (the user's, 2026-10-20, an enhancement): "autoengage with sword at
  close range", "autotarget with bow at medium range": a hostile coming within it begins fighting on (melee's
  close range the engagement zone and a square more, closed in on; a missile weapon's the book's bands).  Built in
  part (the auto-target range, the missiles' section, above); today a hostile merely coming within it begins nothing,
  and only an enemy engaging the character, or one attacked, is fought.

## Open

- **Sight's detail by distance** (the user's, 2026-10-20): the light limits what's seen more than distance does,
  but within it the eyes set how much detail is made out (movement only, then a shape, humanoid or not, then the
  race), by the sight attribute, unless blind; modelled if it can be, perhaps later.  For now a look one way's
  reach stays capped at 31 squares.  (Built of it so far: What's seen of an actor, in Outfits.)
- **Each occupation's balance** (the user's, 2026-10-20: "we will need to look at game balance for each
  occupation"): now that hmchar offers them all, each one's worth in the game looked at, not only its points (the book's
  run from 1 to 21; a dungeon favours some skills, a fighter's, a thief's, a miner's, over others), and evened out
  as the user decides.
- **D9, hearing:** how far each sound carries and how well each actor hears, tuned by ear once the real
  environmental sounds are in.
