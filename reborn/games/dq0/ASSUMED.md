# DQ0: assumed answers

**This is the list the user may veto.** Each entry is an open question that a rule of WORLD-MODEL.md already answers.
The answer is taken as decided, and work goes ahead on it. To veto one, say so: the rule changes, and so do the other
cases it settles (AGENTS.md's Scope). Silence accepts.

Made 2026-11-24 (branch docs2) by triaging OPEN.md as at d35194df against WORLD-MODEL.md: 64 of its 91 entries are
here. OPEN.md keeps the questions that are the user's (section 9), asked as rules, and an engineering backlog. Each
entry gives the old id, the question in a line, the answer at the generality the rule implies, and the rule. Entries
are grouped by the rule that settles them; a second rule is named in the answer. The old entries, with their sources,
are in `git show d35194df:reborn/games/dq0/OPEN.md`.

## P1. Realism first

- **D8.** Forcing a braced door ignores the chest against it, so a hostile denizen bursts it. **Answer:** whatever
  braces a door adds to what forcing it must overcome, for every actor and every way of forcing. The heavy thing's
  weight makes the force harder (a realistic house figure, marked tunable), and a brace too heavy for the forcer
  holds the door shut, whatever happens to its lock or bar (T2). (P1)
- **H18.** Oil pools' numbers (an hour lying, two or three minutes burning, 2d6, the stumble 4 steps harder): right?
  **Answer:** the book's figure wherever it gives one (a burn's harm); otherwise ans1's figures stand as realistic house
  figures, marked tunable. (P1; AGENTS.md's Scope, a number)
- **S5.** A robber's critical failure at hiding its stash leaves it hidden at hiding 0. **Answer:** a critical
  failure at hiding anything, by any actor, leaves it in plain sight; a marginal failure hides it poorly. The grades
  are the book's (Ac3), and the same for the character (P2). (P1)
- **G15.** A creature in a pit beside the character fights from it, and is seen as "Something": what rules?
  **Answer:** one below fights one above by the book's terms for a foe below or above, and by realistic house terms,
  marked tunable, where the book is silent: what it can reach, and the advantage of the one above. It is seen as
  what it is wherever the looker's light and facing reach into the pit (S1). It is never "Something" when seen.
  (P1)
- **G16.** Not built, for want of a decision: water (wading, swimming, breath, drowning); the dark's stumbles; darting
  past a body; dart poison on a creature; a burning pool hurting a creature; an alarm out of sight told. **Answer:**
  none of these needs a decision. Each is built, in its turn, for every actor (P2): by the book where it speaks
  (swimming, drowning, poison) and by realism where it doesn't. Harm is harm (B4), and an alarm is told only where it
  can be heard (S2). (P1)
- **R2.** The numbers:
  - caught filching, the attitude's steps by nature;
  - a creature's Legerdemain as its Dexterity x5;
  - "better" armour judged by worth;
  - the errands' cadence;
  - a fellow's need of a light;
  - the brace only against the character.

  **Answer:**
  - A creature's skill is the book's skill base from its own attributes, Legerdemain's as the book gives it (Ac3).
  - Armour is judged better by what it protects (the referee's coverage and protection), not by its price, for every
    actor choosing armour.
  - The caught steps by nature stay as realistic house figures, marked tunable (Ac4).
  - Folk whose life needs light carry one: a fellow with no light at all is given one, dark or not (T5).
  - Any actor braces a door against any foe, and fetches or pushes furniture to do it, as the character can (P2,
    Ac4).
  - The errands' cadence is the engineer's to set so their cost is met (P7).

  (P1)

## P2. One path

- **G3.** A denizen's useless arm doesn't stop its chisel, as the character's does. **Answer:** a useless limb stops
  every act that needs it, for every actor: a chisel, a two-handed hold, a climb, a blow. (P2)
- **G5.** A denizen's grope while engaged (C17); its worn places (one of a kind only). **Answer:** an engaged actor
  gropes by the same test as the character. Every actor wears things by the character's worn-places rules, at every
  place its body has. (P2)
- **G7.** A gift to a denizen is taken whatever its hands hold. **Answer:** whatever is given is received by the
  receiver's take rule, the character's: a free hand, else hands freed by B5's order, else refused. (P2)
- **G10.** Climbing by rounds for denizens (shafts, ropes, ledges, pit sides, wells, windows, jumping down), and the
  rope acts (tie, untie, cut, spike, grapple). **Answer:** every climb and every rope act runs the character's code for
  every actor, with step1's fall for an actor. An ally no longer comes along in a second. (P2)
- **G11.** Denizens changing stratum of their own accord (patrols, fleeing up stairs). **Answer:** any actor leaves
  its stratum by stair, shaft or rope when its mind chooses (Ac4), and the world keeps it where it went. (P2)
- **S4.** A hunter's sniff costs 3 s and a test: test its following of the fresh trail too? **Answer:** following a
  trail is tested as the character's tracking is, by the skill the book names (Ac3), at every step where the
  character's would be. (P2)

## P3. Generalize

- **P5.** "call on" alone calls on the character's own deity, as "pray" alone does: right? **Answer:** yes. Every
  prayer that names no deity is to the actor's own deity, as the user's prayer was (2026-11-10). (P3)

## P4. Verbs do what they say

- **P4.** "double time" sets a jog: a jog, or a run? **Answer:** a jog. Double time is the marching quick pace, and
  each pace word sets the pace it means (P1). (P4)

## B3. Consciousness

- **K1.** Lines waiting as the character is knocked out are refused, after "Everything goes black.", not dropped:
  keep it? **Answer:** keep. Every line given to a senseless actor, waiting or new, is refused the same way (the user's
  answers of 2026-11-13 and 2026-11-14). (B3)
- **K2.** A line the UI sent just before it learnt of the knockout is dropped by the world unanswered. **Answer:**
  refused as K1's are. The answer can't depend on when the UI learnt of it; closing the race is the engineer's. (B3)
- **K5.** As it comes round: the stir, and the whole look only if its light changed; or always the whole look?
  **Answer:** always the whole look. "What's around is learnt as it comes round" (the user's, 2026-11-14), for every
  actor coming round: a mind learns afresh what's around it. (B3)
- **K6.** Its own limbs' news as it falls ("You drop your torch.") is told with the black-out: keep it? **Answer:** no.
  What happens as the body falls senseless isn't told after "Everything goes black." It's found as the character comes
  round, in the look (K5). (B3)
- **K8.** "You've begun to learn Club by doing it." is told as it lies senseless: keep it? **Answer:** no. Nothing is
  told to a senseless character; news that comes while it lies senseless is told as it comes round. (B3; section 7)
- **G8.** A sleeping denizen's hearing isn't halved against a filch, as the character's is. **Answer:** halved. The
  asleep hear by half, every hearer, in every hearing test. (B3)

## B4. Harm is harm

- **H19.** A creature in a burning pool isn't burnt: the referee has no harm for a creature outside a blow.
  **Answer:** fire burns every body in it, by the same harm as the character's, and tests its courage as a blow's
  wound does. So do a dart's poison and a roof's fall on any creature (G16; dig1's question 3). (B4)

## B5. Hands

- **L3.** "hold another torch" with both hands full lays the lit torch on the ground. **Answer:** a hand is freed by
  putting things home, the light last. The other hand's thing goes home and the lit torch stays in hand; a thing is
  laid down only when it has no home. For every actor. (B5)

## B6. The helpless

- **R1.** Robbing:
  - one held, robbed, has no test and its attitude is unmoved;
  - a sleeper robbed is tested as a filch, though all its things go;
  - a robber goes straight home after, without fleeing first, and one blocked flees and comes again.

  **Answer:**
  - One held or bound knows it's being robbed, and its attitude moves as one's that catches a filch does (Ac4).
  - A sleeper is robbed thing by thing. Each thing takes its time and is a chance for it to wake by hearing at half
    (B3), so taking everything is riskier than taking the purse.
  - A robber's way home, and what it does when blocked, are its mind's, by its nature and what threatens it (Ac4). Its
    takings go with it (T3).

  (B6)

## B7. One clock for fatigue

- **G2.** A denizen doesn't tire by its spells, falls and climbs. **Answer:** every body tires by every act's fatigue,
  on the one clock, as the character does. (B7)

## T1. Every thing is unique, and somewhere

- **H21.** The flask is called "flask of oil" whatever it holds, is worth a full one's 2 pennies even empty, and weighs
  nothing by itself. **Answer:** a container and its contents are separate things. A container is named by what it
  holds ("flask of water", "empty flask"). Its worth is its own plus its contents', and so is its weight. Every
  container alike. (T1, T2)
- **R3.** Takings carried are never refused for room or weight. **Answer:** a robber carries takings as any actor
  carries anything: in a holder with room, a free hand or at its belt, within its load. What it can't carry it leaves,
  lying where it's left (T3). (T1)

## T2. Physics

- **H22.** Plain "take the X" doesn't reach into containers, though the chooser of other verbs does. **Answer:** it
  does. A thing known to be in a container is taken out of it, at any depth, each level the longer, as every verb's
  chooser does (H17). (T2)

## T5. A creature carries what its life needs

- **D6.** Kobolds and orcs rob but carry no lockpicks, so they force locks. **Answer:** a kind that robs carries the
  tools it has the skill to use: picks if its sheet has Lockpicking (the ruleset's data says which kinds have it).
  One without the skill forces locks. (T5)
- **D7.** A bandit's lockpicks are drawn when it's first met, so a robber may have none yet. **Answer:** a creature has
  its kit from its making, met or not, so a robber never lacks its picks. Finding the room for it is the engineer's
  (P7). (T5)
- **G13.** Denizens refilling lanterns and putting out lights as they rest; keepers relighting sconces (D1).
  **Answer:** every act the character can do with a light (filling, putting out at rest, lighting a sconce) is offered
  to the minds (Ac4). Folk whose life needs light keep their lights. (T5)

## Pl1. A stratum follows its history

- **M1.** More style sets than the house's and the open one? **Answer:** a stratum's style follows from who built it
  and when: themegen chooses the set by the theme's history, and new sets come as histories name new builders (the
  world plug-in's peoples, P6). (Pl1)

## Pl2. The denizens shape the dungeon too

- **G1.** A denizen digging down: when would a mind want a shaft? **Answer:** digging down is offered to the minds
  like every ability (Ac4). A mind digs a shaft when its nature and knowledge call for it (a miner following a vein, a
  tunneller whose prey went below), and the shaft breaks through for any actor. (Pl2)
- **R6.** Building:
  - walling up a doorway;
  - rubble taken for stones clearing its square;
  - the work's time spent after it's done (or a long task?);
  - a failure spending all the time (or a marginal one slower?);
  - Masonry by Strength;
  - the work's fatigue not spent;
  - the seven new rows' wording.

  **Answer:**
  - Any actor builds whatever any actor can: a doorway can be walled up as a passage can (Pl2).
  - Rubble taken for stones clears its square (T2).
  - Building is a long task, done at its end, and broken off by "stop" or the world (Ti2).
  - A result is graded as the book grades a test: a marginal failure does part of the work at its time, a critical
    failure spoils it (Ac3).
  - Masonry and Timbercraft use the book's skill bases, for every actor (Ac3).
  - The work's fatigue is spent (B7).
  - The rows are worded in the house's style, one wording for any actor (section 7).

  (Pl2)

## S1. Only what is perceived is known or told

- **D2.** "Close the door" at a portcullis, with no door in sight, says "You reach for the door.": keep it?
  **Answer:** no. The first words name what the character reaches for as it sees it ("You reach for the
  portcullis."), whatever word the player used. (S1; section 7)
- **D4.** A spy-hole's cover left open after a look, seen from the far side as a lit hole with an eye at it: wanted?
  **Answer:** yes. An open spy-hole is an opening, and sight works both ways through it for every looker: a lit hole,
  and an eye at it while one looks through. (S1)
- **D9.** "The bandit goes to the door." is also said for an errand at a window. **Answer:** the words name what is
  there ("goes to the window"), for every feature. (S1; section 7)
- **K3.** A blow landing while it lies senseless is told only by its result, on the status line and the sheet: or a
  word as it comes round? **Answer:** as it comes round, the character feels its body's state. A new wound is told
  then, in plain words, and its numbers go on the sheet. (S1; section 7)
- **K4.** A robbery while it lies senseless is told as it comes round, by said.txt's TASK.coins: keep the words?
  **Answer:** keep them. As it comes round it is told what it can perceive is gone: by feel from its hands, belt and
  purse, and the rest as it looks. (S1)
- **L5.** A sconce's torch going out in a room its lamps light: no look, and the torch shown unlit at the next look.
  **Answer:** keep the no-look: what's seen is the same, by the user's rule of 2026-11-14. A light going out within
  sight is told in its sentence, as a lying light's is, for every light, whether carried, lying or set. (S1)
- **G4.** Wording:
  - "The dwarf miner strikes a door behind the facing.";
  - a denizen's light going out told as a lying light's;
  - no "comes down the stairs" row;
  - "strains at something heavy" unnamed;
  - "hands ... to the soldier" for "to its fellow";
  - a trap's words only if its square was seen.

  **Answer:** each is worded by the telling rules:
  - One wording for each action, for any actor: a flight's arrival gets its "comes down the stairs" row.
  - Only seen names are used: what it strains at is named when it's seen, and a fellow by the name seen.
  - A light going out is told as its bearer's when the bearer is seen ("The bandit's torch gutters and goes out.").
  - A trap's words come with its harm whenever either is perceived.
  - The dwarf miner's row is the engineer's to make read true.

  (S1; section 7)

## S2. One hearing rule

- **D3.** Sound and smell through a door's grille: as through a closed door, or through the grille? **Answer:** through
  the grille. Sound and air pass an open grille or spy-hole as they pass any opening, for every hearer (S1: "through
  any opening"). (S2)
- **D5.** A portcullis's noise (heaving 7, raised 8, bent bars 8, winding 5) in one row for both. Which numbers?
  **Answer:** one row for every actor, the act table's noise. Its figures are HOUSE.md's where it gives them (winding 7),
  and otherwise realistic house figures, marked tunable. (S2; P2)
- **R5.** A cry's loudness: 5, a fight's din (it was 3). **Answer:** 5, as built: a realistic house figure, marked
  tunable, the same for every crier, heard by the one hearing rule. (S2)

## S3. Knowledge

- **S1.** A secret door or trap a denizen finds is found for everyone: keep, or knowers per faction or per finder?
  **Answer:** per knower. A find is known to the finder, to those who see it found (the character included), to those
  told, and to the faction that made the thing; to no one else. The user ruled this for traps (2026-11-23 and
  2026-11-24); it is the same for secret doors. (S3)
- **S3.** A denizen's search doesn't bring hiders of its own faction out of hiding: right? **Answer:** right. An actor
  knows where its own fellows hide, and its mind doesn't flush them out (Ac4). (S3)
- **R4.** Three cases:
  - a trap's faction is read from a lair of folk's chunk;
  - a factionless creature's find is shared by all beasts;
  - a denizen stepping onto a trap it knows still sets it off.

  **Answer:**
  - A trap belongs to the faction that made it, recorded as it's made or placed; how that is read is the engineer's.
  - A factionless creature's find is known to it alone, and to any that saw it found.
  - A knower's walk goes round a known trap, and a step onto one goes as the character's step onto a known trap does
    (P2).

  (S3)

## Ti1. One clock

- **G6.** The character's take-up and drop times are the UI's own constants, not the world's reply. **Answer:** every
  act's time is the act table's, through the world, the character's too. (Ti1)
- **G9.** An ally's errands now take their act's time, not a round; coins a denizen takes go to its purse: right?
  **Answer:** right. Every act takes its act's time for any actor, and a thing taken goes where it belongs: coins to a
  purse (T1). (Ti1)
- **G12.** The walk to a flight's foot is counted straight, three squares at most, not the way-finder's path.
  **Answer:** every walk is the way-finder's path, at its steps' time, for every actor (Ac2). (Ti1)

## Ti2. Long tasks are one system

- **T3.** Should the denizens' long work move onto activity records, as the character's has? **Answer:** yes. A
  denizen's long work (a door forced, a lock picked, a pack searched) is timed work on the one long-task system. It
  ends at its time, or when the world or its own mind breaks it off, for every actor (P2). That this changes when a
  denizen's work ends is the rule's intent. (Ti2)

## Ti3. The save is the whole state

- **H20.** Oil pools aren't saved with the game. **Answer:** saved, as everything in force is. No old save needs to
  load till the release (DECISIONS.md, 2026-10-17). (Ti3)
- **K17.** Only the eight actors' shock rolls are saved. **Answer:** every senseless creature's state and its rolls are
  saved. (Ti3)
- **S6.** The null ruleset's senseless character: save its time out with the game? **Answer:** yes, under any ruleset.
  (Ti3; P6)

## Ac1. The thing named is the thing used

- **H8.** The thing a tool is for is kept in hand only when its request names it: a rope tied to a held spike keeps the
  rope and stows the spike. **Answer:** every thing an act names or uses stays in hand through the act, and after it as
  the act leaves them: the spike stays in hand with its rope. Hands are freed, by B5's order, only when another act
  needs them. For every actor. (Ac1)

## Ac4. Minds

- **S2.** Who searches: hostile denizens tailing the character, and robbers. Others too, by their natures?
  **Answer:** any actor searches when its nature and knowledge call for it. The user's words (2026-11-17): "Any actor
  should be able to search about". (Ac4)

## Section 7. Telling

- **L1.** A light of another kind lit while one already burns: look again, naming it, or is its own sentence enough?
  **Answer:** its own sentence. A look comes only when what's seen changes (the user's rule of 2026-11-14), and nothing
  is told twice. (Section 7)
- **L2.** A candle lit beside a torch: no look, and the candle's light not worded: keep it? **Answer:** keep it, by the
  same rule. Its lighting is told in its own sentence. (Section 7)

## P6. Swappable: the world plug-in

The world plug-in owns history, peoples, lands, deities, names, scripts and calendars (P6). The boundary's other
cases are OPEN.md's Q3.
- **W2.** The character's deity: from the world? **Answer:** yes. The character worships a deity of its world's
  pantheon, and the ruleset's rules for prayer apply to it. (P6)
- **W3.** hmchar's cultures, tongues and scripts: the world's, or the ruleset's? **Answer:** the world's, as its
  peoples, their tongues and their scripts. What they give in skills is the ruleset's numbers. (P6)
- **W4.** The calendar: the world's? **Answer:** yes. (P6)
- **W6.** Regions: made by the dice, or picked from a world's map? **Answer:** from the world's lands. A world names its
  regions, and the dice choose among them. (P6)
- **W7.** Stand-in themes: each world its own? **Answer:** yes. A stand-in theme is part of its world's history (Pl1).
  (P6)
- **W8.** Between dungeons (on hold): the town's and scholar's names, the script, the tongue: the world's? **Answer:**
  yes, once that work resumes. (P6)
- **W10.** One set of peoples per world, once the history reads peoples.txt? **Answer:** yes. A world's peoples are
  the world's own. (P6)
