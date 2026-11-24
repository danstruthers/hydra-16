# DQ0: the decisions archive, 2026-11-10

The agents' accounts of what they built, moved out of DECISIONS.md on 2026-11-24 (branch docs2) with their text
unchanged, as AGENTS.md's Records rule asks: DECISIONS.md holds the user's words only. Each section left a one-line
pointer there; where a section held the user's words too, those stay in DECISIONS.md, quoted, and the whole section
is here. Each section is marked with its lines in DECISIONS.md as at d35194df.

<!-- DECISIONS.md:2790-2859 at d35194df -->
## Prayer, fasting, swimming and following (2026-11-10, as built, for review: the verbs plan's B5 and B6)

Five of the verbs that said "not yet" now do something in the world: pray and pray for (one handler, `H_PRAY`), fast
(`H_FAST`), swim (`H_SWIM`) and follow (`H_FOLLOW`).  Their work is a part of its own, part 27 (hself.c's and hmove.c's
`SW27CODE` blocks), because part 2 has no room.  Part 2 keeps the stubs, each now a call.  What's built:

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
- **Swim** (the plan's default, for review):
  - With a way or a thing ("swim north", "swim to the far bank") it is a move, `H_GO`'s, and the world's water rules
    do the rest (wading, swimming out of its depth, `P_SWIM`).  The UI knows only the water where the character stands
    (the hub's `pc_wet`), not the square beside it.  So "swim north" from dry ground walks north and swims if the water
    there is deep, rather than asking whether deep water is beside it.
  - "swim" alone, or "tread water", in water five feet deep or more treads water ("You tread water."), a minute.  The
    world (`I_WAIT`'s new a bit, `WA_TREAD`) tests the minute's swim at once: dq0w.c's part 73, `swim_still`, makes
    wade_t's test due.  A critical failure goes under ("You go under!"), and under, a tread comes up ("You come up,
    gasping.").  This is HOUSE.md's "not yet" of a swim's test as the character treads water, built for this verb
    only; an ordinary "wait" in deep water still isn't tested.
  - "dive" or "swim down" goes under on purpose (`WA_DIVE`: the minute's test first, then under, the breath held a
    minute, `WB_UNDER`, the work three times a swim's).  It is a timed work of a minute (reach 3), and at its end the
    character strikes out for the surface: a tread of moments, whose test brings it up or keeps it under.  Under past
    its breath, it drowns, as a sinking does.  "swim up" when under is the same tread.
  - With shallower water or none: "There's no water deep enough."
  - The parser: "dive" is swim's word (commands.verbs' [words]), and "tread water" a two-word swim; the grammar has
    both.  The handler tells a dive by the line's "dive" or the way down.
- **Follow** (the plan's default, for review):
  - "follow the X" trails a denizen it sees (the resolver's square, matched to the sight's record: its actor's `den`
    kept).  It walks after it with the world's close-in, walked alone (`I_CLOSE` w1 1, the world re-aiming it as the
    denizen moves), and its command is carried out again as each walk ends (main.c's reach 1).
  - Caught up, it waits a round beside the denizen, and goes on if the denizen goes.  If the denizen stays, the follow
    ends: "You stop beside the giant rat."
  - A walk that got nowhere (no way, or no nearer) ends it: "You can't keep up with the giant rat."
  - Out of sight but within six squares (gone behind it), the character turns to look for it once.  Not found then,
    or further off, or on another stratum, it's lost: "You've lost the giant rat. ("track" may find its trail.)"
  - A thing that isn't a denizen: "The sack isn't going anywhere."
  - "follow me" is the ally's order still.  The parser now gives every "follow X" `VB_FOLLOW_OBJECT` (commands.verbs'
    `thing=`), and the handler sends "me" (and nothing named) to the sway, as `VB_ALLY_COME`.  "follow" alone is
    `VB_ALLY_COME` in the parser as before.  The grammar already had "follow X".
  - *For review:* the world's "You can't get near enough to the giant rat." is told on each leg that falls short of a
    denizen that keeps its distance.  "tail" stays the grammar's alone: a [words] alias would have been a parser
    record more.
- **The parser's records**: 253 of 255, as before.  "dive" shares swim's record, and the two-word phrases ("tread
  water", "break my fast") are verbs of several words (119 of 255).  The corpus is unchanged at 645 of 660.
- **Room**:
  - The banks were full: part 27's 3.1K fitted in none of the 24 overlay banks.  So there is a 25th (dq0.cfg's OV25,
    bank.s's OVMAX, dq0.h's ovbank), and part 27 wishes for it.  The packer put part 3 there too.
  - Part 2 has 102 bytes left (110 before).
  - dq0's main RAM has 2,202 bytes free (2,272 before: the stubs, the new part's record, the vow and the follow's
    state).  The world's has 2,477 (2,483 before: `swim_still`'s stub).
- **Tests**:
  - dq0pray: Peoni's hour twice, the clock an hour on each; the vow, eating refused, "stop", "break my fast",
    "stop fasting" with none; no deity's prayer.
  - dq0tread: "swim" out of water and on the landing refused; a tread; a dive and its coming up, the world's beats;
    "tread water"; "swim south" out.
  - dq0follow: "follow me" to a befriended rat, the ally's order (SW_ORDER OR_FOLLOW, SR_OK); "follow the sack"
    refused; a shy rat followed, walked after, and lost.

<!-- DECISIONS.md:2860-2934 at d35194df -->
## Worn places (2026-11-10, as built, for review: the user's, "Wearing should extend to rings, necklaces, earrings, and other jewelry, as well as scarves, and should keep track of where worn")

The user's words: "Wearing should extend to rings, necklaces, earrings, and other jewelry, as well as scarves, and should
keep track of where worn (as should all worn items that can be worn in more than one place)."  The user's decision:
rings by the hand only (left or right), up to four a hand, not by the finger.  The design's part (A) (the worn places;
the bandages and the materials are other work).  What's built:

- **The place is the item's own, its sn.**  A worn thing's sn was used by nothing (a held one's is its hand, IH_*; one
  on a square's, its stratum).  A worn thing of a kind worn in more than one place keeps its place there (items.h's
  WP_*): a HarnMaster location (harn.h's HARN_SKULL ... HARN_FOOT, the low four bits), WP_RIGHT its right side (harn.h's
  HARN_RIGHT, 0x80), WP_PAIR (0x40) a pair (earrings), and WP_AT (0x10) its place given.  WP_AT is the build's, not the
  design's: an outfit's worn thing is made with sn 0 (outmake.c's: its hand, none), which is HARN_SKULL, "on your head";
  without the mark a scarf drawn into an outfit would be on its head.  An sn without it (0x20 clear) is its kind's own
  place.  outmake.c was left as it was: its part (58) has 4 bytes free.  Nothing in the save changes size.
- **The table is kinds.c's** (its wplaces and wroom), each multi-place kind its place unsaid, the locations it may be
  worn at (a 16-bit mask), and each location's room (of everything worn there with a place): a ring the hand (four a
  hand); a brooch the breast (two), a shoulder (one), the neck; a necklace and a pendant the neck (three); a talisman the
  neck, a wrist (two); a scarf the neck, the head (one), the face (one), the waist (one); earrings the ears (a pair: the
  head's location with WP_PAIR, so a scarf on the head and earrings don't take each other's room).  The table is linked
  into part 107's image (its rodata's segment), not the catalogue's bank: that bank's code was 29 bytes from its end.
  Everything else worn (clothes, armour, a cloak, the mask, a pack, a belt, a bandolier, a slung shield or bow) keeps
  sn WP_NONE and the one-of-a-kind rule as it was.
- **The rule's in a swapped part, reached by its stub.**  things.c's don (the catalogue's bank: 6,371 of 6,400 bytes)
  lost its one-of-a-kind loop to part 107's worn_at (wworn.c: worn already, SA_WORN; one of a kind for the rest; the
  place said checked and its room counted; unsaid, the kind's own, a ring's the off hand's and then the other's), and
  sets the item's sn from its answer: the bank's code 6,345 bytes now, the four new kinds' throws included.  Its refusals
  are the part's own words: "There's no room left on your left hand for the ring.", "There's no room left on your hands
  for the ring." (unsaid, both full), "You can't wear the ring about your neck.".
- **Defaults when no place is said:** a ring the off hand (the referee's limbs: the left, the right if left-handed),
  then the main hand when the off hand has four; a brooch the breast; a necklace, a pendant, a talisman and a scarf the
  neck; earrings the ears.
- **Phrasing.**  The grammar's `<worn-phrase>` (commands.ebnf: on, over, around, about, at, in; "my"; left or right; the
  part, an open noun, or "head", "hand", "hands", "face", which are the grammar's own words) after `<equip>` and `<put>`.
  The meaning the parser makes of it is uneven ("over" and "head" are little words; "wear the scarf over my face" makes
  the face the thing), so the UI reads the place from the line as tend.c reads "left" and "right": worn.c's worn_said
  (overlay 14, sway.c's place_said's first): a wear, a don, a put on or a put whose line has " my " then (left, right)
  a part of the body (head, ears, ear, face, neck, throat, shoulder, breast, chest, waist, hands, hand, finger, wrist) is
  a don there, its thing's word the one before the preposition if the parser took the part for it; the other the place
  in the world's words ("@", the location as a letter, the side l, r or -).  "put the ring on my pack" is still a put.
  main.c's hand_said no longer makes "on my left hand" a hold (a line with prep on).  "don" and "put on" take a place as
  "wear" does (their aux IO_DON; wear's DN_WORN refuses a weapon as before).
- **Display.**  The inventory: ", on your left hand", ", about your neck", ", in your ears", ", over your face", ", at
  your breast", ", worn" for the rest; alike things worn in one place together ("(j) four rings, on your left hand"),
  in two places apart.  A look: "Your scarf is worn on your head."  A denizen examined: "It wears earrings in its ears, a
  ring on its right hand, a belt ...".
- **Which, by place.**  "take off the ring" with rings worn in two places or more: the world's which (IR_WHICH, as a
  take's), each place's first by its id, its state WS_AT and the place's words after it; caps.c words it, "Which one? (E)
  a ring, on your left hand, (F) a ring, on your right hand", and a capital, a number, or "left"/"right" answers.  For
  it, the UI's first words no longer fix a take off's thing by its own choice (sway.c's bind: not for IO_DOFF), and both
  choosers (the world's pick_, the UI's ix_pick) put a worn thing last for a wear and an unworn one last for a take off:
  "wear the ring" with one worn takes another, not "You wear the ring already.".
- **Denizens.**  A denizen's thing's sn is the stratum it's filed in, so its place isn't kept: it's drawn from the
  thing's own number (its id) among its kind's places, and its side by the id's second bit; the same each time.  The
  outfits' jewelry worn (jewel@worn, the talisman, the ring of rank) shows at its place.
- **kit.dat** keeps the place in its ofit's in (meaningless for a worn thing): WP_KIT (0x80) | the location | WP_PAIR,
  0x20 its right side; never a container's entry (NOFIT, 64, at most), so outmake, wfetch's fit and dq0acct read it
  as none.  Part 63 lays it on the thing as the kit's carried on.
- **New kinds** (items.h's, after KI_COPY; kinds.c's): scarf (6d, worn; MT_CLOTH till the materials' split: wool),
  earrings (40d, silver, a pair, worn), linen sheet (24d; MT_CLOTH: linen), towel (4d; MT_CLOTH: linen).  outfits.txt:
  a bandit's bed a linen sheet (40 in 100), a bandit lord's and a priest's always; a scarf for a bandit lord (30); earrings
  for the people of standing (25), lords (20) and a bandit lord (25), a scarf for the people of standing (30).  A
  bandit's scarf was left out: its section's 255 bytes (outdraw.c's) were 5 over.  town.txt's market sells the scarf and
  the towel.
- **Left for review:** veils and kerchiefs (optional in the design) aren't added; a brooch at the shoulder and a talisman
  at the wrist are the house's; the place isn't changed by wearing a thing worn already somewhere else ("You wear the
  ring already."): take it off first.  Taking off by place ("take off the ring on my left hand") isn't read: the which
  asks instead.
- **Room:** dq0's main RAM 2,262 bytes free (2,272 before), dq0w's 2,408 (2,483); part 107 2,167 bytes of 3,648; overlay
  14 153 free.  The build's packer moved parts among the banks to make room.
- **Tests:** dq0worn (four rings on the left hand by wear, put on and don, a fifth refused; a ring at the neck refused; a
  ring unsaid on the right hand; a scarf on the head, a necklace around the neck, earrings by default; the inventory and
  a look by place; "take off the ring" asks which by place, "right" answers; the scarf over the face), dq0wornkit (a
  ring on each hand and a scarf over the face kept in the save and into the next dungeon), dq0wornden (a bandit's
  jewelry examined at its places).

<!-- DECISIONS.md:2935-3001 at d35194df -->
## Words said, a knock, the minds swayed (2026-11-10, as built, for review: the verbs plan's batches B2 and B3)

Six of the stubs (say, knock, wake, taunt, orate, pretend) now do a thing in the world, the user's rule.  Their words are
minds.c's (the UI's part 27, a part of its own in a new bank, overlay 25: the UI's banks had nowhere 3.8K bytes left,
part 2 only 110), the minds the denizens' (dq0d.c's part 24, `s_minded`, new: parts 2 and 16 full), the noise the world's.

- **I_NOISE, the world's** (part 37): the intent made the denizens' E_NOISE (its loudness, what it is, its square or
  the character's), two seconds' work.  The denizens' `hear` took the character's noises already: each actor in hearing
  (the senses' sound layer, a sleeper's hearing halved) turns to it and stands a while, and a sleeper that hears it is
  woken (its rouse).  New: words or a knock (E_NOISE's b `NZ_VOICE`, `NZ_KNOCK`, hub.h) draw a foe that hears them on
  to where they were made (drawn: it comes to look).  The sound task plays no sound for a voice (dq0s.c), a knock a blow's.
- **say, yell, whisper** (and a line in quotes: y"..." yelled, w"..." whispered, one ending in ! yelled): the words said
  back ('You say, "hello there"'), the noise 3, 6 or 1.  The words are the line's (cl): the parser keeps none.  A
  denizen awake, near (within the loudness in squares) and facing the character, is asked as a talk (SW_TALK, the one
  meant by den_meant), its words said only if it's taken so (a beast, a tongue not shared, as far as it goes: nothing
  said).  "say" and "yell" with no words the grammar doesn't take, as before.
- **knock**: on a door (a portcullis, a window), walked up to and faced first (feature's), "You knock on the door.",
  noise 3, heard beyond it by the sound layer, so a foe comes to look.  Along a wall ("knock on the wall", and the
  grammar's new "knock on the north wall": commands.ebnf, commandcompile.js), a short search by ear: the world's wall
  search, half a minute, its b `SF_KNOCK`, the referee asked P_NOTICE_SOUND for each hidden door and door trap in reach,
  not P_SEARCH (the world's `srch_p`, set by `work`: wtrap.c's part 8 had 10 bytes); noise 2.  On a thing to hand a
  rap (noise 2); else "You can't knock on ...".  For review: a door hidden by magic is tested by ear as any other (the
  referee bars only P_SEARCH from those; no room in the world's part 8 to bar it); a wall's side is the heading rounded
  to the four ways (the search's own rule).
- **wake**: "wake" alone asleep is a stop (task.c's `free_cmd` takes it at once, as a stop); awake, "You're awake."
  "wake the bandit": the one of that name beside the character; seen further off, walked over to first (the reach's,
  the command again as it's there); then SW_WAKE: not asleep, "The bandit isn't asleep."; asleep, roused up and
  watching, turned to the character, drawn on to it, and its attitude decides what follows: a foe beside it strikes
  ("You shake the bandit awake. It comes to with a snarl, and turns on you!"), wary and worse "eyes you warily", the rest
  "blinks up at you".  No test: a shake wakes anyone.  (Not tested: the walk over; in the test's scene the knock had
  woken the far sleeper first.)
- **taunt**: SW_TAUNT, a test of Rhetoric (P_PERSUADE; a beast's Animalcraft: one can goad an animal), no tongue wanted
  (jeers carry, as threats do), within earshot (12 squares, a beast 8), at steps the easier the less it likes the
  character (indifferent 0, unfriendly +2, friendly -1, an ally -2), its kind's sway, a low mind +1, a cunning one -1;
  hostile already, as far as that goes.  A success makes it hostile, an ally no more, drawn on to the character from its
  post (a foe drawn on is before its day's doing in its mind); a failure leaves it as it was ("It ignores your jeers.";
  a critical failure "It only laughs at you.").  For review: the book's contest is opposed; here the character's test at
  steps, as persuasion is.
- **orate** ("orate", "address ..."): noise 4, then one Oratory test (P_ORATE) in the tongue of the nearest minded one
  awake in earshot (common to a leader, as words are); a tongue the character doesn't speak, the sway's refusal.  The
  grade is applied to every minded one awake within 12 squares whose tongue it is, as a persuasion's: a critical success
  two steps, a success one, a critical failure a step back, never past friendly (a speech wins no ally): "You hold forth
  to those about you, in common. Your words carry: 6 of them seem the friendlier for it."; half a minute.  None to hear,
  "There's no one here to hear you."  (The hub's sway: SW_PERSUADE with order `SWO_ALL`, the reply's ix those swayed, iy
  those it reached, word2 the tongue.)
- **pretend dead or asleep** (the plan's default, for review): a pretence naming death, sleep or senselessness lies
  prone (I_RISE, unless prone already), then an Acting test (P_ACT) for each foe (unfriendly or hostile) awake that
  senses the character, the cunning -2, a low mind +1, the mindless +2 (SW_DECEIVE with `SWO_ALL`); then a wait lying
  still, ten minutes.  The fooled take it for helpless (HOUSE.md's rule, `decide`'s: killers strike, robbers rob, the
  rest leave) while it lies prone on that square in its wait (dq0d's `pcfeign`); moved, up, the wait over or stopped,
  the pretence is over till it's feigned again.  Foes that come later, or weren't fooled, see through it.  "The bandit
  is taken in." / "It isn't taken in." / "No foe is watching you."  Any other pretence ("pretend ignorance") is a
  deception of the nearest (deceive's own words).  For review: the book would have Acting against each foe's Awareness;
  here the character's test at steps.
- **The shared files**: hub.h (`NZ_VOICE`, `NZ_KNOCK`, `SF_KNOCK`, `SWO_ALL`, I_NOISE's comment), msgs.def, cmds.h
  (`minds`), dq0.cfg (OV25, `#@SW27`), bank.s (OVMAX 25), dq0.h (`ovbank[25]`), parts.txt (27), dq0d/parts.txt (24);
  dq0d.c's `cap[]` comment (wake and taunt aren't influence's), `s_sway`'s SW_WAKE, SW_TAUNT and `SWO_ALL` sent to
  `minded`.  sway.c's `tried[]` stays to SW_BEFRIEND: its words are never asked for wake or taunt (minds.c words a reply
  SR_OK itself, the sway's words only its refusals).
- **Room**: dq0's main RAM 2,260 free (2,272 before), part 27 495 of 4,352 free, overlay 25 1,269 (the map's part 3 moved
  there by the packer); dq0d's main RAM 2,184 (2,457: `hear`'s drawing, `decide`'s fooled, the pretence's state), its
  part 24 2,026, part 2 18; dq0w's 2,482 (part 8 3, part 37 1,869); dq0harn's 2,169, unchanged.
- **Test dq0noise**: say taken as talk, a whisper heard at 1, a yell at 6 by the bandits about, a quoted line, a knock
  on the door heard at 3, a knock along the wall, "wake" awake, ignorance feigned (a deception), death feigned (the
  bandit taken in, robbing the "body"), a hostile sleeper shaken awake, a taunt, a speech; seed 7's hall, its doors made
  secret, the door found by knocking along the north wall, and a speech with no one to hear it.

<!-- DECISIONS.md:3002-3068 at d35194df -->
## Cloth, bandages and dressings (2026-11-10, as built, for review: the user's bandages, the worn-places-and-bandages design's parts M and B, its rip and wash)

The user: "A bandage's location should be known, and different material should have different suitability as bandage
material.  'rip' and 'tear up' verbs should be allowed to turn a garment, bed sheet, towel, etc. into bandages ...
differing effectiveness (for material type and cleanliness)."  Decided with the user: no cotton; dressings soil over
days and are changed; wash, not boil (boiling when there are fires).  HOUSE.md's bandages and dressings has the rules as
built; this is how, and what's the house's for review.

**What the code said before** (checked): heal.c's treat and bonus asked only `has ("bandage")`, the table's bonus
whole or none; the reply's "\nbandage" made tend.c ask the world's IO_USE, s_use_key using up the first lot keyed
"bandage"; harn_heal_roll's bonus always 0; join merged any two lots of a kind whatever their material or wear;
nothing kept a cloth's cleanliness; bandages one kind, MT_CLOTH.  s_tools sent each tool's quality as its digit.

- **Materials** (items.h): MT_LINEN, MT_WOOL, MT_SILK after MT_GOLD (the numbers before kept: the outfits' file, the
  stores' records); MT_CLOTH is hemp and sacking (its name "hemp": appraise.c's and things.c's).  Linen: shroud,
  bandages, surcoat; wool: cloak, blanket, robe; hemp: sack, rope, and what wasn't named (clothes, a gambeson, paper,
  a net, a letter) *(for review: clothes and the gambeson might be linen or wool)*.  The outfits' file may name them
  (outfits.js's MATS).  The worn places' scarf, earrings, linen sheet and towel are another branch's kinds: their
  materials and their yields (wcloth.c's table, commented) on the merge.
- **Cleanliness is the wear byte**, each band one value as the bandages have it (CL_WORN 64, CL_SOILED 192,
  CL_BLOODY 255); join's guard: bandages join only of one wear and one material.  Where a cloth's comes from, the
  house's: found lying in the dungeon (make_own: linen, wool, silk) soiled; a garment torn up worn at best; a shroud, a
  denizen's bedding, soiled.  *Not built*: don marking a garment worn (don is in the catalogue's bank, 29 bytes free,
  and the worn places' branch is in it), a denizen's outfit worn as it's made (outmake's part 58 full): the tear's
  rule stands in for both.
- **The grade to the referee**: s_tools' digit for a bandage lot is CL_GRADE (items.h: its suitability by its
  cleanliness), not its quality; the referee's bonus (dress.c's) is the table's times the best bandage's grade over 9;
  the reply's key carries the grade ("\nbandage9"), and s_use_key reads a digit after a key as the grade wanted (a lot
  of that grade alone; "water", "food" as before).  A bloody bandage's grade is 0: never used.
- **The dressing per wound** is the referee's (pc_dr, a byte a wound: its grade plus 1, DR_SOILED once the days have
  soiled it), kept in step wherever pc_by is (sheet.c's room, heal.c's by_drop), saved as a "dressed" line, each
  dressing its grade and its wound's location ("dressed :ag"), read back onto the first undressed wound there: the
  account (dq0acct) keeps the line as it is, and its days at rest heal wounds and move them down, so the location,
  not the place in the list, finds the wound.  As built the days between dungeons didn't soil it, and the sheet's
  words were "dressed, clean", "dressed", "dressed, poorly", "dressed, soiled", the cloth not named.  *The user's,
  2026-11-11: the sheet names the cloth and its cleanliness ("dressed in linen, soiled"), and the days between
  dungeons soil it as the dungeon's do* (below, "The user's review of five defaults").
- **Soiling** (heal.c): at each day's mark (after that day's rolls) a dressing on a serious or grievous wound three
  grades the worse, told once it's soiled.  The healing roll's bonus +5 (grade 7-9), 0 (4-6), -10 (0-3): a poor hemp
  dressing counts as a soiled one.  Creatures: the treatment's bonus alone, no dressing kept.
- **Changing** rides first aid: tend.c sends the part said with a * before it when the line has "change"
  (commands.verbs' "change the bandage on", "change my dressing" ... are tend's words: several words each, none a
  record of dq0c's); the referee's plan TR_CHANGE (RS_TREAT's c 5), five minutes, no treatment roll; its reply's key
  "bandage9+", the + the old dressing's: s_use_key makes a bandage of the lot it used from, one, bloody, beside it (at
  the feet if the lot was in hand).  A wound tended without a bandage can be dressed this way too ("You dress ...").
- **Rip** is one verb, VB_RIP_OBJECT (commands.actions' last line, actions.c's last row: H_HANDLE), aux IO_RIP; "tear",
  "tear up", "rip up", "cut up" its words.  **Wash** is the same verb, "wash = rip", its request the word's (hthings.c:
  the line has "wash", IO_WASH), as grapple's moves are: dq0c's word records are at 254 of 255 with rip's, and wash's
  own verb would be the last.  The world's IO_RIP and IO_WASH are part 107's (wcloth.c, s_textile): the yield table,
  a blade (the one said, else any carried: a knife's or a sword's family), a minute a bandage with one and two by
  hand, wool and a gambeson wanting one; the time a timed wait (the handled things' x), not a long task *(for review:
  a big sheet's sixteen minutes can't be stopped)*.  Wash at running water or a well beside, three minutes, worn.
- **For "use"** (B4's H_USE, another branch): "use the knife on the cloak" is `handle (IO_RIP, "cloak", "knife", 0)`
  (s_handle, the UI's part 2); "wash" `handle (IO_WASH, "bandages", "", 0)`; "use the bandage on my arm" is first aid:
  cmd.action VB_TEND, the part in cmd.obj.word ("arm"; the line's "left", "right" and "change" read by tend.c from cl),
  then `tend ()` (s_tend's stub, part 12; s_early's 1 for VB_TEND, its own patient found).
- **Room**: the referee's first aid (remedy, bonus, at_part, to_treat, treat) moved from heal.c (overlay 6) to dress.c
  (overlay 9) with a copy of the library's Treatment table, and sheet.c's o_spent to overlay 9 (overlay 1's room for
  the dressing's words and room's step); two new stubs (treat, dress_line).  Main RAM: the referee 2,135 free (2,169
  before), the world 2,448 (2,483), the UI 2,272 (unchanged); dq0c's records 254 of 255.
- **Tests**: dq0rip (a worn robe, a leather vest, a rope refused; a wool cloak without a blade refused; a linen
  surcoat torn by hand into 4, eight minutes; the cloak cut up with the dagger into 6; two worn lots), dq0dress (a
  dressing, the sheet's "dressed, clean"; another wound's soiled dressing from the file, "dressed, soiled", changed,
  a bloody bandage in the pack (the days' soiling dq0rules': the denizens wake a sleeper here); wash refused with
  no water, done at running water, worn), dq0rules' dressings (the bonus by linen, hemp, soiled linen, none; the
  sheet's words; a day's soiling, two; a change and its refusals; the five-day roll's +5 and -10).

<!-- DECISIONS.md:3069-3129 at d35194df -->
## The things' verbs: use, push, pull, touch, taste (2026-11-10, as built, for review: the verbs plan's B4)

Five rows that said "not yet" (B0's stubs in hthings.c) now do something in the world.  The user's rule: each verb a
meaningful action; push and pull include moving furniture.  What's built:

- **Use** (and "activate", its alias).  The theme's puzzle is still asked first (theme.c's o_puzzle: its rules for the
  thing, or for the thing on another; the way out).  What it has no rule for is no longer "Use what, and on what?" nor
  "Nothing comes of using ..." but the handler's, by the thing's kind (the things' IO_FIND): a bandage tends (the
  thing it's used on its patient or its place: "use the bandage on my arm" is "tend my arm"); food is eaten; a
  waterskin, a vial, an antidote, a remedy is drunk; a light is lit; a tinderbox lights what it's used on; a key or the
  lockpicks unlock what they're used on, none said the door (the one there is, or "Which door?"); a rope or a grapple
  is tied (to what it's used on, or what's nearest that holds); a whetstone sharpens what it's used on, none said the
  blade in hand ("Sharpen what?" with none); a thing held in a hand (a weapon, a tool) is wielded.  Anything else:
  "You can't see how to use the X.", or with another "Nothing comes of using the X on the Y."  Each is that verb's own
  command carried out (hthings.c's as_verb: the action changed, its row's handler called), so its words, its timed work
  and its walk over first are the verb's.  **Rip is the materials' batch's:** a TODO at the head of the routing marks
  where "use the knife on the cloak" goes (VB_RIP_OBJECT, IO_RIP), before the kinds.  (Wired as the batches merged,
  2026-11-10: a thing but the bandages used on a thing whose kind is cloth, MT_TEXTILE, is the textiles' IO_RIP, the
  cloth cut up with it, wcloth.c saying what won't cut; dq0rip's "use the dagger on the towel".)
- **Push, pull.**  A door pushed opens, pulled closes (a window's shutters alike); a portcullis pushed is raised,
  pulled is lowered (the winch loosed); a door's bar pulled is lifted out (the word "bar" first: the resolver makes it its door) (PA_LIFT), pushed is set in its brackets
  (PA_BAR); a rope pulled is untied, so from below a grapple is shaken loose (wrope2.c's, HOUSE.md's rule); there are
  no levers.  Anything else named is furniture, the world's new **IO_DRAG** (dq0w/wdrag.c): a thing lying about whose
  kind is 40 pints or more (DRAG_BULK) and isn't fixed (a chest, a table, a cask, a body) is shoved a square away from
  the character (on its own square, the way it faces), or dragged a square toward it, the character stepping back a
  square (I_STEP, its facing kept).  Out of reach, walked over to first.  Strength against its weight, what's in it
  too (P_FORCE, two steps the better, a step the worse each 10 pounds: an empty chest one step worse), five seconds,
  a noise of 3 (E_NOISE, heard whether or not it moves); failed, "You strain at the chest, but it won't budge."  It
  goes only where a walker could step (step8's), not into a doorway, not onto anyone, not onto another heavy thing;
  smaller things: "The pot isn't a thing to shove about: pick it up.", a fixed one "The well is fixed where it stands." (each answer names its thing, the sweeps' rule)
- **A shoved thing blocks a door.**  A door is a T_DOOR square with its portal; furniture is an item on a square.  A
  heavy thing on either square that leads through a door (the four ways from it: the walls' have none) braces it shut:
  opening it, by anyone (the character, a denizen: wdoor.c's PA_OPEN, a new PO_BLOCKED, "Something heavy stands
  against it: it won't open."), or a known door opened as a walk goes through (dq0w.c's door_on), is refused till it's
  moved.  Told as it's shoved there ("It stands against the door.").  The maker's own chests and tables beside a door
  brace it too (a chest stands where it stands, whoever put it there): *the user's, 2026-11-11, reviewed and kept.*
  **For review:** forcing a braced door isn't
  refused (its lock or bar bursts, it stays shut); a heavy thing can't be shoved into a doorway, so passage is
  blocked only by the door kept shut, not by a chest in an open doorway (walks don't treat things as obstacles).
- **Touch** ("feel" is its word too).  In the dark, a wall, the floor or nothing named: the felt words, a feel about
  (H_FEEL's: "It is dark. You feel ... Your hands find ...").  Else the thing examined (H_EXAMINE), which the hands do
  in the dark as the eyes do in the light (its make, its state, its lock).  For review: its first words are the look's.
- **Taste**: a sip, not a dose (the world's IO_DRAG with DG_TASTE: the thing to hand, what's in it, that one's wear;
  nothing used up; told "You taste the X. ...": a thing in a pack or a sack isn't, only a vessel's liquid).  An antidote or a remedy: a Lore test (P_LORE) names it and what it's for ("a theriac, an
  antidote against poisons and venoms"; "a physician's draught, a remedy against disease"), failed "Bitter, and
  strange: you can't tell what it's for."  Water (a waterskin's, or the water it stands in): fresh, or "Flat and
  stale: still water, not fresh. It may carry the flux." (the water's wear, still water's mark, feed.c's).  Food:
  "still good" (nothing goes off in this game).  An empty vessel: "There's nothing in it to taste."
- **The flux:** as built, a taste of still water didn't risk it (a sip too little; tasting told, drinking risked).
  *The user's, 2026-11-11: a sip can bring on the flux,* at a sip's small dose (below, "The user's review of five
  defaults").
- **Room.**  The UI's handlers are part 29's (27 as built, the merge's number) (hthings.c, part 2 full: its five stubs one call now); theme.c's overlay 5
  smaller.  fixt.c's overlay 4 had a byte free: alias.c's o_alias_said moved to overlay 12 for PO_BLOCKED's words.  The
  world's IO_DRAG and the brace are part 109's (107 as built, the merge's number; wdrag.c, 2,600 bytes), in a new overlay bank, the world's 29th in
  dq0w.ovl (the rope file's three now OV30-OV32: OVBANKS 32; ropes_in reads the last three as ever): no bank of either
  file had room.  Main RAM free: the UI's 2,237 (2,272 before: the part's stub, things_use's), the world's 2,465 (2,483: the stubs drag, braced), the others as they were; part 2 169 of 659 (its five stubs gone), part 27 2,050 bytes; the world's catalogue bank untouched (things.c's serve a case more, part 92).
- **Test dq0things**: use routed to each kind; a door pushed open and pulled shut; a chest shoved against a door, the
  door refused, the chest dragged away and the door opened; a portcullis pulled down; a bar pulled out; a grapple's
  rope pulled from below; touch in the dark; taste an antidote, a remedy, an empty vial, and still water.
  dq0notyet's "activate the torch" now lights it (the use's).

<!-- DECISIONS.md:3130-3141 at d35194df -->
## Charge, a throw a way, engage, a split attack, and again (2026-11-10, as built, for review: the verbs plan's B1 and B7)

Five of the stubs the verbs plan left saying "not yet" (B0's) now do something in the world.  What's built:

- **Charge** ("charge", "charge!", "fast charge"; hdens.c's `H_CHARGE`): the nearest hostile denizen the character sees ahead (ahead, or ahead to either side: REL8 7, 0, 1; seen, the senses' last look's) is run at.  A war cry first ("\"Charge!\"  You run at the giant rat."): `I_NOISE` with loudness 6 (a yell, SOUND.md's), b 0, the character's square; the world does nothing with it till the noise batch (B2) builds it.  Then the close-in (`I_CLOSE`, a blow as it gets there) with a new field, `run` (w2: msgs.def's `I_CLOSE ... w2 run`), the pace its walk is planned at: `GA_RUN`, "fast charge" `GA_SPRINT` (the world's `runs`, the flight's way of setting a pace for one walk; the player's own pace isn't changed).  The engage test is the walk's own, as it comes into the foe's zone.  One beside it already is struck at once ("You fall on the giant rat.").  None: "There's no foe ahead to charge." (the grammar's `ERROR:NO_TARGET`).  For review: only the first leg is at a run (a foe that moves on, the later legs at the character's pace: the world's part 24 had no room for more).
- **A throw a way** ("throw the dagger north", "throw the dagger 3 north", "throw the dagger hard south"): the parser makes these `VB_THROW_OBJECT` with a heading (no grammar makes `VB_THROW_DIRECTION`), so `H_MISSILE` passes a throw with a heading to `H_THROW_WAY`, whose row stays for the action.  The mark is that many squares along the line (none said, a dozen: as far as it'll go), then the world's throw at a place (`IO_THROW`, the referee's Throwing).  **The world stops a thrown thing short on its line** (things.c's `s_throw_at`, the character's throws: each square from it to the mark asked of `ln_pass`, a missile's: rock, a wall, a door shut, glass stop it; it falls on the last square it got to).  That's every throw of the character's, at a thing too (one seen through glass now falls at the glass); a denizen's throw is as it was.  Words: "You throw the dagger south: it lands 2 squares off.", "...: it strikes the wall and falls 4 squares off.", "...: it strikes the wall and falls at your feet.", ", but it falls short, 9 squares off." (past its extreme range).  ("The wall" for a door shut too, for review.)
- **Engage** ("engage the rat"): `I_CLOSE` with its walk alone (w1 1, "go to the rat"'s), no blow; the engage test the walk's; engaged by another on the way, the world's refusal (`TE_ENGAGED`).  Beside it already: "You're already engaged with the giant rat."  A door, a window, a portcullis: "You can engage only a creature."; none of that name: the fight's words.
- **A split attack** ("attack the rat and the goblin"; attack, fight, hit, strike): parts.c no longer cuts such a line in two at its "and" (a blow's verb first, a determiner after the "and"), and `parts_op` (its `PO_AGAIN` look at every command) makes the parser's attack in a line with " and " and no "with" `VB_ATTACK_SPLIT`.  The parser's thing is the last named (the second); the first is the word before the "and".  Both beside it: one `I_ATTACK` with a new field, `split` (w1: the second, plus 1), "You strike at the giant rat and the giant bat by turns."; the world's fighting on (`fsplit`) strikes at the other each round while both stand (its target down, the other's alone; the other down, its target's).  The second not beside it, the first is attacked alone; the first not beside it, "The rat is out of reach: go up to it first."  For review: a close-in to the first, then the split, would want `I_CLOSE` to carry the second too; two of one name can't be told apart by name (the second word finds the same one: the first's attack alone); "punch the rat and the goblin" is still two attacks (a blow's own word not among the split's).
- **Again** ("again", "repeat"; hself.c's `H_AGAIN`): the last line typed that didn't say "again" is run again, its parts queued first, before any line typed ahead, each carried out as if typed.  term.c's `last` (cursor up's) is `t_last` now, and a line with a word begun "again" or "repeat" isn't kept in it (term.c's `said_again`, overlay 16's), so "north then again" can't run itself for ever.  Nothing typed yet: "There's nothing to do again."  As built "g" stayed give's (commands.verbs' `g = give`); *the user's, 2026-11-11: "g" is again's* (Infocom's), give spelt out (below, "The user's review of five defaults").
- **Room.**  The handlers' work is part 24's (the commands seen to less often), through two stubs (`fights`, `again_go`): part 2 is 89 bytes from full (110 before); a third stub, `said_again`.  Overlay 6 is 67 bytes from full (183 before: parts.c's split), overlay 16 25 (88).  Part 24's state is the part's own (`bss-name` SW24RODATA: none kept across a call of another part).  The UI's main RAM 2,202 bytes free (2,272 before).  The world: part 24 (fighting on) full, the charge's pace set in part 37 (the intent's).  The UI's banks are close to full (some 4,000 bytes free over all, the most 514 in one): a later batch may need parts moved, or a bank more.
- **Test dq0fightverbs**: again with nothing, charge with none, the throws (two squares; the wall behind, at its feet), turn left then again and repeat (three turns), a charge (the war cry, the close-in at a run or the blow, a blow struck), engage and already engaged, a split attack (both struck at).
