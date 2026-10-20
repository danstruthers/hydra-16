// make.js - Dingus Quest Zero's data files, after its build (build.js calls build): its overlays' file
// (obj/games/dq0.hyx.ovl, ld65's: each overlay filled out to 8K with $FF) cut after the last overlay that has code,
// so the game reads no more than it needs as it starts; and its stand-in themes (themes/*.txt, in their files' order:
// the format's in the first) made obj/games/dq0t.dat, THEME_SIZE bytes a theme (theme.h's struct theme, then its
// words), each one checked first (the theme checker: every goal reachable, every faction placed).  A game's own theme,
// its history's, is themegen's (games/themegen), made as the game begins in the same format and into the same record:
// themegen's compiler is this one's, in C, and the tests hold them to the same bytes (compile, here, is theirs).
'use strict';
const fs = require('fs');
const path = require('path');

const THEME_SIZE = 0x1000, HEAD = 1321, FILE_AT = 1305, NTKINDS = 8, NPIECES = 8, NRULES = 8, TSTRATA = 8;   // theme.h's
const TLAYERS = 3, TERAS = 6, TFACTIONS = 8, TPEOPLES = 8;
const KIND = 37;                                                    // items.h's struct kind
const WT_LB = 4, SQ_FT = 5;                                         // rs.h's units: a pound's quarters, a square's feet
const STYLES = { b: 0, n: 1, m: 2, x: 3 };                          // level.h's ST_*
const WAYS = { c: [0, 0x01], h: [0, 0x10], t: [0, 0x90], w: [0, 0x08], g: [0, 0x20], p: [0, 0x40],
  s: [1, 0x10], f: [1, 0x80] };                                     // items.h's K_* (0), KF_* (1: its keeps)
const MATS = ['none', 'wood', 'iron', 'steel', 'bronze', 'leather', 'cloth', 'bone', 'clay', 'stone', 'glass', 'silver',
  'other'];                                                         // items.h's MT_*
const CRAFTS = ['weapon', 'armour', 'metal', 'wood', 'hide', 'cloth', 'pottery', 'jewel', 'book', 'food', 'bone',
  'stone', 'glass'];                                                // rs.h's AP_*
const TR = ['read', 'combine', 'put', 'out'], TE = { '-': 0, open: 1, rule: 2 };   // theme.h's TR_*, TE_*
const ENDS = { escaped: 1, ruled: 2, died: 3 };                    // theme.h's END_*
const PURPOSES = ['halls', 'keep', 'temple', 'cellars', 'crypt', 'works'];   // theme.h's P_*
const USES = ['kitchen', 'pantry', 'barracks', 'armoury', 'chapel', 'bedchamber', 'solar', 'well room', 'cistern',
  'gatehouse', 'guardroom', 'workshop'];                            // (The uses' words, the theme's if it has them)
const DID = ['built', 'ruined', 'reused'];                          // theme.h's TL_*
const ENDINGS = ['abandoned', 'conquered', 'collapsed', 'flooded', 'cursed', 'held'];   // EN_*
const WORKS = ['keep', 'temple', 'mine', 'tomb', 'town', 'hold'];   // W_*
const GOALS = ['none', 'artifact', 'evil', 'prisoner', 'through'];  // G_*
const AT = { h: 0, u: 1, w: 2, i: 3, f: 4 };                        // hub.h's AT_*

// A theme's file read: its lines, each its key and its values (a line begun with a space goes on with the last: its
// last value, or with a bar first, more values)
function lines(text, f) {
  const out = [];
  text.split(/\r?\n/).forEach((l, i) => {
    if (!l.trim() || l.startsWith('#')) return;
    if (/^\s/.test(l)) {
      if (!out.length) throw new Error(`${f}: line ${i + 1}: goes on with nothing`);
      const v = out[out.length - 1].v, t = l.trim();
      if (t.startsWith('|')) v.push(...t.slice(1).split('|').map(s => s.trim()));   // (More values)
      else v[v.length - 1] += ' ' + t;
      return;
    }
    const v = l.split('|').map(s => s.trim());
    out.push({ key: v.shift(), v, n: i + 1 });
  });
  return out;
}

// The bestiary (dq0d/kinds.txt): each kind's key, the styles it lives in, its strata (for the checker)
function bestiary(dir) {
  return fs.readFileSync(path.join(dir, '..', 'dq0d', 'kinds.txt'), 'latin1').split(/\r?\n/)
    .filter(l => l.trim() && !l.startsWith('#')).map(l => l.split('|').map(s => s.trim()))
    .map(f => ({ key: f[1], styles: f[3], strata: f[4].split('-').map(Number) }));
}

// A theme's text compiled into its record (theme.h's struct theme, then its words: THEME_SIZE bytes): OUT { data,
// used (its words' end), t (what it says, for the checker) }.  Throws, its line, at what's wrong
function compile(text, f, beasts, count) {
  const keys = beasts.map(k => k.key);
  const b = Buffer.alloc(THEME_SIZE);
  let words = HEAD;
  const bad = (l, why) => { throw new Error(`${f}: line ${l.n}: ${why}`); };
  const text_ = (l, s) => {                                         // Its words put after the record: their offset
    const at = words;
    if (at + s.length + 1 > THEME_SIZE) bad(l, 'more words than the theme has room for');
    b.write(s, at, 'latin1');
    words += s.length + 1;
    return at;
  };
  const str = (l, s, at, max) => {
    if (s.length >= max) bad(l, `"${s}" is longer than ${max - 1}`);
    b.write(s, at, 'latin1');
  };
  const num = (l, s, per, max, what) => {                           // A value in rs.h's unit: the data's (pounds,
    const n = +(s || 0) * per;                                      //   feet, pints, steps) made the game's, once
    if (!Number.isInteger(n) || n < 0) bad(l, `${what}: ${s}?`);
    return Math.min(n, max);
  };
  const range = (l, s) => {                                         // "first-last" (or one): [first, last]
    const d = String(s || '').split('-').map(Number);
    if (d.length === 1) d.push(d[0]);
    if (d.length !== 2 || !d.every(Number.isInteger) || d[0] > d[1] || d[1] > 255) bad(l, `strata ${s}: first-last`);
    return d;
  };
  const kinds = [], strata = [], uses = USES.slice(), peoples = [], eras = [], factions = [], layers = [];
  const t = { mission: 0, kinds: [], pieces: [], rules: [], factions, strata, goal: null, eras };
  let npieces = 0, nrules = 0, dens = null;
  b[0] = count;
  for (const l of lines(text, f)) {
    const v = l.v;
    switch (l.key) {
      case 'theme': str(l, v[0], 4, 32); break;
      case 'mission':
        b[1] = t.mission = v[0] === 'both' ? 3 : v[0] === 'rule' ? 2 : v[0] === 'escape' ? 1 : bad(l, 'escape, rule or both');
        break;
      case 'story': b.writeUInt16LE(text_(l, v[0]), 776); break;
      case 'stratum':
        if (strata.length >= TSTRATA || !/^[bnmx]{1,2}$/.test(v[1] || '')) bad(l, 'a name, and one or two styles');
        if (v[2] && !/^(halls|keep|temple|cellars|crypt|works)( fortified| open)?$/.test(v[2])) {
          bad(l, 'a purpose: halls, keep, temple, cellars, crypt or works, maybe fortified or open');
        }
        strata.push(v);
        break;
      case 'rooms':
        if (v.length !== 8) bad(l, 'eight words for rooms');
        v.forEach((w, k) => str(l, w, 212 + 16 * k, 16));
        break;
      case 'uses':
        if (v.length !== USES.length) bad(l, `${USES.length} words for the uses`);
        v.forEach((w, k) => { uses[k] = w; });
        break;
      case 'light': str(l, v[0], 340, 40); break;
      case 'denizens': {
        let m = 0;
        for (const k of v[0].split(/\s+/)) {
          if (k === '*') m = 0xFFFFFFFE;
          else if (keys.indexOf(k) < 0) bad(l, `no denizen ${k} in dq0d/kinds.txt`);
          else m |= 1 << (keys.indexOf(k) + 1);
        }
        dens = m >>> 0;
        break;
      }
      case 'kind': {
        if (kinds.length >= NTKINDS || v.length < 8) bad(l, 'a kind: name, ways, weight, bulk, how far seen, ...');
        const at = 384 + KIND * kinds.length, mat = MATS.indexOf(v[5]), craft = CRAFTS.indexOf(v[6]);
        if (mat < 0 || craft < 0) bad(l, 'its material or its craft');
        str(l, v[0], at, 14);
        let flags = 0, keeps = 0;
        for (const c of v[1] === '-' ? '' : v[1]) {
          if (!WAYS[c]) bad(l, `a way ${c}`);
          if (WAYS[c][0]) keeps |= WAYS[c][1]; else flags |= WAYS[c][1];
        }
        if (v[8] && !(keeps & 0x10)) {          // (Room: a container's, keeping things whole and small)
          flags |= 0x01;
          keeps |= 0x03;
        }
        if (+v[4] % SQ_FT) bad(l, `how far off it's made out: feet, a square's ${SQ_FT} each`);
        b[at + 14] = flags;
        b[at + 15] = num(l, v[2], WT_LB, 255, 'its weight, pounds (to a quarter)');
        b[at + 16] = num(l, v[3], 1, 255, 'its bulk, pints');
        b[at + 17] = num(l, v[8], 1, 255, 'its room, pints');
        b.writeUInt16LE(num(l, v[9], WT_LB, 0xFFFF, 'the weight it bears, pounds (to a quarter)'), at + 18);
        b[at + 22] = num(l, v[4], 1, 15 * SQ_FT, 'how far off it\'s made out, feet') / SQ_FT;   // (Squares)
        b[at + 31] = mat;
        b[at + 32] = craft;
        b.writeUInt16LE(num(l, v[7], 1, 0xFFFF, 'its worth, pennies'), at + 33);
        b[at + 35] = keeps;
        kinds.push(v[0]);
        t.kinds.push({ name: v[0], fixed: (keeps & 0x80) !== 0 });
        break;
      }
      case 'piece': {
        const k = kinds.indexOf(v[0]), d = (v[1] || '').split('-').map(Number), at = 680 + 5 * npieces;
        if (npieces >= NPIECES || k < 0 || d.length !== 2 || !(d[0] <= d[1])) bad(l, 'a piece: kind, strata ...');
        [k, d[0], d[1], v[2] === '-' ? 0xFF : num(l, v[2], 1, 30, 'concealed, steps'), +(v[3] || 1)]
          .forEach((x, i) => { b[at + i] = x; });
        const era = v[4] && v[4] !== '-' ? +v[4] : 0;
        if (!Number.isInteger(era) || era > TERAS) bad(l, `its era ${v[4]}`);
        b[1112 + npieces] = era;
        t.pieces.push({ kind: k, d0: d[0], d1: d[1], qty: +(v[3] || 1), era, guard: v[5] && v[5] !== '-' ? v[5] : null, l, i: npieces });
        ++npieces;
        break;
      }
      case 'rule': {
        const verb = TR.indexOf(v[0]), at = 720 + 7 * nrules, kind = s => {
          const k = kinds.indexOf(s);
          if (k < 0) bad(l, `no kind ${s} of the theme's`);
          return k;
        };
        let a, bb = 0xFF, c = 0xFF, effect = 0, w;
        if (nrules >= NRULES || verb < 0) bad(l, 'a rule: read, combine, put or out');
        if (verb === 0) [a, w] = [kind(v[1]), v[2]];
        else if (verb === 1) {
          const m = /^(.+?)\s*\+\s*(.+?)\s*=\s*(.+)$/.exec(v[1]) || bad(l, 'combine X + Y = Z');
          [a, bb, c, w] = [kind(m[1]), kind(m[2]), kind(m[3]), v[2]];
        } else if (verb === 2) [a, bb, effect, w] = [kind(v[1]), kind(v[2]), TE[v[3]], v[4]];
        else [a, effect, w] = [kind(v[1]), TE[v[2]], v[3]];
        if (effect === undefined || !w) bad(l, 'its effect (- open rule) and its words');
        [verb, a, bb, c, effect].forEach((x, i) => { b[at + i] = x; });
        b.writeUInt16LE(text_(l, w), at + 5);
        t.rules.push({ verb, a, b: bb, c, effect });
        ++nrules;
        break;
      }
      case 'lock': b.writeUInt16LE(text_(l, v[0]), 778); break;
      case 'opened': b.writeUInt16LE(text_(l, v[0]), 780); break;
      case 'ending':
        if (!ENDS[v[0]]) bad(l, 'an ending: escaped, ruled or died');
        b.writeUInt16LE(text_(l, v[1]), 782 + 2 * ENDS[v[0]]);
        break;
      // ---- The history (HISTORY.md): the record keeps what the engine builds by; the rest is the file's to read
      case 'seed': case 'region': break;
      case 'people':
        if (peoples.length >= TPEOPLES || v.length < 4 || !(+v[2] >= 1 && +v[2] <= 8)) bad(l, 'a people: name, what, tongue (1-8), builds');
        peoples.push({ name: v[0], tongue: +v[2] });
        break;
      case 'era': {
        const p = peoples.findIndex(x => x.name === v[0]), w = WORKS.indexOf(v[1]), e = ENDINGS.indexOf(v[2]);
        if (eras.length >= TERAS || p < 0 || w < 0 || e < 0 || !(+v[3] >= 0 && +v[3] <= 255)) {
          bad(l, 'an era: a people of the theme\'s, what they made, how it ended, how long ago (centuries)');
        }
        const at = 1082 + 5 * eras.length;
        [p, peoples[p].tongue, w, e, +v[3]].forEach((x, i) => { b[at + i] = x; });
        eras.push({ people: p, work: w, end: e, ago: +v[3] });
        break;
      }
      case 'layer': {
        const sn = +v[0], era = +v[1], did = DID.indexOf(v[2]);
        if (!(sn >= 0 && sn < TSTRATA) || !(era >= 0 && era <= eras.length) || did < 0) bad(l, 'a layer: stratum, era, built ruined or reused, ...');
        if ((layers[sn] = layers[sn] || []).length >= TLAYERS) bad(l, `more than ${TLAYERS} layers for stratum ${sn}`);
        let work;
        if (did === 1) work = ENDINGS.indexOf(v[3]);
        else {
          const p = String(v[3] || '').split(' ');
          work = PURPOSES.indexOf(p[0]) | (p[1] === 'fortified' || (p[0] === 'keep' && p[1] !== 'open') ? 0x80 : 0);
        }
        if (work < 0 || (did !== 1 && (work & 0x7F) >= PURPOSES.length)) bad(l, did === 1 ? 'how it was ruined: ' + ENDINGS.join(', ') : 'its purpose');
        layers[sn].push({ era, did, work });
        break;
      }
      case 'faction': {
        if (factions.length >= TFACTIONS || v.length < 7 || v[0].length > 11) bad(l, 'a faction: name, kinds, strata, number, leader, lair, attitude, others');
        const ks = v[1].split(/\s+/).map(k => keys.indexOf(k));
        if (ks.some(k => k < 0)) bad(l, `no kind ${v[1]} in dq0d/kinds.txt`);
        const [d0, d1] = range(l, v[2]), lead = v[4] === '-' ? 0 : keys.indexOf(v[4]) + 1;
        if (lead < 0 || !(+v[3] >= 0 && +v[3] <= 255) || !(+v[5] >= 0 && +v[5] <= 255) || !(v[6] in AT)) bad(l, 'its number, its leader (a kind, or -), its lair (a stratum), its attitude (h u w i f)');
        const at = 1129 + 22 * factions.length;
        b.write(v[0], at, 'latin1');
        b.writeUInt32LE(ks.reduce((m, k) => m | 1 << (k + 1), 0) >>> 0, at + 12);
        [d0, d1, +v[5], +v[3], lead, AT[v[6]]].forEach((x, i) => { b[at + 16 + i] = x; });
        factions.push({ name: v[0], kinds: ks, d0, d1, lair: +v[5], leader: lead, l });
        break;
      }
      case 'goal': {
        const g = GOALS.indexOf(v[0]);
        if (g < 1 || !v[1] || !(+v[2] >= 0 && +v[2] <= 255)) bad(l, 'a goal: artifact, evil, prisoner or through; its name; its stratum; its guardians');
        t.goal = { kind: g, name: v[1], sn: +v[2], guard: v[3] && v[3] !== '-' ? v[3] : null, l };
        break;
      }
      default: bad(l, `no key ${l.key}`);
    }
  }
  if (!strata.length || !b[4] || !b[1]) throw new Error(`${f}: no name, mission or strata`);
  const faction = (l, name) => {                                    // (A faction's number, 1 ...)
    const k = factions.findIndex(x => x.name === name);
    if (k < 0) bad(l, `no faction ${name} of the theme's`);
    return k + 1;
  };
  for (let k = 0; k < TSTRATA; ++k) {                               // (Past the last stratum, the last again)
    const s = strata[Math.min(k, strata.length - 1)], ls = layers[k] || [];
    b.write(s[0], 52 + 20 * k, 'latin1');
    b[36 + 2 * k] = STYLES[s[1][0]];
    b[37 + 2 * k] = STYLES[s[1][s[1].length - 1]];
    const p = (s[2] || 'halls').split(' ');                         // (Its purpose: a keep fortified, unless open)
    let purpose = PURPOSES.indexOf(p[0]) | (p[1] === 'fortified' || (p[0] === 'keep' && p[1] !== 'open') ? 0x80 : 0);
    for (const y of ls) if (y.did !== 1) purpose = y.work;          // (Its layers' last built or reused, if it has any)
    b[982 + k] = purpose;
    b[1001 + k] = ls.length;
    ls.forEach((y, i) => { [y.era, y.did, y.work].forEach((x, j) => { b[1009 + 9 * k + 3 * i + j] = x; }); });
    const old = ls.find(y => y.era);                                // (Its age: its oldest layer's era's, else the dice)
    b[993 + k] = old ? Math.min(255, eras[old.era - 1].ago + 1) : 0;
  }
  uses.forEach((w, k) => {
    if (w.length >= 16) throw new Error(`${f}: "${w}" is longer than 15`);
    b.write(w, 790 + 16 * k, 'latin1');
  });
  b[2] = npieces;
  b[3] = nrules;
  if (t.goal) {
    b[990] = t.goal.kind;
    b[991] = t.goal.sn;
    b[992] = t.goal.guard ? faction(t.goal.l, t.goal.guard) : 0;
  }
  for (const p of t.pieces) b[1120 + p.i] = p.guard ? faction(p.l, p.guard) : 0;
  b[1081] = eras.length;
  b[1128] = factions.length;
  if (dens === null) dens = factions.reduce((m, x) => x.kinds.reduce((a, k) => a | 1 << (k + 1), m), 0) >>> 0;
  b.writeUInt32LE(dens, 380);
  return { data: b, used: words, t };
}

// The theme checker (HISTORY.md): every goal reachable (its mission's ending a rule can bring about, each rule's
// things to be had: a piece put down somewhere, or a thing a rule makes from things to be had, carried if it's to be
// carried; a far exit's stratum one there is), every faction placed (each of its kinds able to live somewhere in its
// strata, by the bestiary's styles and strata, its lair among them; a piece's guardians where the piece may lie).
// OUT: the faults, each a sentence
function check(t, beasts, f) {
  const faults = [], have = new Set(), made = new Set();
  for (const p of t.pieces) if (p.qty > 0) have.add(p.kind);
  const can = r => r.verb === 0 ? have.has(r.a) :
    r.verb === 1 ? have.has(r.a) && have.has(r.b) && !t.kinds[r.a].fixed && !t.kinds[r.b].fixed :
    r.verb === 2 ? have.has(r.a) && have.has(r.b) && !t.kinds[r.a].fixed : have.has(r.a) && !t.kinds[r.a].fixed;
  for (let more = true; more;) {                                    // (What the rules make from what's to be had)
    more = false;
    for (const r of t.rules) if (r.verb === 1 && can(r) && !have.has(r.c)) { have.add(r.c); made.add(r.c); more = true; }
  }
  const ends = new Set(t.rules.filter(can).map(r => r.effect));
  const through = t.goal && t.goal.kind === 4;
  if ((t.mission & 1) && !ends.has(1) && !through) faults.push('its way out can\'t be opened: no rule that opens it has its things to be had');
  if ((t.mission & 2) && !ends.has(2)) faults.push('the place can\'t be ruled: no rule that rules it has its things to be had');
  if (through && t.goal.sn >= 255) faults.push('its far exit is on no stratum');
  t.rules.forEach((r, i) => { if (!can(r)) faults.push(`rule ${i + 1} can't be done: its things aren't all to be had`); });
  const styleOf = sn => { const s = t.strata[Math.min(sn, t.strata.length - 1)]; return s[1]; };
  const lives = (k, sn) => sn >= beasts[k].strata[0] && sn <= beasts[k].strata[1] && [...styleOf(sn)].some(c => beasts[k].styles.includes(c));
  for (const x of t.factions) {
    const where = [];
    for (let sn = x.d0; sn <= x.d1 && sn < 64; sn++) if (x.kinds.some(k => lives(k, sn))) where.push(sn);
    if (!where.length) faults.push(`faction ${x.name}: none of its kinds lives in its strata ${x.d0}-${x.d1}`);
    else if (!where.includes(x.lair)) faults.push(`faction ${x.name}: its lair (stratum ${x.lair}) isn't where its kinds live (${where.join(' ')})`);
    if (x.leader && !lives(x.leader - 1, x.lair)) faults.push(`faction ${x.name}: its leader doesn't live in its lair's stratum`);
  }
  const guard = (name, d0, d1, what) => {
    const x = t.factions.find(y => y.name === name);
    if (x && (x.d1 < d0 || x.d0 > d1)) faults.push(`${what}'s guardians, ${name}, don't live where it lies (${d0}-${d1})`);
  };
  for (const p of t.pieces) if (p.guard) guard(p.guard, p.d0, p.d1, t.kinds[p.kind].name);
  if (t.goal && t.goal.guard) guard(t.goal.guard, t.goal.sn, t.goal.sn, 'the goal');
  for (const p of t.pieces) if (p.era > t.eras.length) faults.push(`${t.kinds[p.kind].name}: no era ${p.era}`);
  return faults.map(s => f + ': ' + s);
}

function build({ dir, objdir, say = () => {} }) {
  const f = path.join(objdir, '..', 'dq0.hyx.ovl');
  const data = fs.readFileSync(f);
  let n = data.length / 0x2000;
  while (n > 1 && data.subarray((n - 1) * 0x2000, n * 0x2000).every(b => b === 0xFF)) n--;
  if (n * 0x2000 !== data.length) fs.writeFileSync(f, data.subarray(0, n * 0x2000));
  const beasts = bestiary(dir);
  const files = fs.readdirSync(path.join(dir, 'themes')).filter(t => t.endsWith('.txt')).sort();
  const themes = files.map(t => compile(fs.readFileSync(path.join(dir, 'themes', t), 'latin1'), t, beasts, files.length));
  themes.forEach((t, i) => {                                       // (Its file's name, for popgen to read it: theme.h's file)
    if (files[i].length > 15) throw new Error(files[i] + ': a stand-in\'s name, 15 letters at most');
    t.data.write(files[i], FILE_AT, 'latin1');
  });
  const faults = themes.flatMap((t, i) => check(t.t, beasts, files[i]));
  if (faults.length) throw new Error('the theme checker:\n  ' + faults.join('\n  '));
  fs.writeFileSync(path.join(objdir, '..', 'dq0t.dat'), Buffer.concat(themes.map(t => t.data)));
  return { overlays: n, themes: themes.length, words: themes.map(t => t.used) };
}

module.exports = { build, compile, check, bestiary, THEME_SIZE, HEAD };
