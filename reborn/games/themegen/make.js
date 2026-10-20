// make.js - (themegen) its data file, after its build (build.js calls build): history.txt (the history's words),
// goals.txt (the goal kinds) and the bestiary (../dq0d/kinds.txt and factions.txt: the kinds that may live in a
// dungeon, by style and stratum, and their factions) made obj/games/themegen.dat, which themegen reads as it starts:
// "TG", 1, the lines' count (two bytes), each line's key (themegen.c's TK_*) and its place in the text (two bytes),
// then the text: each line its values' count and its values, each 0-ended, no line across an 8K bank's edge (themegen
// reads the text into banks of its own).  Each goal kind's lines are held to a theme's (../dq0/make.js's compile,
// their slots filled), and each piece of the history to the values it should have.
'use strict';
const fs = require('fs');
const path = require('path');
const dq0 = require('../dq0/make.js');

// themegen.c's TK_*: the keys, numbered (a goal block's theme lines from TK_THEME on, by THEME_KEYS)
const KEYS = { people: 1, land: 2, work: 3, name: 4, built: 5, reused: 6, noun: 7, ended: 8, clue: 9, rooms: 10,
  uses: 11, light: 12, folk: 13, now: 14, open: 15, ago: 16, goal: 20, thing: 21, exit: 22, title: 23, beast: 40,
  faction: 41, called: 24 };
const TK_THEME = 30, THEME_KEYS = ['kind', 'piece', 'rule', 'story', 'lock', 'opened', 'ending'];
const WORKS = ['keep', 'temple', 'mine', 'tomb', 'town', 'hold', 'deep'], ENDS = ['abandoned', 'conquered', 'collapsed',
  'flooded', 'cursed', 'held'];
const BANK = 0x2000;

// A data file's lines (the theme format's: ../dq0/make.js's reader), each { key, v, n }
function read(dir, f) {
  const text = fs.readFileSync(path.join(dir, f), 'latin1'), out = [];
  text.split(/\r?\n/).forEach((l, i) => {
    if (!l.trim() || l.startsWith('#')) return;
    if (/^\s/.test(l)) {
      const v = out[out.length - 1].v, t = l.trim();
      if (t.startsWith('|')) v.push(...t.slice(1).split('|').map(s => s.trim()));
      else v[v.length - 1] += ' ' + t;
      return;
    }
    const v = l.split('|').map(s => s.trim());
    out.push({ key: v.shift(), v, n: i + 1, f });
  });
  return out;
}

const SLOTS = { R: 'the Kharum Hills', P: 'the Khazrim', K: 'dwarves', N: 'Durnak', X: 'Orth', O: 'the Aldmar men',
  C: '31', W: 'they delved deep', A: 'the Horn of Durnak', a: 'silver horn', B: 'the altar of Durnak', b: 'black altar',
  M: 'Ilsa', S: 'the Deep Works', s: '2', E: '1', G: 'dead', g: 'the dead', U: 'the Undercroft', u: '1', e: '1',
  T: 'Orth Gate' };
const fill = (s, slots) => s.replace(/%(.)/g, (m, c) => (c in slots ? slots[c] : m));

// A goal block's lines made a theme (its slots filled, a header and strata round them) and compiled: throws if
// they're not a theme's
function checkGoal(g, lines, beasts) {
  const things = lines.filter(l => l.key === 'thing');
  const a = things.find(l => l.v[0] === 'a'), b = things.find(l => l.v[0] === 'b');
  const slots = Object.assign({}, SLOTS, a ? { a: a.v[2], A: g.v[0] === 'through' ? SLOTS.A : fill(a.v[1], SLOTS) } : {},
    b ? { b: b.v[2], B: fill(b.v[1], SLOTS) } : {});
  const out = ['theme | the test', 'mission | ' + g.v[1], 'stratum | One | b | halls', 'stratum | Two | bn | crypt',
    'stratum | Three | n', 'people | Khazrim | dwarves | 1 | hewn', 'era | Khazrim | mine | cursed | 31 | the dead | they delved deep',
    'faction | dead | ghoul skel | 0-7 | 9 | - | 1 | h | -'];
  for (const t of things) out.push('kind | ' + t.v.slice(2).join(' | '));
  for (const l of lines) if (THEME_KEYS.includes(l.key)) out.push(l.key + ' | ' + fill(l.v.join(' | '), slots));
  if (g.v[0] === 'through') out.push('goal | through | the Far Door | 2 | -');
  const r = dq0.compile(out.join('\n'), 'goals.txt (' + g.v[0] + ', line ' + g.n + ')', beasts, 1);
  const faults = dq0.check(r.t, beasts, 'goals.txt (' + g.v[0] + ')');
  if (faults.length) throw new Error(faults.join('\n'));
  if (!lines.some(l => l.key === 'title') || !lines.some(l => l.key === 'called')) throw new Error('goals.txt: line ' + g.n + ': no title or no called');
}

function build({ dir, objdir, say = () => {} }) {
  const beasts = dq0.bestiary(path.join(dir, '..', 'dq0'));
  const hist = read(dir, 'history.txt'), goals = read(dir, 'goals.txt');
  const need = { people: 5, land: 5, work: 3, name: 3, built: 2, reused: 2, noun: 2, ended: 2, clue: 10, rooms: 9,
    uses: 13, light: 2, folk: 2, now: 2, open: 1, ago: 1 };
  for (const l of hist) {
    if (!(l.key in need)) throw new Error(`history.txt: line ${l.n}: no key ${l.key}`);
    if (l.v.length < need[l.key]) throw new Error(`history.txt: line ${l.n}: ${l.key} wants ${need[l.key]} values`);
    if (['work', 'name', 'built', 'reused', 'noun', 'clue'].includes(l.key) && !WORKS.includes(l.v[0])) throw new Error(`history.txt: line ${l.n}: no work ${l.v[0]}`);
    if (l.key === 'ended' && !ENDS.includes(l.v[0])) throw new Error(`history.txt: line ${l.n}: no ending ${l.v[0]}`);
  }
  for (const w of WORKS.slice(0, 6)) for (const k of ['built', 'reused', 'noun', 'clue'])
    if (!hist.some(l => l.key === k && l.v[0] === w)) throw new Error(`history.txt: no ${k} line for ${w}`);
  // The goal kinds: each block held to a theme's
  let at = -1;
  goals.forEach((l, i) => {
    if (l.key === 'goal') {
      if (at >= 0) checkGoal(goals[at], goals.slice(at + 1, i), beasts);
      at = i;
    } else if (!(l.key in KEYS) && !THEME_KEYS.includes(l.key)) throw new Error(`goals.txt: line ${l.n}: no key ${l.key}`);
  });
  if (at >= 0) checkGoal(goals[at], goals.slice(at + 1), beasts);
  // The bestiary: each kind's key, styles, strata, group, lairs, faction; each faction's attitude and others
  const lines = f => fs.readFileSync(path.join(dir, '..', 'dq0d', f), 'latin1').split(/\r?\n/)
    .filter(l => l.trim() && !l.startsWith('#')).map(l => l.split('|').map(s => s.trim()));
  const out = [...hist, ...goals];
  for (const k of lines('kinds.txt')) out.push({ key: 'beast', v: [k[1], k[3], ...k[4].split('-'), ...k[5].split('-'), k[6], k[11]] });
  for (const x of lines('factions.txt')) out.push({ key: 'faction', v: [x[0], x[2], x[3]] });
  // The files: themegen.dat, all of it, and bestiary.dat, the bestiary's lines alone (themec's, for its checker)
  const n = writeData(out, path.join(objdir, '..', 'themegen.dat'));
  writeData(out.filter(l => l.key === 'beast' || l.key === 'faction'), path.join(objdir, '..', 'bestiary.dat'));
  return { lines: out.length, text: n };
}

// A data file of lines (themegen.dat's format, above): its index, then its text.  OUT: its text's length
function writeData(out, file) {
  const index = [], text = [];
  let off = 0;
  for (const l of out) {
    const key = l.key in KEYS ? KEYS[l.key] : TK_THEME + THEME_KEYS.indexOf(l.key);
    const rec = Buffer.concat([Buffer.from([l.v.length]), ...l.v.map(s => Buffer.from(s + '\0', 'latin1'))]);
    if (rec.length > 1024) throw new Error(`${l.f || 'the bestiary'}: line ${l.n}: longer than 1024`);
    if (Math.floor(off / BANK) !== Math.floor((off + rec.length - 1) / BANK)) {   // (No line across a bank's edge)
      const pad = BANK - off % BANK;
      text.push(Buffer.alloc(pad));
      off += pad;
    }
    index.push(key, off & 255, off >> 8);
    text.push(rec);
    off += rec.length;
  }
  if (off > 3 * BANK) throw new Error(path.basename(file) + ': its text more than three banks');
  const head = Buffer.from([0x54, 0x47, 1, out.length & 255, out.length >> 8]);
  fs.writeFileSync(file, Buffer.concat([head, Buffer.from(index), ...text]));
  return off;
}

module.exports = { build, KEYS };
