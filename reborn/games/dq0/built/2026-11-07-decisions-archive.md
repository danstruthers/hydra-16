# DQ0: the decisions archive, 2026-11-07

The agents' accounts of what they built, moved out of DECISIONS.md on 2026-11-24 (branch docs2) with their text
unchanged, as AGENTS.md's Records rule asks: DECISIONS.md holds the user's words only. Each section left a one-line
pointer there; where a section held the user's words too, those stay in DECISIONS.md, quoted, and the whole section
is here. Each section is marked with its lines in DECISIONS.md as at d35194df.

<!-- DECISIONS.md:1888-1897 at d35194df -->
## The build places the swapped parts (2026-11-07, as built, for review)

The user: "Go ahead with the auto-packer" (ENGINE-CRITIQUE.md's §1.2, after the parts' stubs and config were made by the build).

- **The parts' banks are the build's.** parts.txt's bank is a part's wish.  The build reads every segment's size from the objects before the link (od65).  A bank whose wishers don't fit moves out the fewest bytes (the smallest part that makes room), each to the bank of the same file with the least room that holds it: the world's dq0w.rop, read only as ropes are first wanted, keeps its own.  A growing overlay pushes part images elsewhere instead of failing the build.
- **Calls are routed by part** (pack.js's route, every build): s_NAME called from outside its part becomes NAME, its stub; NAME called from inside its part becomes s_NAME, straight.  So a function moved is its pragma alone, whatever its callers write.  Two of the UI's stubs went (called only from inside their part).
- **A part past the swap area has pieces moved out by the build** (pack.js's plan).  A piece is a stubbed function and what only it reaches (its static helpers, its tables).  It moves the piece with the fewest calls to the rest of its part, to the part with the most calls to it whose bank has room, in the same file.  It won't move a piece whose calls across the new boundary would carry a pointer (in the prototype), or that stores an address in a global the other side reads, or that assembly touches, or a move that takes a function's estimated stub calls in calls past SWDEPTH.  The moves are listed in the room table.  Today nothing is past its area, so nothing moves.
- **Limits, for review:** the swap-depth estimate is static and high (the world's main loop serves requests as it waits, so its estimate is 29 against SWDEPTH 12); where it's already past, it can't judge, and the tests are the guard.  A part that's one big function (the world's part 6, s_blows, 3,630 bytes) can't be split.  The world is bound by its banks, not its parts: only one bank has more than 1,100 bytes free.
- **Tested:** with dq0d's parts 2 and 9 and seven of the world's parts given less room (DQ0_PACK_SQUEEZE=TASK:PART=BYTES,...), ten functions moved, and all 412 tests passed.  A part forced into a full bank (dq0d's 13) went to another and linked.  banksChecked and planChecked hold the rules each build.

<!-- DECISIONS.md:1898-1905 at d35194df -->
### Taking a thing you carry, lighting the torch you hold lit (2026-11-07, as built, for review)

The two wrong answers left from the sweep (dq0sweep), mended now that the designators' work in the take is in.

- **"take the backpack" while it's worn, "take the sack" while it's in hand** said "You see none of those here."  Now, when nothing of that name lies about but it's worn or in hand, the take gives the wear's or the hold's words: "You wear the backpack already.", "You hold the sack in your right hand already."  (The world's take, things.c's serve.)  One that's only in the pack is still "none here": taking it out is a hold, and that's the player's to say.
- **"light the torch" with the torch in hand already lit** reached into the shut backpack for a spare.  HOUSE.md's rule (2026-11-04) is "the one in a hand first, whichever way it's lit, none put away for another", so now: "The torch is lit already."  A torch unlit in the other hand is still lit first, and "light a new torch" still gets a spare out.  The UI's first words follow the same rule (sway.c's chooser): "You raise the lit torch." instead of "You reach for your backpack."
- **Room:** the UI's overlay 20 has 53 bytes left after this; the catalogue bank's code (where the world's light is) had too little room for the check, so it's in part 92 instead.

<!-- DECISIONS.md:1906-1914 at d35194df -->
### The stairs looked at, examined, sniffed toward (2026-11-07, as built, for review: COMMANDS-CRITIQUE.md's step 6, its first slice)

The sweep's last three gaps, and the first of the features as things (the stairs, a ladder, a shaft, the way out: the senses' openings).

- **"look at the stairs", "examine the stairs"** turned the character and told the whole room again.  Now the one meant (the chooser's, which asked of several) is told in the room look's own words for it: "You stand by the stairs up to the way out."; the way out's with the theme's words for its door, shut or open ("The way out is shut fast. Whatever opens it is somewhere below. A door of black stone closes the stair, a round hollow in its face.").  One only remembered, out of sight, still gets the look about.  (way.c's opened; words.c's one-opening mode.)
- **"smell the stairs"** said only the air's words.  Now it names what it sniffs toward first: "You sniff toward the stairway.", then the draught and the smell that way as before.  (Its turn to face them comes after those words, as the turn is carried out later; it reads well enough.)
- **The sweep has no gaps left**: every one of its 134 lines names its thing, and the take, light, look, examine and sniff answers are held word for word.
- **The UI's banks: a last resort, measured and kept out of use.**  At one point these few hundred bytes left no UI bank with room for part 17, even with one part moved out for it, so the bank packer has a last resort: every part of that file placed again, the biggest first, each in the bank with the least room that holds it.  It worked, but the packed overlays came out 3.5K bigger (the parts scattered pack worse), and the start went 1.7M cycles over its budget (dq0starttime: 226.7M of 225.0M).  So it stays a last resort, and part 17 was trimmed instead (8 bytes): nothing moves today.  The UI's banks have about 4K free between them, in pieces; the next sizeable UI feature needs a bank more (as overlay 23 was for the designators) or space won back (the refusal table, features as data).

<!-- DECISIONS.md:1915-1922 at d35194df -->
### The resolver's choice kept (2026-11-07, as built, for review: COMMANDS-CRITIQUE.md's §5.1, step A)

A survey found six choosers deciding what a name means, and twelve ways they disagree (COMMANDS-CRITIQUE.md's §5.1).  The first: the UI's resolver chose one of several things lying about ("the closest", "the largest", an answer to its "Which do you mean?"), then sent the world only the name, and the world chose again by its own rules.  "take the closest torch", with a lit one at your feet and an unlit one a step off, asked "Which one? (E) a torch (lit), (F) a torch".

- **Now** the resolver's choice of several is sent as that thing's id (letters.c's des_seen adds it to the line's handles, as a letter's is), and the world takes that one: "take the closest torch" takes the lit one at once.  Test dq0closest (it fails on the old code with the "Which one?").
- **One candidate alone** is still left to the world, whose rule puts what's in hand first: "light the torch" with one lying about and one lit in hand is still "The torch is lit already." (the sweep caught this when every choice was sent).
- **Next** (§5.1's order of work): "it" and "take all" carrying ids; then one chooser for things in the UI, over what's carried and what's seen together, which will need a staging overlay bank.

<!-- DECISIONS.md:1923-1931 at d35194df -->
### The carried thing chosen once (2026-11-07, as built, for review: COMMANDS-CRITIQUE.md's §5.1, step C's first half)

The user approved a staging overlay bank (overlay 24) and step C.  What's built:

- **The thing carried that the first words choose is the one the world is asked of.**  The UI already chooses a carried thing to say its first words at once ("You reach for your backpack."), by a copy of the world's ranking; then it sent the world the name, and the world chose again by its own.  Now that choice goes as the thing's id, as a letter's does, for put down, hold, stow, put in or on, wear, take off, light and put out.  The two can't name different things any more.
- **Left to the world:** a state said ("the spent torch": the world knows the state better; the UI's flags call a lit torch with no fuel left not spent, the world's spent); another thing named ("put out the torch in the sconce", "put the sack in the sack": the world's rule that a thing isn't put in itself goes by the names); a lit thing to light ("The torch is lit already.", the world's words).  The suite caught all three.
- **Not done, and why:** a chooser of its own in overlay 24 copied what's carried a second time and made the first words 30,000 cycles slower, past the user's 720,000 for a first letter (dq0torchwords); so the choice is made final in sway.c, where the first words already make it.  Things lying about stay the world's to choose: the UI's sight record holds only six of them, and the world also chooses among things remembered in the dark.
- **Overlay 24** is in the game, empty of code for now; the bank packer moved part 6's image into it when overlay 20 grew by this change.  It's for step D (one "which?" question).

<!-- DECISIONS.md:1932-1940 at d35194df -->
### One "Which one?" for things and denizens (2026-11-07, as built, for review: COMMANDS-CRITIQUE.md's §5.1, step D)

The user: "Do D then finish C, and if you can do them in parallel, do that."  (Step C's second half is being done alongside, separately.)

- **The resolver's question is now asked the designators' way.**  Before: "Which do you mean: 1. the giant rat ahead to your left, on the right, 2. the giant rat ahead to your left, on the left, or 3. the giant rat ahead?"  Now: "Which one? (A) the giant rat ahead to your left, on the right, (C) the giant rat ahead to your left, on the left, (B) the giant rat ahead".  The capitals are the things' and denizens' own (a thing keeps its capital, so they aren't always in list order); the world's "Which one?" (a take of several that differ) was already worded so.
- **One answer reader for both** (letters.c's which_in): a capital, the choice's number, a way ("left", "ahead", "north"), "closest" or "nearest", a word of one choice's alone ("bones", "spent"), each with "the" before it or not.  The answer remakes the command on the chosen one's capital ("take" then "the bones" is "take E"), so it goes through the same path as a letter typed; the resolver's own answer reader and its kept copy of the command are gone.
- **Not yet:** the doors' and openings' question (choose.c's "Which door: 1. ..."), answered by a number or a way; doors have no capitals until features are lettered (step E).  And "Drop what? [a, d, f]" keeps its own form.
- **Room:** the new code is in overlay 24 (which.c); resolve.c's overlay 11 went from 7 bytes free to 1,097.

<!-- DECISIONS.md:1941-1949 at d35194df -->
### A state said, the carried thing chosen once too (2026-11-07, as built, for review: COMMANDS-CRITIQUE.md's §5.1, step C)

The carried thing the first words choose is now the world's with a state said too ("drop the spent torch", "drop the full pot", "hold the open sack"), as the step before left out.  What's built:

- **The UI's flags are the world's chooser's.**  The world makes what the UI knows of each thing carried (s_ix_make); it now makes each state flag by the tests its own chooser uses (pick_ and unlike), so the UI's tests (sway.c's ix_pick) take what the world would.  Where they differed: "open" (the world needs a kind that closes: a basket isn't open, it doesn't close); "full" (only a container, a surface or a fuel is full: potsherds aren't); "spent" (an empty container is spent too, and a light burnt out though it's still marked lit); "shut" (closed, whatever the kind).  The kind that closes is a bit in the kinds' flags the UI copies (in K_BELT's place, which the UI never reads); "full or empty" is the bit that said "a container" (CX_HOLDS, read by nothing).  The words that tell things apart ("You put down the spent torch.") read the same flags and say what they did.
- **Not to light or put out.**  The world's lighting doesn't choose by its chooser but by the name ("light a new torch" lets the old one in hand fall by its name); given the id, it put the spent torch in the pack instead.  That's what failed dq0torchwords when this was first tried, not the flags.  So a light lit or put out with a state said is still the world's to choose.
- **Test dq0statesel:** potsherds before a pot of pitch in the pack, "drop the full pot" puts down the pitch (with the old flags the UI chose the sherds and the world, given them, said "You carry none of those."); "drop the open basket" puts nothing down; "drop the spent torch" is the one burnt out in hand.
- **Room:** the UI's overlay 20 has 2,876 bytes free (73 used by this), the world's part 102 931 (60 used).  The start 225.8M cycles at seed 1 (its budget 226M).

<!-- DECISIONS.md:1950-1957 at d35194df -->
### The doors' "Which door?" the same question (2026-11-07, as built, for review: COMMANDS-CRITIQUE.md's §5.1, step E)

The user: "Finish up 5.1, please, on parallel threads."  (Steps B and F are being done alongside, separately.)

- **The question about doors and openings is now asked and answered the same way as the things'.**  Before: "Which door: 1. the door ahead to your left (closed), 2. the one to your right (open), or 3. ...?", answered only by a number or a way, and only when the answer read as a move.  Now: "Which door? (1) the door ahead to your left (closed), (2) the one to your right (open), (3) ...", answered by the same reader as the things' question: a number, a way ("left", "north"), "closest", or a word of one choice's alone ("open", "double", "barred").  The answer makes the command again as it was typed, its door chosen.
- **Not lettered.**  Giving doors and openings capitals would change every room's words ("a door (G) ahead"); the designators' design gave capitals to things and denizens only.  So doors are numbered in the question, and nothing else about them changed.  If you want them lettered too, that's a small step from here.
- **Room:** choose.c's own answer and kept command are gone (overlay 7: 40 bytes free to 255); the new code is in overlay 24.

<!-- DECISIONS.md:1958-1968 at d35194df -->
### One name match (2026-11-07, as built, for review: COMMANDS-CRITIQUE.md's §5.1, step F)

Six rules decided whether a word typed names a thing: the world's chooser's (a word of the name or its start, a plural, the end of the name: "pack" a backpack), the world's other copy without the end, the resolver's (whole words, a plural, and a qualifier checked), the first words' (a copy of the chooser's, and `strstr` for things seen), and the blows' `strstr`.  "pack" found the backpack in the world and nothing in the resolver.  What's built:

- **One rule, the world's chooser's,** in the UI as `name_is` (nameis.s, the main RAM's, so every overlay and part reaches it): a word of the name or its start ("key" an iron key, "bon" the bones), that word made plural by s or es ("sacks", "torches"), or the end of the name for a word longer than 3 ("pack", "skin").  With the qualifier check only the resolver had (help.c's, gone): the word before it in the line, if it's one that makes one kind another ("black", "giant", "iron", or its start), must be in the name too, by the same rule.
- **Used by** the resolver's gathering (things and denizens seen, the denizens remembered), the first words (the kinds carried, `ix_pick`; the things seen, and "You look them over."), the blow's denizen (blows.c), the tending's patient (tend.c) and a walk's new things (task.c's new_words).  **The world:** its `named` took the end-of-name rule and the chooser's copy (`named4`) went into it, so the chooser, the light, the market and the search ask one rule.
- **Left with their own, on purpose:** `o_may_den` (three letters anywhere in an actor's name: looser, as it only says "maybe, ask the denizens"); letters.c's `which_in` (a word of a "which?" choice, not a name); main.c's `bow_named` (a launcher's kind by `strstr`: "bow" names a shortbow there, which the rule's end of a name, 4 letters at least, wouldn't).
- **Looser in the resolver now, for review:** a word's start names a thing, so "go to the rat" with rations lying in sight would ask which of the two (the world's take already took rations for "rat").
- **Test dq0namematch:** the backpack put down, "target the pack" and "target the bon" find it and the bones (before: "You see no pack here.", "You see no bon here."); "target the black rat" finds none of the giant rats.  No other test's expectations changed.
- **Room:** in assembly, as the C was 550 bytes and left the UI's main RAM under its 2,048: the main RAM 2,560 bytes free to 2,194; overlays 1 from 1 to 443, 11 from 1,097 to 1,507, 20 from 2,876 to 3,197.  The world's main RAM 2,894 to 2,791, its part 4 134 to 447.  The start 225.3M cycles at seed 1 (its budget 226M); all 421 tests pass.

<!-- DECISIONS.md:1969-1978 at d35194df -->
### "It" and "take all" by the thing, not its word (2026-11-07, as built, for review: COMMANDS-CRITIQUE.md's §5.1, step B)

Two more places where the UI knew which thing was meant and sent the world only its name, for the world to choose again (the survey's disagreements 8 and 12).

- **"It" is the thing last named, by its id where that's known.**  Before, the resolver kept the last thing named as its word alone, so "take it" after "look at the closest torch" (a lit one at the feet, an unlit one a step off) asked "Which one? (E) a torch (lit), (F) a torch".  Now its id is kept with it (it_id, in the main RAM) when a handle had it: a letter's or a capital's, the resolver's choice of several, or the carried thing the first words chose (sway.c's bind).  "It", and a verb's unsaid thing that's the last one named ("drop", "take" after "examine the book"), make that id the line's handle again, as a letter's is, so the world is asked of that thing ("#" and its id).
- **Only while it's still carried or in sight.**  The id is checked against the letters of what's carried and the things in sight; a thing gone (eaten, burnt, or joined to a pile of its kind as it was taken up, its record gone with it) or out of sight is named by its word, the world's to choose, as before.  Without that, "take E" of a ration lying about and then "eat it" would have found none.
- **"Take all" takes each thing it listed, by its id.**  It lists the things it sees lying about, nearest first, then took each by its name, and the world took the nearest of that name it knew: with a torch hidden at the character's feet and a lit one a step ahead, "take all" took the hidden one and left the lit one lying.  Now each one's id is read from its store record as it's listed; the first is the line's handle, and each next one's is set as it's handed on to be taken (taken_next: the line's other handles let go then, a "take all" line has none of its own).
- **Test dq0it:** both, in one game at seed 3; it fails on the old code with the hidden torch taken, and "Which one?" for "take it".
- **Room:** the handles' adder (letters.c's des_add) is in overlay 24 (542 bytes of it: 1,736 left), as overlay 22 has 570 and the code reads the store's records and the letters itself; resolve.c's overlay 11 used 171 (926 left), sway.c's overlay 20 27 (2,849 left); the main RAM 13 bytes (2,547 left).  The start 225.5M cycles at seed 1 (its budget 226M); all 421 tests pass.

<!-- DECISIONS.md:1979-1986 at d35194df -->
### §5.1's last disagreements, and the start's budget (2026-11-07, as built, for review)

The user: "Don't worry too much about the startup time.  If you need to rewrite some of the portions in assembly, do so.  Do whatever cleanup you can, and then start the next piece of the critiques work."

- **The start's budget** (dq0starttime) is now 235M cycles at seed 1 and 315M at the big seed (it was 226M and 305M; the start is 225.8M and some 302M now), so the work to come isn't held to the last megacycle.  The measure is kept: it still fails a start that grows a lot.
- **The disagreements the A–F steps didn't close**, closed: the first words of a thing named by a capital or another handle never borrow a carried thing of its name (7); "attack the rat with the other sword" takes the other sword in hand, the instrument's own selector, not the rat's (9); "attack the rat" with several rats in sight goes for the nearest, the resolver's choice, its square the fight's to go by, not the first in the actors' table (11); "You look them over." is said only when two of those seen differ, as the world then asks which; two alike, the one taken is reached for (5, test dq0leftovers).  "The second" settles as one order for each kind: carried things the UI's, things lying about the world's, denizens the resolver's (2).
- **Clean-up:** the three parallel agents' worktrees and branches removed (one folder under .claude/worktrees still held open by a process, left to go); stray single-byte section signs written into four sources made UTF-8.

<!-- DECISIONS.md:1987-1995 at d35194df -->
### The second sweep: every command that names a thing answers about it (2026-11-07, as built, for review: COMMANDS-CRITIQUE.md's §5.2, step G)

The next piece of the critiques' work, after §5.1: the dispatch (§5.2).  Its first step is a second sweep of the verbs the first one didn't cover, the same rule (an answer names the thing named), and the gaps it found closed.

- **Test dq0sweep2:** eat, drink, fill, look in, tie, untie, wear, push, pull, unlock, lock, raise, lower, disarm, force, bend and bar, each at the pot, the well, the sconce, the stairs, the wall, the backpack worn, the torch in hand and the sack carried: 136 lines, 55 of them broke the rule, none now.
- **What changed:** "eat the pot" said "You're not hungry."; now "You can't eat the iron pot." (a thing named must be food, its kind's key, or water, a waterskin, the well; one not to hand, "You have no apples."); not hungry with food named, "You're not hungry. You leave the rations be."  "raise the pot", "bend the pot", "bar the pot", "disarm the pot" went looking for a door or a trap; now "You can't raise the pot." (a door's or a window's own part by its word still taken: "lift the bar", "bend the bars").  Eat, drink, fill, tie, untie, wear and look in at the stairs or a wall: "You can't fill the stairs." (look in still works at a door, a window, a passage, a shaft).
- **For review, odd but named:** "tie the torch" says "You can't close the torch."; "wear the torch" says "You hold the lit torch in your left hand already."
- **"take all" holds its next thing while a line is being typed.**  The test of the user's rule for a line typed during a reach (dq0reachstop) caught "stop" typed during "take all" acted on only after the next thing was reached for and taken: the next was started as soon as the UI was free, whatever was being typed, so the order turned on timing.  Now the next waits while a line is half typed; a stop, once entered, ends the take as the rule says; any other line is held till the take's end, as before.

<!-- DECISIONS.md:1996-2015 at d35194df -->
### Refusals as data (2026-11-07, as built, for review: COMMANDS-CRITIQUE.md's §5.2, step K)

The same refusal was worded, or built, in many places: "You see no %s here." in eight, "out of reach" four ways, a dozen "<verb> what?".  What's built:

- **One table, one routine.**  refuse.c's `said[]` is the table of refusals, its first rows the line's (U_VERB, U_YET, U_CANT, U_NOTE, and now U_WHAT), then the A_* (dq0.h): "You see no %s here.", "You have no %s." (each with a nameless form: "You see none of those here.", "You carry none of those.", said when there's no name to put in, none or a handle's "#"), "There's nothing here to take.", "You haven't seen a(n) %s here." and "... that way.", "%s is out of reach: go up to it first." ("the" put in before a name that isn't capitalised, the sentence's start capitalised), "You can't get near enough to %s.".  Said by `say_no (a, w)` (w the thing's name, 0 "it"; copied first, so it may be lb, any of the main RAM's, not another overlay's own), or `unknown (U_*)`, from the line, as before; `A_LB` adds it to lb, unsaid, for the sway's words and a blow's note, which are built there.  Each site calls it in place of its own `sayf`, nothing round it moved (thirty-five sites, fourteen files).
- **refuse.c is overlay 24's now** (overlay 1 had 286 bytes; the table and say_no wouldn't fit beside the title and the help).  One more stub in the main RAM (11 bytes).
- **Every wording that changed** (old, then new; where):
  - "You see none of those here." → "You see no pumpkin here." (the thing named): take (main.c's s_took_up), hide behind a thing (s_hide), lift a bar with no door by it (s_lift), search for a thing (search.c).
  - "You carry none of those." → "You have no pumpkin.": drop (main.c), hide a thing (s_hide), throw (resolve.c).  "You have no such thing." (give, with nothing named: errand.c) → "You carry none of those.".
  - "The giant rat is too far off: you'd need to be within reach." (give, a beast's ways: sway.c; a trade: errand.c), "It's out of reach: go up to it first." (a blow's note: blows.c), "You'd have to be beside the rat." (first aid: tend.c) → "The giant rat is out of reach: go up to it first.", "It is out of reach: go up to it first.", "The rat is out of reach: go up to it first.".  ("... is too far off to hear you." kept: talking range isn't reach.)
  - "You can't get any nearer to it." (a walk to a thing that stopped short: main.c's reach_go) → "You can't get near enough to it." (the fight's close-in's words, task.c's).
  - "Take what?", "Put down what?" (main.c), "Read what?", "Copy what?" (read.c), "Name what?" (names.c) → the line's verb and "what?", as "Drop what? [a, b ...]" was already: "Take what?", "Get what?", "Put down what?", "Drop what?", "Read what?", "Copy what?", "Name what?", "Call what?" (in play the take's and the drop's are seldom reached: "take" alone asks which, "drop" alone lists the letters).
  - "There's nothing written on it." (read, a part of the place named with no word: read.c) → "There's nothing written on the way out." (U_NOTE, the line after its verb, as refuse.c's rows say it).
  - "You can't eat the pot." (feed.c, from its word) → the same from the line (U_CANT): "eat pot" says "You can't eat pot.".
  - Not changed in wording, only where it's kept: "You see no %s here." (resolve.c twice, words.c twice, way.c, tend.c, errand.c twice, sway.c), "You have no %s." (errand.c, feed.c twice), "There's nothing here to take." (resolve.c twice), "You haven't seen a(n) %s here/that way." (way.c, choose.c).
- **Left as they are, on purpose:** the questions with a hint or another shape ("Throw it at what? (throw the dagger at the rat ...)", "Give it to whom?", "Fetch what?" (an order: its line isn't "fetch"), "Name it what?", "Use what, and on what? ...", "Combine what with what? ..."); the "Which one?" headings (caps.c, which.c: a question's start, its choices after it); the per-action templates of §4.5 (refuse.c's rules[] rows are what there is of them).
- **Tests:** expectations changed only where a wording did: dq0hide ("You see no ... here", either form), dq0firstword ("take the pumpkin": "You see no pumpkin here."), dq0moving ("can't get near enough"), dq0parse (the gift out of reach), the repeats allowed in dq0engage (both forms).  No behaviour changed, so no new test.
- **Room:** the main RAM 2,150 bytes free to 2,139 (the stub); the banks, overlays and parts together, 8,619 free to 8,723 (the build repacked the parts among the banks, so single overlays moved a lot: refuse.c's 700-odd bytes out of overlay 1, the four strings out of tell.c's overlay 14).
- **All 424 tests pass,** dq0sweep and dq0sweep2 among them.

<!-- DECISIONS.md:2016-2030 at d35194df -->
### One path for things, one walk over (2026-11-07, as built, for review: COMMANDS-CRITIQUE.md's §5.2, steps H and I)

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

<!-- DECISIONS.md:2031-2047 at d35194df -->
### One path for features (2026-11-07, as built, for review: COMMANDS-CRITIQUE.md's §5.2, step J)

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
