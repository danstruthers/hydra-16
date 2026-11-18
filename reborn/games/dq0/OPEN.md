# DQ0: open questions

Every question about the game that is still open, in one place, as of 2026-11-17 (commit a46c5c38). Each entry gives
the question, its options where there are some (what's built now is marked "built"), and where it was raised: a
DECISIONS.md entry's title and line, or a file:line (relative to reborn/games; a bare file name is
games/dq0's own).

**How it was made.** From the open-question blocks of DECISIONS.md ("Open questions", "Open, for the user", "Open,
for review", "For review / open questions", and the notes left "the user's to decide"), the open notes of
ENGINE-CRITIQUE.md and COMMANDS-CRITIQUE.md, and HOUSE.md's own "Open" section. A question the user has answered
since is dropped (DECISIONS.md's answer entries, the latest "The critique's questions: the user's answers
(2026-11-17)"), and so is one that later work settled.

**Keeping it.** When a question is answered, the user's answer goes in DECISIONS.md and the entry leaves this file.
A new question goes here, under its area, not in DECISIONS.md or a commit message. The "as built" reports in built/
end with their open questions: copy them here when the branch is merged.

**Not here:**
- **Unbuilt work** that needs no decision. It's in GAME-CRITIQUE.md's section 10 (the order of work, each piece
  placed in a phase by the user's answers of 2026-11-17), the last "Left:" note of each row of ENGINE-CRITIQUE.md's
  status table (its section 11), and COMMANDS-CRITIQUE.md's plan.
- **Rules and builds awaiting the user's review**, with no question put: the 73 DECISIONS.md entries headed "as built,
  for review", and the 48 rules HOUSE.md marks *(new, for review)* (`grep -n "new, for review" HOUSE.md`).

---

## The parser and the words

- **P1. Should doors and other features get letters, as things and denizens have?**
  - Options: keep them numbered in "Which door? (1) ... (2) ..." (built); or give them capitals in the look and in
    the question ("a door (G) ahead"), which changes every room's words.
  - The user's rule for doors (a numbered list unless one is directly ahead, DECISIONS.md:362-363) stands either way.
  - DECISIONS.md:1955, "The doors' 'Which door?' the same question" ("Not lettered ... If you want them lettered
    too, that's a small step from here"); COMMANDS-CRITIQUE.md:454-457 (its optional step 7: "Whether to letter
    features is a presentation choice").
- **P2. Should a look letter a zone of the floor that has things lying in it?** An option the plan left open; floor
  places are named today by the locatives ("on the floor", "at my feet", "in the corner").
  - COMMANDS-CRITIQUE.md:579-581.
- **P3. Should each action carry flags (NEEDS_REACH, NEEDS_HELD, NEEDS_LIGHT ...), with a handler per class of
  thing (COMMANDS-CRITIQUE.md's 4.4)?**
  - Not built: the shared paths (main.c's things, feature, reach_go) do those steps for every verb already, and the
    coverage report's "-" cells are where a row's refusal would go.
  - DECISIONS.md:6045, "The dispatch table's rest".
- **P4. "empty X" and "empty X onto Y".** The grammar takes them, but there's no world act for them yet. Should the
  things come out and be put down here, or on Y; a liquid poured away; and how long does it take?
  - built/2026-11-17-pb.md, open question 1.
- **P5. A thing put on a ledge, in a corner, the centre, a side or a passage** is still refused as a put; a drop puts
  it down at the character's feet (built). Placing it needs that place's square: walk there and place it?
  - built/2026-11-17-pb.md, open question 2.
- **P6. "unlock the door with the lockpicks" when a key fits** uses the key (built, by the letter of the answer of
  2026-11-17: "pick" always picks; "unlock" uses a key if one fits). The lockpicks were named, so pick instead?
  pb built that rule first, then dropped it to win back overlay 11's room (53b17b15); it would cost a few bytes there.
  - built/2026-11-17-pb.md, "What the rebase changed".

## Doors, portcullises and windows

- **D2. "Close the door" at a portcullis**, with no door in sight, keeps a door's first words ("You reach for the
  door."), the thing named as the player named it (built). Keep it?
  - DECISIONS.md:5820-5821, "Opening a portcullis".
- **D3. Sound and smell through a door's grille**: as through a closed door (built), or through the grille?
  - DECISIONS.md:5954, "Looking through a door's little window".
- **D4. A spy-hole's cover left open** after a look, seen from the far side as a lit hole with an eye at it: not
  built. Wanted? The answer of 2026-11-17 that sight through a grille or spy-hole works both ways bears on it.
  - DECISIONS.md:5955, "Looking through a door's little window".

## Light and the look

L1-L5 from DECISIONS.md:5878-5885, "No look when the light's the same".
- **L1. A light of another kind lit while one already burns** (a lantern lit while a torch burns, the place seen
  whole): look again, naming it ("by the light of your lantern"), or is its own sentence enough (built)?
- **L2. A candle lit beside a torch**: no look, and the candle's light not worded (built). Keep it?
- **L3. "hold another torch" with both hands full** lays the lit torch on the ground and tells no look (built, as
  before). Keep it? The order for freeing a hand chosen on 2026-11-17 (the light last) bears on it.
- **L4. A light put out and lit again by a reach's own steps** (dq0packsteps' dip in the dark): no look, the dark
  never told (built); or a look whenever a light is lit after none was carried? Also ENGINE-CRITIQUE.md's row 6, its
  last "Left:" note.
- **L5. A sconce's torch gone out in a room lit by its lamps**: no look; the sconce's torch shown unlit at the next
  look (built). Keep it?
- **L6. A light at the feet through a rest.** A rest puts out the burning lights the character carries (built). A
  light the hands' rule laid down burning, or one the player put down, burns on through it. Put out every light
  within reach too?
  - built/2026-11-17-polish.md, open question 3.
- **L7. A stir told between a look's near part and its far part.** A look one way now tells what's near first, then
  what lies further (built). A denizen's stir held while the line was answered is told after the near part, so
  before the far part's words; before, it came after the whole look. It shows only where the far part adds
  something. Keep it, or hold the stir for the far part?
  - built/2026-11-17-looks.md, open question 1.

## Hands, lights and the pack

H8 from built/2026-11-17-things.md; its other questions were answered on 2026-11-17 (DECISIONS.md).
- **H8. The thing a tool is for is kept in hand only when its request names it.** A rope tied to a held spike keeps
  the rope and stows the spike. Protect it by the act instead?
- **H9. A thing put back where there's no room.** At the full reach, a pack packed past its room (a kit's) won't take
  back the key that came out of it. "You put away the iron key." is told as the step begins, and the key stays in
  hand (built, dq0keypack). Look for the room before the step is told ("You keep the iron key in hand."), or let a
  thing always go back where it came from?
  - built/2026-11-17-polish.md, open question 1.
- **H10. Things in a pit.** Things let fall into a pit lie on its floor's square: the look tells them "On the floor
  to your left", and they're within reach from its edge (built; the reach knows no depth). Tell them "In the
  pit ...", out of reach from its edge?
  - built/2026-11-17-polish.md, open question 2.

## Knocked out

The user answered three questions on 2026-11-14 ("Knocked out: the three questions": the words "You lie senseless.",
only quit and help taken, nothing sensed). These are the rest.
- **K1. Lines waiting as the character is knocked out** are refused with the same words, after "Everything goes
  black." (built), not dropped unsaid. Keep it?
  - DECISIONS.md:5577-5578, "Commands while knocked out" (its point 3).
- **K2. A line the UI sent the moment before it learnt of the knockout** is dropped by the world unanswered: its
  echo, then the black-out's words, no refusal (built). Keep it?
  - DECISIONS.md:5578-5580, "Commands while knocked out" (its point 4).
- **K3. A blow landing while it lies senseless**: told only by its result, on the status line and the sheet (built),
  or a word as it comes round ("You were struck as you lay.")?
  - DECISIONS.md:5659-5660, "A knocked-out character senses nothing" (its point 1); ENGINE-CRITIQUE.md's row 6, its
    last "Left:" note.
- **K4. A robbery while it lies senseless**: told as it comes round by said.txt's TASK.coins (built). Keep the words?
  - DECISIONS.md:5660-5661 (its point 2).
- **K5. As it comes round**: the denizens' stir ("Nothing stirs in sight now.", "Something stirs. ..."), and the whole
  look only if its light changed (built); or always the whole look?
  - DECISIONS.md:5661-5662 (its point 3).
- **K6. Its own limbs' news as it falls** ("You drop your torch.") is still told, with the black-out (built). Keep it?
  - DECISIONS.md:5662-5663 (its point 4).
- **K7. The sound task goes silent by the hub's pc_out as it reads it**, a moment early or late at the black-out's
  edges, so the knockout blow's own thud may go unplayed (built). Keep it?
  - DECISIONS.md:5663-5664 (its point 5).
- **K8. "You've begun to learn Club by doing it."** is still told as it lies senseless: the referee's news, not a
  sense (built). Keep it?
  - DECISIONS.md:5664-5665 (its point 6).
- **K9. The UI's main RAM.** The change spent 141 bytes of it (of 314 then over the floor). numb (main.c:998), the
  test of each event against a black-out, is called on every event: leave it in main RAM (built), or move it to an
  overlay, at a cost in time?
  - DECISIONS.md:5665-5666 (its point 7).

## The step planner, the reach and time

- **T1. The reach isn't planned whole ahead.** Each step walks the tree again, since the world changes between steps
  (a denizen takes the pack, a grope fails, the dark comes); only the after-work is planned ahead (built). Keep it?
  - DECISIONS.md:4606-4609, "The scheduler table, the continuations' stack, the after-work's queue";
    ENGINE-CRITIQUE.md's row 6.
- **T2. rs_after_on and rs_using stay flags** read from the reach's state, not queued steps: the tool put back and the
  pack put on (built). Keep them?
  - DECISIONS.md:4599-4605, the same entry.
- **T3. Should the denizens' long work move onto activity records, as the character's has?** An activity record per
  actor in the world's bank, and the denizens' long work (a door forced, a lock picked, a pack searched) made timed
  work whose end the world pops, in place of the mind asking again each turn. It changes when a denizen's work ends:
  "for the user's word first".
  - DECISIONS.md:4610-4616, the same entry.
- **T4. A job come due during a free pass's own pass** still rides that pass (built): a free pass from its top
  bounds it to one pass. Keep it? The single give-way rule on one clock, chosen on 2026-11-17, will be built over it.
  - DECISIONS.md:6017-6018, "The hold-back's holes".

## The maker and the dungeon

- **M1. More style sets than the house's and the open one?** Options: sets of the user's own; or themegen choosing a
  set by a theme's history. styles.txt has only the two today. The answers of 2026-11-17 (a stratum's size from what
  was built there, room uses varied, the world plug-in) bear on it.
  - ENGINE-CRITIQUE.md:519-520 (section 5, its step 3's "Left:").
- **M2. A generated theme's room.** With seed 70002's grave-slab clue now whole, the ledger's clue no longer fits
  under linegen's THEME_SIZE, so that theme has one clue fewer (the other 59 of 60 seeds keep every clue). Accept,
  or grow the theme's room?
  - built/2026-11-17-text.md, its open question 3.

## Architecture

- **A1. Game code in the paged ROM, for good?** The user's answer of 2026-11-17: tried for one module, measured, then
  decided. Open until that trial is measured.
  - DECISIONS.md:6758, "The critique's questions: the user's answers (2026-11-17)"; GAME-CRITIQUE.md, section 6.2.

## Tests

- **X1. dq0hold's own poke of burn_due is lost if a minute is under way as it lands.** The test passes at its timing.
  Make it poke burn_ask, a sturdier probe?
  - DECISIONS.md:6022-6023, "The hold-back's holes".
- **X3. A test's `cycles` limit still acts as a budget where it sits close to the measure** (dq0save: 940M against
  851M). Make each a generous timeout?
  - built/2026-11-17-guards.md, its open question 2.

## The user's own open items (HOUSE.md's "Open")

- **U1. Sight's detail by distance.** Within the light, how much detail the eyes make out (movement only, then a
  shape, humanoid or not, then the race), by the sight attribute, unless blind; modelled if it can be, perhaps later.
  A look one way reaches 31 squares at most for now.
  - HOUSE.md:4283-4286; DECISIONS.md:769.
- **U2. Each occupation's balance** ("we will need to look at game balance for each occupation"): each one's worth in
  the game, not only its points, evened out as the user decides.
  - HOUSE.md:4287-4290; DECISIONS.md:729.
- **U3. D9, hearing**: how far each sound carries and how well each actor hears, tuned by ear once the real
  environmental sounds are in.
  - HOUSE.md:4291-4292; DECISIONS.md:526; SOUND.md:128.

## On hold: the between-dungeons work

The town, the scholar's house, the account and prices are on hold (GAME-CRITIQUE.md:31). Their open questions:
- DECISIONS.md:4737-4745, "dq0acct split: the account and the ruleset's hooks": **For review / left**.
- DECISIONS.md:5134-5151, "The town and the scholar's house: the ruleset's hooks": **For review / open questions**.
- DECISIONS.md:5432-5449, "Town prices, local": **For review / open questions**.
- ENGINE-CRITIQUE.md:683-686, and its status table's row 9 (line 826): the time between's prices, "Left:".
