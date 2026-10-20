// hubown.js - Dingus Quest Zero's hub writers' check (Phase 2: one authority on state): every write to the hub's
// banks, in a test, by the task that made it, held against the owners' table in games/dq0/hub.h (its "own" lines:
// each region's owner, those that write it by design, and those that write it still till a move of Phase 2's).
//
// map() reads the table and works out its regions' bytes: each region's C expressions (offsetof, sizeof, the banks'
// numbers) compiled by cc65 into a file of constants and read back (obj/hubown.json keeps the map, made again as the
// headers change).  createCheck(map) gives { hook, attach(m), result() }: hook is the machine's opt.sharedWrite
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
const crypto = require('crypto');
const { execFileSync } = require('child_process');

const ROOT = path.join(__dirname, '..', '..');                // (reborn/)
const DQ0 = path.join(ROOT, 'games', 'dq0');
const HEADERS = ['hub.h', 'items.h', 'level.h', 'theme.h', 'rs.h', path.join('..', 'dq0w', 'world.h')];
const TASKS = ['ui', 'world', 'denizens', 'referee', 'sound', 'maker', 'senses', 'commands'];   // (hub.h's T_*)
const PROGS = ['dq0', 'dq0w', 'dq0d', 'dq0harn', 'dq0s', 'dq0m', 'dq0p', 'dq0c'];  // (Each role's program)
const TA_NAME = 0x0230;                                       // (A task's name in its RAM: base/sim/run.js's)

// The records a region names (REC.FIELD): their struct, their bank and place (n of them: k's)
const RECS = { hub: 'struct hub', sight: 'struct sight', uirec: 'struct uirec', wrec: 'struct wrec',
  chunk: 'struct chunk', item: 'struct item' };

// The table's lines: { region, owner, plus: [task], known: [{ move, task }], line }
function table() {
  const out = [], text = fs.readFileSync(path.join(DQ0, 'hub.h'), 'latin1').split(/\r?\n/);
  text.forEach((l, i) => {
    const k = l.match(/^\*\*   own ((?:hub|sight|uirec|wrec|chunk|item|banks?)\b.*)$/);
    if (!k) return;
    const words = k[1].replace(/\s+\(.*$/, '').trim().split(/\s+/), region = words.shift(), owner = words.shift();
    const e = { region, owner, plus: [], known: [], line: i + 1 };
    const role = t => { if (!TASKS.includes(t) && t !== 'each' && t !== 'none') throw new Error('hub.h:' + (i + 1) + ': no task ' + t); return t; };
    role(owner);
    for (const w of words) {
      if (w.startsWith('+')) e.plus.push(role(w.slice(1)));
      else if (/^\d+\.\d+:/.test(w)) { const [move, ts] = w.split(':'); for (const t of ts.split(',')) e.known.push({ move, task: role(t) }); }
      else throw new Error('hub.h:' + (i + 1) + ': "' + w + '"');
    }
    out.push(e);
  });
  return out;
}

// A region's arguments, split at their own commas: bank(DEN_BANK,SND_AT,FIGHT_AT) -> ['DEN_BANK', 'SND_AT', 'FIGHT_AT']
function args(s) {
  const out = [];
  let d = 0, cur = '';
  for (const c of s) {
    if (c === ',' && !d) { out.push(cur); cur = ''; continue; }
    if (c === '(' || c === '[') d++;
    if (c === ')' || c === ']') d--;
    cur += c;
  }
  out.push(cur);
  return out;
}

// The C expressions each line wants: { exprs: [string], want(e) -> the values' indices }
function exprsOf(lines) {
  const exprs = [], idx = e => { let i = exprs.indexOf(e); if (i < 0) { i = exprs.length; exprs.push(e); } return i; };
  const base = ['HUB_BANKS', 'NTASKS', 'offsetof (struct hub, task)', 'HUB_MAGIC', 'CACHE_BANK (0)', 'CHUNK_SIZE', 'NCSLOTS',
    'ITEM_BANK (0)', 'sizeof (struct item)', 'NITEMS'];
  const B = {}; for (const e of base) B[e] = idx(e);
  for (const l of lines) {
    const r = l.region, f = r.match(/^(\w+)\((.*)\)$/);
    if (f) {
      const a = args(f[2]);
      if (f[1] === 'bank' && a.length === 1) l.ex = { kind: 'bank', b: idx(a[0]), n: idx('1') };
      else if (f[1] === 'bank' && a.length === 3) l.ex = { kind: 'range', b: idx(a[0]), from: idx('(unsigned) (' + a[1] + ')'), to: idx('(unsigned) (' + a[2] + ')') };
      else if (f[1] === 'banks' && a.length === 2) l.ex = { kind: 'bank', b: idx(a[0]), n: idx(a[1]) };
      else throw new Error('hub.h:' + l.line + ': ' + r);
      continue;
    }
    const d = r.indexOf('.'), rec = d < 0 ? r : r.slice(0, d), field = d < 0 ? '' : r.slice(d + 1), st = RECS[rec];
    if (!st) throw new Error('hub.h:' + l.line + ': ' + r);
    const p = (st + '*') , m = field.match(/^(.*?)\[\*\](.*)$/);
    if (!field) l.ex = { kind: 'rec', rec, off: idx('0'), size: idx('sizeof (' + st + ')'), stride: idx('1'), count: idx('1') };
    else if (!m) l.ex = { kind: 'rec', rec, off: idx('offsetof (' + st + ', ' + field + ')'), size: idx('sizeof (((' + p + ') 0)->' + field + ')'),
      stride: idx('1'), count: idx('1') };
    else {
      const el = m[1] + '[0]' + m[2];
      l.ex = { kind: 'rec', rec, off: idx('offsetof (' + st + ', ' + el + ')'), size: idx('sizeof (((' + p + ') 0)->' + el + ')'),
        stride: idx('sizeof (((' + p + ') 0)->' + m[1] + '[0])'), count: idx('sizeof (((' + p + ') 0)->' + m[1] + ') / sizeof (((' + p + ') 0)->' + m[1] + '[0])') };
    }
  }
  return { exprs, B };
}

// The expressions' values, by cc65 (a file of constants: each a .dword in its .s)
function evaluate(exprs) {
  const dir = fs.mkdtempSync(path.join(require('os').tmpdir(), 'hubown-'));
  try {
    const c = path.join(dir, 'own.c'), s = path.join(dir, 'own.s');
    fs.writeFileSync(c, '#include <stddef.h>\n#include "hub.h"\n' + exprs.map((e, i) => 'const unsigned long v' + i + ' = (unsigned long) (' + e + ');\n').join(''));
    const bin = require('../../../base/build.js').tool('cc65');     // (The build's cc65)
    const home = process.env.CC65_HOME || path.dirname(path.dirname(bin));
    execFileSync(bin, ['-t', 'none', '--cpu', '65C02', '-I', path.join(ROOT, 'sdk', 'c', 'include'), '-I', path.join(home, 'include'),
      '-I', DQ0, '-o', s, c], { stdio: ['ignore', 'pipe', 'pipe'] });
    const v = [], text = fs.readFileSync(s, 'latin1');
    for (const k of text.matchAll(/^_v(\d+):\s*\.dword\s+\$([0-9a-fA-F]+)/gm)) v[+k[1]] = parseInt(k[2], 16);
    if (v.length !== exprs.length || v.includes(undefined)) throw new Error('hubown: cc65 gave ' + v.length + ' values of ' + exprs.length);
    return v;
  } finally { fs.rmSync(dir, { recursive: true, force: true }); }
}

// The map: the table's regions, and each byte of the hub's banks' region.  OUT: { banks, ntasks, taskOff, magic,
// regions: [{ name, owner, allowed (a mask of roles), known: { role: move }, line }] (regions[0]: none's),
// at: [Uint16Array(0x2000) a bank: each byte's region] }; kept in obj/hubown.json as the headers stand
function map() {
  const hash = crypto.createHash('sha1');
  for (const h of HEADERS) hash.update(fs.readFileSync(path.join(DQ0, h)));
  hash.update(fs.readFileSync(__filename));
  const key = hash.digest('hex'), cache = path.join(ROOT, 'obj', 'hubown.json');
  let j = null;
  try { j = JSON.parse(fs.readFileSync(cache, 'utf8')); if (j.key !== key) j = null; } catch (e) { j = null; }
  if (!j) {
    const lines = table(), { exprs, B } = exprsOf(lines), v = evaluate(exprs), V = i => v[i];
    const c = { banks: V(B['HUB_BANKS']), ntasks: V(B['NTASKS']), taskOff: V(B['offsetof (struct hub, task)']), magic: V(B['HUB_MAGIC']),
      cache0: V(B['CACHE_BANK (0)']), chunk: V(B['CHUNK_SIZE']), slots: V(B['NCSLOTS']), item0: V(B['ITEM_BANK (0)']),
      item: V(B['sizeof (struct item)']), items: V(B['NITEMS']) };
    const BANK = { hub: 0, sight: null, uirec: null, wrec: null };
    // (Each record's places: [bank, offset] each one; the records in a bank of their own at its start)
    const places = rec => {
      if (rec === 'chunk') return Array.from({ length: c.slots }, (_, k) => [c.cache0 + Math.floor(k / 3), (k % 3) * c.chunk]);
      if (rec === 'item') { const per = Math.floor(0x2000 / c.item); return Array.from({ length: c.items }, (_, i) => [c.item0 + Math.floor(i / per), (i % per) * c.item]); }
      return [[BANK[rec], 0]];
    };
    j = { key, ...c, lines: [] };
    // (The records' banks: hub.h's, by name)
    const more = evaluate(['SIGHT_BANK', 'UI_BANK', 'WORLD_BANK']);
    [BANK.sight, BANK.uirec, BANK.wrec] = more;
    for (const l of lines) {
      const x = l.ex, spans = [];                             // ([bank, from, to, element])
      if (x.kind === 'bank') for (let b = 0; b < V(x.n); b++) spans.push([V(x.b) + b, 0, 0x2000, -1]);
      else if (x.kind === 'range') spans.push([V(x.b), V(x.from) - 0x8000, V(x.to) - 0x8000, -1]);
      else for (const [b, o] of places(x.rec)) for (let e = 0; e < V(x.count); e++) {
        const from = o + V(x.off) + e * V(x.stride);
        spans.push([b, from, from + V(x.size), V(x.count) > 1 ? e : -1]);
      }
      for (const s of spans) if (s[0] < 0 || s[0] >= c.banks || s[1] < 0 || s[2] > 0x2000 || s[1] > s[2]) throw new Error('hub.h:' + l.line + ': ' + l.region + ' outside the hub: bank ' + s[0] + ' $' + s[1].toString(16) + '-$' + s[2].toString(16));
      j.lines.push({ region: l.region, owner: l.owner, plus: l.plus, known: l.known, line: l.line, spans });
    }
    try { const t = cache + '.' + process.pid; fs.writeFileSync(t, JSON.stringify(j)); fs.renameSync(t, cache); } catch (e) { /* (Kept next time) */ }
  }
  // The regions (an each line's elements apart: each its own owner), painted line by line over those before
  const regions = [{ name: '(no region)', owner: -1, allowed: 0, known: {}, line: 0 }], at = [];
  for (let b = 0; b < j.banks; b++) at.push(new Uint16Array(0x2000));
  for (const l of j.lines) {
    const ids = new Map();
    for (const [b, from, to, e] of l.spans) {
      const owner = l.owner === 'each' ? (e >= 0 && e < TASKS.length ? e : -1) : TASKS.indexOf(l.owner);
      const k = l.owner === 'each' ? e : -1;
      if (!ids.has(k)) {
        let allowed = owner >= 0 ? 1 << owner : 0;
        for (const p of l.plus) allowed |= 1 << TASKS.indexOf(p);
        const known = {};
        for (const { move, task } of l.known) known[TASKS.indexOf(task)] = move;
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
    for (let r = 0; r < M.ntasks && r < TASKS.length; r++) {
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
      return { region: g.name, line: g.line, owner: g.owner >= 0 ? TASKS[g.owner] : 'none', writer: TASKS[e.role], count: e.count,
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

module.exports = { map, createCheck, judge, line, table, evaluate, TASKS, PROGS };
