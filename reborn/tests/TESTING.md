# Testing: the tests a change can affect, and the tiers of checks

Several agents build at once on one machine. If each runs the whole suite (460 tests) several times, plus
RULES=null, the soaks and dq0answer at three waits, the machine spends most of its time testing. So each agent runs
only the tests its change can affect while it works and before it reports, and the lead runs everything once a
round, after all the merges.

## The selector: `--changed`, `--near`, `--list`

`node sim/test.js --changed [REF]` runs only the tests that the files changed since REF can affect (since its merge
base with HEAD). `--changed=REF` names it. Name it every time: the default REF (sim/test.js, sim/tiers.js) is still
`reborn-game`, a branch that no longer exists, so `--changed` alone stops with "no merge base". Give the branch your
work began from, today `reborn-commands`. The files are everything committed since the merge base, everything staged
or not, and the new files git doesn't ignore.

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

`--tier report --changed REF` names the REF; without it the tier uses the missing `reborn-game` and its suite steps
stop. `--tier report --list` shows each step and what it would run.

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
(12 s of CPU).

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
