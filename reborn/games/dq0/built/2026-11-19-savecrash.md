# "save" typed halted the UI: the save's window kept

As built, for review.  Branch savecrash, 2026-11-19, from reborn-commands' 265839c4.

## The cause

"save" is overlay 7's (task.c's o_meta, through its stub meta), which called part 14's s_save_game (main.c).  That
function wrote the hub's header (HUB->slot, ->mreq, ->dsave, ->wreq) as if the hub were in the window, but overlay 7's
bank was, so the writes went into overlay 7's bank.  It ended with the hub's bank 0 in the window (letters.c's
s_letters_keep's last hub_map (0)), and returned through swap.inc's sw_back (the guard's "rp_x+963", $0D69) into
overlay 7's code at $939C, the instruction after its jsr _save_game, which ran the hub's header as code.  It's the
pattern of GAME-CRITIQUE.md's 6.1 and 79083a98: a call from an overlay into code that maps the hub.

Every game that typed "save" halted there: new or carried on, with -C or not (pb's 15 of 16 played "save"; the
16th, s7C1, never reached it).  No test typed "save": the suite's "save" is the quit's.

## Which commit

66a2eae7 (2026-10-21, "the UI's pause and save into overlay 3").  At its parent, 97a965c0, "save" says "The game is
kept."; at 66a2eae7, and at 4228767f and 84242dcd after it, "save" ends the game (the shell's prompt next, nothing
said); at bdf3a1ad and since, the fetch guard halts it.  So it predates the merges since bdf3a1ad.  (Each a new game at
seed 5, "look", "turn right", "save": pb's fhash.js, the commands fixed.)

## The fix

s_save_game puts the hub's header in the window itself (hub_here, hub_map (0)) and the window back as it found it at
its end (hub_back), look_ov's way.  No new stub, so the UI's main RAM is as it was: 3,066.  Its other callers (s_finish,
part 1; came, part 6) are main RAM code and get the window back as it was.

## The same pattern elsewhere

Every overlay's calls into a swapped part's stub, in every task (dq0: body_words, clues_kept, den_seen, early, hide,
lie_words, look_ov, reach_go, reach_said, read_cmd, room_where, wield; none in the others), read for a window left
changed: each puts it back (hub_back, hub_mem) or doesn't touch the hub; hthings.c's hide from overlay 1 is the rope's
early return, the hub untouched.  A scan of every overlay's calls to a function that calls hub_map without hub_back
found only false hits (kind_in, which puts its bank back by setbank; caves.c's own static open_in).  The save's other
tasks' (the world's, the maker's, the denizens') run under the guards in the new test.

## The test

dq0savemid: seeds 3 and 5, the caches small (-C) and not.  Each game played twice, the same commands, one with "save"
typed and one without, both quit; the two folders' game.dat and world.dat byte for byte the same; both carried on
(resumed), played on, "save" typed in one, quit and compared again; the saved one carried on once more.  "The game is
kept." six times.  At 265839c4 it fails with the guard's halt.

## The suite

501 tests, with the guards on: all pass.
