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
  Cookery, stone Masonry, glass Glassworking.  (dq0harn.c's appraiser table.)  It's asked for by name, and tells only
  what's asked: its value, its quality, or both (Looking at things, below).
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
  an hour, the book's last shock roll then deciding whether it's in shock (2026-10-21: it woke coherent however badly
  hurt).  (sheet.c's o_harm, fight.c's senseless.)  Why: no game should stall on a run of failed rolls.  Struck at as
  it lies, it's finished (Fights: a helpless character); a creature struck senseless again as it lies would be out
  till the later of the two, its black-out told once (fight.c's senseless, AF_AGAIN).  A blow fumbled at one
  senseless or in shock is a miss, not the Ignore table's tactical advantage: it can't take one.  (fight.c's
  attack.)  Why: the advantage is an action, and it has none.
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

- **The creatures' sheets come from the referee's bestiary** (fight.c's, by the kinds' keys): the first six (the
  giant rat, the cave spider, the ghoul, the skeleton, the cave bear, the kobold) a stand-in of the game's own; the
  rest the HarnWorld Bestiary's averages for the like creature (the page each came from beside it), and for folk the
  book's for men (attributes about 10 to 12, skills by their trade); each attribute varied as the book suggests: less
  7, and 2d6, at least 1 and at most twice its own; by a die of its own, the same creature each time.  Why: the game's
  own world (G3), its creatures modelled on the book's (the user: "for denizens, read the harnmaster bestiary"); the
  first six keep their numbers till the tests are cut to meaning (the ghoul is the book's grave-wight, the cave bear
  its black bear: Bestiary pp48, 13).
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
  - *Nothing named*: what's in its hands strikes (of two, the better by the ruleset's reckoning: HarnMaster's, at a
    denizen its attack mastery, at a thing the impact it does; a soft thing the least), never a better thing it
    carries, unasked: a torch in hand, the torch.  Its hands empty and something better carried (by the same
    reckoning: at a denizen the best weapon, but a shield or a torch, carried for its light; at a thing the tool that
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
    shards, a flask of oil, a pot of pitch, a heap of sand or of straw, a body, every piece of armour and clothing.
    The book's staff (Spear, 20, 15, blunt 4) is for a theme's pole or staff (its key "staff": its data can't tell a
    pole from a club, it has no length).
  - Why: the user's; a ruleset's own weapons as they are, and a thing that isn't one as the book treats one (its
    bows), the nearest of its own; the rest the house's, from the data every thing has (its material, weight, bulk,
    form, whether it's worn).  Not yet: the book's weapon damage (a weapon that strikes something hard may break, its
    quality's test: Combat 12), a thing's own quality and wear in its blow.  (gear.c's row_of, o_pc_side, o_pc_wield;
    rshouse.h's RS_HOUSE_SWING; choose.c's wield; resolve.c's tool_ok; dq0w.c's door_work, swatted, in_hand.)
- **A creature defends** by dodging, or by counterstriking if it's better at that, as the book advises for animals.
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
  fatigue, a level each five minutes of it): each round it strikes charged so; a level more asked as a hurt is
  ("You're tiring.  Keep fighting?").  To come, the same task: a missile weapon's (in
  range, seen, a clear line; its next missile each shot, none left the end; an enemy engaging the character ending it,
  the melee's fighting on taking over), a setting widening where it begins ("autoengage with sword at close range"),
  an ally's.  (dq0w.c's part 24.)
- **Disengaging, the book's** (the user's, 2026-10-21: "try to have it follow the book"; Combat 7): an engaged
  character moves a square, and then, if it's no longer engaged, a half-move more (its walk's squares a round),
  straight on, stopped by a wall, a ledge, a denizen, or another enemy's zone (still engaged after the square, it
  stops there: "You're engaged with the ghoul: you stop walking."), with no blow that round, and the fight's over
  (fought on again if one engages it again).  "disengage", "withdraw", "retreat", "back away", "back off",
  "break off", a way or none: none, away from the foe engaging it (straight away, then half left or right, then
  aside, the first open; a passage's four ways alone).  "flee" ("escape", "run away") the same, facing the way
  it goes, then, free of them, a run on that way (none if the way's shut: the disengage enough); not engaged, a run
  from the nearest foe within a dozen squares.  Neither engaged: told ("You aren't engaged.", "There's nothing to
  flee from.").  "pass" a round given up, defending.  A walk as it's engaged goes as a disengage does: its first
  square stopping it if it's still in the zone, then on.  As it fights, either takes the fight over, its round out
  first.  Not yet: rising first if it's prone (the book's), a foe's free blow (the book has none).  (dq0w.c's part
  26.)
- **The fatigue rates** (the user's, 2026-10-21: running more than strenuous work, sprinting more again): a sprint a
  level a minute, a run each two minutes, strenuous work and a jog each five, a walk a day (the book's, Combat 6 and
  Skills 6).  (sheet.c's o_spent.)
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
  faster pace (a run in shock is that walk); it ignores any blow (no defence) and is in no state to fight.
  (dq0harn.c's status.)
- **A helpless character and its foes, by their nature** (the user's choice, 2026-10-21; helpless: senseless, or in
  shock).  Each kind's way is kinds.txt's (its ways k and r).  A killer (k: the ghoul, the skeleton, the cave bear,
  the basilisk, the lindworm, the wight, the shade, the cultists and their high priest) strikes on, and one senseless
  it finishes where it lies, as the character finishes a creature (the book's dispatch: a mortal wound to its neck,
  to its skull by a blunt weapon; "The ghoul bites at you: a mortal deep bite to your neck. You are dead.": the game's
  end).  A robber (r: the kobolds and their lord, the bandits and their lord) comes up to it and takes what it holds
  (but a light lit: it leaves it its light) and every penny it has, a round's work, off to its lair (put down on its
  lair's home square, a stash to be found; where it stands, if that can't be had), and goes ("The bandit goes through
  your things as you lie helpless, and takes what it finds."); it doesn't strike at it.  One that only defended itself
  (neither: the giant rat, the cave spider, the bat, the cave adder) stops once the threat is down and goes, fleeing
  a while (24 of its turns) and unfriendly after: it keeps a few squares off, and strikes only if the character,
  itself again, comes beside it.  Nobody engages a helpless character (no engage initiative test): what's done to it
  is each one's mind's; a blow at it senseless from any but a killer is held (one posted as it fell: the referee's,
  RS_KILLER).  The referee is told what's to hand and the load left at once, as it's robbed (the user's, 2026-10-22:
  "Weight should be recounted immediately after a robbery": its encumbrance the less as the robber goes).  (dq0d.c's
  decide, K_HELP; dq0w's wfight.c, s_den_rob; fight.c's attack.)  Not yet: being led away (by an ally, or a captor),
  a captive's fate, the robbers' stash hidden, a defender cornered fighting back (the user's, 2026-10-22: "Rats
  should attack if cornered": defence only for now).
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
- **What it's done with** ("hit the door with the bar", "strike the rat using my sword", "dig down with the shovel",
  "light the torch with the tinderbox", "pick the lock of the chest with the lockpicks"; the user's playtest, 2026-10-22:
  "`hit door with bar` hits the door, even if I don't have a bar"): every command done with a thing (a blow, breaking,
  forcing, bending, raising or lowering, disarming, digging, lighting or putting out, opening, closing, locking or
  unlocking, filling, knocking, pushing, pulling, touching, barring, climbing, searching, examining) takes it only if it's
  to hand, carried or lying within reach (the things' IO_FIND: the rule a fill's fuel and a door's bar already had, used
  where it lies); else it's refused, the things' words ("You have no bar to hand.", "The bar is out of reach: go over
  to it first."), never done with bare hands or another thing instead.  A blow's thing is taken in hand first ("You take
  the bar in both hands.", its time), as a named weapon was (names.c's); no hand free for it, refused, the things' words
  ("Your hands are full: stow something, or put it down."); what strikes is then the referee's to choose, as ever, from
  what's held (a weapon held; at a door, the best tool to hand).  A part after "with"
  is what it's done with, not the door's ("hit the rat with the bar" no longer batters a door).  (resolve.c's tool_ok;
  cmdmean.s.)  Not yet: a thing the referee has no weapon for (a bar, a rope) struck with as itself (it strikes with its
  fists); a named tool chosen over a better one for a dig or a blow at a door.
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
- **Shortcuts** (the user's, 2026-10-22: "alias should be for shortcuts, and name for naming things"): "alias x is
  north then dig down" (or "alias x = ...") makes a word stand for a line; a line typed that begins with it is that
  line, the words typed after it put after it ("ex the torch": "examine the torch"), made once, as it's entered, before
  its parts, so a shortcut may hold several commands.  A shortcut whose line begins with another's is given that one's
  line as it's made (as the grammar's reference has it).  Its word already a shortcut, the player's asked first ("yes"
  replaces it; anything else keeps it).  Refused: a word the commands need ("look", "north", a number: as for a name),
  the shortcuts' own words, a definition inside one; "alias the sword as Linda" is sent to name.  "alias x" shows one,
  "alias" lists them and then the game's own words (commands.aliases, the parser's: on the card as aliases.txt);
  "unalias x" or "forget alias x" removes one.  **The game's own words can't be removed** ("unalias peer" says so):
  they're the parser's tables, the same for every game; but the player may make one a shortcut ("alias peer is
  status"), which then takes its place as a line's first word, and removing that brings the game's back.  Sixteen
  shortcuts, each a word of 15 letters and a line of 95, kept in the theme bank (hub.h's ALIAS_AT) and the save.
  (alias.c, part 9, a bank of its own; term.c's enter.)

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
  penalties.  Why: flat odds, the steps the only difficulty.  Keeping its feet over a thing in the dark (P_STUMBLE)
  the same, its steps the darkness's, the thing's and the pace's (Walking and searching in the dark): seldom kept.
- **The character takes 10 wounds** (dead at 10), **a creature 6.**  A fall does a wound each 10 feet, a trap's
  strike or a burn 2.  Health: hurt under a third, badly hurt under two thirds, critical past that.
- **A blow** is a flat test (a prone defender four steps easier, two harder for each other foe); a hit 2 wounds, a
  critical 4.  A prone attacker gets up instead.  A creature wounded that stands holds on a flat test (a shy one four
  steps worse), else it flees.  A blow at a thing does 6, less the thing's least resistance.  What strikes (Fights')
  the house's: a thing named strikes, its words its name, whatever it is (a soft one the house's single attempt);
  nothing named, the first thing in its hands; never a better thing carried asked about (none's better).
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
  no blow, and the player says what next (a hostile one fought: Fights' fighting on).  Engaged already, a blow at
  one out of reach isn't closed with (the user's).  The denizens move by the same rule: one coming at the character
  stops in its zone.  Not used: the book's
  long-reach weapons' two squares (with the missiles, slice 4.7) and its reaction zones (the game's 30 feet stands for
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
  Wearing and taking off, dropping, lighting and putting out aren't gropes.
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
  **Soft** (it asks, "A rat comes into the light.  Keep digging?"; yes carries on, anything else stops it, and a new
  command typed instead stops it and runs): something new seen, heard or smelled (a denizen not hostile, a hostile
  one farther off than 30 feet, a sound), a fatigue level more.
- **Stopping**: "stop" stops what can be stopped; "stop digging" (battering, kicking, punching, climbing, searching,
  walking, waiting, and the timed work's: studying, disarming, picking, forcing, heaving, bending, winding, tracking,
  sniffing, filling, hiding) names it, and a name that isn't the task's is refused ("You aren't digging.").  What's
  imposed can't be stopped: senseless, a fall under way.  A question's answered yes by "y", "yes", "go on", "carry
  on", "continue", or "keep" and anything ("keep at it", "keep kicking").
- **What stopping costs**: a walk stops where the character is; a dig keeps the feet dug (the change record: a hole
  half dug stays half dug), and the part of its spell done tires; battering keeps the harm done; a search cut short
  covers the squares nearest first as far as it went, each at the full chance, and a wait rests for the part done; a
  climb, disturbed hard, is a Climbing test to keep the grip (the plan's holding on), a fall from where it is if it
  fails; a timed work, nothing: no test made, no outcome, the time spent (a lock not picked, a mechanism not made
  out, a winch's portcullis still down).
- **Looking one way** ("look north", "look left", "look at the door"): a focused look that way, 3 seconds, twice as far
  as a glance about, as far as the light and the eyes allow (the light layer, distant lights, the dark's adapting);
  through what lets sight through (a doorway, a window, a grating, bars, a slit, a hole), what lies beyond.  Listening
  or sniffing one way ("listen at the door", "sniff at the grating") the same, through what lets sound or air through.
- **Things told as found**: whatever a test finds (a search, a trap noticed underfoot, a hidden door, a sound heard by
  listening) is told after what's plain at a glance, as its own line, saying how it was found: "You arrive at a
  closed door.", "Searching about...", "You find a gas trap, set to go off when the door is opened."

## Light and traces *(slice 4.1: drafted 2026-10-21; reviewed by the user, 2026-10-22: as it is for now)*

Every ruleset's.  The book gives no burning times: these are the house's (the stopped design's hour for a torch).  Nor
any numbers for the traces (its Tracking and a notice test of smell are its tests): the house's too.

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
  of pitch four hours' (a torch renewed four times).  Neither is in the maker's stock yet: the kit's flask, and dq0
  -k's.
- **The kit** (D5): a spare torch and a flask of oil in the pack, besides the torch lit in hand.
- **Found lights have some life left:** part of each used before it was left, 2 to 13 sixteenths of a whole one (by the
  thing), a sixteenth less for each century of its stratum's age; a candle has always some left (wax keeps); a
  lantern's oil has dried, or a torch's pitch crumbled, when that leaves none.
- **The status line** shows the minutes left of the light the character carries, the brightest's ("torch 40m",
  "lantern 5h59m"), when there's room for it beside the referee's words, the facing and the clock.
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
  dust two deep or more keeps them three months, a skin a day.  The dust is the stratum's age's: 3, and a level more
  at 3, 6 and 12 centuries (6 the most: "deep in dust").  **Swept** (a chunk that's a lair of a kind that keeps its
  floors swept, kinds.txt's t: the cultists and their priest, as their lair is placed): no dust at all ("swept
  clean"), and every hold a point less, the marks scattered.
- **How clearly a trail shows**: the floor's hold, a third of its kind's tread, a point more for 4 passings, another for
  12 (the scents' rule too, below: hub.h's TR_MORE); less a point for each quarter of the print's life gone, none at its
  end; a tread of 0 (a bat, a shade) leaves no print.  Each kind's tread, marks, smell, scent and nose are kinds.txt's
  (the character's own: its sweat's smell 3, its prints by what it wears on its feet, bare feet 2, sandals or shoes 3,
  boots 4, hobnailed boots 5).
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
  A timed work ("You sniff the air.", the smell told at its 3 seconds' end: the long tasks').
- **Smoke**: a flame the character carries leaves smoke on the squares it walks (a torch's strength 5, a lantern's or
  a candle's 2, by its name); a light lying lit (a sconce's torch, a fire) smokes 4 where the air reaches; a light of
  its own lit, the character smells no other smoke.  Smoke fades as a scent does, with no nose to read it longer.
- **Denizens by smell**: a kind with a keen nose (6 or more: giant rats, ghouls, cave bears, cave adders, lindworms)
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
- **Still to come**: the air layer (slice 4.5, with sound) carries a scent further downwind and less upwind, and a
  denizen downwind smells the character from far off; rain and running water (slice 4.9) wash scents and prints away
  faster.  Each has its one place to come in: a scent's fading (wtrace.c's faded, hub.h's SCENT_LIFE), its carrying
  (wtrace.c's sniff, the trail's way, wtrscent.c), the smelling in still air (dq0p/sense.c).

**Kept light** (DUNGEON.md's light kept; the user's, 2026-10-16: lights are somebody's):

- **Who keeps lights:** the kinds with kinds.txt's f (kobolds, the kobold lord, bandits, the bandit lord, cultists,
  the high priest).  The dead, the beasts, the vermin and the things that see in the dark keep none.
- **How far:** a lair keeps lit every room whose sconce stands within its kind's range (kinds.txt's: a kobold's 150
  feet, 30 squares) either way of the middle of the lair's chunk (its home isn't known till it's placed).  Passages
  have no sconces, so they stay dark.
- **Sconces:** the style's old odds of a room lit by its own light are now the odds of a room built with sconces:
  built 5 in 8, mixed 4, mine 2 (a natural stratum's caves have none), one fewer each stratum down; and every room its
  keepers keep lit has one, their own.  A glowing cave chamber never has one (its light is nobody's).  One sconce a
  room, standing for its sconces, on a square of its floor beside a wall.  An unkept room's holds a cold torch, part
  burnt, three times in four; else nothing (burnt away, or taken, long ago).
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
- **Commands:** take the torch from the sconce, light the torch in the sconce, put out the torch in the sconce (each
  walked over to first, if it's out of reach); put the torch in the sconce (within reach: "go over to it first").
- **The same dungeon each game:** a stratum's populations are decided before its first chunk is made (the world waits
  for them, ten seconds at most), so the same seed stocks the same sconces and lights the same rooms.

The user's playtest's faults (2026-10-21: things looked into and opened, things of several and how many, walking
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
  that isn't locked opens it.  Locking: no keys to things' locks yet ("you have no key to it").
- **A thing of several** (the one a command means, the world's to choose; never asked which): the one most to hand:
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

## Looking at things, and what opening shows *(2026-10-22: drafted for the user to review)*

Every ruleset's.  The user's, 2026-10-22: "When examining an item, I want to know about its state, and for containers
if they are open or closed, and what is on/in them.  `look [at] item` instead of examine should give these things.
Examine gives too many unimportant details (like value)"; the aspects asked by name, as the user chose them; and opening
something tells what it reveals.

- **Looking at a thing** ("look at the chest", "look the torch over"; LM_AT, the world's IO_LOOK): a second, no test.
  What it is and where: "Your torch is in your hand.", "The chest is beside you.", "Your dagger is on your belt.", "The
  torch is in the sconce.", "The sword is some 20 feet off."; its state, as anyone sees it: lit or unlit, burning low,
  burnt out (a lantern out of oil); open or closed, tied shut or untied, stoppered or unstoppered; a fuel empty;
  battered, nearly broken, dulled, dull, worn; with a lock (its lock broken, once forced); a container or a surface
  empty, bare ("It's lit, burning low.", "It's closed, with a lock.", "It's untied, empty."); and what's in it or on
  it, within reach, look in's words, alike told together ("In it lie a tinderbox, a bar and two torches.").  Whether a
  lock is locked doesn't show: trying it tells.  Too far off to make out (1 + sight / 2 + its kind's seen squares, as
  a glance's), only its size ("Something small is some 40 feet off."); further off than NEAR_SQ (2 squares), nothing in
  it.  Of several of a name, the one most to hand (A thing of several, above).  A thing of a kind with no words of its
  state says where it is only ("The bones are beside you.").
- **Examining a thing** ("examine the sword", "x", "inspect", "check", "study", "look closely at"; LM_CLOSE): a closer
  look, three seconds, no test, no value nor quality: what looking at it tells, and what a closer look shows besides:
  what it's made of, how thick (a candle's), its wear (as good as new, little worn, well worn, battered, close to
  breaking: its durability left, in tenths, 10, 7, 4, 2 and less), a blade's edge dulled or dull, how much of a light
  or a fuel is left (a torch's or a candle's hardly burnt ... half burnt down ... burnt out; a lantern's full of oil ...
  out of oil; a flask's full ... empty: by quarters of a whole one's life), and a lock's make ("Its lock is a simple
  one.": a chest's 1 + make / 3 steps to pick, 2 or less simple, 3 sound, 4 good).  ("It's made of wood; it's little
  worn, half burnt down.")  Further off than NEAR_SQ: "You'd have to go closer to make out more."  A portal examined
  (a door, a window, a portcullis) tells its plain features as before (its kind, size, material, thickness, a grille,
  a spy-hole, studs, its hinges, a bar's brackets, its lock's make: simple 4 steps or less, sound to 10, intricate),
  and the blows' marks, but no judgement of its state: that's its quality, asked for.
- **The aspects asked for** (the user's choices, 2026-10-22), each a word of the command's (commands.words' [aspects],
  the verb's own: struct cmd's aux, EX_*):
  - **Value:** `assess X`, "value X", "price X", "what is X worth", "the value of X", "assess / determine / examine
    the value (the worth, the price) of X": the referee's appraisal test as before (its craft's), its value alone ("You
    judge it worth about 115 pennies."), no time.
  - **Quality** (how well made): `judge X`, "how well made is X", "the quality (the make) of X", "judge the quality
    of X": the same test, its make alone ("You judge it a fine chest."); a portal's, its state and how stout it is
    judged, as the old examine did ("You judge it rotting, and stout: ...").
  - **Both:** `appraise X`: the whole appraisal as it was (its make, its material and what it stands up to, its size,
    its life left, its worth).
  - **Weight:** `heft X`, "weigh X", "the weight of X": in hand, or lifted where it lies (out of reach, walked over to
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
    own forms ("search the chest for traps", "search the table for a dagger") are the search's (agent dark's,
    2026-10-22): till they come, a search about it, as ever; "search for traps", the same.  (wtrap.c's PA_CHECK;
    things.c's LM_TRAPS.)
  - **Condition:** "check its condition", "examine the condition (the damage) of X", "the condition of X": its wear and
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
    into a wall."); a jog's a blunt blow of a die, a run's two, a sprint's three, to the head or shoulders (HC_STRIKE,
    high: the armour there counts), "You run headlong into a wall!"; a door or bars a thump the denizens hear (as loud
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
    its feet 3 seconds, beside it 6.4, a corner 9.6, two squares off 16), "You find a dagger.", "Your hands find an
    iron key."; none, a minute, "You find no dagger here."  Cut short before it comes to it, not found.
  - **"search the table", "search on the table", "search the chest"**: a thing within reach (in the light, out of reach,
    walked over to first; in the dark, one within reach, felt: "You feel about you, but there's no table within
    reach."): what's on it or in it ("On the table you find two chisels.", "Your hands find two chisels on the
    table.", "There's nothing in the chest."; closed, not looked into: "It's closed: you go over the outside of it."),
    then it gone over for traps, examine's check, one rule for it ("The chest shows no sign of a trap: you go over it
    carefully, and find none.": things have no traps yet).  A minute.
  - **"search the chest for traps"**: in the light, examine's check alone (ten seconds: Looking at things, above); in
    the dark, the search on it by touch.  **"search the door"**, "search the door for traps": the door gone over for
    traps (the world's PA_CHECK, examine's).  **"search for traps", "search for hidden things"**: a search, as "search"
    is.

## Hands and limbs *(slice 4.1b, 2026-10-22: drafted for the user to review)*

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
  it's left-handed; ambidextrous, the right, with no off hand.  The book's arm missing or deformed (Character 9:
  "activities which require two hands ... are impossible") makes its off arm the useless one, the house's choice of
  which.  (gear.c's limbs_of.)
- **What strikes, nothing named, is the main hand's** *(every ruleset's)*: the thing in the main hand (or both), else
  the other's; a soft thing only if that's all it holds (the user's rule of the soft, as before).  This replaces "the
  better of two" (the strike agent's draft): a sword in the off hand and a torch in the main, the torch strikes, till
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

## Decided, not yet built

- **D13, Strength and weapons:** Strength doesn't add to impact; it sets how heavy a weapon can be wielded without
  penalty; heavier versions hit harder, and strong characters are made with heavier weapons.  Today the weapons
  have one weight each and Strength does nothing in a blow.
- **D13, free pounds:** the first X pounds carried are free of encumbrance, X 10 at Strength 10 and doubling each 4
  points (4 lb at 5, 14 at 12, 24 at 15, 40 at 18, 57 at 20, 134 at 25).  Today it's the book's: every pound counts.
- **D13, a wrecked door, window or portcullis appraises by its state.**  Today appraisal doesn't see the damage.
- **D13, the pit climb:** a slip climbing out of a pit ends the climb, and the player chooses what next.  Today a
  slip slides 5 feet back and the climb goes on.
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
- **Fighting on with missiles** (the user's, 2026-10-20; slice 4.7, with the missiles): a bow's arrows, a brace of
  throwing daggers, the same way: shot or thrown at the target while it's in range, the next taken each time, the
  next foe in range after it, till none is left, the missiles run out, or an enemy engages the character (then
  fighting on in melee).  Today nothing thrown wounds.
- **Auto-engaging at a range the player sets** (the user's, 2026-10-20, an enhancement): "autoengage with sword at
  close range", "autotarget with bow at medium range": a hostile coming within it begins fighting on (melee's
  close range the engagement zone and a square more, closed in on; a missile weapon's the book's bands).  Today
  only an enemy engaging the character, or one attacked, is fought.

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
