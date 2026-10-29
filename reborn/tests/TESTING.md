# Testing: the tests a change can affect, and the tiers of checks

Several agents build at once on one machine. If each runs the whole suite (252 tests) several times, plus
RULES=null, the soaks and dq0answer at three waits, the machine spends most of its time testing. So each agent runs
only the tests its change can affect while it works and before it reports, and the lead runs everything once a
round, after all the merges.

## The selector: `--changed`, `--near`, `--list`

`node sim/test.js --changed [REF]` runs only the tests that the files changed since REF can affect. The default REF
is `reborn-game` (its merge base with HEAD). `--changed=REF` names another. The files are everything committed since
the merge base, everything staged or not, and the new files git doesn't ignore.

- Each file maps to areas through the table in `tests/areas.js` (`rules`: the first rule that matches the path).
  For example, `base/kernel`, `reborn/modules/rc`, `reborn/sim` and `reborn/tools` run every test.
  `reborn/games/` and `reborn/bank/` run the game's tests (dq0..., hmchar...). `reborn/modules/basic` runs BASIC's
  tests and those that see the ROM's layout. `tests/mod/t_task` runs the tests that boot `t_task`. The design
  documents (`*.md` in games/, docs/, manual/) run nothing.
- A path that no rule names runs **every** test. A `word:NAME` rule that matches no test runs every test too.
- `tests/tests.js` maps by its entries. A changed line inside a test's entry runs that test. A changed line in a
  helper (a top-level declaration) runs the tests whose entries use that helper, directly or through other helpers.
  A changed line in the requires or the exports runs every test. A changed comment outside an entry runs nothing.
  The selector's own test, `tiers`, runs whenever `tests.js` changes.
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

After a run, `obj/testtimes.json` has each test's wall time and cycles.

## The tiers

**1. While working**: the agent's own tests, and `--near` its area:

    node sim/test.js dq0shield dq0dens --near fight

**2. Before reporting** (the brief's checks): everything the change can affect, in one command:

    node sim/test.js --tier report

This runs, one step after another with one summary at the end:

- `node sim/test.js --changed=reborn-game`: only the game's tests when only games/ changed. It includes dq0answer
  at DQ0ANS_WAIT=0 whenever the game changed.
- `RULES=null node sim/test.js --changed=reborn-game`: the meaning tests among those.
- The parser's checks (`commandmatch-test.js`, `commandmatch-test.js --mean`, `commandcorpus.js`), when a grammar
  file changed (`GRAMMAR` in `tests/areas.js`: `games/dq0/commands.*`, `games/dq0c/`, the command tools,
  `resolve.c`).

`--tier report --changed REF` uses another REF. `--tier report --list` shows each step and what it would run.

**3. The lead, once per round after all the merges**: everything:

    node sim/test.js --tier lead

This runs the whole suite (dq0answer at DQ0ANS_WAIT=0 in it), RULES=null's meaning tests, the soaks
(`soak.js -n 200` and `-n 100 -fight 0.8`), dq0answer at DQ0ANS_WAIT=1 and 2, and the parser's checks.

## Keeping it safe

The selector errs on the side of running more. The lead's whole run each round catches anything the selector
missed. The `tiers` test checks the selector itself:

- The tests files read as entries.
- Sample paths map to the tests they must run, and not to the tests they mustn't (`CASES` in `tests/areas.js`).
- tests.js's lines map to their entries, their helpers' tests, or every test.
- Every area a rule names is some test's.
- `NEAR`'s names are tests.

When you add a rule, add a case for it. When you add a non-game test that depends on a language or a library,
check `node sim/test.js --list --near basic` (or the area you mean) shows it, or give it `area: 'basic'`.

## What the tiers save

TIMES
