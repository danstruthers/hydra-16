# Dingus Quest Zero's house rules

Every rule the game makes where the book (HarnMaster 3.5) is silent, or where the game chooses how to use it: what
each says, its numbers, where the code applies it, and why.  The book's own rules are the referee's (games/dq0harn,
on the C SDK's harn.h) and aren't repeated here; their places are given as the book gives them (Skills 8, Combat 6).
The game's words only: nothing here is the book's text.
The design the rules serve is DUNGEON.md's, DENIZENS.md's and SOUND.md's; when and why each was decided, in the
user's words, is DECISIONS.md's.

Drafted 2026-10-19 (Phase 3A, "rules on record") from the code's comments, DUNGEON.md and DENIZENS.md, for the user
to review.  The last two sections are what's decided but not built yet, and what's still open.

One scale each (rs.h's scales and units): every difficulty and every circumstance is in **steps**, 5 points each on a
d100 test (a request's steps, plus easier, minus harder; a thing's, a lock's or a trap's difficulty the steps worse a
test of it goes); every size in pints; every weight in quarter pounds (told, and written in the data, in pounds);
every distance in feet, kept in the map's squares (5 feet each) where the map counts it, a thickness in eighths of
an inch.  The data files (the themes, the bestiary's kinds) are written in steps, pints, pounds and feet, and their
compilers make them the game's, once.  As a guide to the steps: 1 or 2 a little harder (haste, a simple lock), 4
hard (an iron door to force), 6 to 8 very hard (a sound lock, a thing hidden well, a window's bars), 10 or more all
but beyond most (a thing hidden cunningly).

The rules marked *(every ruleset's)* are the game's own whatever the ruleset: their code is games/dq0/rshouse.h,
which each referee includes (RULESETS.md), so the null ruleset (dq0 -r null: its own rules are the last section's)
plays them as HarnMaster's referee does.

## Tests

- **A purpose names one skill.**  The game asks for a purpose, never a skill (rs.h's P_*); the referee picks the
  skill a GM would call for: sight, sound, smell and searching Awareness; stealth and hiding oneself Stealth;
  climbing Climbing; jumping Jumping; swimming Swimming; balance Acrobatics; forcing and lifting Strength (an
  attribute times 5); picking a lock, disarming and making out a mechanism Lockcraft; enduring Condition; resisting
  poison Endurance times 5; resisting the mind Will times 5; dodging a hazard Dodge; first aid Physician; lore Lore;
  tracking Tracking; persuading and intimidating Rhetoric; animals Animalcraft; digging Mining; guile Intrigue;
  throwing Throwing.
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
  Cookery, stone Masonry, glass Glassworking.  (dq0harn.c's appraiser table.)
- **Words reach a creature only in a tongue the character speaks:** its own, common, and any its file's "language"
  lines give.  Persuading, intimidating and deceiving in another tongue does nothing (the reply's result 2).
  (dq0harn.c's speaks.)
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
  climb's, answered no, goes on: there's no stopping part way up).  Swimming the same, when there's water (slice
  4.9).  The null ruleset tires nothing.  (dq0w.c's arrive, stop and s_batter_on; sheet.c's o_spent, WORK.)  Why:
  the book's rate; a run's (a level each two minutes) would leave one spell of digging five levels down, every
  physical test 25 the worse.
- **Encumbrance** is the book's (the load over Endurance, rounded), told in words: none, a little burdened (1-2), heavily
  burdened (3-4), overloaded (5 or more).  (harn.c's harn_get_ep; dq0harn.c's status.)  See D13 below: free pounds
  by Strength are decided, not built.

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
  an hour.  (sheet.c's o_harm.)  Why: no game should stall on a run of failed rolls.
- **A window's sill is climbed through** with a Climbing test; a critical failure is a slip and a fall.  (wwin.c.)
- **A pit is climbed out of** as a ledge is.  (wtrap.c, dq0w.c.)  See D13: a slip is to end the climb.

## Digging

The book has no digging rules.  A spell of work is ten minutes, with the tool to hand that suits the material best
(bare hands if none does better; of two as good, the one it wears less).  Earth, soft soil or hard-packed, is dug at
the tool's rate, no test: hard-packed earth half as far as soft soil, and never with bare hands ("you make no
progress").  Anything harder (a floor's flags, masonry, stone) only a pick bites into, and each spell is a Mining
test: the rate at a marginal success, twice at a critical, half at a marginal failure, nothing at a critical
failure.  Feet a spell (kept in half-feet), and the wear a spell does the tool:

| Tool             | Soil | Hard-packed earth | Flags | Stone | Masonry | Wear (each)            |
|------------------|------|-------------------|-------|-------|---------|------------------------|
| bare hands       | ½    | -                 | -     | -     | -       | -                      |
| a blade, a knife | 1    | ½                 | -     | -     | -       | soil, earth 40 (it dulls) |
| pick             | 2    | 1                 | 3     | 2     | 1       | flags, stone, masonry 1 |
| shovel, spade    | 3    | 1½                | -     | -     | -       | none                   |
| mattock          | 3    | 1½                | -     | -     | -       | none                   |

Each spell is strenuous work: two fatigue levels at HarnMaster's rate (Moving and tiring, above), so a long dig wants
rests between its spells.
A natural cave's earthen floors are soft soil; a built, mined or mixed stratum's are trodden hard (dq0m/fixtures.c).
A floor's flags are lifted, not cut (the user: a material of their own).  The user's, 2026-10-20: "a sword digs
earth too, poorly, and dulls"; bare hands "should only work in soft soil.  Hard-packed soil or harder should result
in a 'you make no progress' result"; "Hard-packed earth should be harder to dig for any tool (half the soft soil
rate)"; earth takes no Mining, "Anything harder uses mining skill, and rate depends on material (only a pick will
work for harder materials)".  (rshouse.h's digger table, the tool's choice and DIG_TESTED; dq0harn.c's RS_TEST
P_DIG.)  *(Every ruleset's.)*

## Portals

- **Picking a lock:** Lockcraft, at the lock's difficulty, a minute a try: 2 to 10 steps worse, two a grade of its
  make (the maker's dice: a simple lock 2 or 4, a sound one 6 to 10; the closer look tells which).  (pass.c;
  wdoor.c.)
- **Forcing a door:** Strength, by its material: wood as it is, iron-bound two steps worse, iron four; one worse
  locked or barred; a rotten door (quality 0-1) two better, a worn one (2-3) one better, a new one (8-9) one worse.
  Its lock or bar bursts; it's loud, and takes 10 seconds.  (wdoor.c.)  Why: the book's Strength test, scaled by
  what the door is made of and how it's kept.
- **Bending a window's bars:** Strength, six steps worse; loud, 10 seconds.  **A portcullis's bars,** down: six
  worse and one more for each inch of their thickness (a portcullis's two), and it's a way through for good,
  jammed.  **Heaving a
  portcullis up** from the far side: six worse, a round.  A portcullis is never opened: raised by its winch on its
  own side (30 seconds' cranking, loud), lowered by it only (2 seconds, louder).  (wwin.c.)
- **Battering a thing** (the user, 2026-10-16: "model it after unarmed combat"): a blow the thing ignores (the
  book's Ignore defence), struck with what hits it hardest to hand (a tool, else a fist, a foot, the head); its
  impact for the aspect that does it most harm, less the material's resistance, worn off the portal's body (its
  material, size, thickness and state), the harm kept.  A door broken down stays open for good; a portcullis
  broken through is a way through for good, jammed.  A blow every 3 seconds as the clock goes, a dozen at most (four
  that make no mark at all: given up), a long task (below), the harm kept as each lands; each blow heard (the user's,
  2026-10-20: a noise of 8 for the denizens, a window's shutters or bars 7, and a blow's sound), and strenuous work
  (Moving and tiring).  (wblow.c; fight.c's thing.)
- **A part that strikes a thing is hurt by a blow gone wrong:** a marginal failure a d6, a critical failure 2d6, and
  half the thing's blunt resistance, less the part's armour (gauntlets, boots' soles, a helm): a blunt hit on that
  part.  The character stops when it's hurt enough: a serious injury, three levels or more to that part, shock or
  senselessness.  (fight.c's thing; sheet.c's o_harm, HC_STRUCK.)  Why: the book's H2 for a bare part meeting
  something hard, with the game's numbers.

## Traps, searching and hiding

- **A trap stepped on, if noticed** (see Noticing), stops the character short.  (wtrap.c.)
- **A search** looks over every square within 3 that the character can see: a hidden trap or thing is found by
  Awareness at its hiding's steps worse, a hidden door at no steps.  (wtrap.c; things.c.)
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
  character knows where it hid it.  (things.c's s_hide; main.c's hide.)
- **Disarming:** first the mechanism is made out (Lockcraft, half its power in steps worse: never deadly, however
  it goes), then disarmed (Lockcraft, half its power worse, two better if it's a keen one known well); magic only
  by magical means; a pit not at all.  (wtrap.c.)
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

- **The creatures' sheets come from the referee's bestiary** (fight.c's, by the kinds' keys): the first six (the
  giant rat, the cave spider, the ghoul, the skeleton, the cave bear, the kobold) a stand-in of the game's own; the
  rest the HarnWorld Bestiary's averages for the like creature (the page each came from beside it), and for folk the
  book's for men (attributes about 10 to 12, skills by their trade); each attribute varied as the book suggests: less
  7, and 2d6, at least 1 and at most twice its own; by a die of its own, the same creature each time.  Why: the game's
  own world (G3), its creatures modelled on the book's (the user: "for denizens, read the harnmaster bestiary"); the
  first six keep their numbers till the tests are cut to meaning (the ghoul is the book's grave-wight, the cave bear
  its black bear: Bestiary pp48, 13).
- **A creature defends** by dodging, or by counterstriking if it's better at that, as the book advises for animals.
- **Aiming high or low** is two steps worse (10); **a prone defender** four steps easier to strike (20);
  **outnumbered,** two steps worse for each enemy more.  (fight.c.)
- **Armour's quality:** a poor piece a point less protection, a fine one a point more.  (fight.c.)
- **Bones that move** (mindless) feel no shock, bleed nothing, fear nothing, and fall apart when their injuries
  reach their Endurance; so too the death god's mindless dead, the wights (the book's: Bestiary p78).  (fight.c.)
- **A creature's courage** is its Initiative test, its injuries its penalty: a marginal failure holds it back a
  round; a critical failure breaks it, a d100: berserk (1-25), desperate (26-50), broken (51-75), cautious (76-100).
  A wounded creature still standing is tested; a shy one that holds back flees.  (fight.c's courage, left.)
- **Bleeding** loses a point of blood a minute for each bleeding wound; past the Endurance, death.  (fight.c's
  bled.)
- **Health in words,** for every ruleset: fine with no injuries; hurt while the injury points are under a third of
  the Endurance; badly hurt under two thirds; critical past that; down when senseless or in shock.  (dq0harn.c's
  health_of.)

## Throwing

- **Anything thrown goes by Throwing** (P_THROW), whatever it is, at the book's missile ranges (Combat 15): a
  short band, then medium, long and extreme, each twice as far as the last, at 0, 20, 40 and 80 less (0, 4, 8 and 16
  steps).  (dq0harn.c's purpose table; dq0w/things.c's s_throw_at.)
- **A kind's reach is its short band, in squares** (5 feet each): 2 for a thing a hand throws well (a dagger, a
  torch, rations, a key, a skull, bones, pennies, a candle), 1 for a heavy or an awkward one (a pick, a shovel, a
  sword, a mace, a shield, a helm, boots, a basket, a rope, a pane), 4 for a spear (a javelin's), 0 for what isn't
  thrown (a chest, a table, a chair, a cloak, body armour, a backpack, a belt, a body, a well); a theme's own kinds
  none yet.  (dq0w/kinds.c's throws: the plan's affordances in the kinds' data.)  Why: the book gives thrown
  weapons' ranges; other things are judged by their weight and their shape.
- **Thrown hard: twice as far, and 10 less** (2 steps).  ("throw the dagger hard at the rat": struct cmd's manner.)
- **Beyond the extreme band it falls short**, at the band's end on the line to the mark; **a miss lands beside the
  mark** (on its square, as yet); **a hit strikes it**, but wounds nothing yet: the referee's missile attack (Combat
  15's: its defences, its impact) is to come.  The thing lands out of the thrower's hands, where it falls.

## Commands

- **"It" is the last thing named,** of any kind (the user's answer 7): a thing, a denizen, a part of the place.
  "Take it" with nothing named yet says so.  (resolve.c.)
- **A verb said alone takes its own thing** (commands.verbs' unsaid): drop, put, wear, wield, hold, read, use the
  last thing named; take the last thing named if it lies near, else the thing near, asking which of several, and
  saying so when there's none (the user's answer 8); attack and throw the target; open, close, break ... what's in
  front; look and examine the place.  (resolve.c; main.c.)
- **"Take all"** takes each thing it sees lying about, the nearest first (those in reach before those walked to),
  in turn; one it can't take, or no longer sees, ends it.  (resolve.c's taken_next.)
- **Which of several:** the closest by squares (and the lead, as yet, the closest); the largest and the smallest by
  a thing's size (how easily it's seen); the second as they lie from the character's left round to its right; any,
  the closest.  Several of a name and none said: the one the world's services choose, as ever (the nearest thing,
  the denizen beside it), but a throw's mark and a target set must be one: then which, asked.  (resolve.c.)
- **A question** lists the choices from the left round to the right, how far off each is where two lie the same
  way, and takes a number, a way ("left"), a selector ("the closest", "the second") or a name; any other line lets
  it go (the user's answer 13).  (resolve.c; the doors' and the stairs' own, choose.c.)
- **The target** is set by "target the rat", or by any "at the rat"; shown by "target"; it's what attack and throw
  take when nothing's said.  It's let go by "clear target"; as it's down while another denizen in sight is up; as
  it's dropped or thrown away; as it's out of sight; as the character's made to turn: a denizen it didn't see before
  comes into sight, a blow comes from one it doesn't see, a trap is set off.  (resolve.c; blows.c, fixt.c:
  untarget.)

## Characters *(hmchar: HarnMaster's character generator)*

- **Every occupation the book lists, and one of the player's own** (the user's, 2026-10-20).  hmchar offers the
  Occupational Skills table (Character 15: its 55 occupations, unguilded, guilded and noble; those the character's
  class and sex allow it, below); the soldiers of the character's culture, by unit (Character 27: the Feudal's with
  the fighting orders', the Imperial's, the Vikings', the Khuzdul's, the Sindarin's; the Tribal have none); and the
  clerics, by church (Character 23: a character with a deity its own church's; one with none any of the ten, the
  church making its deity the character's, its ritual skill opened).  Not yet the mages (Character 25): their skills
  come with magic.  A soldier has every soldier's skills too, a cleric every cleric's; a skill both list, the higher.
  The book's specialties are their skills (Ratcraft and Dogcraft Animalcraft, a lance Polearm, a falchion or a
  longknife Sword, a hartbow Bow); a second language and more scripts are left out (harn.h has one Language and one
  Script).  The table is games/hmchar/occupations.txt (on the card in games/lib/hmchar, beside hmchar), read a line at
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

## The null ruleset (dq0null)

A second referee with no book behind it (Phase 3A: RULESETS.md), to show the game asks a ruleset nothing but rs.h's
requests; the house's rules above marked *(every ruleset's)* it plays as HarnMaster's referee does.  Its own:

- **A test** succeeds on a d100 of at most 60 and 5 a step (5 to 95); the lowest fifth of the success band is a
  critical success, the highest fifth of the failure band a critical failure.  No skills, no attributes, no
  penalties.  Why: flat odds, the steps the only difficulty.
- **The character takes 10 wounds** (dead at 10), **a creature 6.**  A fall does a wound each 10 feet, a trap's
  strike or a burn 2.  Health: hurt under a third, badly hurt under two thirds, critical past that.
- **A blow** is a flat test (a prone defender four steps easier, two harder for each other foe); a hit 2 wounds, a
  critical 4.  A prone attacker gets up instead.  A creature wounded that stands holds on a flat test (a shy one four
  steps worse), else it flees.  A blow at a thing does 6, less the thing's least resistance.
- **No fatigue, no bleeding, no shock, no senselessness, no encumbrance.**  Senses all 5 (hearing 7 listening as it
  goes); a square takes 4.5, 3, 1.5, 0.8, 0.4 and 0.3 seconds at a creep, a slow walk, a walk, a jog, a run and a
  sprint.
- **Its character** is any file's name, occupation, pace and language lines (hmchar's will do), with its wounds;
  without one it makes its own, Dingus, a wanderer.  It speaks common and its file's tongues.

## The history and the theme *(every ruleset's)*

- **A game's own theme** (G2): a seed of 65,536 or more has its region's history and its theme made as it begins
  (themegen: HISTORY.md); a seed under it, a stand-in theme's, by its second byte.  (main.c's s_begin.)  Why: the
  clock's seeds are all past it, so every game has a region of its own; the tests' small seeds keep the themes
  they were written against.
- **A far exit is open** from the start: a way out below the top stratum (the through goal's) lets the character up
  it to the daylight, the game won (escaped); the way in, above, has fallen in (no rule opens it).  (dq0w.c's
  s_climb.)  Why: winning through is the goal; the way in shut is what makes it one.
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
- **The evil's goal won where it's done:** the relic laid on the curse's altar ends the game, the curse broken (the
  ruled ending's words); its dead aren't stilled first.  Why: the denizens' answer to it is popgen's, to come.
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
  so on; a thing or a denizen the same ("a rat in the far corner"), and "against the wall" ahead, behind, to the left
  or right when it's within a square of it.  Only what's known: with the walls beyond the light, "somewhere in a
  cavern, its walls beyond your light".
- **A room's size and shape words**, by its floor: small under 150 square feet, a plain room to 600, large to 2,500,
  vast beyond; long when it's at least twice as long as it's wide, and narrow too if it's 10 feet across or less;
  wide when it's 40 feet across or more ("a long, narrow hall", "a wide hall", "a vast cavern").
- **Going places**: the centre (the middle), the far side (straight ahead to the wall: "walk forward to the other
  side"), the near side, the left or right side, a corner ("the far left corner"), a wall, a feature or a thing seen
  ("the door ahead to my left", the pillar, the body).  The way goes round what's in it through squares the
  character knows; one it doesn't know of is found as it's met.  The small moves stay for fine work.
- **Out of reach, walked to first** (the user's, 2026-10-20): an action that needs its thing within reach (taking it
  up, holding it, wearing it, putting it in or on something, opening a chest; a blow) walks over to it first, a long
  task like any walk, then does it; a blow at a denizen out of reach closes with it first (the walk ends beside it,
  or as anything engages the character), unless the character's engaged already or strikes with a missile weapon.
- **Engagement** (the book's engagement zones, Combat 6; the user's reminder, 2026-10-20): an enemy on its feet and
  conscious (a hostile or unfriendly denizen: one that strikes at a character beside it) has a zone, its own square
  and the eight round it, but not through a wall or a shut door, nor round a corner (a diagonal with solid squares on
  both sides).  One lying down has only its own square, one senseless none; a friendly one's or an ally's doesn't
  count.  The character in an enemy's zone is engaged with it.  Any walk stops as it comes into an enemy's zone, a
  hard stop ("The giant rat engages you."), and one begun in a zone goes a single square and stops there if it's
  still in one (the book's one square a turn within them): a walk over to a thing, or past a hostile, stops where it's
  engaged.  A blow's close-in ends in its target's zone, beside it, and the blow is struck; engaged by another first,
  no blow, and the player says what next.  Engaged already, a blow at one out of reach isn't closed with (the
  user's).  The denizens move by the same rule: one coming at the character stops in its zone.  Not used: the book's
  long-reach weapons' two squares (with the missiles, slice 4.7) and its reaction zones (the game's 30 feet stands for
  them: a hostile seen that near stops a long task).  As built: a close-in's target
  struck at the character as it comes beside it is no stop (it's the fight it went for: the character's blow follows);
  "attack the rat" with several in sight and none beside the character closes with the nearest; a bare "attack" with
  no target, nothing beside it, is refused, as before.
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
  Wearing and taking off, dropping, lighting and putting out aren't gropes.
- **Going places, as built:** the place's square worked out from the room's floor as the character faces (the floor
  in thirds for the centre, its edge rows for a side or a corner, the nearer end of each when a corner's not said);
  where it's rock (a cave's), the nearest square the way reaches; "You're there already." if it is.  The search's
  minute a long task, its tests at its end, cut short only the squares nearest as far as it went (Traps, searching
  and hiding).  Tiring asked of a walk, a dig, a climb and a battering (Moving and tiring: strenuous work).
- **A long task** is anything the character does that takes more than a round (10 seconds): a walk, a dig, a climb, a
  search, battering a door, a wait.  Its progress is a bar on the status line, ten cells, its part done of its length
  (a walk's squares, a dig's feet, a climb's feet, a wait's time, a battering's blows), redrawn at most once a real
  second; and words as it goes (the user's: both).
- **A walk's words**: a beat at each change (the way turns, a side passage or a door is passed, it widens or opens
  into a room, the light changes, a sound is heard), and on a quiet stretch a short line every 6 squares (30 feet) of
  none ("You go on down the passage."; the user's: about every 30 feet).
- **Interruptions, hard** (the task stops at once, and says why): any wound taken (the user's: all wounds), a blow at
  the character, a trap sprung, a fall, its light going out, a hostile denizen coming into sight within 30 feet.
  **Soft** (it asks, "A rat comes into the light.  Keep digging?"; yes carries on, anything else stops it, and a new
  command typed instead stops it and runs): something new seen, heard or smelled (a denizen not hostile, a hostile
  one farther off than 30 feet, a sound), a fatigue level more.
- **Stopping**: "stop" stops what can be stopped; "stop digging" (battering, climbing, searching, walking, waiting)
  names it, and a name that isn't the task's is refused ("You aren't digging.").  What's imposed can't be stopped:
  senseless, a fall under way.
- **What stopping costs**: a walk stops where the character is; a dig keeps the feet dug (the change record: a hole
  half dug stays half dug), and the part of its spell done tires; battering keeps the harm done; a search cut short
  covers the squares nearest first as far as it went, each at the full chance, and a wait rests for the part done; a
  climb, disturbed hard, is a Climbing test to keep the grip (the plan's holding on), a fall from where it is if it
  fails.
- **Looking one way** ("look north", "look left", "look at the door"): a focused look that way, 3 seconds, twice as far
  as a glance about, as far as the light and the eyes allow (the light layer, distant lights, the dark's adapting);
  through what lets sight through (a doorway, a window, a grating, bars, a slit, a hole), what lies beyond.  Listening
  or sniffing one way ("listen at the door", "sniff at the grating") the same, through what lets sound or air through.
- **Things told as found**: whatever a test finds (a search, a trap noticed underfoot, a hidden door, a sound heard by
  listening) is told after what's plain at a glance, as its own line, saying how it was found: "You arrive at a
  closed door.", "Searching about...", "You find a gas trap, set to go off when the door is opened."

## Decided, not yet built

- **D13, Strength and weapons:** Strength doesn't add to impact; it sets how heavy a weapon can be wielded without
  penalty; heavier versions hit harder, and strong characters are made with heavier weapons.  Today the weapons
  have one weight each and Strength does nothing in a blow.
- **D13, free pounds:** the first X pounds carried are free of encumbrance, X 10 at Strength 10 and doubling each 4
  points (4 lb at 5, 14 at 12, 24 at 15, 40 at 18, 57 at 20, 134 at 25).  Today it's the book's: every pound counts.
- **D13, a wrecked door, window or portcullis appraises by its state.**  Today appraisal doesn't see the damage.
- **D13, the pit climb:** a slip climbing out of a pit ends the climb, and the player chooses what next.  Today a
  slip slides 5 feet back and the climb goes on.
- **D5, light:** a spare torch and oil in the kit; lights burning down (a torch's, a lantern's oil).  Today lights
  burn for ever.
- **D6, bleeding with first aid** (P_FIRST_AID binds a bleeder).  Today nothing stops bleeding.
- **Bearings and mental maps** (DUNGEON.md: the character's sense of where it is, by Awareness or Mining; a map in
  its head that fades): slices 4.1-4.2.
- **Sniffing one way** (slice 4.0's looking one way: "sniff at the grating"): the trace layer's (slice 4.1: smells in
  the deposit layer).  Today a sniff takes its 3 seconds and smells nothing out of the ordinary.
- **A look one way's twice as far:** it reaches twice a glance's squares, but 31 at most (155 feet: the senses' grid,
  63 squares across), so a sight of 6 or more (a glance's 16 squares or more) looks one way less than twice as far.

- **A theme's own tests name a purpose** (the user's, 2026-10-20), as the rest of the game's do, which each
  ruleset's referee maps to its own skill.  Today they're an even chance (P_THEME on).
- **Climbing's rate by the face's grade** (the user's, 2026-10-20): faster up an easy face, slower up a sheer one,
  once the maker gives faces a grade.  Today it's 10 feet a round at a marginal success and 20 at a critical,
  everywhere.

- **Swimming tires** (the user's, 2026-10-20: "swimming, climbing, running, battering down a door"), strenuous work
  as climbing, digging and battering are (built: Moving and tiring), once there's water (slice 4.9).

## Open

- **Sight's detail by distance** (the user's, 2026-10-20): the light limits what's seen more than distance does,
  but within it the eyes set how much detail is made out (movement only, then a shape, humanoid or not, then the
  race), by the sight attribute, unless blind; modelled if it can be, perhaps later.  For now a look one way's
  reach stays capped at 31 squares.
- **Each occupation's balance** (the user's, 2026-10-20: "we will need to look at game balance for each
  occupation"): now that hmchar offers them all, each one's worth in the game looked at, not only its points (the book's
  run from 1 to 21; a dungeon favours some skills, a fighter's, a thief's, a miner's, over others), and evened out
  as the user decides.
- **D9, hearing:** how far each sound carries and how well each actor hears, tuned by ear once the real
  environmental sounds are in.
