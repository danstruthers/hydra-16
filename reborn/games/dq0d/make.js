// make.js - (dq0d) the denizens' data files, after its build (build.js calls build): kinds.txt (the game's bestiary)
// made obj/games/dq0d.dat, 40 bytes a kind (dq0d.c's struct dkind), the first none (its flags the kinds' count, the
// first too, its styles the factions'); then factions.txt's factions, 32 bytes each (struct faction), the first none.
// And the world's lore (lore.h): peoples.txt made obj/games/dq0f.dat, deities.txt obj/games/dq0g.dat, 64 bytes a
// record (struct people, struct deity), the first none (its first byte the records' count), their words after; and
// the deities' names, spaces between, obj/games/dq0g.txt (the UI's: the commands' words for them).
'use strict';
const fs = require('fs');
const path = require('path');

const WAYS = { s: 0x01, l: 0x02, w: 0x04, a: 0x08, f: 0x20, t: 0x40 };   // dq0d.c's K_SHY, K_LAIR, K_WAYS, K_BOLD, K_LIGHTS, K_SWEEPS
const K_BONES = 0x10;                           //   and K_BONES
const STYLES = { b: 0, n: 1, m: 2, x: 3 };      // level.h's ST_*: a bit each
const MINDS = { m: 0, b: 1, l: 2, a: 3, c: 4 }; // dq0d.c's M_*
const AT = { h: 0, u: 1, w: 2, i: 3, f: 4 };    // hub.h's AT_*
const KREC = 40, FREC = 32, NFACT = 8;          // (dq0d.c's: a kind's record, a faction's, the factions at most)
const SND_KINDS = 32;                           // (hub.h's: the kinds at most, the first none; the themes' bits)
const LANDS = 'mhfscp', CLIMATES = 'ctw';       // lore.h's LA_*, CL_*: a bit each, in order
const LIGHTS = { '*': 0, d: 1, l: 2 }, NATURES = { a: 0, v: 1, m: 2, f: 3, d: 4 };  // LI_*, NA_*
const ARMS = ['bite', 'claws', 'sting', 'crush', 'horns', 'talons', 'touch', 'weapon'];  // AR_*: a bit each
const PURPOSES = ['halls', 'keep', 'temple', 'cellars', 'crypt', 'works'];   // theme.h's P_*
const DOMAINS = ['war', 'fire', 'cruelty', 'death', 'undead', 'chaos', 'wealth', 'pleasure', 'bargains', 'beasts',
  'change', 'serpents', 'honour', 'battle', 'protection', 'shadow', 'deceit', 'nightmares', 'thieves', 'healing',
  'harvest', 'mercy', 'courage', 'storms', 'cold', 'knowledge', 'puzzles', 'law', 'dreams', 'magic', 'stars',
  'stone'];                                     // lore.h's DO_*
const GOOD = ['good', 'neutral', 'evil'], ORDER = ['order', 'balance', 'chaos'];   // TE_*
const BURIALS = ['none', 'crypt', 'barrow', 'pyre', 'earth'];                       // BU_*
const LREC = 64;                                // (lore.h's struct people, struct deity)
const SQ_FT = 5;                                // rs.h's: a square's feet (kinds.txt's distances in feet)

const lines = f => fs.readFileSync(f, 'utf8').split(/\r?\n/).filter(l => l.trim() && !l.startsWith('#'))
  .map(l => l.split('|').map(s => s.trim()));

// A file of keys and values (peoples.txt, deities.txt): its lines, each its key and its values (a line begun with a
// space goes on with the last: its last value, or with a bar first, more values), in blocks each begun by first
function blocks(f, first) {
  const out = [];
  fs.readFileSync(f, 'latin1').split(/\r?\n/).forEach((l, i) => {
    if (!l.trim() || l.startsWith('#')) return;
    const b = out[out.length - 1];
    if (/^\s/.test(l)) {
      if (!b || !b.last) throw new Error(`${f}: line ${i + 1}: goes on with nothing`);
      const t = l.trim();
      if (t.startsWith('|')) b.last.push(...t.slice(1).split('|').map(s => s.trim()));
      else b.last[b.last.length - 1] += ' ' + t;
      return;
    }
    const v = l.split('|').map(s => s.trim()), key = v.shift();
    if (key === first) out.push({ n: i + 1, f: {} });
    else if (!out.length) throw new Error(`${f}: line ${i + 1}: ${key} before the first ${first}`);
    out[out.length - 1].f[key] = v;
    out[out.length - 1].last = v;
  });
  return out;
}

// Letters' bits: each letter's place in set ('*': all)
const bits = (s, set, what) => s === '*' ? (1 << set.length) - 1 : [...s].reduce((a, c) => {
  if (set.indexOf(c) < 0) throw new Error(`dq0d: ${what}: ${s}`);
  return a | 1 << set.indexOf(c);
}, 0);

// The words after the records: each 0-ended, its offset from the file's start (0: none)
function words(base) {
  const parts = [];
  let at = base;
  return {
    add(s) {
      if (!s || s === '-') return 0;
      const b = Buffer.from(s + '\0', 'latin1'), o = at;
      parts.push(b);
      at += b.length;
      if (at > 0xFFFF) throw new Error('dq0d: the lore\'s words past 64K');
      return o;
    },
    list(v) {                                   // (Several, one after another, each 0-ended)
      const o = at;
      v.forEach(s => { const b = Buffer.from(s + '\0', 'latin1'); parts.push(b); at += b.length; });
      return o;
    },
    get data() { return Buffer.concat(parts); }
  };
}

function build({ dir, objdir, say = () => {} }) {
  const facs = lines(path.join(dir, 'factions.txt')), names = facs.map(f => f[0]);
  const kinds = lines(path.join(dir, 'kinds.txt')), keys = kinds.map(f => f[1]);
  if (facs.length >= NFACT) throw new Error('dq0d/factions.txt: more than ' + (NFACT - 1) + ' factions');
  if (kinds.length >= SND_KINDS) throw new Error('dq0d/kinds.txt: more than ' + (SND_KINDS - 1) + ' kinds');
  const out = Buffer.alloc(KREC * (kinds.length + 1) + FREC * (facs.length + 1));
  out[20] = kinds.length + 1;
  out[21] = facs.length + 1;
  kinds.forEach((f, i) => {
    const at = KREC * (i + 1), pair = s => s.split('-').map(Number), fac = names.indexOf(f[11]) + 1;
    const bad = () => { throw new Error(`dq0d/kinds.txt: line ${i + 1}: ${f.join(' | ')}`); };
    if (f.length !== 22 || f[0].length > 11 || f[1].length > 7 || !fac || !(f[12] in MINDS) || !(f[18] in LIGHTS) ||
        !(f[19] in NATURES) || !/^[1-6]$/.test(f[15]) || !/^(body|bones)$/.test(f[21])) bad();
    out.write(f[0], at, 'latin1');
    out.write(f[1], at + 12, 'latin1');
    const sq = ft => {                          // (A distance's feet made the map's squares, once: in fives)
      if (!(+ft >= 0) || +ft % SQ_FT || +ft > 255 * SQ_FT)
        throw new Error(`dq0d/kinds.txt: line ${i + 1}: ${ft}: a distance in feet, a square's ${SQ_FT} each`);
      return +ft / SQ_FT;
    };
    const [d0, d1] = pair(f[4]), [n0, n1] = pair(f[5]), [sl, sd] = f[8].split('-').map(sq);
    const flags = [...f[2]].reduce((a, c) => a | (WAYS[c] || 0), 0) | (f[21] === 'bones' ? K_BONES : 0);
    const styles = [...f[3]].reduce((a, c) => a | (1 << STYLES[c]), 0);
    const arms = f[20].split(/\s+/).reduce((a, w) => {
      if (ARMS.indexOf(w) < 0) bad();
      return a | 1 << ARMS.indexOf(w);
    }, 0);
    [flags, styles, d0, d1, n0, n1, +f[6], sq(f[7]), sl, sd, sq(f[9]), +f[10], fac, MINDS[f[12]], +f[13] & 0xFF, +f[14],
      +f[15] | NATURES[f[19]] << 3 | LIGHTS[f[18]] << 6, bits(f[16], LANDS, 'land'), bits(f[17], CLIMATES, 'climate'),
      arms].forEach((v, k) => { out[at + 20 + k] = v; });
  });
  const rel = names.map(() => names.map(() => AT.i)), base = KREC * (kinds.length + 1);
  facs.forEach((f, i) => {
    if (f.length !== 4 || f[0].length > 11 || f[1].length > 7 || !(f[2] in AT))
      throw new Error(`dq0d/factions.txt: line ${i + 1}: ${f.join(' | ')}`);
    rel[i][i] = AT.f;
    f[3].split(',').map(s => s.trim()).filter(s => s && s !== '-').forEach(s => {
      const [n, a] = s.split(/\s+/), j = names.indexOf(n);
      if (j < 0 || !(a in AT)) throw new Error(`dq0d/factions.txt: line ${i + 1}: ${s}`);
      rel[i][j] = rel[j][i] = Math.min(rel[i][j], rel[j][i], AT[a]);   // (The worse of the two ways)
    });
  });
  facs.forEach((f, i) => {
    const at = base + FREC * (i + 1);
    out.write(f[0], at, 'latin1');
    if (f[1] !== '-') out.write(f[1], at + 12, 'latin1');
    out[at + 20] = AT[f[2]];
    rel[i].forEach((v, j) => { out[at + 21 + j + 1] = v; });
    out[at + 21] = AT.i;                         // (To none: indifferent)
  });
  fs.writeFileSync(path.join(objdir, '..', 'dq0d.dat'), out);

  // The deities (dq0g.dat), first: the peoples name them
  const ds = blocks(path.join(dir, 'deities.txt'), 'deity'), dnames = ds.map(d => d.f.deity[0]);
  if (ds.length >= 16) throw new Error('dq0d/deities.txt: more than 15 deities');
  const dw = words(LREC * (ds.length + 1)), dout = Buffer.alloc(LREC * (ds.length + 1));
  dout[0] = ds.length + 1;
  const dbits = (v, what, n) => v === '-' ? 0 : v.split(/\s+/).reduce((a, w) => {
    if (dnames.indexOf(w) < 0) throw new Error(`dq0d/deities.txt: line ${n}: ${what}: no deity ${w}`);
    return a | 1 << (dnames.indexOf(w) + 1);
  }, 0);
  ds.forEach((d, i) => {
    const at = LREC * (i + 1), f = d.f, bad = w => { throw new Error(`dq0d/deities.txt: line ${d.n}: ${w}`); };
    for (const k of ['deity', 'title', 'domains', 'temper', 'followers', 'rite', 'feast', 'number', 'colours', 'sign',
      'temple', 'burial', 'servants', 'enemies']) if (!f[k]) bad('no ' + k);
    if (!/^[A-Z][a-z]{1,10}$/.test(f.deity[0])) bad('its name: one word, 11 letters at most');
    dout.write(f.deity[0], at, 'latin1');
    const dom = f.domains[0].split(/\s+/).reduce((a, w) => {
      if (DOMAINS.indexOf(w) < 0) bad('no domain ' + w);
      return (a | 1 << DOMAINS.indexOf(w)) >>> 0;
    }, 0);
    dout.writeUInt32LE(dom, at + 12);
    if (GOOD.indexOf(f.temper[0]) < 0 || ORDER.indexOf(f.temper[1]) < 0) bad('its temper');
    dout[at + 16] = GOOD.indexOf(f.temper[0]);
    dout[at + 17] = ORDER.indexOf(f.temper[1]);
    dout[at + 18] = +f.number[0];
    dout[at + 19] = f.rite[0] === '-' ? 0 : +f.rite[0];
    if (BURIALS.indexOf(f.burial[0]) < 0) bad('its burial');
    dout[at + 20] = BURIALS.indexOf(f.burial[0]);
    dout.writeUInt16LE(dbits(f.enemies[0], 'its enemies', d.n), at + 22);
    dout.writeUInt32LE(f.servants[0] === '-' ? 0 : f.servants[0].split(/\s+/).reduce((a, k) => {
      if (keys.indexOf(k) < 0) bad('no kind ' + k);
      return (a | 1 << (keys.indexOf(k) + 1)) >>> 0;
    }, 0), at + 24);
    [f.title[0], f.followers[0], f.rite[1], f.feast[0], f.colours[0], f.sign[0], f.temple[0]]
      .forEach((s, k) => dout.writeUInt16LE(dw.add(s), at + 28 + 2 * k));
  });
  fs.writeFileSync(path.join(objdir, '..', 'dq0g.dat'), Buffer.concat([dout, dw.data]));
  fs.writeFileSync(path.join(objdir, '..', 'dq0g.txt'), dnames.join(' '), 'latin1');  // (Their names, the commands' words)

  // The peoples (dq0f.dat)
  const ps = blocks(path.join(dir, 'peoples.txt'), 'people');
  const pw = words(LREC * (ps.length + 1)), pout = Buffer.alloc(LREC * (ps.length + 1));
  pout[0] = ps.length + 1;
  ps.forEach((p, i) => {
    const at = LREC * (i + 1), f = p.f, bad = w => { throw new Error(`dq0d/peoples.txt: line ${p.n}: ${w}`); };
    for (const k of ['people', 'kinds', 'tongue', 'lands', 'climate', 'builds', 'styles', 'deities', 'walls', 'doors',
      'light', 'marks', 'rooms', 'uses']) if (!f[k]) bad('no ' + k);
    if (f.people[0].length > 23 || !f.people[1] || f.people[1].length > 7 || f.tongue[0].length > 7) bad('its name');
    pout.write(f.people[0], at, 'latin1');
    pout.write(f.people[1], at + 24, 'latin1');
    const ks = f.kinds[0] === '-' ? [] : f.kinds[0].split(/\s+/);
    if (ks.length > 4) bad('more than 4 kinds');
    ks.forEach((k, j) => { if (keys.indexOf(k) < 0) bad('no kind ' + k); pout[at + 32 + j] = keys.indexOf(k) + 1; });
    pout.write(f.tongue[0], at + 36, 'latin1');
    pout[at + 44] = f.builds[0].split(/\s+/).reduce((a, w) => {
      if (PURPOSES.indexOf(w) < 0) bad('no purpose ' + w);
      return a | 1 << PURPOSES.indexOf(w);
    }, 0);
    pout[at + 45] = f.styles[0].split(/\s+/).reduce((a, w) => {
      if (!(w in STYLES)) bad('no style ' + w);
      return a | 1 << STYLES[w];
    }, 0);
    pout.writeUInt16LE(dbits(f.deities[0], 'its deities', p.n), at + 46);
    pout[at + 48] = bits(f.lands[0].replace(/\s+/g, ''), LANDS, 'peoples.txt: lands');
    pout[at + 49] = bits(f.climate[0].replace(/\s+/g, ''), CLIMATES, 'peoples.txt: climate');
    [f.walls[0], f.doors[0], f.light[0], f.marks[0]].forEach((s, k) => pout.writeUInt16LE(pw.add(s), at + 50 + 2 * k));
    if (f.rooms.length !== 8 || f.uses.length !== 12) bad('eight rooms\' words and twelve uses\'');
    if (f.rooms.concat(f.uses).some(w => !w || w.length > 15)) bad('a room\'s word: 15 letters at most');
    pout.writeUInt16LE(pw.list(f.rooms), at + 58);
    pout.writeUInt16LE(pw.list(f.uses), at + 60);
  });
  fs.writeFileSync(path.join(objdir, '..', 'dq0f.dat'), Buffer.concat([pout, pw.data]));
  return { kinds: kinds.length, factions: facs.length, deities: ds.length, peoples: ps.length };
}

module.exports = { build };
