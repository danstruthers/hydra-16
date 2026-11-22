// make.js - (themegen) its data files, after its build (build.js calls build): each world's (../dq0/worlds/NAME:
// ../dq0/WORLDS.md) history.txt (the history's words), with goals.txt (the goal kinds, the game's) and the bestiary
// (../dq0d/kinds.txt and factions.txt: the kinds that may live in a dungeon, by style and stratum, and their factions,
// the game's), made obj/games/worlds/NAME/history.dat (the card's lib/worlds/NAME/history.dat), which themegen
// reads as it starts (themegen -w NAME; the default world's with none):
// "TG", 1, the lines' count (two bytes), each line's key (themegen.c's TK_*) and its place in the text (two bytes),
// then the text: each line its values' count and its values, each 0-ended, no line across an 8K bank's edge (themegen
// reads the text into banks of its own).  Each goal kind's lines are held to a theme's (their slots filled, compiled and
// checked by the game's theme compiler, themec, in the emulator: tools/themec.js), and each piece of the history to the
// values it should have.
'use strict';
const fs = require('fs');
const path = require('path');

// themegen.c's TK_*: the keys, numbered (a goal block's theme lines from TK_THEME on, by THEME_KEYS)
const KEYS = { people: 1, land: 2, work: 3, name: 4, built: 5, reused: 6, noun: 7, ended: 8, clue: 9, rooms: 10,
  uses: 11, light: 12, folk: 13, now: 14, open: 15, ago: 16, goal: 20, thing: 21, exit: 22, title: 23, beast: 40,
  faction: 41, called: 24, why: 25, tongue: 42, common: 43, realm: 44, coin: 45, raiders: 46, holds: 47, colony: 48,
  tribe: 49, wild: 50 };
const WORLD = require('../dq0/worlds/worlds.js');
const TK_THEME = 30, THEME_KEYS = ['kind', 'piece', 'rule', 'story', 'lock', 'opened', 'ending', 'wayin', 'wayout'];
const WORKS = ['keep', 'temple', 'mine', 'tomb', 'town', 'hold', 'deep'], ENDS = ['abandoned', 'conquered', 'collapsed',
  'flooded', 'cursed', 'held'];
const BANK = 0x2000;

// A data file's lines (the theme format's: ../../tools/dq0build.js's keyLines), each { key, v, n, f }
const read = (dir, f) => require('../../tools/dq0build.js').keyLines(fs.readFileSync(path.join(dir, f), 'latin1'), f, { f });

const SLOTS = { R: 'the Kharum Hills', P: 'the Khazrim', K: 'dwarves', N: 'Durnak', X: 'Orth', O: 'the Aldmar men',
  C: '31', W: 'they delved deep', A: 'the Horn of Durnak', a: 'silver horn', B: 'the altar of Durnak', b: 'black altar',
  M: 'Ilsa', S: 'the Deep Works', s: '2', E: '1', G: 'dead', g: 'the dead', U: 'the Undercroft', u: '1', e: '1',
  T: 'Orth Gate', O: '1', Q: 'Haldan', Y: 'Haldan, a scholar, will pay well for it.', D: '87 pennies' };
const fill = (s, slots) => s.replace(/%(.)/g, (m, c) => (c in slots ? slots[c] : m));

// A goal block's lines made a theme (its slots filled, a header and strata round them): its { name, text }, for themec;
// throws if its other lines aren't as they should be
function checkGoal(g, lines) {
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
  if (!lines.some(l => l.key === 'title') || !lines.some(l => l.key === 'called')) throw new Error('goals.txt: line ' + g.n + ': no title or no called');
  if (g.v[3] !== undefined && g.v[3] !== 'heard') throw new Error('goals.txt: line ' + g.n + ': its fourth value heard, or none');
  // Its reasons (the plan's step 7): each a name, the rumour's words (no slot but %R and %Q: the rest aren't made till the
  // dungeon is), the why (in the story, %Y: so the story has its %Y), the pennies (a range), the standing, a thing, the words
  const whys = lines.filter(l => l.key === 'why');
  if (!whys.length) throw new Error('goals.txt: ' + g.v[0] + ': no why line');
  if (!lines.some(l => l.key === 'story' && l.v[0].includes('%Y'))) throw new Error('goals.txt: ' + g.v[0] + ': its story has no %Y');
  for (const w of whys) {
    const bad = s => { throw new Error('goals.txt: line ' + w.n + ': ' + s); };
    if (w.v.length !== 7) bad('a why: its name, the rumour, the why, the pennies (lo-hi), the standing, a thing (or -), the words');
    if (/%[^RQ]/.test(w.v[1])) bad('the rumour\'s slots: %R and %Q alone');
    const r = /^(\d+)-(\d+)$/.exec(w.v[3]);
    if (!r || +r[1] > +r[2] || +r[2] > 9999) bad('its pennies: lo-hi, 9999 at most');
    if (!/^-?\d$/.test(w.v[4])) bad('its standing: -9 to 9');
    if (whys.filter(x => x.v[0] === w.v[0]).length > 1) bad('its name twice in the kind');
    if (w.v[0].length > 11 || /\s/.test(w.v[0])) bad('its name: a word, 11 letters at most');
  }
  return { name: 'goals.txt (' + g.v[0] + ', line ' + g.n + ')', text: out.join('\n') };
}

// A world's history.txt read and held to what themegen wants of it (../dq0/WORLDS.md): its lines; throws if it isn't
function world(name, kinds, facs) {
  const f = 'worlds/' + name + '/history.txt', hist = read(WORLD.dir(name), 'history.txt').map(l => Object.assign(l, { f }));
  const need = { people: 7, land: 5, work: 3, name: 3, built: 2, reused: 2, noun: 2, ended: 2, clue: 10, rooms: 9,
    uses: 13, light: 2, folk: 2, now: 2, open: 1, ago: 1, tongue: 5, common: 1, realm: 2, coin: 2, raiders: 2, holds: 4,
    colony: 4, tribe: 3, wild: 1 };
  const bad = (l, s) => { throw new Error(f + ': ' + (l ? 'line ' + l.n + ': ' : '') + s); };
  const of = k => hist.filter(l => l.key === k), peoples = of('people').map(l => l.v[0]), tongues = of('tongue');
  const isKinds = s => s.split(/\s+/).every(k => kinds.includes(k)), lands = of('land').map(l => l.v[0]);
  for (const l of hist) {
    if (!(l.key in need)) bad(l, 'no key ' + l.key);
    if (l.v.length < need[l.key]) bad(l, l.key + ' wants ' + need[l.key] + ' values');
    if (['work', 'name', 'built', 'reused', 'noun', 'clue'].includes(l.key) && !WORKS.includes(l.v[0])) bad(l, 'no work ' + l.v[0]);
    if (l.key === 'ended' && !ENDS.includes(l.v[0])) bad(l, 'no ending ' + l.v[0]);
    if (l.key === 'people' && !(+l.v[1] >= 1 && +l.v[1] <= tongues.length)) bad(l, 'its tongue not a tongue line\'s');
    if (l.key === 'people' && (l.v[0].length > 11 || l.v[5].length > 8 || l.v[6].length > 8)) bad(l, 'its what 11 letters at most, its tongue\'s and script\'s words 8');
    if (l.key === 'people' && !l.v[4].split(/\s+/).every(w => WORKS.includes(w))) bad(l, 'what it makes: works');
    if (['coin', 'raiders', 'holds', 'colony'].includes(l.key) && !peoples.includes(l.v[l.key === 'coin' || l.key === 'raiders' ? 0 : 1])) bad(l, 'no such people');
    if (['folk', 'holds', 'colony', 'tribe'].includes(l.key) && !facs.includes(l.v[0]) || l.key === 'raiders' && !facs.includes(l.v[1])) bad(l, 'no such faction (the bestiary\'s)');
    if (l.key === 'holds' && (!isKinds(l.v[2]) || l.v[3] !== '-' && !kinds.includes(l.v[3])) || l.key === 'colony' && l.v[3] !== '-' && !kinds.includes(l.v[3]) ||
      l.key === 'tribe' && !isKinds(l.v[1])) bad(l, 'no such kind (the bestiary\'s)');
    if (l.key === 'tribe' && !of('colony').some(c => c.v[0] === l.v[0])) bad(l, 'no colony line of its faction');
    if (l.key === 'colony' && !of('tribe').some(c => c.v[0] === l.v[0])) bad(l, 'no tribe line');
    if (l.key === 'realm' && (!(+l.v[0] >= 1 && +l.v[0] <= tongues.length) || !/^(\?h|-|[lgph]+)$/.test(l.v[1]))) bad(l, 'a tongue line\'s number, and its ways');
    if (l.key === 'coin' && !/^[g-]$/.test(l.v[1])) bad(l, 'its ways g or -');
    if (l.key === 'wild' && !l.v[0].split(/\s+/).every(w => lands.includes(w))) bad(l, 'its lands: land lines\'');
    if (l.key === 'tongue' && l.v.slice(1, 4).some(s => !s.trim())) bad(l, 'its onsets, vowels and codas: a word each at least');
  }
  tongues.forEach((l, i) => { if (+l.v[0] !== i + 1) bad(l, 'the tongues numbered in order, 1 first'); });
  const th = path.join(WORLD.dir(name), 'tongues.h');  // (Its tongues in C, if it has them: the same as its lines)
  if (fs.existsSync(th)) {
    const c = [...fs.readFileSync(th, 'latin1').matchAll(/\{((?:\s*"[^"]*",?)+)\s*\}/g)].map(m => [...m[1].matchAll(/"([^"]*)"/g)].map(x => x[1]).join('|'));
    if (c.join('\n') !== tongues.map(l => l.v.slice(1, 5).join('|')).join('\n')) bad(null, 'its tongues.h not its tongue lines');
  }
  if (tongues.length > 8) bad(null, 'more than 8 tongues (a theme\'s people lines\' tongue: themec\'s 1-8)');
  if (!tongues.length || of('common').length !== 1 || !(+of('common')[0].v[0] >= 1 && +of('common')[0].v[0] <= tongues.length))
    bad(null, 'a tongue line at least, and a common line, one, its number a tongue line\'s');
  if (of('people').length < 2 || !of('land').length) bad(null, 'two people lines at least (an era\'s not the one before it), and a land line');
  if (of('raiders').length > 1 || of('wild').length > 1) bad(null, 'one raiders line at most, one wild line at most');
  for (const k of ['open', 'ago']) if (of(k).length !== 1) bad(null, 'one ' + k + ' line');
  for (const k of ['rooms', 'light']) if (!of(k).some(l => l.v[0] === 'default')) bad(null, 'no ' + k + ' line for default');
  for (const e of ENDS) if (!of('ended').some(l => l.v[0] === e)) bad(null, 'no ended line for ' + e);
  for (const w of WORKS.slice(0, 6)) for (const k of ['built', 'reused', 'noun', 'clue'])
    if (!hist.some(l => l.key === k && l.v[0] === w)) bad(null, 'no ' + k + ' line for ' + w);
  for (const w of WORKS) {                      // (Each work's strata named: histgen's names, by the work's and the stratum's)
    const wl = of('work').find(l => l.v[0] === w);
    if (!wl) bad(null, 'no work line for ' + w);
    for (let s = 0; s < (wl.v.length - 1) / 2; s++) if (!of('name').some(l => l.v[0] === w && +l.v[1] === s)) bad(wl, 'no name line for its stratum ' + s);
  }
  return hist;
}

function build({ dir, objdir, bios, say = () => {} }) {
  const goals = read(dir, 'goals.txt');
  // The goal kinds: each block held to a theme's
  const themes = [];
  let at = -1;
  goals.forEach((l, i) => {
    if (l.key === 'goal') {
      if (at >= 0) themes.push(checkGoal(goals[at], goals.slice(at + 1, i)));
      at = i;
    } else if (!(l.key in KEYS) && !THEME_KEYS.includes(l.key)) throw new Error(`goals.txt: line ${l.n}: no key ${l.key}`);
  });
  if (at >= 0) themes.push(checkGoal(goals[at], goals.slice(at + 1)));
  // The bestiary: each kind's key, styles, strata, group, lairs, faction; each faction's attitude and others
  const lines = f => require('../../tools/dq0build.js').rows(path.join(dir, '..', 'dq0d', f));
  const best = [];
  for (const k of lines('kinds.txt')) best.push({ key: 'beast', v: [k[1], k[3], ...k[4].split('-'), ...k[5].split('-'), k[6], k[11]] });
  for (const x of lines('factions.txt')) best.push({ key: 'faction', v: [x[0], x[2], x[3]] });
  // The files: each world's history.dat, its history's lines, the goal kinds' and the bestiary's; and bestiary.dat, the
  // bestiary's lines alone (themec's, for its checker)
  const made = {}, kinds = best.filter(l => l.key === 'beast').map(l => l.v[0]), facs = best.filter(l => l.key === 'faction').map(l => l.v[0]);
  for (const w of WORLD.names()) {
    const out = [...world(w, kinds, facs), ...goals, ...best];
    fs.mkdirSync(path.join(objdir, '..', 'worlds', w), { recursive: true });
    made[w] = { lines: out.length, text: writeData(out, path.join(objdir, '..', 'worlds', w, 'history.dat'), 320) };
  }
  writeData(best, path.join(objdir, '..', 'bestiary.dat'), 64);
  // The goal kinds' themes compiled and checked by themec (built before this; its checker's bestiary.dat just written)
  require('../../tools/dq0build.js').themes(themes, { out: false, bios });
  say('history.dat: ' + Object.entries(made).map(([w, m]) => w + ' ' + m.lines + ' lines, ' + m.text + ' bytes of words').join('; '));
  return made;
}

// A data file of lines (history.dat's format, above): its index, then its text, max lines at most (data.c's TGMAX, the program's
// that reads it: themegen's, themec's).  OUT: its text's length
function writeData(out, file, max) {
  if (out.length > max) throw new Error(path.basename(file) + ': ' + out.length + ' lines, more than ' + max + ' (data.c\'s TGMAX)');
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
