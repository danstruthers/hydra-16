#!/usr/bin/env node
// ****************************************************************************
// test.js - HydraOS's regression tests (tests/tests.js), and the base's before them: the base's runner
// (../../base/sim/test.js: its options, -v, -j N, --seed, --build, --dl, are there) with HydraOS's images (its
// modules/rom.txt's modules, less a test's without, and the test's own; the ROM disk), its PC folders (/pc: a test's
// pc), the danlang emulator (sim/dl) and its build; and the hub writers' check (sim/lib/hubown.js: hub.h's owners'
// table) in every Dingus Quest Zero test (dq0...): a write to the hub's banks by a task its region doesn't allow, and
// not known (the table's moves), fails the test.  HUBOWN=1: each test's every violation said, known or not, and a
// summary of them all after the tests (grouped by the move that's to end them); HUBOWN=0: no check.  And in every dq0
// test (HUBOWN=0 too), nothing told twice in one answer, nor a chunk's door or trap made twice (tests.js's dq0Check);
// and what happened recorded, the hub's events, replies and state a line at a time (sim/lib/dq0meaning.js: m.dq0, for
// the tests' meaning checks, tests.js's dq0Turns).
// RULES=NAME (null ...): the dq0 tests run with that ruleset's referee (games/dq0/RULESETS.md: dq0 -r NAME added to
// each dq0 line typed), and with no test named, those that are meaning checks (tests.js's meaning: true: their checks
// what happened, not its words) are the ones run (of --near's, or --changed's, with them).
// The tests a change can affect (--changed [REF], --near AREA, --list: the base's runner's, ../base/sim/select.js, with
// tests/areas.js's table), and the tiers of checks (--tier report|lead: sim/tiers.js, tests/TESTING.md).
//
// Usage: [HUBOWN=0|1] [RULES=NAME] node sim/test.js [NAME ...] [--changed [REF]] [--near AREA,...] [--list] [--build]
//          [-v] [--seed N] [-j N] [--dl]
//        node sim/test.js --tier report|lead [--changed REF] [-j N] [--seed N] [--list]
'use strict';
const fs = require('fs');
const path = require('path');
const runner = require('../../base/sim/test.js');
const { createPcHost } = require('./lib/pchost.js');
const hydrafs = require('./tools/hydrafs.js');
const romimg = require('../tools/romimg.js');
const images = require('../../base/tools/images.js');
const romfs = require('../tools/romfs.js');
const { readManifest, hwtest } = require('../build.js');
const { tests, IRQ_OFF_MAX, dq0Check } = require('../tests/tests.js');
const hubown = require('./lib/hubown.js');
const dq0meaning = require('./lib/dq0meaning.js');

const ROOT = path.join(__dirname, '..');
const bin = (dir, n) => fs.readFileSync(path.join(ROOT, 'obj', dir, n + '.bin'));

function image(t) {
  const sys = readManifest(path.join(ROOT, 'modules', 'rom.txt')).modules.filter(n => !(t.without || []).includes(n));
  const own = [...(t.modules || [])];
  if (!sys.includes(t.init) && !own.includes(t.init)) own.unshift(t.init);
  const own_ = n => {                                     // (A test's, the SDK's, HydraOS's; or the base's: a test
    const d = ['tests', 'samples', 'modules'].find(d => fs.existsSync(path.join(ROOT, 'obj', d, n + '.bin')));   //   module, a module
    if (d) return bin(d, n);
    const b = ['tests', 'modules'].find(d => fs.existsSync(path.join(ROOT, '..', 'base', 'obj', d, n + '.bin')));
    return fs.readFileSync(path.join(ROOT, '..', 'base', 'obj', b || 'modules', n + '.bin'));
  };
  const mods = [...sys.map(n => ({ file: n, data: bin('modules', n) })), ...own.map(n => ({ file: n, data: own_(n) }))];
  return romimg.build({ modules: mods, init: t.init, hwtest: hwtest(), bios: images.bios(ROOT, images.built(ROOT)),
    romfs: romfs.manifest(path.join(ROOT, 'romfs', 'romfs.txt')) }).image;
}

// A test's PC folder (t.pc: { files: { name: text or bytes (or a function giving them), 'dir/': '' } (or a function
// giving that), readOnly, damage }), made afresh in
// obj/pc/NAME, its files stamped 2026-10-03 15:04:05; and the PC tool for it (sim/lib/pchost.js).  OUT: { dir, host }
// With t.pc.image ({ mb, label }): the files in a HydraFS image instead, obj/pc/NAME.img, which the PC tool serves
// as it does a folder (sim/tools/pcfs.js); what's written to it in the test is stamped the same.  OUT: { dir: the
// image file, image: the same, host }
function pcFolder(t, opt) {
  const dir = path.join(ROOT, 'obj', 'pc', t.name), when = new Date(2026, 9, 3, 15, 4, 5);
  fs.rmSync(dir, { recursive: true, force: true });
  const files = typeof t.pc.files === 'function' ? t.pc.files() : t.pc.files || {};
  if (t.pc.image) {
    const img = dir + '.img';
    fs.mkdirSync(path.dirname(img), { recursive: true });
    const now = Date.UTC(2026, 9, 3, 15, 4, 5) / 1000 - Date.UTC(2000, 0, 1) / 1000;
    hydrafs.setNow(now);
    hydrafs.mkfs(img, t.pc.image.mb || 1, t.pc.image.label || '', undefined, true);
    const v = new hydrafs.Volume(img);
    for (const [n, data] of Object.entries(files)) {
      if (n.endsWith('/')) v.mkdir(n);
      else { if (n.includes('/')) v.mkdir(path.posix.dirname(n)); v.put(n, Buffer.from(typeof data === 'function' ? data() : data)); }
    }
    v.close();
    const host = createPcHost({ dir: img, readOnly: !!t.pc.readOnly, damage: t.pc.damage || [], now,
      log: opt.verbose ? s => console.log('  [pc] ' + s) : undefined });
    return { dir: img, image: img, host };
  }
  fs.mkdirSync(dir, { recursive: true });
  for (const [n, data] of Object.entries(files)) {
    const f = path.join(dir, n);
    if (n.endsWith('/')) fs.mkdirSync(f, { recursive: true });
    else { fs.mkdirSync(path.dirname(f), { recursive: true }); fs.writeFileSync(f, typeof data === 'function' ? data() : data); }
  }
  const stamp = d => { for (const n of fs.readdirSync(d)) { const f = path.join(d, n); if (fs.statSync(f).isDirectory()) stamp(f); fs.utimesSync(f, when, when); } };
  stamp(dir);
  const host = createPcHost({ dir, readOnly: !!t.pc.readOnly, damage: t.pc.damage || [],
    log: opt.verbose ? s => console.log('  [pc] ' + s) : undefined });
  return { dir, host };
}

// The hub writers' check, a dq0 test's (HUBOWN=0: none): its violations each in obj/hubown/NAME.json, for the summary
const HUBOWN = process.env.HUBOWN || '';
const hubDir = path.join(ROOT, 'obj', 'hubown');
function instrument(t) {
  if (!/^dq0/.test(t.name)) return null;
  const r = dq0meaning.createRecorder(), on = mm => { r.attach(mm); mm.dq0 = r; };   // (What happened: m.dq0, tests.js's dq0Turns)
  if (HUBOWN === '0') return { boot: { sharedWrite: r.hook }, attach: on, judge: m => ({ failures: dq0Check(t.name, m), notes: [] }) };
  const M = hubown.map(), c = hubown.createCheck(M);
  return { boot: { sharedWrite: (k, o, v, T, pc, W) => { c.hook(k, o, v, T, pc, W); r.hook(k, o, v); } }, attach: mm => { c.attach(mm); on(mm); }, judge: m => {
    const r = c.result(), { known, fresh } = hubown.judge(r), notes = [], failures = [...fresh.map(e => 'hub: ' + hubown.line(e)), ...dq0Check(t.name, m)];
    try { fs.mkdirSync(hubDir, { recursive: true }); fs.writeFileSync(path.join(hubDir, t.name + '.json'), JSON.stringify(r)); } catch (e) { /* (No summary) */ }
    if (!r.found) return { failures, notes: ['hub writers: no hub made'] };
    const moves = [...new Set(known.map(e => e.known))].sort();
    notes.push('hub writers: ' + r.writes + ' writes checked, ' + known.length + ' known violation(s)' + (moves.length ? ' (moves ' + moves.join(', ') + ')' : '') +
      ', ' + fresh.length + ' new' + (r.stray.length ? '; ' + r.stray.map(e => e.count + ' by ' + e.task).join(', ') + ' (no role)' : ''));
    if (HUBOWN === '1') for (const e of known) notes.push('  ' + hubown.line(e));
    return { failures, notes };
  } };
}
// After the tests (HUBOWN=1): every violation seen, the tests' together, by the move that's to end it (NEW: none),
// and the table's known ones none of the tests showed
function summary(list) {
  if (HUBOWN !== '1') return;
  const all = new Map();
  for (const t of list) {
    let r = null;
    try { r = JSON.parse(fs.readFileSync(path.join(hubDir, t.name + '.json'), 'utf8')); } catch (e) { continue; }
    for (const e of r.regions) {
      const k = e.line + ' ' + e.region.replace(/\[\d+\]/g, '[*]') + ' ' + e.writer, a = all.get(k);
      if (a) { a.count += e.count; a.tests.add(t.name); if (!a.pcs.includes(e.pc)) a.pcs.push(e.pc); }
      else all.set(k, Object.assign({}, e, { region: e.region.replace(/\[\d+\]/g, '[*]'), tests: new Set([t.name]), pcs: [e.pc] }));
    }
  }
  const by = new Map();
  for (const e of all.values()) { const k = e.known || 'NEW'; if (!by.has(k)) by.set(k, []); by.get(k).push(e); }
  console.log('hub writers: ' + all.size + ' violation(s) over ' + list.filter(t => /^dq0/.test(t.name)).length + ' dq0 tests');
  for (const k of [...by.keys()].sort()) {
    console.log('  ' + (k === 'NEW' ? 'NEW (not in hub.h\'s table)' : 'move ' + k) + ':');
    for (const e of by.get(k).sort((x, y) => x.line - y.line || x.writer.localeCompare(y.writer)))
      console.log('    ' + e.region + ' (' + e.owner + '\'s) by the ' + e.writer + ': ' + e.count + ' writes in ' + e.tests.size +
        ' test(s), at ' + e.pcs.slice(0, 4).join(', ') + (e.pcs.length > 4 ? ' ...' : ''));
  }
  const seen = new Set([...all.values()].map(e => e.line + ' ' + e.writer));
  const idle = hubown.table().flatMap(l => l.known.filter(k => !seen.has(l.line + ' ' + k.task)).map(k => l.region + ' by the ' + k.task + ' (move ' + k.move + ')'));
  if (idle.length) console.log('  known, but none of these tests wrote it: ' + idle.join('; '));
}

// RULES=NAME: each dq0 test's machine with dq0 -r NAME typed (its input's dq0 lines, after a prompt), and the list
// to run, if none's named, the meaning checks
const RULES = process.env.RULES || '';
function rulesOf(t) {
  const d = Object.getOwnPropertyDescriptor(t, 'machine');
  Object.defineProperty(t, 'machine', { configurable: true, enumerable: true, get() {
    const m = d ? (d.get ? d.get.call(this) : d.value) : {};
    if (typeof m.input !== 'string') return m;
    return Object.assign({}, m, { input: m.input.replace(/(\u0101dq0(?: [^\r]*)?)\r/g, '$1 -r ' + RULES + '\r') });
  } });
}
if (RULES) for (const t of tests) if (/^dq0/.test(t.name)) rulesOf(t);

// Every dq0 test's dq0 lines with -a (games/dq0: a long task goes on at every soft question, "Keep walking?", unasked:
// slice 4.0's questions only in the tests about them, those with asks: true), so a test's next line isn't taken for
// an answer
function asksOf(t) {
  const d = Object.getOwnPropertyDescriptor(t, 'machine');
  Object.defineProperty(t, 'machine', { configurable: true, enumerable: true, get() {
    const m = d ? (d.get ? d.get.call(this) : d.value) : {};
    if (typeof m.input !== 'string') return m;
    return Object.assign({}, m, { input: m.input.replace(/(\u0101dq0(?: [^\r]*)?)\r/g, '$1 -a\r') });
  } });
}
for (const t of tests) if (/^dq0/.test(t.name) && !t.asks) asksOf(t);
// And with -I (no engage initiative test, nor a grope's test while engaged: HOUSE.md's, the user's way to keep the
// tests' fights' dice as they were), but in the tests of them, those with initiative: true
function initiativeOf(t) {
  const d = Object.getOwnPropertyDescriptor(t, 'machine');
  Object.defineProperty(t, 'machine', { configurable: true, enumerable: true, get() {
    const m = d ? (d.get ? d.get.call(this) : d.value) : {};
    if (typeof m.input !== 'string') return m;
    return Object.assign({}, m, { input: m.input.replace(/(\u0101dq0(?: [^\r]*)?)\r/g, '$1 -I\r') });
  } });
}
for (const t of tests) if (/^dq0/.test(t.name) && !t.initiative) initiativeOf(t);
// And with -N (the character's bearings never tested, as one that knows north's: slice 4.2's, the tests' compass words
// going where they went), but in the tests of them, those with bearings: true
function bearingsOf(t) {
  const d = Object.getOwnPropertyDescriptor(t, 'machine');
  Object.defineProperty(t, 'machine', { configurable: true, enumerable: true, get() {
    const m = d ? (d.get ? d.get.call(this) : d.value) : {};
    if (typeof m.input !== 'string') return m;
    return Object.assign({}, m, { input: m.input.replace(/(\u0101dq0(?: [^\r]*)?)\r/g, '$1 -N\r') });
  } });
}
for (const t of tests) if (/^dq0/.test(t.name) && !t.bearings) bearingsOf(t);
// And with -K (the kit as it was before the outfits, the denizens with none of theirs: the tests' kit, their dice as
// they were, 2026-10-27), but in the tests of the outfits, those with outfit: true
function kitOf(t) {
  const d = Object.getOwnPropertyDescriptor(t, 'machine');
  Object.defineProperty(t, 'machine', { configurable: true, enumerable: true, get() {
    const m = d ? (d.get ? d.get.call(this) : d.value) : {};
    if (typeof m.input !== 'string') return m;
    return Object.assign({}, m, { input: m.input.replace(/(\u0101dq0(?: [^\r]*)?)\r/g, '$1 -K\r') });
  } });
}
for (const t of tests) if (/^dq0/.test(t.name) && !t.outfit) kitOf(t);
// DQ0_ROLLS=N: every dq0 line typed with -R N (the dice's own offset: the same dungeon, other rolls), to find again
// the rolls a test was written for when the dice change (the dice by who asks, 2026-10-30); a test's own -R kept.
// DQ0_ROLLS=A-B: each dq0 test once more for each N from A to B, named NAME@N (node sim/test.js dq0cmd@1 dq0cmd@2 ...)
// DQ0_ROLL_LINE=K as well: only the test's K'th dq0 line (from 1) given -R N, its others as written (a test of several
// games, each game's rolls found in turn)
function rollsOf(t, n) {
  if (!(+n >= 0 && +n <= 252)) throw new Error('DQ0_ROLLS: an offset 0 to 252 (dq0 -R\'s, rshouse.h\'s DICE_BEGIN: 3 + n a byte)');
  const d = Object.getOwnPropertyDescriptor(t, 'machine'), only = +(process.env.DQ0_ROLL_LINE || 0);
  Object.defineProperty(t, 'machine', { configurable: true, enumerable: true, get() {
    const m = d ? (d.get ? d.get.call(this) : d.value) : {};
    if (typeof m.input !== 'string') return m;
    let k = 0;
    return Object.assign({}, m, { input: m.input.replace(/(ādq0(?: [^\r]*)?)\r/g, (l, c) => (++k, only && k !== only) || / -R \d/.test(c) ? l : c + ' -R ' + n + '\r') });
  } });
  return t;
}
if (/^\d+$/.test(process.env.DQ0_ROLLS || '')) for (const t of tests) if (/^dq0/.test(t.name)) rollsOf(t, process.env.DQ0_ROLLS);
if (/^\d+-\d+$/.test(process.env.DQ0_ROLLS || '')) {
  const [a, b] = process.env.DQ0_ROLLS.split('-').map(Number), more = [];
  for (const t of tests.filter(t => /^dq0/.test(t.name))) for (let n = a; n <= b; n++) {
    const c = Object.defineProperties(Object.create(Object.getPrototypeOf(t)), Object.getOwnPropertyDescriptors(t));
    c.name = t.name + '@' + n;
    more.push(rollsOf(c, n));
  }
  tests.push(...more);
}
// RULES=NAME and no test named: of the list (all, or --near's), the meaning checks (the runner's restrict)
function meaningOf(list) {
  if (!RULES) return list;
  if (!tests.some(t => /^dq0/.test(t.name) && t.meaning)) { console.error('RULES=' + RULES + ': no dq0 test is a meaning check (meaning: true)'); process.exit(2); }
  return list.filter(t => /^dq0/.test(t.name) && t.meaning);
}

runner.setup({ root: ROOT, tests, IRQ_OFF_MAX, image, pcFolder, self: __filename, instrument, summary,
  build: () => require('../build.js').buildBoth({ quiet: true }),
  runDl: (...a) => require('./dl/bridge.js').runDl(...a),
  areas: require('../tests/areas.js'), ref: 'reborn-game', restrict: meaningOf,
  testFiles: { 'base/tests/tests.js': require('../../base/tests/tests.js').tests, 'reborn/tests/tests.js': tests } });

if (require.main === module) {
  const argv = process.argv.slice(2), k = argv.indexOf('--tier');
  if (k >= 0) require('./tiers.js').main(argv[k + 1], [...argv.slice(0, k), ...argv.slice(k + 2)]);
  else runner.main(argv);
}
module.exports = { runTest: runner.runTest, runTestDl: runner.runTestDl, image };
