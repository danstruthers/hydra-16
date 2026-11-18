# The build and the cycle made faster: the kernel per chip, the rest once, and a cache

As built, for review.  Branch buildspeed, 2026-11-17: GAME-CRITIQUE.md's 7.2 (phase C's E13).  Built and measured on
reborn-commands' f7468c55 (actor 0), rebased onto a2168fdb (polish, loadguard and looks merged) and checked again
there with the whole suite, then onto 43832bd7 (pb merged: none of this branch's files) and its images checked again.
No game code changed: the images (bin/hydra-16.html, sdcard.img, prom0-2.bin, bios_rockwell.bin, bios_wdc.bin, and
base/bin's three) are the build before it's, byte for byte, cold and warm, on each base.  Five commits:

- 25afc7c6 The build: the kernel per chip, the rest once; cc65 and ca65 cached
- a2b2d5af buildcache: the build cache's rules held, on sources of its own
- 02135b2e Docs: the kernel per chip, the build's cache, the games' build
- 43449700 Images read, written and removed again while Windows holds them a moment
- this record, as built

In short: the build compiled the whole OS twice, once per serial chip, and kept nothing.  Now the kernel is built for
each chip and the rest once; cc65's, ca65's and od65's runs are kept in obj/cache by a hash of everything each read,
and taken from there when it's all unchanged; the runs that aren't kept are run side by side; and `node build.js
games` builds only the games' part.  The build before ran every tool every time, 86-115 s of CPU and some 2,150
processes; now a build with nothing changed is 20 s and 187 processes (the games' build 13 s and 43), one from nothing
60 s, and a change to every task's header (hub.h) 38 s.  In wall time, back to back on a shared machine, 103 s became
28 s.  The player sees nothing different.

## 1. The kernel for each chip, the rest once

Only the kernel is told the serial chip (ACIA_CHIP, kernelDefines); the modules, the tests' modules and programs, the
games and the paged ROM are built without it, and came out the same for both (one prom0.bin, one sdcard.img).  So
base/build.js's buildBoth builds the other chip's kernel alone first (kernel(): its BIOS ROM), then the whole build for
the last chip (Rockwell's, unless --acia wdc), and writes the images for both chips as before (images.js): the BIOS ROM
per chip, the rest one file each.  obj/ is the last chip's, as before.

The BIOS ROM reaches the rest in two places only, both in the emulator: themec's runs (the stand-in themes, the goal
kinds' checks) and hysnap (hylang's snapshot); the paged ROM's table of the BIOS's checksums is the same for every BIOS
(fixSums).  `node build.js --both` still builds everything for each chip, and says whether any image but the BIOS ROM
differs: "The chips' builds the same but the BIOS ROM" (2026-11-17), its images the build before's, byte for byte.

## 2. The cache (base/tools/objcache.js)

A run of cc65 (a .c to its .s), ca65 (a .s to its .o and listing) or od65 (its dump of objects) is taken from
obj/cache only when nothing it read can differ:

- **Its key**: the cache's format; the tool's own file's sha1 (its version, however it was built: this cc65 says only
  "V2.19"); the folder it runs in; every CC65_*, CA65_* and LD65_* variable; every argument as given (the flags, the
  defines, the include folders, the outputs' names); its input's name and sha1.
- **Under the key, the runs kept** (six at most), each with every file the tool read and its sha1 (the tool's own list:
  cc65's --create-full-dep, ca65's --create-dep: the headers, cc65's own headers, the .include, .incbin and .macpack
  files), and **every place a name could have been found**: each #include, .include, .incbin and .macpack name in
  each of those files, in every folder the tool might look in (the one it runs in, each file's own, each -I and
  --bin-include-dir, cc65's own include and asminc), and whether a file was there.  A run is taken only if every file
  read has its sha1 still and every place has a file or none as it had: a header put in a folder searched before the
  one the run found it in is a miss.
- **A run is kept only if** every file it read is found by such a name (a file read that no name explains means the
  tool looks somewhere the cache doesn't know: never kept, run every time), and no source outside obj/ changed within
  two seconds of the run's start (an edit during the run would otherwise be kept with the outputs of what was there
  before it).  A run whose arguments aren't of the forms the build gives is run outside the cache; a failed run is
  never kept.
- ca65's --create-full-dep also lists the C files cc65's `.dbg file` lines name, which ca65 never opens: its plain
  --create-dep is what it reads.  The first build here kept none of the C's assembly for that reason, and none of the
  290 sources with `.macpack longbranch` before the .macpack names were known: the "explained" rule held both back
  (not kept, never a wrong hit).
- **od65's runs** are kept by the tool, the arguments and each object's sha1; its output is its stdout.
- **What a hit gives** is the outputs as the tool made them.  ca65 stamps each object with the time it made it, and
  -g's debug information records each source's time (cc65's `.dbg file` lines): an object from the cache carries the
  times of its making, as any incremental build's does.  The code, the data, the maps, the labels and every image are
  the same byte for byte.
- **A moved tree, or another worktree's copy of the cache.**  cc65 and ca65 are given the repository's files by their
  names from the folder they run in (relative: reborn/'s, the base's), so what they make names them that way, the same
  wherever the tree is; the records name the repository's files from its top (@/reborn/...).  A run kept in one place
  is found in another, and what it gives names the files where they now are.  Two readers of those names resolve them
  from that folder now: bankcheck.js (base/'s; and it now fails if a source named in the debug information isn't
  there, where it used to read it as empty and so pass), and guards.js's reading of a C line (reborn/'s).
- Within one build a source's sha1 is found once while its size and time stay, and whether a place outside obj/ has
  a file once; obj/'s files, which the build writes as it goes, are looked at every time, and a new record is made
  from what the run read, read anew.
- **Its size**: held to 1 GB (HYDRA_BUILD_CACHE_MB) as each build ends, the least recently used records and the
  outputs only they had going first.  One build's worth is some 120 MB (835 records, 1,332 outputs).
- **Forcing a clean build**: `node build.js --clean` (obj/cache emptied first, every tool run, all kept again);
  `node build.js --no-cache` (none used or kept: the build as before but chip-once); HYDRA_BUILD_CACHE=0 the same
  from the environment; deleting reborn/obj/cache does it by hand.  HYDRA_BUILD_CACHE_DEBUG=1 says each run not kept,
  and why.  The build's last line says how many runs it took from the cache, ran and kept.
- **Its test**: buildcache (tests/suites/system.js; objcache.js's selfCheck): sources of its own in obj/buildcache,
  run through a cache of their own, each rule held: a run kept, then taken; an .include's .include changed (a miss,
  then a hit again once it's back); an .include put in a folder searched first (a miss); an .incbin's file changed; a
  source changed as the run began (not kept); a header changed; a computed #include (its header found where no name
  says: not kept); ca65 on cc65's assembly (kept); od65; two runs side by side; and the tree moved with its cache (its
  runs found, their outputs the first's).  Each rule broken on purpose (the places not checked, the explaining off,
  the two seconds off) fails it.

## 3. The runs side by side

A batch of runs (a folder's sources: base/build.js's and build.js's assemble, compileC's C and then its assembly,
swappedParts' compiler runs: tools/dq0build.js's cc65all) is looked up first; those not in the cache are run side by
side, HYDRA_BUILD_JOBS at a time (the cores), by a worker thread (objjobs.js), kept, then taken in turn as the build
asks for each.  Not if two runs of the batch would write one file, or one read another's output.  A run that fails
is left to the build's own run of it, which says why.

## 4. The games' build: `node build.js games`

For a change in games/ (or bank/'s programs): the generated headers, the C library, the test RAM programs (t_rules
reads hub.h), the games and their make.js data, the bank programs (their callers' exports, the programs', read from
the last whole build's .exp files), the room table and the floors, the SD card and the browser's page; the BIOS ROM
and the paged ROM as the last whole build left them, and its chip and clock.  It does the whole build itself if
there's no whole build to build on (or the last one failed outside the games), or if a module of the paged ROM isn't
what that build made it of: each module's objects made again in place (from the cache) and held to the whole build's,
byte for byte (dq0lib .includes games/dq0/lnzp.inc and the hub's numbers, so a change in games/ can reach the paged
ROM).  Its failure is marked as the games' build's (obj/build.failed), so the next games' build may go on from it;
the tests refuse it as any failed build.

## 5. What the player sees

Nothing: no game code changed, and every image is the one the build before made, byte for byte (cold, warm, after
each change put back, and with --both).  The room: as before, every task's (dq0 3,043; dq0w 2,150; dq0d 2,964; dq0p
4,422; dq0m 3,852; dq0harn 2,067; dq0s 17,526; themec 573; themegen 564): the build's report is the old one's line for
line, but for the hub's generators' two lines, said now as there's one pass.  The start's time: as before (the images
are the same).

The transcripts (the cold suite's, sim/tools/sentdiff.js against mA5, which is 1250c7f6's, before actor 0): 21 tests
differ, none by this branch.
- 9 in the known noise (tests/noise.txt): dq0answer, dq0denportc, dq0jobs, dq0lookfar, dq0mine, dq0paceword,
  dq0passes, dq0reachstop, dq0skykin.
- 9 by f1cdc9ec's change of them: each only the line it now types, "echo b115200 >/dev/serctl" (dq0doff, dq0letcmd,
  dq0letdens, dq0letseveral, dq0letsight, dq0letstow, dq0letters, dq0letwhich, dq0torchwords).
- dq0rockwall (the rock named is the rock there) and dq0hold (one "Something stirs." fewer): actor 0's, as its record
  says; dq0savebytes, new with actor 0; buildcache, new here (not a game's).
- round (the sound suite's: four tasks sing a round): "its latest note late by 1 1 1 3 ticks" where every run before
  had "1 1 2 3", and its words' order with it.  Not the game, and not the images (the same, and round.hyx is the
  card's, byte for byte): the test copies round.hyx through the PC tool, which gives the Hydra the host file's time
  and its id (sim/tools/pcfs.js: the local time of its modification, the file system's inode number), and the runner
  writes that file as the test starts.  The tree moved from C: to N: today, and the hour is later.  Run alone twice it
  gives 1 1 1 3 both times, and on the build before this branch's own outputs (a2168fdb's build files) 1 1 1 3 too:
  not this branch's.

## 6. The timings

Each build in reborn/ (`node build.js`, or `games`), measured as one Windows job: its wall time, and the CPU time of
every process of it (node, cc65, ca65, ld65, od65, the cmd that ran it), whether they ended or not.  The machine was
saturated by other agents' suites throughout (the CPU at 100%, the disk idle), so a wall time is mostly the queue for
the CPU, and swings two to five times between like runs; the CPU time is the measure that holds.  On f7468c55:

| Build | CPU s | Processes | Wall s | From the cache |
|---|---:|---:|---:|---|
| The build before (f7468c55), cold | 115.3 | 2,178 | 453.5 | (none) |
| This one, cold (obj/ emptied) | 60.4 | 1,020 | 1,054.7 | 0 of 831 runs |
| Warm, nothing changed | 19.7 | 187 | 234.9 | 926 of 926 |
| A game task's file changed (games/dq0w/wdoor.c), whole build | 20.4 | 191 | 361.4 | 922, 4 run |
| The same change, the games' build | 17.0 | 47 | 338.2 | 721, 4 run |
| The games' build, nothing changed | 12.8 | 43 | 56.1 | 725 of 725 |
| An OS file changed (modules/rc/rc.s), whole build | 19.2 | 188 | 221.5 | 925, 1 run |
| games/dq0/hub.h changed (every task's header), the games' build | 38.4 | 409 | 307.8 | 271, 364 run side by side |
| --both, warm (each chip's whole build) | 36.4 | 370 | 297.6 | 1,806 of 1,806 |

The whole suite on f7468c55 with this build (`node sim/test.js`, -j 8, the same load; 481 tests, 477 passing, the 4
failing f1cdc9ec's known misses on their 9 measures: dq0answer, dq0lookfar, dq0letstow, dq0letdens):

| Suite | CPU s | Wall s |
|---|---:|---:|
| Cold (obj/scenes and obj/imgcache emptied) | 4,603.1 | 2,062.8 |
| Warm (its scenes and ROM images kept) | 3,256.0 | 1,703.4 |

The suite is the cycle's larger part still: a build with nothing changed is now under 1% of a cold suite's CPU, and
a whole cold build some 1.3%.

On the rebased branch (a2168fdb: polish, loadguard and looks merged), back to back, the same load:

| Build | CPU s | Processes | Wall s |
|---|---:|---:|---:|
| The build before (a2168fdb's build files; its themec answers kept from the runs before) | 86.2 | 2,150 | 1,187.3 |
| This one, the merged branches' sources new to its cache | 44.2 | 570 | 671.3 |
| This one, warm (nothing changed) | 20.1 | 187 | 742.4 |
| This games' build, warm | 12.8 | 43 | 337.3 |

Every image the build before's, byte for byte, after the warm build and after the games' build.

Then on 43832bd7 (pb merged on top: its generators and dq0c's tables changed, the cache still holding a2168fdb's runs,
so a key blind to any of pb's inputs would have given a stale run, and images unlike the build before's):

| Build | CPU s | Processes | Wall s |
|---|---:|---:|---:|
| The build before (43832bd7's build files) | 89.0 | 2,150 | 733.5 |
| This one, pb's sources new to its cache | 49.6 | 562 | 49.2 |
| This one, warm | 20.8 | 187 | 39.3 |
| This games' build, warm | 13.1 | 43 | 19.0 |

Every image the build before's, byte for byte, after each; buildcache, tiers, dq0guards and dq0starttime pass.  The
load eased after the first row; the two again, back to back on the quieter machine:

| Build | CPU s | Processes | Wall s |
|---|---:|---:|---:|
| The build before (43832bd7's build files) | 85.0 | 2,148 | 102.8 |
| This one, warm (nothing changed) | 20.7 | 187 | 27.6 |

So with the machine shared but not saturated, a build after no change or a change in one file takes some 30 s where
it took 100 s and more (and the games' build some 20 s); on a saturated machine each takes its share of the queue,
about a quarter of the old build's.

The suite on a2168fdb (its scenes cold: new images): 490 tests, 487 passing; the 3 failing are known
misses on 5 measures (dq0letstow's "drop 1g", dq0letdens' "attack C" and "attack A", dq0lookfar's map and look
across); dq0answer passes since looks.  CPU 4,755.0 s, wall 2,175.3 s.  Its transcripts against mB2 (the latest
baseline, taken before looks merged): 7 differ, 5 in the known noise (dq0answer, dq0jobs, dq0lookfar, dq0restbusy,
dq0skykin), dq0hold (the known noise's "Something stirs." once more, and the words it brought: "The giant rat ahead
to your left is gone from sight.", as mA5 had them) and round (above); dq0looknear (looks') and buildcache new.  None
is this branch's: its images are the build before's.

**The final suite, on 43832bd7** (pb merged): 492 tests, 489 passing; the 3 failing the same known misses on the same 5
measures.  CPU 4,808.8 s, wall 2,134.5 s.  Against mB2: 20 differ.
- 6 in the known noise: dq0answer, dq0jobs, dq0lookfar, dq0reachstop, dq0sway, dq0tiring.
- 8 of the game's, by pb's parser (merged after mB2): dq0busyhands, dq0compdesk, dq0darksearch, dq0namematch,
  dq0parsefix, dq0played, dq0tiepoints, dq0tongue2; dq0forms and dq0parseanswers new with it.
- 6 not the game's: round (above); chorus, philo, race (samples run from the SD card, its tasks' interleaving),
  tools (the disks' listings) and hmcharhl (a character made in hylang).  These five differ between this branch's own
  suites on a2168fdb and on 43832bd7 too, and not between mB1 and mB2: they follow the SD card's files, which pb's
  merge changed (the card's files sort games/ before sample/, so the samples moved).
None is this branch's: on 43832bd7 too its images are the build before's, byte for byte.

## 7. Scenes keyed on what they depend on: left as they are

A scene's key (sim/lib/scenes.js) is its input up to the point, the paged ROM's, the BIOS ROM's and the SD card's
images (their sha1), the seed, the chip, the RAM modules, the core, the watchers' switches and the emulator's and the
runner's sources.  Keying it on what the machine read instead (the card's blocks and the ROM's banks read before the
point, each with its sha1) could be proved valid: the machine's run to the point is a function of what it read, its
input and its code.  It isn't built, because it would save almost nothing for the case it's for, a change to one
task:

- **After the dq0 line** (the point most tests are taken up from: the game begun, its first dungeon grown), the
  machine has read every task the game starts with and their data (dq0, dq0w, dq0d, dq0p, dq0m, dq0s, dq0harn, dq0c
  and their files): a change to any of them changes what it read, so those scenes are rightly cold anyway.
- **Before the dq0 line** (booted, the character made: hmchar), the card is laid out afresh each build in its files'
  order (tools/sdcard.js: by path), so a task that grows or shrinks moves every file after it, and the directory's
  entries (each file's size and first block) change: the blocks the boot and hmchar read change with them.
- Unchanged images keep the scenes already (a build is the same byte for byte when nothing changed, and the games'
  build leaves the BIOS ROM and the paged ROM alone).
- It would need the emulator's SD card and ROM reads recorded (base/sim/lib: loadguard's area today).

The cycles at each point (the cold suite's 333 scenes): before the dq0 line, a median of 63.4M (58 scenes); after it,
322.4M (275).  So the most a dependency-keyed scene could keep warm through a change to one task is the boot and the
character's making, a fifth of a test's start, and only when the card's files after the task don't move: seldom.

## 8. Other branches, and what would break the cache's rules

- **guards.js (loadguard, merged before this branch's rebase).**  This branch resolves a C file's name from a listing
  against reborn/; loadguard had rewritten that line to try the name, then the same file under this tree (its answer
  to the moved worktree).  They met in the rebase: the reader now tries `path.resolve(ROOT, file)` first (a name
  relative to reborn/, as this build writes them), then loadguard's fallback (an absolute name from a tree that
  moved).  Without the first, a C line's store isn't checked against the hub's header when the tests run in another
  folder: the guard falls silent, it doesn't fail.  The guards read the build's maps and labels (ld65's, made anew
  every build), its listings (written by every run, from the cache as made) and the programs' own outputs: each in
  place and current after a build from the cache.
- **The image files held a moment** (a build at the head failed once opening bin/sdcard.img: "UNKNOWN: unknown
  error"): images.js's retry tries a read, write or removal of an image again every quarter second, twenty times,
  for UNKNOWN, EBUSY, EPERM and EACCES alone.
- **A new kind of input.**  A new include directive, a computed #include or .include, a macro package not named by
  .macpack, a file the tools find by a search this doesn't model: the run reads a file no name explains, so it's never
  kept (it runs every time, and HYDRA_BUILD_CACHE_DEBUG=1 says which and why), never taken wrongly.  A flag the build
  doesn't give today (-W, --feature, -T, a listing's options...): the run is made outside the cache ("run outside it"
  on the build's last line).  Another cc65 (a rebuilt toolchain): its file's sha1 is in every key.  A new variable the
  tools read: CC65_*, CA65_* and LD65_* are in the key; another name would need adding to envOf.
- **A generator that writes a source outside obj/ after the tools begin** (none does: apigen and hubgen write before):
  within that one build a header's sha1 found earlier could be taken again if it were rewritten with the same size in
  the same tick of the clock.  Records are always made from the files as read anew, and a source changed within two
  seconds of a run's start isn't kept.
- **cc65 and ca65 run other than through build.js's run** (hubgen's evaluate, in a temporary folder; sim/tools'
  own): outside the cache, as before.

## 9. Open questions

1. **ld65's runs aren't cached.**  In a build with nothing changed they're most of what's left (some 180 links,
   with the paged ROM's packing's), with hysnap's run in the emulator and the build's own JS.  Caching them needs each
   link's outputs read from its config (`file = "%O.ovl"`, `"%O.1"` ... `"%O.rop"`, `"%O.cat"`): the next step if it's
   wanted.
2. **themec's answers** (tools/themec.js's own cache in obj/themec, from before this branch) are kept by themec,
   the bestiary, the styles, the modules and the theme's text, not by the BIOS ROM, the ROM disk's files or the
   emulator's code.  By this job's rule ("hash every input that can change an output") the key is short of those;
   left, as it isn't the compiler's or the assembler's.  Should it get them?
3. **The rest once rests on** themec's and hysnap's runs in the emulator not depending on the serial chip (their
   only way to the BIOS ROM); `--both` holds it (2026-11-17: the same but the BIOS ROM).  Should the lead's round run
   `node build.js --both` once (a warm one is about a warm build more)?

No asymmetry between the character and the denizens: no game code changed.
