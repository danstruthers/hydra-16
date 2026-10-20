# Dingus Quest Zero's house rules

Every rule the game makes where the book (HarnMaster 3.5) is silent, or where the game chooses how to use it: what
each says, its numbers, where the code applies it, and why.  The book's own rules are the referee's (games/dq0harn,
on the C SDK's harn.h) and aren't repeated here; their places are given as the book gives them (Skills 8, Combat 6).
The game's words only: nothing here is the book's text.

Drafted 2026-10-19 (Phase 3A, "rules on record") from the code's comments, DUNGEON.md and DENIZENS.md, for the user
to review.  The last two sections are what's decided but not built yet, and what's still open.

A step is the game's unit of circumstance everywhere: 5 points on a d100 test (rs.h: the steps of a request).

## Tests

- **A purpose names one skill.**  The game asks for a purpose, never a skill (rs.h's P_*); the referee picks the
  skill a GM would call for: sight, sound, smell and searching Awareness; stealth and hiding oneself Stealth;
  climbing Climbing; jumping Jumping; swimming Swimming; balance Acrobatics; forcing and lifting Strength (an
  attribute times 5); picking a lock, disarming and making out a mechanism Lockcraft; enduring Condition; resisting
  poison Endurance times 5; resisting the mind Will times 5; dodging a hazard Dodge; first aid Physician; lore Lore;
  tracking Tracking; persuading and intimidating Rhetoric; animals Animalcraft; digging Mining; guile Intrigue.
  (dq0harn.c's purpose table.)  Why: the game's code stays the same whatever the ruleset; only the referee knows
  skills.
- **A skill never opened is tried at its opening level,** less five a point of the physical penalty, as if opened
  on first use.
  (dq0harn.c's test_sk.)  Why: a character can try anything a person could.
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
  10 or 11 gives 4.  (dq0harn.c's sense.)  Why: the senses' task (dq0p) works in one scale for every actor.
- **Listening as it goes** makes hearing two better (at most 9), and a sound test two steps better.  (dq0harn.c's
  status; sheet.c's o_noticing.)
- **Pace while noticing:** a creep or a slow walk one step better (but not feeling the way in the dark), a jog two
  worse, a run four, a sprint six.  (sheet.c's by_pace.)  Why: the faster one goes, the less one takes in.
- **In shock, four steps worse** on top of the universal penalty already in every test.  (sheet.c's o_noticing.)
- **A hazard underfoot** (a trap stepped onto) is noticed by a passing glance: twice its hiding's steps worse, and
  two more.  Searching as it goes (watching for traps), the character notices it as a search would: its hiding's
  steps worse.  (sheet.c's o_noticing; wtrap.c.)
- **The Khuzdul always know north underground** (the summary's northsense), a trait of their species.
  (dq0harn.c's status.)

## Moving and tiring

- **Paces and their times a square** (5 feet), from the character's Move: a walk 200 / Move tenths of a second
  (half its Move in squares a round, the book's walking); a jog 100 / Move, a run 50 / Move, a sprint 33 / Move
  (the book's jog, run and sprint); a slow walk twice a walk's time, a creep three times.  None slower than a walk.
  (dq0harn.c's status.)  Why: the book has no creep or slow walk; they trade speed for quiet and noticing.
- **A step takes a rise or a drop of 2 feet**, steps or stairs 4; more is a ledge, climbed.  (world.h's STEP_MAX,
  STAIR_MAX.)
- **Fatigue builds by the time spent at each pace, added up across moves.**  A level of fatigue is 6,000 units of
  exertion: a sprint adds 10 a tenth of a second (a level a minute), a run 5 (one each two minutes), a jog 2 (one
  each five minutes), a walk one each 144 tenths (a level a day); a slow walk or a creep tires as a walk does.
  Standing still wears exertion off first, a unit a tenth, then takes a level off each ten minutes.  (sheet.c's
  o_spent.)  Why: the book's movement fatigue, made continuous so short dashes add up.
- **Encumbrance** is the book's (the load over Endurance, rounded), told in words: none, a little burdened (1-2), heavily
  burdened (3-4), overloaded (5 or more).  (harn.c's harn_get_ep; dq0harn.c's status.)  See D13 below: free pounds
  by Strength are decided, not built.

## Climbing and falling

- **A ledge is climbed a round (10 seconds) at a time,** a Climbing test each: a critical success 20 feet, a
  marginal success 10, a marginal failure a slip back of 5 feet (the climb goes on), a critical failure a fall from
  as high as it got (climbing down: to the foot).  Holds may catch the fall: then the climb goes on.  (dq0w.c's
  s_climb_on.)  Why: the book gives the test, not the rate; 10 feet a round is a careful climber's pace.
- **Jumping down a ledge** is a clear fall of its height.  (dq0w.c's start_ledge.)
- **A fall strikes where a middle strike would,** or low if it lands on its feet, with no armour taken off; the
  book's falling dice and grabbing a hold (Skills 8) otherwise.  (sheet.c's o_harm.)  Why: armour doesn't soften
  the ground.
- **Knocked senseless, the character comes round by the book's shock rolls,** a round each, and wakes anyway after
  an hour.  (sheet.c's o_harm.)  Why: no game should stall on a run of failed rolls.
- **A window's sill is climbed through** with a Climbing test; a critical failure is a slip and a fall.  (wwin.c.)
- **A pit is climbed out of** as a ledge is.  (wtrap.c, dq0w.c.)  See D13: a slip is to end the climb.

## Digging

The book has no digging rules.  A spell of work is ten minutes, a Mining test with the tool to hand that suits the
material best (bare hands if none does better; of two as good, the one it wears less).  Feet dug a spell at a
marginal success, by material (earth, stone, masonry, a floor's flags), and the wear a spell does the tool:

| Tool         | Earth | Stone | Masonry | Flags | Wear (each)        |
|--------------|-------|-------|---------|-------|--------------------|
| bare hands   | 1     | -     | -       | -     | -                  |
| shovel, spade| 3     | -     | -       | 1     | none               |
| pick         | 2     | 2     | 1       | 3     | stone, masonry, flags 1 |
| mattock      | 3     | 2     | 1       | 3     | stone, masonry, flags 1 |
| chisel       | -     | 1     | 2       | 1     | stone, masonry, flags 1 |
| a blade, a knife | 1 | -     | -       | -     | earth 40 (it dulls) |

A critical success digs twice as far, a marginal failure half, a critical failure nothing.  A floor's flags are
lifted, not cut (the user: a material of their own).  (dq0harn.c's digger table, RS_TEST P_DIG.)

## Portals

- **Picking a lock:** Lockcraft, two steps worse for each grade of the lock's complexity, a minute a try.
  (wdoor.c.)
- **Forcing a door:** Strength, by its material: wood as it is, iron-bound two steps worse, iron four; one worse
  locked or barred; a rotten door (quality 0-1) two better, a worn one (2-3) one better, a new one (8-9) one worse.
  Its lock or bar bursts; it's loud, and takes 10 seconds.  (wdoor.c.)  Why: the book's Strength test, scaled by
  what the door is made of and how it's kept.
- **Bending a window's bars:** Strength, six steps worse; loud, 10 seconds.  **A portcullis's bars,** down: six
  worse and one more for each grade of their thickness, and it's a way through for good, jammed.  **Heaving a
  portcullis up** from the far side: six worse, a round.  A portcullis is never opened: raised by its winch on its
  own side (30 seconds' cranking, loud), lowered by it only (2 seconds, louder).  (wwin.c.)
- **Battering a thing** (the user, 2026-10-16: "model it after unarmed combat"): a blow the thing ignores (the
  book's Ignore defence), struck with what hits it hardest to hand (a tool, else a fist, a foot, the head); its
  impact for the aspect that does it most harm, less the material's resistance, worn off the portal's body (its
  material, size, thickness and state), the harm kept.  A door broken down stays open for good; a portcullis
  broken through is a way through for good, jammed.  (wblow.c; fight.c's thing.)
- **A part that strikes a thing is hurt by a blow gone wrong:** a marginal failure a d6, a critical failure 2d6, and
  half the thing's blunt resistance, less the part's armour (gauntlets, boots' soles, a helm): a blunt hit on that
  part.  The character stops when it's hurt enough: a serious injury, three levels or more to that part, shock or
  senselessness.  (fight.c's thing; sheet.c's o_harm, HC_STRUCK.)  Why: the book's H2 for a bare part meeting
  something hard, with the game's numbers.

## Traps, searching and hiding

- **A trap stepped on, if noticed** (see Noticing), stops the character short.  (wtrap.c.)
- **A search** looks over every square within 3 that the character can see: a hidden trap or door is found by
  Awareness at its hiding's steps worse; a hidden thing at two steps better, less two for each grade of its hiding.
  (wtrap.c; things.c.)
- **Hiding a thing** where the character stands (the user: hiding is a skill, Concealment; the game's Stealth): its
  hiding is 1 for the place (a floor's dust and corners) and more by the test: a critical success 6 more, a
  marginal success 3, a marginal failure 1.  The character knows where it hid it.  (things.c's s_hide.)
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
- **Aiming high or low** is ten worse; **a prone defender** twenty easier to strike; **outnumbered,** ten worse for
  each enemy more.  (fight.c.)
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

## Open

- **D9, hearing:** how far each sound carries and how well each actor hears, tuned by ear once the real
  environmental sounds are in.
- **The theme's own purposes:** an even chance till each theme says which skill its puzzle's tests use.  Should a
  theme name a skill, or a purpose the referee maps?  (Recommended: a purpose, as the rest of the game does.)
- **Untrained skills:** tried at their opening level.  The book lets a GM refuse a test outright (no Lockcraft, no
  picking).  Keep every test open?  (Recommended: yes, but for tests the tools gate, as lockpicks do now.)
- **Climbing's rate:** 10 feet a round at a marginal success, 20 at a critical.  Faster for an easy face, slower
  for a sheer one?  (Recommended: by the ledge's grade, once the maker gives faces a grade.)
