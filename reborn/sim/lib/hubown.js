// hubown.js - Dingus Quest Zero's hub writers' check (Phase 2: one authority on state): every write to the hub's
// banks, in a test, by the task that made it, held against the owners' table in games/dq0/hub.h (its "own" lines:
// each region's owner, those that write it by design, and those that write it still till a move of Phase 2's).
//
// map() paints the table's regions' bytes, each region's as the build worked it out (games/dq0/hubgen.js: the table
// read from hub.h's owners block, each region's C expressions, offsetof, sizeof, the banks' numbers, made numbers by
// cc65 as the game's built; obj/gen/dq0hub.json's own).  createCheck(map) gives { hook, attach(m), result() }: hook is the machine's opt.sharedWrite
// (machine.js: each write to a shared bank, before it's made); attach(m) gives it the machine once it's booted.  The
// hub is found as the UI makes it (its magic written at bank 0's start: the hub's banks a run of shared bank IDs
// from there, the kernel's segment), each game afresh; a write's task is the hub's task[] table's (a role, T_*),
// checked by its name (the kernel's task record) where it matters.  A write by a role a region doesn't allow (its
// owner, its + tasks, its known ones) is a violation; the UI's hub_create (its banks cleared, before the header's
// there) isn't counted.  result(): { writes, regions: [{ region, owner, writer, count, pc, from, known }], stray }
// (stray: writes to the hub's banks by tasks of none of its roles).  The cost: a call and a table's look a write to
// the shared banks, the slow way only for a write that isn't its region's owner's: none to be measured (dq0denizens
// and dq0sway, 2026-10-16: 119 s with it, 120 s without), so it's on in every dq0 test.
'use strict';
const fs = require('fs');
const path = require('path');

const ROOT = path.join(__dirname, '..', '..');                // (reborn/)
const hubgen = require('../../games/dq0/hubgen.js');
const PROGS = ['dq0', 'dq0w', 'dq0d', 'dq0' + (process.env.RULES || 'harn'), 'dq0s', 'dq0m', 'dq0p', 'dq0c'];  // (Each
                                                              //   role's program, hub.h's T_* order: the referee's
                                                              //   the ruleset's, RULES)
const TA_NAME = 0x0230;                                       // (A task's name in its RAM: base/sim/run.js's)

// The tasks' names (hub.h's T_*, lower case: ui world ...), as the build read them
const tasks = () => hubgen.load().tasks;

// The table's lines: { region, owner, plus: [task], known: [{ move, task }], line } (the build's: hub.h's owners block)
function table() {
  return hubgen.load().own.lines.map(({ region, owner, plus, known, line }) => ({ region, owner, plus, known, line }));
}

// The values of simple C expressions, by the build's layout of the headers (obj/gen/dq0hub.json: nothing compiled at
// test time): offsetof (struct S, PATH), sizeof (struct S), a header's name (ITEM_BANK (0) too).  OUT: [value]
function evaluate(exprs) {
  const H = hubgen.load();
  return exprs.map(e => {
    let m;
    if ((m = e.match(/^\s*offsetof\s*\(\s*struct\s+(\w+)\s*,\s*([^)]+?)\s*\)\s*$/))) return H.off(m[1], m[2]);
    if ((m = e.match(/^\s*sizeof\s*\(\s*struct\s+(\w+)\s*\)\s*$/))) return H.size(m[1]);
    if (H.C[e.trim()] !== undefined) return H.C[e.trim()];
    throw new Error('hubown.evaluate: ' + e + ' (offsetof, sizeof or a header\'s name)');
  });
}

// The map: the table's regions, and each byte of the hub's banks' region.  OUT: { banks, ntasks, taskOff, magic,
// regions: [{ name, owner, allowed (a mask of roles), known: { role: move }, line }] (regions[0]: none's),
// at: [Uint16Array(0x2000) a bank: each byte's region] }: the build's spans (hubgen.js's), painted line by line
function map() {
  const j = hubgen.load().own;
  // The regions (an each line's elements apart: each its own owner), painted line by line over those before
  const regions = [{ name: '(no region)', owner: -1, allowed: 0, known: {}, line: 0 }], at = [];
  for (let b = 0; b < j.banks; b++) at.push(new Uint16Array(0x2000));
  for (const l of j.lines) {
    const ids = new Map();
    for (const [b, from, to, e] of l.spans) {
      const owner = l.owner === 'each' ? (e >= 0 && e < tasks().length ? e : -1) : tasks().indexOf(l.owner);
      const k = l.owner === 'each' ? e : -1;
      if (!ids.has(k)) {
        let allowed = owner >= 0 ? 1 << owner : 0;
        for (const p of l.plus) allowed |= 1 << tasks().indexOf(p);
        const known = {};
        for (const { move, task } of l.known) known[tasks().indexOf(task)] = move;
        ids.set(k, regions.length);
        regions.push({ name: l.region.replace('[*]', k >= 0 ? '[' + k + ']' : '[*]'), owner, allowed, known, line: l.line });
      }
      at[b].fill(ids.get(k), from, to);
    }
  }
  return Object.assign({}, j, { regions, at });
}

// A program's labels (obj/games/NAME/NAME.lbl): [addr, name] sorted, and its swap area's start (a label there is
// one of its swapped parts', which of them not known: the nearest's name, "swapped"); and the UI's hub_create, its
// bytes (NAME.map: hub.o's SW1CODE, that function alone)
const lblCache = {};
function lbls(prog) {
  if (lblCache[prog]) return lblCache[prog];
  const out = { list: [], swap: 0x10000, create: null };
  try {
    for (const l of fs.readFileSync(path.join(ROOT, 'obj', 'games', prog, prog + '.lbl'), 'latin1').split(/\r?\n/)) {
      const k = l.match(/^al ([0-9A-F]+) \.(\S+)/);
      if (!k) continue;
      const a = parseInt(k[1], 16);
      if (k[2] === '__SWAP_START__') out.swap = a;
      if (k[2].startsWith('_') && !k[2].startsWith('__')) out.list.push([a, k[2].slice(1)]);
    }
    const map = fs.readFileSync(path.join(ROOT, 'obj', 'games', prog, prog + '.map'), 'latin1');
    const k = map.match(/^hub\.o:\r?\n(?:\s+\w+\s+Offs=.*\r?\n)*?\s+SW1CODE\s+Offs=[0-9A-F]+\s+Size=([0-9A-F]+)/m);
    const e = out.list.find(x => x[1] === 'hub_create');
    if (k && e) out.create = [e[0], e[0] + parseInt(k[1], 16)];
  } catch (e) { /* (None: addresses alone) */ }
  out.list.sort((x, y) => x[0] - y[0]);
  return (lblCache[prog] = out);
}
// Where pc is in prog: its function's name (+ the offset), or the address
function where(prog, pc) {
  const L = lbls(prog);
  let best = null;
  for (const e of L.list) { if (e[0] <= pc) best = e; else break; }
  const hex = '$' + pc.toString(16).toUpperCase().padStart(4, '0');
  if (!best || pc >= 0x8000) return hex;
  return best[1] + (pc >= L.swap ? ' (swapped)' : '') + ' ' + hex;
}

function createCheck(M) {
  let m = null, key0 = -1, dirty = true, writes = 0;
  const roles = new Int8Array(16).fill(-1), names = new Array(16).fill('');
  const viol = new Map(), stray = new Map();
  const nameOf = T => { let s = ''; for (let i = 0; i < 16 && m.taskRam[T][TA_NAME + i]; i++) s += String.fromCharCode(m.taskRam[T][TA_NAME + i]); return s; };
  const rebuild = () => {
    dirty = false;
    roles.fill(-1);
    const b = m.sharedBanks()[key0];
    if (!b) return;
    for (let r = 0; r < M.ntasks && r < tasks().length; r++) {
      const T = b[M.taskOff + r];
      if (T > 0 && T < 16) { roles[T] = r; names[T] = nameOf(T); }
    }
  };
  // (The UI's hub_create: what it writes isn't counted, its banks cleared before the header's there.  The writing
  // instruction's function, or memset's caller's: the return address atop the stack)
  const create = (T, pc) => {
    const c = lbls(PROGS[0]).create;
    if (!c || nameOf(T) !== PROGS[0]) return false;
    const [lo, hi] = c;
    if (pc >= lo && pc < hi) return true;
    const S = m.cpu.S, r = m.taskRam[T], ret = (r[0x100 + ((S + 1) & 0xFF)] | r[0x100 + ((S + 2) & 0xFF)] << 8) + 1;
    return ret >= lo && ret < hi;
  };
  const caller = T => { const S = m.cpu.S, r = m.taskRam[T]; return (r[0x100 + ((S + 1) & 0xFF)] | r[0x100 + ((S + 2) & 0xFF)] << 8) - 2; };
  function hook(k, off, v, T, pc) {
    if (off === 1 && v === (M.magic >> 8) && m) {               // (A hub made: its magic's second byte, the first there)
      const b = m.sharedBanks()[k];
      if (b && b[0] === (M.magic & 0xFF)) { key0 = k; dirty = true; return; }
    }
    if (key0 < 0 || k < key0 || k >= key0 + M.banks) return;
    const bank = k - key0;
    writes++;
    if (dirty) rebuild();                                     // (task[] as the last write to it left it)
    if (bank === 0 && off >= M.taskOff && off < M.taskOff + M.ntasks) dirty = true;
    const r = M.at[bank][off], role = roles[T];
    if (role >= 0 && (M.regions[r].allowed >> role & 1)) return;
    slow(r, role, T, pc, bank, off, v);
  }
  function slow(r, role, T, pc, bank, off, v) {
    if (create(T, pc)) return;
    if (bank === 0 && off >= M.taskOff && off < M.taskOff + M.ntasks && v === T) return;   // (A task's attach: its own entry)
    if (role >= 0 && nameOf(T) !== names[T]) { roles[T] = -1; role = -1; }   // (A role's task gone, its number another's)
    if (role >= 0 && (M.regions[r].allowed >> role & 1)) return;
    if (role < 0) {
      const n = nameOf(T) || 'task ' + T, e = stray.get(n);
      if (e) e.count++; else stray.set(n, { task: n, count: 1, bank, off });
      return;
    }
    const key = r * 16 + role, e = viol.get(key);
    if (e) { e.count++; return; }
    viol.set(key, { r, role, count: 1, pc, from: caller(T), bank, off });
  }
  function result() {
    const regions = [...viol.values()].map(e => {
      const g = M.regions[e.r], prog = PROGS[e.role];
      return { region: g.name, line: g.line, owner: g.owner >= 0 ? tasks()[g.owner] : 'none', writer: tasks()[e.role], count: e.count,
        pc: where(prog, e.pc), from: where(prog, e.from), at: e.bank + ':$' + (0x8000 + e.off).toString(16).toUpperCase(),
        known: g.known[e.role] || null };
    }).sort((x, y) => x.line - y.line || x.region.localeCompare(y.region, undefined, { numeric: true }) || x.writer.localeCompare(y.writer));
    return { writes, found: key0 >= 0, regions, stray: [...stray.values()] };
  }
  return { hook, attach: mm => { m = mm; }, result };
}

// The known violations not to fail a test (the table's: region and writer, by its line) and the new ones.  OUT:
// { known: [], fresh: [] } of result()'s regions
function judge(res) {
  return { known: res.regions.filter(e => e.known), fresh: res.regions.filter(e => !e.known) };
}

// A violation as a line: the region, its owner, the writer, how many writes, the first's PC (and its caller), and
// the move it's known to (or NEW)
const line = e => e.region + ' (' + e.owner + '\'s) written by the ' + e.writer + ' ' + e.count + 'x, first at ' + e.pc +
  ' from ' + e.from + ' (' + e.at + ')' + (e.known ? ' [move ' + e.known + ']' : ' [NEW]');

module.exports = { map, createCheck, judge, line, table, evaluate, get TASKS() { return tasks(); }, PROGS };
