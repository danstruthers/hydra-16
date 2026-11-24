# DQ0: the decisions archive, 2026-11-06

The agents' accounts of what they built, moved out of DECISIONS.md on 2026-11-24 (branch docs2) with their text
unchanged, as AGENTS.md's Records rule asks: DECISIONS.md holds the user's words only. Each section left a one-line
pointer there; where a section held the user's words too, those stay in DECISIONS.md, quoted, and the whole section
is here. Each section is marked with its lines in DECISIONS.md as at d35194df.

<!-- DECISIONS.md:1878-1887 at d35194df -->
## Commands that name a thing always answer about that thing (2026-11-06, as built, for review)

The user asked for a fresh look at the command processor (COMMANDS-CRITIQUE.md and ENGINE-CRITIQUE.md, beside this file), then: "Go ahead and start work on these changes."  The first step is the critique's 4.6 and its migration step 4.

- **A sweep of every verb at every kind of thing** (tests.js's dq0sweep): 17 verbs at the kitchen's pot, well, sconce, stairs and wall and the carried backpack, torch and sack, 134 lines, one rule: the answer to a line that names a thing names it too.  Before: 80 of the 134 broke it.  "sit on the pot" sat down; "climb the pot" climbed the stairs; "light the stairs" lit a new torch; "take the stairs" asked "Take what?"; "kick the pot" said "There's nothing like that here to fight."; "touch the pot" said "You can't touch in this game yet."
- **The rule, as built (refuse.c, overlay 1):** a thing named that its verb would pass over, or do to something else, is refused, said back from the line as typed: "You can't sit on the pot in this game yet.", "You can't climb the pot.", "You can't take the stairs.", "You can't kick the pot.", "There's nothing written on the stairs."  A verb the game doesn't do yet names its thing too ("You can't touch the pot in this game yet."); one it doesn't know at all still names only itself ("I don't know how to bribe.").  Each verb's rule is a row of a table: what it refuses for a thing named by its word, and the parts of the place it does (climb: stairs, a ladder, a shaft, a window, a ledge; a well and a rope by their words).
- **After:** 4 of the 134 left, listed in the test (a look at the stairs the character stands by, a sniff toward them, "light the torch" with the torch in hand lit).
- **Found, not mended (the designators' work is in the same chooser):** "take the backpack" while it's worn, and "take the sack" while it's in hand, say "You see none of those here."  The take looks only at what lies about; a thing carried should be answered as one ("You wear the backpack already.", "You hold the sack already.": the things' SA_WORN, SA_HELD).
- **Overlay 1 has 1 byte free after this** (the old o_unknown moved into refuse.c with it).  Every overlay of the UI's but OV22 (the letters', 912) has under 500 bytes free: ENGINE-CRITIQUE.md's first point.
