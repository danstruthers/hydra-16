// ****************************************************************************
// tests.js - the tests sim/test.js runs, in order: the index of the suites (suites/: the system's, the languages',
// the sound's, the screen's ...; suites/dq0/: the game's, by subject), their helpers in lib/ (common.js, dq0.js).  Each boots a paged ROM of the system's modules and its own
// (tests/mod), with one of them as init, and runs till init says "NAME: PASS" or "NAME: FAIL" (or the cycles run
// out).  A test module's lines are "ok - ..." or "not ok - ..." (tests/mod/testlib.inc).
//
//   name, what       the test
//   init             the module started as init (a test module, or the system's init)
//   modules          more modules for the image (the system's are always there, but those in without)
//   without          system modules left out (a test that owns the serial port itself, or needs its driver as
//                    task F)
//   cycles           at most this many (3.58 MHz: 3579545 a second)
//   expect           lines the output must have (for a test without "PASS")
//   machine          the emulator's options for it (sim/lib/machine.js): faults, keys typed (input), modules
//   send             { after: a mark, bytes: [...] }: the PC sends them, back to back, when the mark is out
//   pc               { files, readOnly, damage }: a folder at /pc, the emulator the PC tool (sim/test.js: pcFolder);
//                    check(m) has it as m.pc ({ dir, host })
//   budgets          [{ what, from, to, minus, per, max }]: the cycles between two marks (less those between the
//                    two marks in minus, a baseline), divided by per, at most max (a number, or a function of the
//                    build's options: { clock, acia }, obj/build.json)
//   check(m, out)    more checks on the machine afterwards: gives a list of failures
//   start(m)         the machine as it's made, before it runs (to listen to its sound, say)
// Every test also checks the longest IRQs-off stretch after the boot (IRQ_OFF_MAX).
'use strict';
const { IRQ_OFF_MAX, sdcardOf } = require('./lib/common.js');
const { dq0Check, dq0Rec } = require('./lib/dq0.js');

// The suites, in their order (sim/test.js runs them so; the selector, ../../base/sim/select.js, reads each as a tests file:
// files, its path from the repository's top -> the tests it makes; helpers, the modules whose helpers they use)
const SUITES = [
  'suites/system.js',
  'suites/forth.js',
  'suites/basic.js',
  'suites/hylang.js',
  'suites/numbers.js',
  'suites/sound.js',
  'suites/pc.js',
  'suites/screen.js',
  'suites/hmchar.js',
  'suites/dq0/map.js',
  'suites/dq0/parser.js',
  'suites/dq0/gear.js',
  'suites/dq0/fight.js',
  'suites/dq0/denizens.js',
  'suites/dq0/senses.js',
  'suites/dq0/body.js',
  'suites/dq0/time.js',
  'suites/dq0/save.js',
  'suites/dq0/theme.js',
  'suites/dq0/screen.js'
];
const files = {}, tests = [];
for (const f of SUITES) { const list = require('./' + f).tests; files['reborn/tests/' + f] = list; tests.push(...list); }

module.exports = { IRQ_OFF_MAX, tests, files, helpers: ['reborn/tests/lib/common.js', 'reborn/tests/lib/dq0.js'], dq0Check, sdcardOf,
  dq0Rec };                                         // (sdcardOf: sim/tools/soak.js's games, the SD card's in device 0; dq0Rec: its scenes')

// The tests that run a sample, or play a song of the SD card's: its image in SD device 0
for (const t of tests) if (t.sdcard) sdcardOf(t);
