// ****************************************************************************
// scenes.js - the tests' scenes (games/dq0/ENGINE-CRITIQUE.md §9): a Dingus Quest Zero test taken up from a saved
// machine instead of typed to its first moment.  Most dq0 tests boot, make a character (hmchar), start the game and
// wait while it grows its first dungeon (some 250M of a test's cycles) before their first command; a scene is the
// whole machine (machine.js's state: its RAM, the CPU, the devices, the output so far, the cards' blocks written) and
// the test's watchers' (the hub's recorder, the hub writers' check) as such a start leaves it, saved in obj/scenes and
// put back by the next test that starts the same way.
//
// Where a test is taken up (points): at the prompt waits ('ā' in its input) just before its first dq0 line (the
// system booted, the character made: rc's prompt shown) and just after it (the game begun: its first prompt shown).
// While the input waits for a prompt nothing that comes after the wait reaches the machine (acia.js: the wait is
// all the serial port looks at), so the machine there is the same for every test whose input is the same up to it.
// A scene's key: its input up to the point, and everything else the machine's run to there depends on: the ROM's
// image, the BIOS, the SD card's image (sha1 of each), the power-up's seed, the serial chip, the RAM modules, the
// core, the hub writers' check (HUBOWN), the guards (GUARDS), and the code that runs it (the emulator, the runner,
// the recorders and the guards: sha1 of their sources).  A rebuilt image is a new key; obj/scenes keeps the newest
// (SCENES_KEEP files, 400).
//
// A test with watchers of its own (t.start: dq0Poke and the like, on what's typed and shown) saves scenes of its own:
// its name and its suite's and the helpers' sources are in their keys, so its watchers' pokes are never left in
// another test's machine.  It's taken up from its own scene, or one a test without watchers saved.  As one is put
// back its watchers are made (t.start) and given what the serial port gave before the point (the bytes read, the
// bytes shown, at their cycles; the hub not there for those before it was made), so they stand as they would have.
// A test whose watchers see more than that before the point (the hub's writes as the game grows) is scene: false
// (dq0layers, dq0looks: a run with scenes and one without differed).  What can't be saved isn't (machine.js's state
// null: the VERA, a /pc ...); a test with marks (budgets, send), a /pc, its own machine's options but the input and
// the SD card, or scene: false, has none.
//
// SCENES=0: none (each test typed to its moment, as before); SCENES=fresh: none put back, each saved anew; else
// (SCENES=1, the default) put back when there, saved when not.  SCENES_TRACE=DIR: each dq0 test's end (its cycles,
// its output's hash, its failures) in DIR/NAME.json, to hold a run with scenes to one without.
'use strict';
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const zlib = require('zlib');
const v8 = require('v8');

const ROOT = path.join(__dirname, '..', '..');
const DIR = path.join(ROOT, 'obj', 'scenes');
const FORMAT = 1;
const KEEP = +(process.env.SCENES_KEEP || 400);
const sha1 = (...parts) => { const h = crypto.createHash('sha1'); for (const p of parts) h.update(p); return h.digest('hex'); };

// The sources a scene's run depends on (the emulator, the runners, the watchers), and those of the tests (a test with
// watchers of its own: theirs)
let code = null, testsCode = null;
const sources = dirs => dirs.flatMap(d => fs.existsSync(d) ? fs.statSync(d).isDirectory() ? fs.readdirSync(d).filter(n => n.endsWith('.js')).sort().map(n => path.join(d, n)) : [d] : []);
const hashOf = files => sha1(...files.flatMap(f => [path.relative(ROOT, f), fs.readFileSync(f)]));
function codeHash() {
  return code || (code = hashOf(sources([path.join(ROOT, '..', 'base', 'sim', 'lib'), path.join(ROOT, '..', 'base', 'sim', 'run.js'),
    path.join(ROOT, '..', 'base', 'sim', 'test.js'), path.join(ROOT, 'sim', 'test.js'), path.join(__dirname, 'dq0meaning.js'), path.join(__dirname, 'hubown.js'), path.join(__dirname, 'guards.js'),
    __filename, path.join(ROOT, 'obj', 'gen', 'dq0hub.json')])));
}
function testsHash(name) {
  const t = path.join(ROOT, 'tests'), idx = require('../../tests/tests.js');
  const suite = Object.keys(idx.files).find(f => idx.files[f].some(x => x.name === name));
  return testsCode || (testsCode = hashOf(sources([path.join(t, 'tests.js'), path.join(t, 'lib'), ...(suite ? [path.join(ROOT, '..', suite)] : [path.join(t, 'suites'), path.join(t, 'suites', 'dq0')])])));
}
// A file's sha1, once a process (the SD card's image: 17 MB)
const files = new Map();
function fileHash(f) {
  const st = fs.statSync(f), k = f + ':' + st.size + ':' + st.mtimeMs;
  if (!files.has(k)) files.set(k, sha1(fs.readFileSync(f)));
  return files.get(k);
}

// The points a test's input can be taken up at: [the input's length before the wait] (before its first dq0 line, and
// after it), those with a wait there
function points(input) {
  const m = /ādq0(?: [^\r]*)?\r/.exec(input);
  if (!m) return [];
  const end = m.index + m[0].length, out = [];
  if (m.index > 0) out.push(m.index);
  if (input[end] === 'ā') out.push(end);
  return out;
}

// A save, written whole or not at all (a temporary file renamed); the folder kept to its newest KEEP
function write(file, s) {
  fs.mkdirSync(DIR, { recursive: true });
  const tmp = file + '.' + process.pid + '.tmp';
  fs.writeFileSync(tmp, zlib.deflateSync(v8.serialize(s), { level: 1 }));
  fs.renameSync(tmp, file);
  try {
    const all = fs.readdirSync(DIR).filter(n => n.endsWith('.scn'));
    if (all.length > KEEP) all.map(n => ({ n, t: fs.statSync(path.join(DIR, n)).mtimeMs })).sort((a, b) => a.t - b.t)
      .slice(0, all.length - KEEP).forEach(x => fs.rmSync(path.join(DIR, x.n), { force: true }));
  } catch (e) { /* (Another process at it) */ }
}
function read(file) {
  try { return v8.deserialize(zlib.inflateSync(fs.readFileSync(file))); } catch (e) { return null; }
}

// A test's scenes, or null (none for it).  t the test, tm its machine's options (t.machine, once), prom its ROM's
// image, opt the runner's ({ seed }), bios the BIOS's image, acia the chip.  OUT: { watch(m), restore(m, ins),
// after(m, ins), end(m, r) }: sim/test.js's runTest calls them (base/sim/test.js)
function of(t, tm, prom, opt, bios, acia) {
  const mode = process.env.SCENES === undefined ? '1' : process.env.SCENES, trace = process.env.SCENES_TRACE;
  const end = trace && /^dq0/.test(t.name) ? (m, r) => {
    fs.mkdirSync(trace, { recursive: true });
    fs.writeFileSync(path.join(trace, t.name + '.json'), JSON.stringify({ cyc: m.cpu.cyc, out: sha1(m.out), failures: r.failures, notes: r.notes, from: from }));
  } : () => {};
  let from = null;
  const none = { watch() {}, restore: () => false, after() {}, end };
  if (mode === '0' || !/^dq0/.test(t.name) || t.scene === false || opt.dl || t.pc || t.send || (t.budgets || []).length) return none;
  const input = tm.input;
  if (typeof input !== 'string' || Object.keys(tm).some(k => k !== 'input' && k !== 'sd')) return none;
  if ((tm.sd || []).some(c => !c.file || !c.state || !c.restore)) return none;
  const at = points(input);
  if (!at.length) return none;
  const own = typeof t.start === 'function';
  const base = { f: FORMAT, code: codeHash(), tests: null, image: sha1(prom), bios: sha1(bios), acia, seed: opt.seed,
    cards: (tm.sd || []).map(c => [c.dev, c.blocks, fileHash(c.file)]), modules: process.env.HYDRA_MODULES || '', core: process.env.HYDRA_CORE || '',
    hubown: process.env.HUBOWN || '', guards: process.env.GUARDS || '' };
  const fileOf = (p, mine = own) => path.join(DIR, sha1(JSON.stringify(mine ? Object.assign({}, base, { tests: [t.name, testsHash(t.name)] }) : base), input.slice(0, p)) + '.scn');
  let events = [], hubAt = -1, rec = true, todo = at.slice();   // (What the serial port gave so far, 4 numbers each: 0 read or 1 shown, the byte, the cycle, the access's)

  // What the serial port gives the CPU, and what it shows, at their cycles: innermost, before the watchers
  function watch(m) {
    const a = m.acia, read_ = a.read, shown = a.shown;
    const hub = () => { if (hubAt < 0 && m.dq0 && m.dq0.games.length) hubAt = events.length / 4; };
    a.read = (r, c) => { if (rec && r === 0 && a.rdrf) { hub(); events.push(0, a.rx, m.cpu.cyc, c); } return read_(r, c); };
    a.shown = (v, c) => { if (rec) { hub(); events.push(1, v, m.cpu.cyc, c); } shown(v, c); };
  }
  // A scene put back (the latest point there is), and the test's watchers made and given what came before it.
  // OUT: true (put back: t.start's called), false (none: the test runs from its start)
  function restore(m, ins) {
    if (mode === 'fresh') return false;
    for (let k = at.length - 1; k >= 0; k--) for (const mine of own ? [true, false] : [false]) {   // (Its own first, then one any test's start makes)
      const file = fileOf(at[k], mine), s = read(file);
      if (!s || s.at !== at[k] || s.cyc > t.cycles) continue;
      const out = s.machine.out.replace(/\r/g, '');                  // (Its result, or what it expects, before the point: not taken up)
      if (t.expect ? t.expect.every(e => out.includes(e)) : new RegExp('^' + t.init + ': (PASS|FAIL)', 'm').test(out)) continue;
      try { const now = new Date(); fs.utimesSync(file, now, now); } catch (e) { /* (Kept by its age: the newest wanted) */ }
      m.restore(s.machine);
      m.acia.rxQueue = [...input.slice(at[k])];
      if (ins) ins.restore(s.ins);
      events = s.events.slice(); hubAt = s.hubAt;
      todo = at.slice(k + 1);
      rec = todo.length > 0;
      from = at[k] === at[at.length - 1] && at.length > 1 ? 'game' : 'rc';
      if (own) replay(m, s);
      return true;
    }
    return false;
  }
  // The watchers made (t.start), on the serial port's ways as they are, and given what it gave before the point: the
  // hub not there for what came before it was made, the cycle each came at
  function replay(m, s) {
    const a = m.acia, read_ = a.read, shown = a.shown;
    let on = true;
    a.read = (r, c) => on ? a.rx : read_(r, c);
    a.shown = (v, c) => { if (!on) shown(v, c); };
    t.start(m);
    const real = m.dq0, before = real ? { bank: () => null, games: [], lines: [], layout: real.layout, final: () => null } : undefined;
    const rx = a.rx, rdrf = a.rdrf, cyc = m.cpu.cyc, ev = s.events;
    for (let i = 0; i < ev.length; i += 4) {
      if (real) m.dq0 = s.hubAt < 0 || i / 4 < s.hubAt ? before : real;
      m.cpu.cyc = ev[i + 2];
      if (ev[i] === 0) { a.rx = ev[i + 1]; a.rdrf = 1; a.read(0, ev[i + 3]); } else a.shown(ev[i + 1], ev[i + 3]);
    }
    a.rx = rx; a.rdrf = rdrf; m.cpu.cyc = cyc; m.dq0 = real; on = false;
  }
  // After each run of the machine: at a point (its wait the next of the input, the prompt shown), the scene saved
  function after(m, ins) {
    if (!todo.length) return;
    const a = m.acia, left = a.rxQueue.length, p = todo[0];
    if (left < input.length - p) { todo.shift(); rec = todo.length > 0; return after(m, ins); }   // (Gone past it: not saved)
    if (left !== input.length - p || a.rxQueue[0] !== 'ā' || !(a.promptFrom >= 0 && a.sent > a.promptFrom && /> $|% $/.test(a.tail))) return;
    todo.shift();
    rec = todo.length > 0;
    if (own && p === at[0] && at.length > 1) return;   // (A test's own scene before its game: never wanted, its own after it there)
    const file = fileOf(p);
    if (mode !== 'fresh' && fs.existsSync(file)) return;
    const st = m.state(), is = ins && ins.state ? ins.state() : null;
    if (!st || (ins && !is)) return;
    st.devs.acia.rxQueue = [];                                     // (The rest of the input: the test's own, put back)
    write(file, { at: p, cyc: m.cpu.cyc, machine: st, ins: is, events, hubAt });
  }
  return { watch, restore, after, end };
}

module.exports = { of, points };
