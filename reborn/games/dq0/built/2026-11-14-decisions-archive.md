# DQ0: the decisions archive, 2026-11-14

The agents' accounts of what they built, moved out of DECISIONS.md on 2026-11-24 (branch docs2) with their text
unchanged, as AGENTS.md's Records rule asks: DECISIONS.md holds the user's words only. Each section left a one-line
pointer there; where a section held the user's words too, those stay in DECISIONS.md, quoted, and the whole section
is here. Each section is marked with its lines in DECISIONS.md as at d35194df.

<!-- DECISIONS.md:5589-5667 at d35194df -->
## A knocked-out character senses nothing (the user's, 2026-11-14; as built, for review)

The user's decision ("Knocked out: the three questions" above): while the character lies senseless nothing is heard,
seen or otherwise sensed; what's around is learnt as it comes round.

- **What it was.**  The UI told everything as it came while the character lay OUT: the denizens' sounds ("You hear
  footsteps to your right.", a hundred "You hear a tapping of picks ..." in dq0tunnel's long black-out), their doings
  seen ("The bandit slams the door shut."), "Something stirs." and the denizens' comings and goings, the blows at it and
  between others (dq0deadonce's six shots, each worded, then "You are dead."), a robber at its things ("The bandit goes
  through your things as you lie helpless, and takes what it finds."), its torch burning low and going out, the look
  that followed; and the sound task played them.  Nothing was told as it came round (the world's look then only its
  eyes', LK_EYES: the lights far off).
- **Built: the UI's gate, by the events' clock.**  `out_told` (main.c; "no line taken" since 2026-11-13) now also means
  "nothing sensed": set as the knockout's told (blows.c's o_blow, if the world has it out still; a harm's E_OUT, s_news)
  with `out_until`, the world's pc_ready then (when it comes round).  `numb` (main.c, main RAM) runs on every event the
  UI reads (events' and heard's loops): an event from out_until on lets it go (come round).  Not the world's pc_out as
  the UI reads it: at the turns pace the world may be up again before the UI has read what came as it lay, or not yet
  down (a blow at its coming round's own tenth, dodged, was dropped so); and no E_READY comes if it comes round fighting
  on (dq0battering: the rise, then a blow).  Struck senseless again as it lies (a blow it doesn't feel, FF_D_DOWN at it),
  out_until the world's pc_ready again.
- **What's dropped while senseless:** a denizen's sound or doing (heard's E_DEN: no "You hear ...", no seen doing);
  another's limbs' news (E_BEAT; its own still told: "You drop your torch." as it falls, part of the black-out); every
  blow (o_blow: at it, between others, a rise: not worded, nothing about it read); what it sees changing (E_VIEW: a
  denizen stirring, V_STIR; a look as its light changes, owed: out_told's bit 2); its light burning low or going out
  (task.c's TE_LOW, as asleep, and TE_OUT); the sounds played (dq0s's listen: the character's stratum taken as none
  while the hub's pc_out is set, its events read and let go).
- **As it comes round:** the denizens it sees now, what's changed since it was knocked out (look.c's stir, as a
  sleeper's waking: "Nothing stirs in sight now.", "Something stirs. (D) A giant rat is ..."; nothing if nothing has),
  or, if its light changed meanwhile, the place looked at whole (look (0): dq0tunnel's torch gone out as it lay, the
  guardroom then told lit by its brackets).  Then the world's own (in no state to fight, no foe left beside it).
- **Harm done as it lies (decided, for review):** a blow landing is **felt only as its result**: not told, its wound on
  the sheet, the status line and the state as it comes round; the referee's body news (bleeding, venom: TE_BODY) held as
  a sleeper's is (tend.c's news_due) and **told as it comes round** (its E_READY); a robbery **found as it comes round**
  (TE_ROBBED with no RB_CAUGHT as it lies: task.c's pursed 2, told with TE_BODY's news then: said.txt's TASK.coins, arg
  1, "You come round to find someone has been through your things."; the sleeper's own "You find your coins gone:
  someone robbed you as you lay." its arg 0, unchanged).  A robbery of one helpless but awake (in shock, feigning death)
  told as before.
- **Death as it lies:** "You are dead." (blows.c: the blow that kills it unworded), then the game's end as ever
  (dq0outcold, dq0deadonce, dq0finished, dq0killrest, dq0fighton, dq0armour, dq0disengage, dq0reach, dq0throws).  Dead of
  its wounds as it lies: the referee's words and "You are dead." as before (tend.c tells a death whatever).
- **Where the gate is.**  In the UI, not the senses: the senses' denizens record gated on pc_out (tried) left them
  unseen after it came round till something moved (the world asks the senses about the denizens only as their bodies
  change, den_seq; and a harm's black-out asks no look as it ends), and a refresh at the eyes' look at a wait's end
  swallowed a stir (dq0denhands).  The senses' record goes on as ever; the UI reads none of it while senseless, and its
  own list of what it told (dens.c's) stays as it was at the black-out, the stir as it comes round told against it.  The
  world unchanged (its main RAM 11 bytes over its floor).
- **Tests.**  dq0outcold extended: the orcs' game from the black-out to "You are dead." only the line refused, though the
  orcs strike at it and go about meanwhile (an E_FIGHT at it and an E_DEN in that turn, or the check proves nothing);
  the rats' game from the black-out to the next prompt only the refusal and one stir (as it comes round), no "fighting
  you" nor "You hear", and an E_DEN meanwhile (both failed by 320ea131's transcripts).  Changed where this decision
  changes them: dq0robbed (the robbery found as it comes round, and not seen: "You come round to find someone has been
  through your things."); dq0finished ("Everything goes black." then "You are dead.", no mortal blow worded);
  dq0helpless (the adders seen fleeing as it lay: now by their moves, an E_DEN DE_MOVED two squares or more off after
  the knockout, and not told); dq0denportal (the slam told unless it lay senseless as it was slammed, by the events'
  times, and then not told).  Every other check as it was.
- **What the player sees differently** (the suite's transcripts, by sentence, sent.js, against 320ea131's: 39 differ).
  The known noise alone: dq0jobs, dq0prompt, dq0reachstop, dq0restbusy, dq0skykin, dq0stopwords, dq0sway, dq0tiring;
  dq0antidote (its real-time fight).  The rest, every one a black-out's: the sounds gone ("You hear ..."), the stirs and
  the denizens' comings and goings gone, one stir or a look as it comes round in their place (dq0battering, dq0bearing,
  dq0denportc, dq0faced, dq0features, dq0helpless, dq0inhab, dq0senseless, dq0strike, dq0turnsame, dq0tunnel, dq0throws,
  dq0reach); the blows at it as it lay gone, "You are dead." on its own (dq0armour, dq0deadonce, dq0disengage, dq0fighton,
  dq0finished, dq0killrest, dq0outcold); the robbery found as it comes round (dq0denpace, dq0denportal, dq0disarm,
  dq0hidelook, dq0hoard, dq0peoples, dq0robbed, dq0robrest, dq0shield, dq0shieldskill; their designators given in
  another order: (G) for (I) ...); the torch burning low and going out as it lay gone (dq0peoples, dq0tunnel).
  "Everything goes black." (and "again"), "You are dead.", "The game is over." as often as before in every test.
- **Room:** dq0's main RAM 3,314 -> 3,173 (numb, out_until, the gates in heard and events, the harm's out_until);
  overlay 3 2,066 -> 2,008, 6 2,227 -> 2,128 (o_blow's), 14 1,473 -> 1,476, 15 335 -> 319 (task.c's), 16 2,005 -> 2,000,
  27 159 -> 154, 28 0 -> 3, 29 1,545 -> 1,487 (TASK.coins' second form); parts 2 333 -> 336, 11 2,322 -> 2,306, 12 2,193
  -> 2,188.  dq0s's main RAM 17,529 -> 17,513.  dq0w, dq0p, dq0d unchanged.  The start: seed 1 232.5M -> 233.3M,
  123456789 309.5M -> 308.7M (timing).  The suite: all 448 pass.
- **Open, for review.**  (1) A blow landing as it lies told only by its result (no words as it comes round: the status
  line's and the sheet's); or a word then ("You were struck as you lay.")?  (2) The robbery's words as it comes round
  (said.txt's TASK.coins, arg 1).  (3) As it comes round, the denizens' stir ("Nothing stirs in sight now.", "Something
  stirs. ..."), the whole look only if its light changed; or always the whole look?  (4) Its own limbs' news still told
  as it falls ("You drop your torch."), with the black-out.  (5) The sound task silent by the hub's pc_out as it reads
  it (a moment early or late at a black-out's edges: the knockout blow's own thud may go unplayed).  (6) "You've begun
  to learn Club by doing it." still told as it lies (the referee's, not a sense).  (7) The UI's main RAM: 141 bytes of
  its 314 over the floor spent; numb could go to an overlay but for its call on every event.

<!-- DECISIONS.md:5668-5781 at d35194df -->
## The line, the way-finder and the chunk lookup in the paged ROM (2026-11-14, as built, for review: ENGINE-CRITIQUE.md's §4 and step 2's next)

The user's go-ahead, 2026-11-14.  The world had 2,059 bytes of main RAM free against its floor of 2,048, and the next
features (containers and surfaces) are the world's.  ENGINE-CRITIQUE.md's §4 had the plan (a fixed zero-page block,
callbacks into the task); the router was to share the contract, so the three went together.  What moved, from each
task that reads the chunks (the world, the denizens, the senses) into the game's library (modules/dq0lib):

- **The chunk lookup** (sq.s): `AT_FIND`, at ()'s long way (each reader's chunk.c's `at_find` and `find`: the
  stratum's bounds, the slot found last, the cache's table searched by the record store's `rp_find` with the same key
  and stamp, the slot this task has in use, the maker asked for a chunk not made, the bank and the chunk by k / 3, the
  quick way set for the next); `CK_MADE` and `CK_AT`, whether a chunk is in the cache, its stamp left (the denizens'
  `made`, `made9`, `made10`, `made16`, `made_at`; the senses' `cached`; the world's loops in `s_sky_list`, `looked_at`
  and wkept.c's `lights`).  at ()'s quick way stays each task's at.s (some 80 cycles a square; a look asks thousands,
  and an XCALL each would double it): the fast path the plan kept.
- **The line** (line.s): dq0w/line.inc's line (sight, fire, air), `ln_pass` and `ln_portal`, the same code; its
  squares read by the library's own at () (sq.s's `sq_at`: the quick way on the task's state, then `AT_FIND`), its
  doors by `sq_door` (portal_at's), LN_CACHED's chunk by `ck_at`.  dq0w/line.inc is now the tasks' stubs (the C stack's
  arguments into the block, an XCALL).
- **The way-finder and its rule for a square** (walk.s): walk.inc's `wk_cell` as `WK_ROW` (a row's squares' cells
  from their kinds and their door bytes, one XCALL a row), `wk_seed` (`WK_SEED`) and `wk_search` (`WK_SEARCH`).
  walk.inc is gone; walk.h is its C side.  Each walker's own rules stay its reader's: the world's `s_ways_go` (a
  denizen's square, a trap it knows of, a hidden trap on the way) and its times by `reached`, called back as each
  square is reached; the denizens' `route` (water deeper than it wades, bad air, a chunk not made).

**The contract** (games/dq0/lnzp.inc, its C side lnzp.h, walk.h, line.h):

- *A block of the task's zero page*, `$44-$70` (the plan had `$50-$7F`; the tasks' own zero page ends at `$41`/`$43`,
  so `$44` on, with room to spare to `$7F`).  The program's zero page is `$22-$7F` and the OS never touches it; each
  reader's cfg now ends its ZP at `$43`, so ld65 can't put anything there.  Kept between calls: the task's chunk state's
  address, its hub_map (a vector), line.h's `ln_way`, `ln_eye`, `ln_n`, `ln_first`, `ln_last`.  Then the line's
  arguments and work (its squares, Bresenham's error, the heights, LN_CACHED's chunk), and over the same bytes, never
  in use at once, the search's (the grid, the floors, the queue, the box, the walker's rules, the square reached).
  C reads and writes them by name (at.s defines them, the headers `#pragma zpsym` them); the line's `ln_way` and the
  search's `wk_reach` are vectors there, called back with `jmp (vector)`.
- *struct ats*, the task's chunk state (at.s's, its RAM): what the library calls back (the maker asked:
  `ask_chunk`, ask_maker's M_CHUNK), the tables it reads (the strata, t_kinds, cornk, dir_x, dir_y: the task's own, not
  copied into the ROM), the quick way's (at_sn ... at_tile), world.h's C, cx_, cy_, and the long way's slot found last.
  at.s's quick way reads it as before, the library through the block's pointer.
- *The call registers* for the lookup's arguments (r0-r2: C's lr0-lr2, lnzp.h's `ck_made`, `ck_at`), and `lib_x`
  (dq0lib.inc's dq0x) for an entry whose arguments are in the block (walk.h's `wk_row`, `wk_seed`, `wk_search`).
- *What the library keeps over a callback*: the block, or the stack (AT_FIND's frame over the maker; the record
  store's block for `rp_find` built on the stack, not in the task's RAM).  The callbacks are the task's own code in
  its main RAM or the part that called (the line's `ln_way`, the search's `reached`), run with the library's bank still
  in at $A000 (each task's bank registers are its own, so the maker's wait in the middle of AT_FIND is safe); hub_map
  is the one called per square, and changes no call register.

**The same squares, ways and sights.**  Every sentence of the suite's 298 dq0 transcripts is the old build's (a
transcript of each saved by a hook in sim/test.js, compared sentence by sentence), but the known timing noise:
dq0antidote's fight, dq0jobs's squeak, dq0restbusy's, dq0skykin's and dq0stopwords's denizens met at other moments,
dq0reachstop's and dq0tiring's accepted race.  dq0looks gives every reply the same.  A probe of the running games (a
hook on the emulator's steps, not committed) checked:

- every WK_ROW and WK_SEARCH of the world and the denizens in fourteen tests' games (dq0looks's eight games at seeds 3,
  5, 7, 12, 4 and 517, dq0answer, dq0travel, dq0doorfar, dq0route, dq0denportc, dq0tunnel, dq0glow, dq0scent,
  dq0line, dq0allythrow, dq0lookfar, dq0letsight, dq0incline) against a JavaScript copy of walk.inc's C run on the same
  input (the kinds and door bytes, the walker's rules, the grid, the floors, the queue, the box): 591 rows and 64
  searches, none differ;
- every line the senses drew in the same games, old build and new (its squares, mode and eyes as the key; its answer,
  ln_n, ln_first, ln_last): 271 lines both builds drew, every answer, ln_n and ln_last the same.  ln_first differs on
  15, each a line stopped at its first square, where the line never sets it (it's what the last line left, BSS's 0
  before, the block's own now) and nothing reads it (wshoot.c's cover asks it only past one square).

**Performance** (cycles a call, in the calling task, counted by the same probe; dq0looks, dq0answer, dq0denportc,
dq0glow on each build):

| | Old | New |
|---|---|---|
| A line (the senses', 336 and 340 calls) | 6,083 | 5,518 (the library's at () and its own portal_at, no C stack) |
| ln_portal (the look's doors, cast.s's rdoors: some 1,430 calls) | 107 | 264 (an XCALL each) |
| at ()'s long way (AT_FIND; a mean, the maker's chunks made in it) | (not counted here; chunk.c's median 1,326, "One record store"'s figure) | 2,021 (the denizens), 3,257 (the senses), 4,049 (the world) |
| CK_MADE, CK_AT (a chunk in the cache) | (C loops, inline) | 1,700-2,200 |
| WK_ROW (a row of 32 squares) | (wk_cell, 32 calls of cc65's C) | 3,013-3,149 |
| WK_SEARCH (a room's ways, the world's) | (cc65's C) | 47,524 |

ln_portal's XCALL is the one path made slower: the look's grid ray asks it of each door it meets, some 150 cycles
more a door (some 0.2M cycles over these four tests' 1,430 doors).  The looks and the start are within their
budgets, so it's kept in the library; if a look ever needs it, cast.s can keep its own copy of the rule (some 60
bytes) as a fast path.  The new AT_FIND's means take in the chunks the maker was asked for; the old long way wasn't
counted by this probe (cc65's C entry), so the two aren't compared here.

The start (dq0starttime): seed 1 233.3M cycles to 231.0M, seed 123456789 308.7M to 307.7M.  D17 (dq0answer,
dq0torchwords: the first letter within 720,000 cycles): pass; dq0torchwords's slowest first letter 611,176 cycles before,
637,110 after (its commands' first letters move by 20,000-30,000 either way between runs: the timing); dq0answer's
walks' first letters 639,670 / 517,175 before, 644,243 / 510,671 after.

**Room** (bytes free):

| | Before | After |
|---|---|---|
| dq0w main RAM | 2,059 | 2,905 |
| dq0d main RAM | 2,379 | 3,030 |
| dq0p main RAM | 3,176 | 4,920 |
| dq0w part 21 (the ways) | 1,402 | 2,198 |
| dq0w part 65 (the line of fire) | 227 | 763 |
| dq0w part 34 (wkept.c's lights) | 1,931 | 2,110 |
| dq0w parts 55, 76 (s_sky_list, looked_at) | 532, 256 | 640, 347 |
| dq0w part 3 (chunks_init) | 166 | 163 |
| dq0d part 18 (route) | 89 | 980 |
| dq0d parts 9, 13, 19 (made9/made_at, made, made10) | 46, 3,376, 1,837 | 356, 3,516, 2,132 |
| dq0lib (ROM, of 16,384) | 2,529 used | 4,705 used |

By stage: the lookup freed the main RAM of all three (dq0w +610, dq0d +623, dq0p +880: chunk.c's at_find was 597
bytes of cc65's code and its find 67, its block 35); the line dq0w +207, dq0p +864; the way-finder the parts (walk.inc's
statics are now the block's, the world's and the denizens' main RAM 29 and 28 bytes the less for its names).

**Left:** the maker's own lookup (dq0m/map.c's `peek`, make.c's choice of a slot: the maker owns the cache and
isn't short); the look's grid ray (dq0p/cast.s: thousands of lines a look, its own grid, the same rule by `ln_portal`,
an XCALL a door now); the glows' list of chunks near (dq0p/eyes.c: a range, not a lookup); the world's `across`
(a straight walk, its own rule, as "One way-finder" left it).

**Open questions:** none of behaviour.  The zero page's block is a fixed contract: a task that reads the chunks must
leave `$44-$70` alone (its cfg's ZP ends at `$43`: one more zero-page variable of a task's own would fail its link,
which is the check).

<!-- DECISIONS.md:5782-5822 at d35194df -->
## Opening a portcullis (the user's, 2026-11-14; as built, for review)

The user's report: "`open portcullis` doesn't actually raise it...it just says `You set to winding the winch.` but it
doesn't raise. `raise portcullis` does open it, though."

- **The cause.**  "Open" on a portcullis reaches the world as PA_OPEN, which wwin.c's s_portals takes as a raise: the
  winding (or, from the far side, the heaving) begun as timed work, its long task told ("You set to winding the
  winch.").  But s_door_work keeps what's to be done at the timed work's end in `act.work`, and PA_OPEN is 0, which
  `act.work` reads as "none": the long task ran out with nothing done at its end (work_done never called), the
  portcullis still down, nothing told.  "Raise" (PA_RAISE) never had the trouble.  A door's opening is never timed, so
  only a portcullis met it; a lowering is at once, so "close" did lower it, but with a door's first words.
- **The fix.**  dq0w.c's s_door_work keeps a timed PA_OPEN as PA_RAISE (`act.work = what ? what : PA_RAISE`: a
  portcullis's raising its only timed opening), so the end of the winding or the heaving is a raise's, the same test,
  change and words.  And the first words (sway.c's first_word): a portcullis opened or closed is told as it's raised or
  lowered ("You look at the portcullis.", "You feel the portcullis." in the dark), not a door's "You try the
  portcullis." and "You reach for the portcullis.".
- **The denizens.**  wport.c's den_portal takes a portcullis's PA_OPEN untimed (its `lng` PA_RAISE's alone), so a
  denizen's "open" would raise it at once, the winch's half minute and a heave's round not spent; but no denizen sends
  one: the minds send PA_RAISE (dq0d's mode 1), and an ally's errand ("ask it to open the door") picks a door, a
  portcullis only where no door is (choose.c's bars).  Left as it is (open question 1).
- **The test.**  dq0portcopen (seed 514's gatehouse, dq0portc's, a strong character): from the winch's side lowered,
  opened (wound up, up, seen up), closed (down, seen down), raised and gone through; from the far side (walked back to
  it, let down meanwhile by a poke of its state) heaved up by "open", neither closed nor lowered (no winch that side),
  let down again and heaved up by "raise", and gone back through; "open" told word for word as "raise" and "close" as
  "lower", from both sides.  Against the images before the fix it failed every one of those (the portcullis left down,
  "The portcullis is down already.").
- **What the player sees differently.**  "Open the portcullis" now
  ends its winding or heaving with "You raise the portcullis: it grinds up, and catches ..." (or "it rises, and
  catches ...") as "raise" does, the portcullis up; its first words "You look at the portcullis." (or "You feel the
  portcullis.") where they were "You try the portcullis."; "close the portcullis" walked up to first or ahead, "You look
  at the portcullis." where it was "You reach for the portcullis.".  The suite's transcripts (sent.js, against
  c1b9a9e1's): 7 differ, all known noise (dq0antidote, dq0jobs, dq0prompt, dq0reachstop, dq0restbusy, dq0robrest,
  dq0tiring); no test opened or closed a portcullis by those words before.
- **Room:** dq0w's part 12 1,101 -> 1,096 (its overlay 20 58 -> 53); dq0's overlay 17 2,547 -> 2,540; every main RAM
  unchanged (dq0 3,173, dq0w 2,059).  The start: seed 1 233.3M -> 233.9M, 123456789 308.7M -> 310.0M (timing: nothing
  of the start changed).  The suite: all 449 pass.
- **Open, for review.**  (1) A denizen's PA_OPEN at a portcullis (none sent today; an ally's errand could, where no
  door is in sight) raises it at once, untimed: make it the minds' PA_RAISE (begun, then posted again), or leave it?
  (2) "Close the door" at a portcullis (no door in sight, choose.c's bars) keeps a door's first words ("You reach for
  the door."), named as the player named it.

<!-- DECISIONS.md:5823-5886 at d35194df -->
## No look when the light's the same (the user's, 2026-11-14; as built, for review)

The user's report: "When you light a new torch from a burning one, it should not re-look since the light state has not
changed."

- **What it was.**  Every light lit or put out by the character (hthings.c's s_after: IO_LIGHT, IO_DOUSE, or its carried
  light changed) is the world's I_LIGHT, and the world answers every one with E_VIEW V_LIT or V_DARK (dq0w.c), as it
  does a light burning out (V_DARK, the lights' minute).  The UI looked about whole at each (main.c's E_VIEW, look (0)).
  So a second torch lit from the burning one, "light a new torch" (the old one put out and let fall: things.c's after-work
  queue), "light the unlit torch with the lit torch", one of two lit put out, one of two burning out ("One of your
  torches gutters and goes out."), a candle lit beside a torch: each told the whole place again, the same as before.
- **What the light is, in the game's rules.**  The character's light is the brightest it carries (things.c's
  s_light_now: the hub's pc_light, a reach in squares, and pc_lamp its name; the catalogue's torch 4, lantern 6, candle 2,
  candle stubs 1): two torches give no more light than one.  But what it sees by is more than what it carries: a room's
  own light (a sconce's torch lit: the senses' record's lit), lights lying on the floor (a torch put down burning), all of
  them the senses'.  A rule on pc_light alone (tried first) lost real changes: a sconce's torch lit or put out with the
  character's own torch burning (dq0kept, dq0glow, dq0lookway, dq0passes: "lit by smoky tallow lamps" untold) and two
  torches lying lit burning out with none carried (dq0busyloop: "It is dark." untold).
- **Built: decided where the look is, by what's seen.**  Each look keeps a sum of the senses' record of the place (look.c's
  sight_same, SS_LOOKED, as look () reads the record): from where it stands to its ways (its square, its dark, the room's
  own light, how much of it's seen, its doors, zones, openings, ways and ceiling); not the record's seq nor first (the UI
  marks it), and not the things lying about nor the denizens (a light lit or put out alone doesn't change them: the old
  torch let fall by "light a new torch" isn't a new light; the denizens moving have their own stir).  At E_VIEW V_LIT or
  V_DARK the UI reads the record again (the world's see (0) came before it) and sums it: the same, and the light of the
  same name (pc_lamp against the last look's, the look's words "by the light of your torch"), no look; else the whole look
  as before.  The light lit or put out keeps its own words ("You light the unlit torch.", "You light the new torch from the
  burning one.", "You put out the torch.", "One of your torches gutters and goes out.").  A greater light (a lantern lit
  beside the torch) sees more: looked at.  The last light put out: dark, "It is dark." as before.  Knocked out, the same
  test first: no look owed for a light changed but not what's seen.  The world unchanged (its main RAM), E_VIEW sent as
  ever (the tests that read it, dq0light's, unchanged).
- **The sum.**  A Fletcher sum (two bytes, mod 256) of the record's bytes in that range: a change it misses (two that
  cancel) would leave a look out; a change in a slot past the record's counts (a stale door) only gives the look back.
- **Tests.**  dq0relight (senses.js, areas.js's senses): at seed 3 a second torch held and lit from the burning one (its
  words, no look); one of two put out (no look); the last put out ("It is dark."); lit in the dark (the place by its
  light); "light a new torch" and "light the unlit torch with the lit torch" (no look); one of two burning out (no look);
  a lantern lit beside the torch ("You are at the near end of a long, narrow cell, by the light of your lantern."); two
  torches lit, put down and burning out as it rests ("It is dark.").  Before the change its no-look checks all fail (the
  probe's transcript).  No other test's check changed.
- **What the player sees differently** (the suite's transcripts, by sentence, sent.js, against c1b9a9e1's: 17 differ).
  The known noise alone: dq0antidote (its real-time fight), dq0jobs, dq0lookfar, dq0prompt, dq0reachstop, dq0skykin,
  dq0stopwords, dq0sway, dq0tiring.  The rest only lose a look (no sentence added), each where the light lit or put out
  left the place seen as it was: a second torch lit from the burning one (dq0twoalike, dq0packsteps' "light the
  torch" with one burning, dq0letcmd's "light j with a"); "light a new torch" (dq0housefaults twice, dq0newlight,
  dq0packsteps twice: "You light the new torch from the burning one. You put out the old torch. You let the old torch
  fall."); one of two burning out (dq0packsteps: "One of your torches gutters and goes out."); a torch lit with the
  lantern burning, candle stubs lit and burnt out with the torch burning (dq0light: the brighter light the same); a
  torch lit in hand with another lying lit at its feet (dq0busyloop, dq0twoalike: the place seen by it already); a
  sconce's torch gone out in a room lit by its own lamps (dq0passes: "The torch gutters and goes out.", the room still
  "lit by smoky tallow lamps"); and a torch put out and lit again inside one command (dq0packsteps' "dip the torch" in
  the dark: its reach's own steps, the dark never looked at, the place relit as it was last told).  "It is dark." as
  often as before in every test; every look of a sconce lit or put out, a lantern lit, a torch lit in the dark, kept.
- **Room:** dq0's main RAM 3,173 -> 3,143 (the E_VIEW test and look ()'s call: the sum itself is part 8's, look.c's
  s_sight_same); part 8 2,183 -> 2,027, its bank's overlay 3 2,008 -> 1,852.  No other task, overlay or part changed
  (dq0w, dq0p, dq0d as they were).  The start: seed 1 233.3M -> 233.1M, 123456789 308.7M -> 308.7M.  The suite: all
  449 pass (dq0relight new).
- **Open, for review.**  (1) A light of another kind carried but no more seen (a lantern lit while a torch burns, the
  place already whole): looked at again, its name changed ("by the light of your lantern"); or its own sentence enough?
  (2) A candle lit beside a torch: no look (the torch the brighter, nothing more seen); the candle's light not worded.
  (3) "hold another torch" with both hands full lays the lit torch on the ground and tells no look (dq0busyloop), as
  before: the place lit by the torch at its feet then; unchanged here.  (4) A light put out and lit again by a reach's
  own steps (dq0packsteps' dip in the dark): no look now, the dark never told; or a look whenever a light is lit after
  none was carried?  (5) A sconce's torch gone out in a room lit by its lamps: no look, the sconce's torch shown unlit
  at the next look.

<!-- DECISIONS.md:5887-5956 at d35194df -->
## Looking through a door's little window (the user's, 2026-11-14; as built, for review)

The user: "I still can't look through a little window in a door."  Doors have been made with a little window, a
grille across it (PF_GRILLE: a door in six), or a spy-hole, its cover on the bar's side (PF_PEEP: a door in six of the
rest), since 2026-10-15 (dq0m/fixtures.c; DUNGEON.md's a portal's make), and examining one says so ("A little window is
cut in it, a grille across it."); nothing was built to look through either.  No record asked for it before (DECISIONS.md,
HISTORY.md, COMMANDS-CRITIQUE.md, DUNGEON.md, the log: the features made and described, no more): the "still" is the
examine's words promising what the look didn't do.  The line's portal rule ("One line of sight" above, now dq0lib's
line.s) never knew of them: a closed door stops the eye, whatever is cut in it.

- **What it was** (dq0grille's scene: seed 1, the cell's south door closed beside the character, given a grille, then a
  spy-hole): "look through the little window", "look through the window": "You haven't seen a window here.";
  "peer through the grille", "look through the spy-hole": a look about, the word passed over; "look through the
  door": "You peer ahead.  You can't see past the door ahead: it's closed."; "open the spy-hole": "You look for the
  spy-hole.  You open the door, revealing a passage, 5 feet of it." (the door opened); "close the spy-hole" closed the
  door; from the passage at a slant, "There's no door that way."
- **Built: asked, the eye at it.**  The UI (way.c's `peep`, part 17) decides a look one way is through a door: the door
  named by its little window or spy-hole, or "look through the door" at one that has either, closed.  It's walked up to
  first (the feature path's FG_NEAR, where a look one way never walks) and faced, and the world asked with I_LOOK's w2
  (msgs.def's `peek`): the world's s_look_way (part 22) asks the senses LK_PEEK in place of LK_WAY.  The senses
  (dq0p/view.c) then, if the square ahead is a closed door with a little window, or a spy-hole whose cover is on the
  character's side (the bar's, as the world's store.c reckons a side), clear that square's G_OPAQUE in the look's grid
  once it's made (a look one way's grid isn't kept), so the look's own rays go through it as through an open door;
  v_way's bits say what (view.h's VP_ASK, VP_GRILLE, VP_PEEP, VP_COVER).  The line is untouched: a look about, a
  denizen's eyes, a missile, the air, a sound, the lights far off are as they were.  So is a look that way not asked
  through it: "look south", "look at the door" beside such a door say "You can't see past the door ahead: it's
  closed." as before (dq0answer's and dq0features's looks ahead, beside doors the maker gave little windows, are
  unchanged).
- **What's told** (dq0p/way.c; the UI's way.c, overlay 13): that door alone, none of the place's other openings:
  "Through the little window in the door you see a passage, 5 feet of it.", "... into the cell.", "... only darkness."
  (a room, its kind, lit by its own light; a passage, how far; along the way straight out of the door's far side, the
  one the eye sees past at a slant); then the things and the denizens beyond it, in the glance's words, those past the
  door within a quarter either side of the way the eye looks (a little window's), or within an eighth (a spy-hole's: no
  further to the side than half the way on).  The lights far off aren't told.  The squares the rays reach are seen
  (marked, remembered) as they would be through a doorway from there; only the telling is narrowed.  A spy-hole whose
  cover is on the far side: "The spy-hole's cover is on the far side: you can't see through it."
- **The words** (commands.words): grille, grill, spy-hole, spyhole, peephole, peep-hole, a door's parts (OB_DOOR
  PART); "the little window", "the small window" (the window, its word "little" or "small"); "the window" with no window
  in sight but a door with a little window; "peek" a look ("peer" was already).  A name picks the nearest door it sees
  that has one (pick_x, pick_y, for the feature path's pick); none: "You haven't seen a little window in a door here.",
  "You haven't seen a spy-hole here."
- **Opened, closed** (hplaces.c's do_door): "open the spy-hole" (or the little window, the grille) is the look through
  it, the cover slid back for it and let fall after (no state kept); "close the spy-hole": "The spy-hole's cover is
  closed."; "close the little window", "close the grille": "The little window has no shutter: only its grille."
- **Test dq0grille** (senses.js): the scene above, from the cell (the door closed, a grille poked into its record):
  examined; looked through by "the little window", "peer through the grille", "the door", "the window"; "look south"
  as ever; "close the little window".  Then from the passage (the character gone on to the guardroom, the door poked
  shut and not known behind it, so the walk back stops at it, at a slant): "look through the grille" (turned to it,
  "into the cell"), the little window; a spy-hole poked in, its cover on the passage's side: examined, looked through
  ("into the cell", the rat there), opened, closed; its cover then the cell's side: "The spy-hole's cover is on the far
  side ...".  Each answer's words checked in order, and none telling more than the door looked through.
- **What a player sees change:** only the commands above.  Every other test's transcript the same, sentence by
  sentence, before and after (but the timing noise the record allows).
- **Room:** the UI's main RAM 3,173 (unchanged); its overlays and parts repacked by the build (part 17 2,221 -> 1,258
  free, part 2 336 -> 45, overlay 13 5 -> 3,978).  The senses' main RAM 4,920 -> 4,891; s_things_lamps moved by the
  packer from part 1 (4 free) to part 3.  The world's main RAM 2,905 (unchanged); part 22 2,811 -> 2,793.  dq0c's
  words 6,092 -> 6,111 of 8,192, its RAM 14,101 -> 14,176 of 14,592.

**Open questions (the user's to decide):**

- *Denizens seeing the character through it.*  Not built.  A denizen's eyes go by the line (ln_portal), which a closed
  door stops: one beyond the grille the character watches doesn't see it watching.  Building it means the line's
  portal rule (dq0lib's line.s) letting sight through a closed door's grille, narrowly (and then the character's look
  about too, from anywhere in the room?), and a spy-hole only with its cover open, a state the door doesn't keep.
- *Its light through it.*  The character's torch lights what the rays reach through the grille as through a doorway,
  and through a spy-hole too, which is generous.  Seeing only by the far side's own light would mean the cast's lamp
  rule knowing the peeked square (dq0p/cast.s).
- *Sound and smell* through a grille: a closed door's, as before.
- *A spy-hole's cover left open* (seen from the far side as a lit hole, an eye at it): not built.

<!-- DECISIONS.md:5957-6024 at d35194df -->
## The hold-back's holes (2026-11-14; as built, for review)

dq0hold (the user's "hold back", 2026-11-04: the world's long jobs and the denizens' work wait from a line's first key,
three seconds at most while the keys go on) failed at 1bbcfcc4 ("held2: run 176326 cycles after it was due, a line
being typed: not held back") and at dbdb5554 ("decide: 1 run(s) in the first 8000000 cycles of the line typed again");
neither change touched the hold.  Each failure was a hole the timing had let through or not; the test's own probes
(the world's kfree, kt0, burn_ask and dst, the minds' gw and gks, each pass) found four.

- **The minute's passes went free for everything (held2's).**  After KEY_HOLD ticks a pass of the world's main loop
  goes free (kfree), and it stayed free while burn_ask was set, so a minute begun in it finished its own passes (the
  sconces', the time it believes).  But every row rode those passes: at 1bbcfcc4 the lights' minute (held1) ran in the
  free pass, set TS_ASK for its next pass, and the traces' journal made due 400,000 cycles later ran in that pass with
  the keys on the line (the passes went on some 4M cycles).  **Built:** the free pass's later passes are the minute's
  alone (dq0w.c's kmore; kburn while the lights' row is walked): keys_hold frees the lights' row in them, holds the rest,
  and their own hold begins as they're first asked (kt0 begun again as the free pass ends), KEY_HOLD ticks of their own.
  ui_first's command rule the same (a cut free pass, kfree 2, frees only the lights' row in them).
- **The free pass given mid-pass (found by the variants).**  keys_hold gave the free pass at whichever ask came first
  once KEY_HOLD ticks had gone, often a kit's look inside the denizens' intents (outdraw's ui_long), after the early
  rows; the main loop's top then ended it, so the journal's row (an early row) was never walked free and waited a whole
  hold more: 15.4M and 16.4M cycles with held2 made due 2M and 1M cycles on (past LIMIT, three seconds).  **Built:** the
  free pass given at the top of a pass only (main's, kt0's KEY_HOLD gone): every row walked in it.  keys_hold only
  starts the clock and holds.
- **The minds' hold for keys by the game's clock (decide's).**  dq0d held for a line being typed GIVE_WAY tenths of
  hub_now, the game's clock: at the normal pace a round of ten game seconds takes three real ones, so its "three
  seconds" ran out in about 0.9 s (3.35M cycles in the variants' traces), and a mind decided some 3.4M cycles into a
  line typed again (dbdb5554's; at 1bbcfcc4 the minds happened to be waiting on a senses' ask, held, and saw the line
  late).  The world and the senses held KEY_HOLD real ticks.  **Built:** a line's keys held KEY_HOLD (600) real ticks
  (hy_ticks) in the minds too; a command's give-way (ui_cmd) still GIVE_WAY tenths of the game's clock, as before.
- **A line typed again not seen between the minds' actors, nor by the intents' hold.**  dq0d looked for a new line
  (gsq, gcm, gks: its own hold) only at the top of its main loop; tend's checks between actors and before its
  enlisting read gw alone, so a line let go and typed again while they decided found gw 2 (the last line's hold run
  out) and the next actor decided.  den_held (the world's, the denizens' intents) had no line of its own: a line let
  go and typed again before a world's pass saw no keys kept the last's run-out hold.  **Built:** dq0d's held (), the
  line looked at and the hold reckoned, asked by main and by tend each time (the four places); den_held keeps the
  line's ui_keys (dks) as the minds' gks and the senses' hks do: a line typed again, held again.

**Not holes.**  Nothing runs the journal outside the table with keys on the line: tr_flush (0)'s other callers are the
character's own (a trace read, I_TRACE, LW_TRACE's end) and the save's.  The UI's ui_keys (main.c) is set at every key
and kept while keys are on the line: steady in every trace, from the first key the UI has to the line let go.

**Tests.**  dq0hold passes, unchanged.  Its timing varied (a scratch copy of the test, not committed: held1's poke
delay, held2's, the typing's pace in 2M-cycle steps, a wait before the first key), the fixed build: 100000/400000 (the
test's own), 50000/100000, 300000/1000000, 100000/2000000, 100000/4000000, 100000/400000 at two and three steps a key,
80000/3000000 at two with a 4M wait: all pass, held1 10.8M-13.3M, held2 10.75M-11.4M, no decide, sense or intents run
in a line's first 8M cycles.  Five more (a 2M, 6M or 8M wait before the first key) fail only as the test's poke of
burn_due landed while a minute's burns_now was under way (it sets burn_due again as it ends: the job never due, "run
null"), the test's own race, not the hold's.  Before the last two fixes 300000/1000000 failed (held2 16.4M, too long)
and the pace of two failed ("decide: 2 run(s) in the first 8000000 cycles of the line typed again").

**What the player sees differently** (the suite's transcripts, by sentence, sent.js, against 1bbcfcc4's, 301
  each): 7 differ, the known noise alone: denizens met at other moments (dq0answer, dq0jobs, dq0lookfar, dq0mine,
  dq0prompt, dq0skykin, dq0stopwords: their stirs, a rat or an adder seen or gone a moment apart, a sound one more or
  less).  No sentence of the game's own changed; only when the denizens move as a line is typed.

**Room:** dq0w's main RAM 2,905 -> 2,818 (the free pass's and the minute's flags, the grant moved to main); dq0d's
3,030 -> 3,044 (held () one function in place of main's and tend's copies, the ticks); dq0 3,143, dq0p 4,891
unchanged.  No zero page.  The start: seed 1 232.1M -> 232.5M, 123456789 308.7M -> 308.1M.  D17 (dq0torchwords' 23
commands' first letters): the slowest 623,224 -> 612,782 cycles, the mean 416,982 -> 421,874.  The suite: all 451 pass.

**Open, for review.**
- A job come due during a free pass's own pass, before its row is walked, still rides it (one pass: the rows are
  walked in order); a free pass from its top bounds that to one pass.
- The minds' command give-way (GIVE_WAY, 30 tenths of the game's clock: some 0.9 s real at the normal pace, 0.6 s at
  the menacing) is the game's clock still: three game seconds, not real ones.  Left as it was (the transcripts' timing);
  a real-time give-way like the senses' (GIVE_WAY ticks) is the user's to choose.
- dq0hold's own poke of burn_due is lost if a minute is under way as it lands (the variants' wait before the first
  key): the test passes at its timing; a sturdier probe would poke burn_ask.
