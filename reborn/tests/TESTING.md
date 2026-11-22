# Testing: the tests a change can affect, and the tiers of checks

Several agents build at once on one machine. If each runs the whole suite (460 tests) several times, plus
RULES=null, the soaks and dq0answer at three waits, the machine spends most of its time testing. So each agent runs
only the tests its change can affect while it works and before it reports, and the lead runs everything once a
round, after all the merges.

## The selector: `--changed`, `--near`, `--list`

`node sim/test.js --changed [REF]` runs only the tests that the files changed since REF can affect (since its merge
base with HEAD). `--changed=REF` names it. Without one, the REF is the repository's main branch, `main` (its merge
base with HEAD), or HEAD's parent where there's none (`defaultRef` in `../base/sim/select.js`). Since `main` is far
behind a branch of the game's work, name the branch your work began from (today `reborn-commands`) to run only what
your change can affect. The files are everything committed since the merge base, everything staged or not, and the
new files git doesn't ignore.

- Each file maps to areas through the table in `tests/areas.js` (`rules`: the first rule that matches the path).
  For example, `base/kernel`, `reborn/modules/rc`, `reborn/sim` and `reborn/tools` run every test.
  `reborn/games/` and `reborn/bank/` run the game's tests (dq0..., hmchar...). `reborn/modules/basic` runs BASIC's
  tests and those that see the ROM's layout. `tests/mod/t_task` runs the tests that boot `t_task`. The design
  documents (`*.md` in games/, docs/, manual/) run nothing.
- A path that no rule names runs **every** test. A `word:NAME` rule that matches no test runs every test too.
- The tests are in suites, by area: `tests/suites/*.js` (the system's, BASIC's, HyForth's, hylang's, the sound's, the
  PC tool's, the screen's, hmchar's, the numbers') and `tests/suites/dq0/*.js` (the game's, by subject: map, parser,
  gear, fight, denizens, senses, body, time, save, theme, screen). Their shared helpers are in `tests/lib/common.js`
  and `tests/lib/dq0.js` (dq0Typed, dq0Turns, dq0Ev, dq0Poke, dq0Check ...); a suite's own helpers are at its top.
  `tests/tests.js` is the index: the suites in their order, and the exports `sim/test.js` and the tools use.
- A suite maps by its entries. A changed line inside a test's entry runs that test. A changed line in a helper (a
  top-level declaration, the suite's own or a helper module's) runs the tests whose entries use that helper, directly
  or through other helpers. A changed line in the requires or the exports runs every test. A changed comment outside
  an entry runs nothing. The index (`tests.js`) runs every test. The selector's own test, `tiers`, runs whenever a
  suite or a helper module changes.
- A test's areas (`areasOf` in `tests/areas.js`): its modules (`mod:t_rc`), `dq0`, `hmchar`, `sdcard`, `pc`, `c`,
  the languages' and libraries' (`basic`, `forth`, `hylang`, `asm`, `numbers`, `sound`: by the words in its entry
  and its helpers), `layout`, a test's own `area: [...]`, and the game's finer areas.
- The game's finer areas are for `--near`: `fight`, `parser`, `map`, `gear`, `money`, `senses`, `denizens`, `theme`,
  `time`, `body`, `save`, `screen` (`NEAR` in `tests/areas.js`, by the dq0 tests' names). A new dq0 test can go in
  one there, or take an entry tag `near: ['fight']`.

`--near AREA,...` runs the tests of those areas, plus any tests named: `node sim/test.js dq0shield --near fight`.
An area can be any area a test has (`fight`, `basic`, `mod:t_rc`), or a test's name.

`--list` prints what would run and why, without running anything. It shows the files changed, each file's areas,
and each test's reason: `node sim/test.js --changed --list`.

With RULES=NAME and no tests named, the runner still takes only the meaning checks (`meaning: true`). Combined with
`--changed` or `--near`, it takes the meaning checks among the tests those select.

After a run, `obj/testtimes.json` has each test's wall time, CPU time (its process's own: the measure to trust when
other runs share the machine) and cycles, and in `all` every test's last; the parallel runner starts the longest
first by them. Each test runs in a process of its own, `-j` (the cores) at a time. A test's ROM image is kept in
`obj/imgcache` by its inputs' sha1 (the modules, the init, the BIOS, the ROM disk's files, the image tools): made
once a build, not once a test.

## Scenes: a dq0 test taken up after its start

Most dq0 tests boot, make a character and wait while the game grows its first dungeon (some 235M cycles at seed 1,
310M at a generated theme's seed 123456789: several seconds) before their first command. `sim/lib/scenes.js` saves
the whole machine at two points of a test's input, the prompt waits just before and just after its first `dq0` line,
in `obj/scenes`, and the next test whose input is the same up to there is taken up from it instead (its watchers
given what the serial port gave before it). A scene's key is that input and the ROM's, the BIOS's and the SD card's
images (their sha1), the seed, the chip, the modules, the core, `HUBOWN`, and the emulator's and the runner's
sources: a rebuild makes new ones. A test with `start(m)` saves its own (its suite's sources in the key). A test with
budgets, `send`, a `/pc`, a machine of its own but the input and the SD card, or `scene: false`, is never taken up.

- `SCENES=0`: none (every test from its start, as before). `SCENES=fresh`: none taken up, each saved anew.
  obj/scenes keeps the newest 400 (`SCENES_KEEP=N` sets another number).
- `SCENES_TRACE=DIR`: each dq0 test's cycles, output hash, failures and notes in `DIR/NAME.json`, to hold a run with
  scenes to one without (a test taken up must end exactly as it does from its start: the same cycles, the same
  output; one that doesn't is `scene: false`).

## The machine's memory: `HYDRA_MODULES`

Each test's machine has three RAM modules (48 banks, 384K a task), as the user's board has (`DEFAULT_MODULES` in
`base/sim/lib/machine.js`, which `sim/run.js`, the test runners, the soaks, the browser's emulator and the danlang
emulator all use). To run a whole suite with fewer, set `HYDRA_MODULES=N` (1 to 3) in the environment, or give
`--modules N` to `sim/test.js`: `HYDRA_MODULES=2 node sim/test.js --tier lead`. The tests' processes, the tiers' steps
and the soaks inherit it, and the summary line names it. A test that sets its own `machine.modules` keeps its own.
`sim/run.js --modules N` sets one run's.

## The guards, the trends and the transcripts

**The emulator's guards** (`sim/lib/guards.js`) run in every test, at no cost in the game's bytes. A hit stops the
machine at once, and the test fails with the hit's words (the task, its program, the function, the bank):

- **fetch**: an instruction fetched from the bank window ($8000-$9FFF) with no code there. Code there is a program's
  link areas at $8000 (its overlays, the world's catalogue), its bytes those of the build's files, or a module of the
  kernel's code cache (a child, a bank program run in place).
- **store**: a store that names the hub's header (a C line's `HUB->`, or a hub macro; an assembly line's hub offset)
  writing into a bank other than the header's.
- **load**: a load that names the hub's header (the same lines: a load, a compare, a sum or a BIT, plain or indexed)
  reading a bank other than the header's; a read-modify-write (`++HUB->x`) is a load and a store. The byte read must
  be in a field of the header its line names (`HUB->pc_x`: pc_x's two bytes; a macro, the fields its body names), so
  a line that names the hub and reads another bank's bytes on purpose (a chunk's, an item's) isn't taken for a read
  of the hub. A line in an overlay's code (run in the window) that names the hub's header can't be right: it's said
  as the instruction's fetched. The hit names the C line and the calls that reached it.
- **swap**: the swap stack past its `SWDEPTH` (swap.inc's `swsp`), the overlay calls' past `DEPTH` (bank.s's `ovsp`).
- **stack**: cc65's C stack pointer below its floor (the swap area's top, or the BSS's end).

What each program has comes from the build's outputs (its map, labels, link config, listings and overlay files),
cached beside its map as `NAME.guards.json`. `KNOWN` in guards.js lists what's taken as it is, each with its reason
(hylang runs the code it compiles in its own banks). `GUARDS=0` turns them off; `GUARDS=note` notes every hit
without stopping, and lists them all after the tests; `GUARDS_FLAGS=N` keeps only some (1 fetch, 2 store, 4 the
stacks, 8 load). dq0guards makes each guard fire on purpose.

**What fails the suite, and what's a trend** (the user's, 2026-11-17). The suite fails on the user's targets (D17,
rethought 2026-11-17: games/dq0/built/2026-11-17-d17.md), the RAM floors (the build's) and a silent start. The
targets, at 115,200 baud and 3.58 MHz (`tests/lib/trends.js`'s `D17`):

- **A command's first letter** within a fifth of a second of its Enter, 720,000 cycles: every command, a look and
  the map as any other.
- **A reading's words flowing**: no pause longer than half a second, 1,790,000 cycles, between two of its letters
  from its first to its last (a look about, one way or across, the map, what's carried, a thing examined or assessed).
  The pause ends at the prompt or at the world's news told after the answer began (a stir, a sound).

They're measured at 115,200 baud only (`echo b115200 >/dev/serctl` before the game: 9,600's slower text is the user's
choice). dq0answer and dq0lookfar type each game three times, each line after 0-2 waits of 2M cycles by its own
pattern, and hold the median of the three (the background's work falls where it falls: 10-20% at the normal pace);
the rest hold their one run. A line is typed only once the character's free (dq0Timer's check of the UI's `busy`). No
measure over its target is let off, by a list or by its trend.

Every other cycle measure is a trend, kept in `tests/trends.json`: a rise of more than 5% over it is noted in the
test's report and after the tests, never failed. A target's measure is a trend too. After the tests, the summary lists
the trends that rose, each target's measure over it (failed), and each past 90% of it (the headroom rule).

To take a measure as it is now (a rise that's meant, a gain to keep, a new measure), run its test, then

    node sim/tools/trends.js --update NAME ...

and commit `tests/trends.json` with the reason. `node sim/tools/trends.js [NAME ...]` shows each measure beside its
trend. A target's trend never lets it off its target.

**Transcripts.** `TRANSCRIPTS=DIR node sim/test.js` saves each test's screen as `DIR/NAME.txt`. Run the suite before and
after a change, then compare the two folders sentence by sentence:

    node sim/tools/sentdiff.js BEFORE AFTER [NAME ...] [--quiet]

Each sentence only in one run is shown with its counts. A test in `tests/noise.txt` (its words move with the tasks'
timing, each with why), or a difference only in the list's noisy sentences, is marked as known noise; `--quiet`
counts those without showing them. Any other difference is a change to what the player reads, to be meant.

**A line's window profiled** (room6's profiler, a tool since speed9, 2026-11-22). A D17 measure that's slow, or one
past 90%, is taken apart where it's spent: the test is run once to find its lines (each Enter's cycle and its answer's
first letter, the test's dq0Timer), then again with the machine's profile on from the chosen line's Enter to its
first letter. The emulator is the same cycle for cycle at one build, so the window is exact (the second run says so
if it isn't).

    node sim/tools/lineprof.js TEST [LINE] [--nth N] [--list] [--top N] [--lead N] [--timeline N] [--out FILE]

`--list` lists every line typed with its first letter; LINE (its first words) picks a line, the slowest of its
typings unless `--nth N` (1 on: the test's own game, then the menacing and the played ones); no LINE, the slowest in
the run. The report: the window's cycles by task (the UI, the world, the referee, the parser, the console, the
kernel ...); each part a task copied into its swap area in the window (its cycle, the copy's cycles, sw_call's for a
stub's call or sw_back's for a return into a part, and whose); the C functions that took the most, the runtime's
helpers, the kernel's calls and the libraries' routines charged to the C function that called them (by the return
addresses on the 6502's stack), a function in an overlay or a swapped part named with it (`_o_pc_side(ov3)`, by the
bank in the task's window and its ovbank; `_has_role(p51)`, by its swapped: since speed11, 2026-11-22, from the C
files' segments in obj/games/TASK/*.s); and each routine with its two callers. `--lead N` starts the profile N cycles before
the Enter (a part copied in, or a round's work begun, just before it). `--timeline N` lists, in order, each run of a
task of N cycles or more in the window with its top functions: who ran when (a denizen's intent under way at the
Enter, the referee's blow, the UI's answer). It works on any test that times its lines
(`this.timer = dq0Timer(m)`); the hooks are the machine's `profileStep` (base/sim/lib/machine.js: each instruction
in the profile's window; the runs before the window as quick as ever) and sim/test.js's `addInstrument`.

The windows vary by build as much as by change: the same line's first letter swings 100,000-200,000 cycles as the
code's layout moves and the background's work (a denizen's move, the world's minute, the senses' look, their parts
copied in) lands in it or not. Compare what a window holds, not one number against another.

**The lag sweep** (speed10's sweeps, a tool since speed11, 2026-11-22). One suite run is one phase of each race between
a line and the background's work. `SIM_PROMPT_LAG=N` (base/sim/lib/acia.js) types every line's first key N cycles
later after its prompt, so the same test lands its lines at other phases (sim/lib/scenes.js keys a scene by the lag).
The sweep runs each test once at each lag, side by side, and gathers every typing of every line in a game (not its dq0
line or its quit, nor a soft question's yes or no unless `--all`):

    node sim/tools/lineprof.js --sweep TEST ... [--lags L,L,...] [--jobs N] [--top N] [--seed N] [--all]

For each test: its typings, their mean, how many are past 85%, 90% and 100% of D17's 720,000, and the worst; then the
`--top` lines (15) by their worst, with the first letter at each lag. The lags are speed10's eight by default (0, 7000,
13000, 23000, 31000, 41000, 53000, 67000: each a prime of thousands, so no two runs land alike); `--jobs` runs at
once (the processors less one). A run whose test fails is said, with its failures. Lag 0 is not quite the suite's
run: SIM_PROMPT_LAG unset and 0 are scenes of their own (scenes.js keys a scene by the variable as given), and the
same line can land 30,000 cycles apart in them. The eight D17 tests at eight lags take some
twenty minutes on eight processors. Profile a slow typing it finds with `SIM_PROMPT_LAG=N node sim/tools/lineprof.js
TEST LINE --nth K` (the same lag, the same window).

## The tiers

**1. While working**: the agent's own tests, and `--near` its area:

    node sim/test.js dq0shield dq0dens --near fight

**2. Before reporting** (the brief's checks): everything the change can affect, in one command:

    node sim/test.js --tier report --changed reborn-commands

This runs, one step after another with one summary at the end:

- `node sim/test.js --changed=REF`: only the game's tests when only games/ changed. It includes dq0answer
  at DQ0ANS_WAIT=0 whenever the game changed.
- `RULES=null node sim/test.js --changed=REF`: the meaning tests among those.
- The parser's checks (`commandmatch-test.js`, `commandmatch-test.js --mean`, `commandcorpus.js`), when a grammar
  file changed (`GRAMMAR` in `tests/areas.js`: `games/dq0/commands.*`, `games/dq0c/`, the command tools,
  `resolve.c`).

`--tier report --changed REF` names the REF; without it the tier takes the same default as `--changed` (`main`, or
HEAD~1). `--tier report --list` shows each step and what it would run.

**3. The lead, once per round after all the merges**: everything:

    node sim/test.js --tier lead

This runs the whole suite (dq0answer at DQ0ANS_WAIT=0 in it), RULES=null's meaning tests, the soaks
(`soak.js -n 200`, `-n 60 -fight 0.8` and `-n 20 -life 3 -same`), dq0answer at DQ0ANS_WAIT=1 and 2, and the parser's checks.
The plain soak hardly meets a denizen in 60 turns.  `-fight` plays scenes (a hostile theme, foes brought near as none come), tells the
blows, the foes felled and the deaths, and fails a run with too few (until 2026-11-04 it was the plain soak over again: its first 100
lines the same).  `-life` plays dungeons in sequence in one save's folder, through dq0life and dq0acct: death, leaving, solving, quitting
and going on, hard mode, the time between's questions answered at random, the record judged after each game.

A soak's machine powers up the same each run (`-power N`, 1 by default), so one tree's seed plays the same game every time and its
failure can be chased; until 2026-11-04 its RAM was random, the tasks' timings other each run, so other words at the player's prompts,
so other commands.  A change to any code still shifts a seed's game: the player reads the words as they fall.  `-same` plays each game
again with its commands and scenes as they were, on a machine powered up otherwise, and fails unless the world's events, the denizens'
and the replies are the same (the turns pace's rule, dq0turnsame's, for every game the soak plays; rest paid in other slices, the same
in all, allowed).

## Keeping it safe

The selector errs on the side of running more. The lead's whole run each round catches anything the selector
missed. The `tiers` test checks the selector itself:

- The tests files read as entries.
- Sample paths map to the tests they must run, and not to the tests they mustn't (`CASES` in `tests/areas.js`).
- The suites' and the helper modules' lines map to their entries, their helpers' tests, or every test.
- Every area a rule names is some test's.
- `NEAR`'s names are tests.

When you add a rule, add a case for it. When you add a non-game test that depends on a language or a library,
check `node sim/test.js --list --near basic` (or the area you mean) shows it, or give it `area: 'basic'`.

## What the tiers save

These figures are from the suite's last whole run: 2026-11-17, at `-j 8` on the shared machine, a46c5c38's images,
460 tests, all passing. Times are each test's own process's CPU time, summed (`obj/testtimes.json`): the measure to
trust when other runs share the machine. Cycles are the emulated machine's.

| Area | Tests | Cycles | Share of cycles | CPU time | Share of time |
|---|---:|---:|---:|---:|---:|
| The game's (dq0...) | 310 | 293.1G | 94% | 65.9 min | 93% |
| hmchar's | 3 | 2.9G | 1% | 0.5 min | 1% |
| hylang's (hy...) | 14 | 5.6G | 2% | 1.0 min | 1% |
| The rest of HydraOS's | 116 | 11.4G | 4% | 3.0 min | 4% |
| The base's | 17 | 0.2G | 0.1% | 0.2 min | 0.3% |
| **The whole suite** | 460 | 313.3G | | 70.5 min | |

The whole suite took 17 minutes of wall time (its processes' wall times sum to 135 minutes). RULES=null runs the 78
meaning tests again: a quarter of the suite's CPU time, as they ran under HarnMaster. One dq0answer run took 19 s
(12 s of CPU); since D17's rethinking (2026-11-17) it types each pace's game three times, some 2.9G cycles and a
minute.

What `--changed` runs, for some sample diffs (the selector's choice, with those tests' share of the CPU time above):

| What changed | Tests | Share of the suite's time |
|---|---:|---:|
| The game only (games/, its rebuilt bin/ images) | 313 | 94% |
| The game, and a new dq0 test's entry in a dq0 suite (it, and `tiers`) | 315 | 94% |
| The design documents only (HOUSE.md ...) | 0 | 0% |
| The BASIC module | 18 | 0.6% |
| The hylang module | 37 | 2.7% |
| A program (edit) | 15 | 0.5% |
| A test module (t_task) | 1 | 0% |
| The kernel, the emulator, an unknown path | 460 | 100% |

The game's tests are most of the suite, so for game work `--changed` saves only the other sixteenth. The report tier
saves more by leaving the soaks and dq0answer at waits 1 and 2 to the lead's single run each round. The working
loop saves the most: a `--near` area is 1% to 26% of the suite (`fight` 13%, `parser` 8%, `map` 19%, `gear` 26%,
`money` 8%, `senses` 12%, `denizens` 15%, `theme` 8%, `time` 11%, `body` 12%, `save` 14%, `screen` 1%).

One report as the brief's checks were, against the report tier, for a game-only change on the same machine (measured
on 2026-11-04, when the suite had 253 tests; not measured since):

| Step | Before (every agent, every report) | `--tier report` |
|---|---|---|
| The suite | the whole suite, 253 tests, about 22 min | 107 tests, 14 min |
| RULES=null | 43 tests, about 5.5 min | 43 tests, 5.5 min |
| The soaks (`-n 200`; `-n 60 -fight 0.8`; `-n 20 -life 3 -same`) | about 5 min + 2 min + 3 min on eight cores (2026-11-04) | the lead's |
| dq0answer at waits 1 and 2 | about 3 min | the lead's |
| The parser's | always | only when the grammar changed |
| **Total** | about 60 min | about 20 min (measured: 19 min 44 s) |
