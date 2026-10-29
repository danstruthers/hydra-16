// ****************************************************************************
// areas.js - HydraOS's tests' areas, for sim/test.js's --changed, --near and --list (../base/sim/select.js; the
// tiers: TESTING.md).  rules: the paths a change can be in, each the areas it affects (the first rule that matches;
// select.js has their kinds: '*', 'none', 'entries', 'word:NAME', an area); areasOf(t, text): a test's areas.  Erring
// on the side of running more: a path no rule names runs every test, and so does a word no test has.
//
// A test's areas:
//   mod:NAME      its init and its modules (tests/mod/NAME)
//   dq0           Dingus Quest Zero's tests (dq0...): the game, its tasks, its makers, its data (games/, bank/)
//   hmchar        hmchar's (hmchar...)
//   sdcard        those with the SD card's image (sdcard: true: the samples, the games, the songs, the benchmarks)
//   pc            those with a PC folder at /pc (pc)
//   c             those that run a C program: the SD card's, or a .hyx of their own (the C SDK's)
//   basic, forth, hylang, asm, numbers, sound     those whose entries (and helpers) have the language's or the
//                 library's words (WORDS)
//   layout        those that see the ROM's layout: its modules, its banks, the ROM disk's files (LAYOUT)
//   tiers         the selector's own test (tiers)
//   NEAR's        the game's finer areas, for --near (a dq0 test's near: [...] too)
//   area          a test's own (t.area: a word or a list)
'use strict';
const base = require('../../base/tests/areas.js');

// The paths: from the repository's top.  Most specific first
const rules = [
  // The tests' own files
  ['reborn/tests/tests.js', ['entries', 'tiers']],
  ['reborn/tests/areas.js', ['*']],
  ['reborn/tests/TESTING.md', ['none']],
  [/^reborn\/tests\/mod\/([^/]+)\//, ['mod:$1']],
  [/^reborn\/tests\/(?:ram|bank)\/([^/]+)\//, ['word:$1']],   // (tests/ram/t_rules: dq0rules; tests/bank/count: bankprog)
  ['reborn/tests/basic/', ['basic']],
  ['reborn/tests/forth/', ['forth']],
  ['reborn/tests/hylang/', ['hylang']],
  ['reborn/tests/hyhydra/', ['hylang']],
  ['reborn/tests/scores/', ['word:PSG_SCORES']],
  ['reborn/tests/dq0/', ['word:LOOKS_REF']],
  ['reborn/tests/speed/', ['none']],                          // (sim/speed.js's, not the suite's)
  ['reborn/tests/numtest.js', ['word:numtest']],
  ['reborn/tests/', ['*']],
  // The game: its programs, tasks, data and bank programs (the SD card's)
  [/^reborn\/games\/.*\.md$/, ['none']],                      // (DUNGEON.md, HOUSE.md ...: the design's)
  ['reborn/games/', ['dq0', 'hmchar']],
  [/^reborn\/bank\/([^/]+)\//, ['dq0', 'hmchar', 'word:$1']],   // (bank/hello: bankrun's)
  // HydraOS's ROM: the modules every boot runs (init, the shell, the drivers' tasks) every test's; the languages and
  // the libraries their tests', and the layout's
  ['reborn/modules/basic/', ['basic', 'layout']],
  ['reborn/modules/forth/', ['forth', 'layout']],
  ['reborn/forthlib/', ['forth', 'layout']],
  ['reborn/modules/hylang/', ['hylang', 'layout']],
  ['reborn/modules/asm/', ['asm', 'basic', 'forth', 'layout']],   // (BASIC's ASM blocks, HyForth's disasm)
  ['reborn/modules/math/', ['numbers', 'basic', 'forth', 'hylang', 'c', 'layout']],
  ['reborn/modules/numbers/', ['numbers', 'basic', 'forth', 'hylang', 'c', 'layout']],
  ['reborn/modules/play/', ['sound', 'layout']],
  ['reborn/modules/', ['*']],                                 // (init, rc, cons, kdev, storage, snd, vid, rom.txt)
  ['reborn/spec/asm.def', ['asm', 'basic', 'forth']],
  ['reborn/spec/numbers.def', ['numbers', 'basic', 'forth', 'hylang', 'c']],
  // The ROM disk's programs: the tests that name them (else every test), and the layout's
  [/^reborn\/programs\/([^/.]+)/, ['word:$1', 'layout']],
  [/^reborn\/romfs\/bin\/([^/.]+)/, ['word:$1', 'layout']],
  ['reborn/romfs/songs/', ['sound', 'layout']],
  ['reborn/romfs/README', ['none']],
  ['reborn/romfs/', ['*']],                                   // (romfs.txt, lib/: the profile, the namespace ...)
  // The SDKs: the assembly one's includes every module's; the C one's library the C programs'
  ['reborn/sdk/c/', ['c', 'dq0', 'hmchar']],
  ['reborn/sdk/', ['*']],
  // The SD card's files; the built images none: the build makes them from the sources, whose rules run the tests
  // (the tests' ROM is made from obj/, their card is bin/sdcard.img, as the build made them)
  ['reborn/sdcard/', ['sdcard']],
  ['reborn/bin/', ['none']],
  // The emulator and its tools: the dq0 tests' watchers, the PC tool's, the danlang emulator (--dl's)
  ['reborn/sim/lib/dq0meaning.js', ['dq0']],
  ['reborn/sim/lib/hubown.js', ['dq0']],
  ['reborn/sim/lib/pchost.js', ['pc', 'word:pcHost']],
  ['reborn/sim/lib/pcproto.js', ['pc', 'word:pcHost']],
  ['reborn/sim/lib/xmpeer.js', ['word:createXmodemPeer']],
  ['reborn/sim/tools/pcfs.js', ['pc', 'word:pcHost']],
  ['reborn/sim/tools/hydrapc.js', ['pc']],
  ['reborn/sim/tools/hysong.js', ['sound', 'word:hysong']],
  [/^reborn\/sim\/tools\/(numref|numfmt|numxcheck)\.js$/, ['numbers', 'basic', 'forth', 'hylang', 'c']],
  [/^reborn\/sim\/tools\/(command\w*\.js|cmdmatch\/)/, ['dq0']],   // (the parser's tables: dq0c's, dq0w's make.js)
  [/^reborn\/sim\/tools\/(soak|sdwrite|nprof)\.js$/, ['none']],    // (the soaks: a tier's own; the card writer)
  ['reborn/sim/tiers.js', ['none']],                         // (The tiers' steps: --tier's, no test's)
  ['reborn/sim/dl/', ['dl']],                                 // (--dl's: every test then)
  [/^reborn\/sim\/(web\.js|web\/|bench\.js|speed\.js|consbench\.js|run\.js)/, ['none']],   // (Not the tests')
  ['reborn/sim/', ['*']],
  ['reborn/tools/', ['*']],
  [/^reborn\/[^/]+\.md$/, ['none']],
  ['reborn/', ['*']],                                         // (build.js, package.json ...)
  // The base's (its kernel, modules, emulator: every test), and the rest of the repository
  ...base.rules,
  ['old/sim/cards/', ['word:OLD_CARDS']],                     // (The fs test's fixture cards)
  ['old/', ['none']],
  ['docs/', ['none']],
  ['manual/', ['none']],
  ['board/', ['none']],
  ['README.md', ['none']],
];

// The language's and library's words, in a test's entry (and its helpers'): the tests of them
const WORDS = {
  basic: /(?<![\/\w])basic\b|\bBASIC\b|hybasic|\w\.bas\b/,      // (Not /lib/basic, a path: the layout's)
  forth: /(?<![\/\w])forth\b|HyForth|\w\.fs\b|\w\.fl\b/,
  hylang: /(?<![\/\w])hylang\b|[ā ']hy [-\w\/'"]|\w\.hl\b|hysuite|HYSUITE|\bhysh\b/,
  asm: /[ā\r']as |\bASM\b|\bdis\b|disasm|\bdb\b|\/lib\/as\b|\bas, the assembler/,
  numbers: /\bnumbers\b|numtest|\bmath\b|\bcnum\b/,
  sound: /\bplay\b|\bsnd\w*|\bmml\b|\.zsm\b|\bpsg\b|\bymLog\b|voices|\bsongs?\b|\bsound\b/i,
  c: /\.hyx\b|sample\/c\b|sdk\/c\b|\bC program|cc65/,
};

// The tests that see the ROM's layout (its modules and banks, the ROM disk's files and sizes)
const LAYOUT = ['boot', 'init', 'rom', 'bootsize', 'tools', 'rc', 'hwtest', 'hwtool', 'standing', 'bankrun'];

// The game's finer areas (--near AREA, an agent's working loop): by the dq0 tests' names (a test's near: [...] too)
const NEAR = {
  fight: ['dq0fight', 'dq0disarm', 'dq0shield', 'dq0armour', 'dq0senseless', 'dq0helpless', 'dq0finished', 'dq0robbed',
    'dq0disengage', 'dq0strike', 'dq0engage', 'dq0fighton', 'dq0flee', 'dq0batter', 'dq0battering', 'dq0break', 'dq0harm',
    'dq0rules', 'dq0rather', 'dq0dens'],
  parser: ['dq0cmd', 'dq0parse', 'dq0alias', 'dq0words', 'dq0wordsfix', 'dq0wordsthings', 'dq0choose', 'dq0prompt',
    'dq0answer', 'dq0several', 'dq0walkwords', 'dq0talk', 'dq0rather', 'dq0names'],
  map: ['dq0', 'dq0deep', 'dq0fog', 'dq0mapturn', 'dq0dig', 'dq0dug', 'dq0vast', 'dq0lookway', 'dq0layers', 'dq0plan',
    'dq0gate', 'dq0start0', 'dq0start1', 'dq0bearing', 'dq0travel', 'dq0stairtrap', 'dq0traps', 'dq0pure', 'dq0rebar',
    'dq0portc', 'dq0break', 'dq0soak', 'dq0kept', 'dq0steps', 'dq0walkwords'],
  gear: ['dq0outfit', 'dq0kit', 'dq0keys', 'dq0lair', 'dq0dens', 'dq0items', 'dq0equip', 'dq0money', 'dq0load', 'dq0usedup',
    'dq0examine', 'dq0inside', 'dq0handy', 'dq0hands', 'dq0shield', 'dq0armour', 'dq0robbed', 'dq0reach', 'dq0several',
    'dq0names', 'dq0rebar', 'dq0strike'],
  money: ['dq0money', 'dq0outfit', 'dq0kit', 'dq0lair', 'dq0dens', 'dq0robbed', 'dq0load'],
  senses: ['dq0looks', 'dq0heard', 'dq0glow', 'dq0dark', 'dq0darksearch', 'dq0light', 'dq0scent', 'dq0traces', 'dq0stay',
    'dq0sound', 'dq0soundx', 'dq0watch', 'dq0lookway', 'dq0kept', 'dq0fog', 'dq0quiet', 'dq0pose'],
  denizens: ['dq0denizens', 'dq0popgen', 'dq0inhab', 'dq0reach', 'dq0sway', 'dq0talk', 'dq0names', 'dq0stay', 'dq0dens',
    'dq0traces', 'dq0lair', 'dq0robbed', 'dq0finished', 'dq0helpless'],
  theme: ['dq0theme', 'dq0pieces', 'dq0themegen', 'dq0goals', 'dq0themec', 'dq0history', 'dq0layers', 'dq0quest',
    'dq0handtheme', 'dq0plan', 'dq0inhab', 'dq0popgen'],
  time: ['dq0longwait', 'dq0tasks', 'dq0timed', 'dq0pace', 'dq0steps', 'dq0tiring', 'dq0watch', 'dq0rather', 'dq0stay',
    'dq0usedup', 'dq0answer'],
  body: ['dq0limbs', 'dq0legs', 'dq0hands', 'dq0handy', 'dq0harm', 'dq0tiring', 'dq0senseless', 'dq0helpless', 'dq0disarm',
    'dq0pose', 'dq0quiet'],
  save: ['dq0save', 'dq0kept', 'dq0soak', 'dq0quest', 'dq0vast'],
  screen: ['dq0vera', 'dq0veraplain', 'dq0veraserial', 'dq0prompt', 'dq0sound', 'dq0soundx', 'dq0answer', 'dq0mapturn'],
};
const nearOf = new Map();
for (const [a, names] of Object.entries(NEAR)) for (const n of names) { if (!nearOf.has(n)) nearOf.set(n, []); nearOf.get(n).push(a); }

// A test's areas (text: its entry's and its helpers', select.js's parseTests: null, unknown)
function areasOf(t, text) {
  if (t.system === 'base') return base.areasOf(t);
  const a = new Set();
  for (const m of [t.init, ...(t.modules || [])]) if (m) a.add('mod:' + m);
  if (/^dq0/.test(t.name)) a.add('dq0');
  if (/^hmchar/.test(t.name)) a.add('hmchar');
  if (t.sdcard) a.add('sdcard').add('c');
  if (t.pc) a.add('pc');
  if (LAYOUT.includes(t.name)) a.add('layout');
  for (const [w, re] of Object.entries(WORDS)) if (text == null || re.test(text)) a.add(w);
  for (const n of [...(nearOf.get(t.name) || []), ...[].concat(t.near || [])]) a.add(n);
  for (const x of [].concat(t.area || [])) a.add(x);
  return a;
}

// The parser's own checks (TESTING.md's report tier: when the grammar's changed): these paths
const GRAMMAR = [/^reborn\/games\/dq0\/commands\./, /^reborn\/games\/dq0c\//, /^reborn\/sim\/tools\/(command\w*\.js|cmdmatch\/)/,
  /^reborn\/games\/dq0\/resolve\.c$/];

// The selector's own check (the tiers test's: nothing a tier skips can break silently): the tests files read as
// entries, every test with its text; paths to the tests they run (CASES: '*' every test, 'none' none, or the tests
// that must run, has, and those that mustn't, not; a NAME* every test of that start); lines of tests.js to their
// tests; every area a rule names some test's; NEAR's names tests.  OUT: { failures, notes }
const CASES = [
  ['base/kernel/sched.s', '*'], ['base/sim/lib/cpu65c02.js', '*'], ['base/tests/tests.js', { has: ['tiers'], not: ['dq0'] }],
  ['reborn/modules/rc/rc.s', '*'], ['reborn/modules/cons/cons.s', '*'], ['reborn/sim/test.js', '*'], ['reborn/tools/romimg.js', '*'],
  ['reborn/build.js', '*'], ['reborn/sdk/asm/include/hydra.inc', '*'], ['reborn/tests/areas.js', '*'], ['base/sim/select.js', '*'],
  ['a/path/no/rule/names.txt', '*'], ['reborn/tests/newfile.js', '*'],
  ['reborn/games/dq0w/wfight.c', { has: ['dq0*', 'hmchar*'], not: ['boot', 'basic', 'forth', 'hylang', 'rc', 'race', 'base'] }],
  ['reborn/games/dq0/commands.ebnf', { has: ['dq0*'], not: ['boot', 'basic'] }],
  ['reborn/bank/popgen/popgen.c', { has: ['dq0*', 'dq0popgen'], not: ['boot'] }],
  ['reborn/games/dq0/HOUSE.md', 'none'], ['docs/x.md', 'none'], ['manual/x.md', 'none'], ['board/x.kicad_pcb', 'none'],
  ['reborn/modules/basic/basic.s', { has: ['basic', 'bsuite', 'bplay', 'rom'], not: ['dq0*', 'forth', 'task', 'base'] }],
  ['reborn/modules/forth/forth.s', { has: ['forth', 'hyforth', 'flib'], not: ['dq0*', 'basic', 'base'] }],
  ['reborn/forthlib/double.s', { has: ['forth'], not: ['dq0*'] }],
  ['reborn/modules/hylang/hylang.s', { has: ['hylang', 'hysuite1', 'hyhydra'], not: ['dq0*', 'basic'] }],
  ['reborn/modules/asm/asm.s', { has: ['as', 'dis', 'bsuite', 'forth'], not: ['dq0*'] }],
  ['reborn/modules/numbers/num.s', { has: ['numbers', 'basic', 'hylang', 'forth', 'cnum', 'dq0'], not: ['task', 'base'] }],
  ['reborn/tests/mod/t_task/t_task.s', { has: ['task'], not: ['dq0*', 'boot'] }],
  ['reborn/tests/ram/t_rules/rules.txt', { has: ['dq0rules'], not: ['boot', 'dq0fight'] }],
  ['reborn/tests/basic/arith.bas', { has: ['bsuite'], not: ['dq0*'] }],
  ['reborn/tests/hylang/x.hl', { has: ['hysuite1'], not: ['dq0*'] }],
  ['reborn/tests/numtest.js', { has: ['numbers'], not: ['dq0*', 'boot'] }],
  ['reborn/programs/edit/edit.s', { has: ['edit', 'rom', 'init'], not: ['dq0fight', 'basic'] }],
  ['reborn/sdk/c/lib/dice.c', { has: ['dq0*', 'race', 'c'], not: ['boot', 'forth'] }],
  ['reborn/sdcard/songs/vera.mml', { has: ['dq0*', 'race', 'tools'], not: ['boot', 'forth'] }],
  ['reborn/bin/sdcard.img', 'none'], ['reborn/bin/prom0.bin', 'none'], ['base/bin/bios_rockwell.bin', 'none'],
  ['reborn/sim/lib/dq0meaning.js', { has: ['dq0*'], not: ['boot', 'race'] }],
  ['reborn/sim/lib/pchost.js', { has: ['pc', 'pc-img', 'dq0rules'], not: ['boot'] }],
  ['reborn/sim/dl/cpu.dl', 'none'], ['reborn/sim/tools/soak.js', 'none'], ['reborn/sim/tiers.js', 'none'],
];
function selfCheck() {
  const path = require('path'), sel = require('../../base/sim/select.js');
  const top = path.join(__dirname, '..', '..');
  const bt = require('../../base/tests/tests.js').tests.map(t => Object.assign(t, { system: 'base' })), rt = require('./tests.js').tests;
  const tests = [...bt, ...rt], failures = [], notes = [];
  const tt = sel.testTexts(top, { 'base/tests/tests.js': bt, 'reborn/tests/tests.js': rt });
  if (!tt.ok) failures.push('a tests file not read as entries (select.js parseTests): --changed runs every test for it');
  const unread = tests.filter(t => !tt.texts.has(t.name)).map(t => t.name);
  if (unread.length) failures.push('no entry found for ' + unread.join(', ') + ' (select.js parseTests)');
  if (new Set(tests.map(t => t.name)).size !== tests.length) failures.push('two tests of one name');
  const of = sel.areasFn(module.exports, tt), ctx = { rules, areasOf: of, texts: tt.texts, entries: () => ({ names: new Set(['tiers']), all: null }) };
  const match = (p, n) => p.endsWith('*') ? n.startsWith(p.slice(0, -1)) : n === p;
  for (const [file, want] of CASES) {
    const run = sel.select(tests, Object.assign({ files: [file] }, ctx)).run;
    if (want === '*') { if (run.size !== tests.length) failures.push(file + ': ' + run.size + ' of ' + tests.length + ' tests, not every one'); continue; }
    if (want === 'none') { if (run.size) failures.push(file + ': ' + run.size + ' tests, not none'); continue; }
    for (const p of want.has) { const t = tests.filter(t => match(p, t.name)); if (!t.length || t.some(x => !run.has(x.name))) failures.push(file + ': not all of ' + p + ' run'); }
    for (const p of want.not) if (tests.some(t => match(p, t.name) && run.has(t.name))) failures.push(file + ': ' + p + ' run');
  }
  // tests.js's lines: an entry's its test's; a helper's the tests that use it; the header's comment none; the
  // exports' and the requires every test's
  const p = tt.parsed['reborn/tests/tests.js'];
  if (p) {
    const line = re => p.lines.findIndex(l => re.test(l)) + 1, names = re => [...sel.namesForLines(p, [line(re)]).names].sort().join(' ');
    const one = (re, want, what) => { const r = sel.namesForLines(p, [line(re)]); const got = r.all ? '*' : [...r.names].sort().join(' '); if (!want(got, r)) failures.push('tests.js\'s ' + what + ': ' + (got.length > 80 ? got.slice(0, 80) + '...' : got || 'none')); };
    one(/^      name: 'dq0shield'/, g => g === 'dq0shield', 'dq0shield\'s entry');
    one(/^function hylangCard/, g => /\bhysuite1\b/.test(g) && /\bhyhydra\b/.test(g) && !/\bdq0\b/.test(g), 'hylangCard (a helper)');
    one(/^const DQ0_ANSWERS/, g => g === 'dq0answer', 'DQ0_ANSWERS (a helper of a test made by code)');
    one(/^\/\/ tests\.js - /, (g, r) => !r.all && !g, 'header comment');
    one(/^const fs = require/, (g, r) => !!r.all, 'require line');
    one(/^  IRQ_OFF_MAX,/, (g, r) => !!r.all, 'exports');
    if (!names(/^      name: 'tiers'/).includes('tiers')) failures.push('tests.js\'s tiers entry not read');
  }
  // Every area a rule names, some test's; NEAR's names, tests
  const sets = tests.map(of), special = a => ['*', 'none', 'entries', 'dl'].includes(a) || /^word:|\$/.test(a);
  for (const r of rules) for (const a of r[1]) if (!special(a) && !sets.some(s => s.has(a))) failures.push('rule ' + r[0] + ': no test of area ' + a);
  for (const [a, ns] of Object.entries(NEAR)) for (const n of ns) if (!tests.some(t => t.name === n)) failures.push('NEAR ' + a + ': no test ' + n);
  const bare = rt.filter(t => /^dq0/.test(t.name) && !Object.keys(NEAR).some(a => of(t).has(a))).map(t => t.name);
  if (bare.length) notes.push('dq0 tests in no NEAR area (tests/areas.js, or the entry\'s near: [...]): ' + bare.join(', '));
  notes.push(CASES.length + ' paths checked; ' + tests.length + ' tests read as entries');
  return { failures, notes };
}

module.exports = { rules, areasOf, WORDS, LAYOUT, NEAR, GRAMMAR, CASES, selfCheck };
