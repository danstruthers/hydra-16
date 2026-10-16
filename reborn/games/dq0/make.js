// make.js - Dingus Quest Zero's data files, after its build (build.js calls build): its overlays' file
// (obj/games/dq0.hyx.ovl, ld65's: each overlay filled out to 8K with $FF) cut after the last overlay that has code,
// so the game reads no more than it needs as it starts; and its themes (themes/*.txt, in their files' order: the
// format's in the first) made obj/games/dq0t.dat, THEME_SIZE bytes a theme (theme.h's struct theme, then its words).
'use strict';
const fs = require('fs');
const path = require('path');

const THEME_SIZE = 0x1000, HEAD = 982, NTKINDS = 8, NPIECES = 8, NRULES = 8, TSTRATA = 8;    // theme.h's
const KIND = 36;                                                    // items.h's struct kind
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

// A theme's file read: its lines, each its key and its values (a line begun with a space goes on with the last: its
// last value, or with a bar first, more values)
function lines(f) {
  const out = [];
  fs.readFileSync(f, 'latin1').split(/\r?\n/).forEach((l, i) => {
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

function theme(f, keys, count) {
  const b = Buffer.alloc(THEME_SIZE);
  let words = HEAD;
  const bad = (l, why) => { throw new Error(`${path.basename(f)}: line ${l.n}: ${why}`); };
  const text = (l, s) => {                                          // Its words put after the record: their offset
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
  const kinds = [], strata = [], uses = USES.slice();
  let npieces = 0, nrules = 0;
  b[0] = count;
  for (const l of lines(f)) {
    const v = l.v;
    switch (l.key) {
      case 'theme': str(l, v[0], 4, 32); break;
      case 'mission':
        b[1] = v[0] === 'both' ? 3 : v[0] === 'rule' ? 2 : v[0] === 'escape' ? 1 : bad(l, 'escape, rule or both');
        break;
      case 'story': b.writeUInt16LE(text(l, v[0]), 768); break;
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
        b.writeUInt32LE(m >>> 0, 380);
        break;
      }
      case 'kind': {
        if (kinds.length >= NTKINDS || v.length < 8) bad(l, 'a kind: name, ways, weight, bulk, size, ...');
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
        b[at + 14] = flags;
        b[at + 15] = Math.min(255, 4 * +v[2]);
        b[at + 16] = +v[3];
        b[at + 17] = +(v[8] || 0);
        b[at + 18] = +(v[9] || 0);
        b[at + 21] = +v[4];
        b[at + 30] = mat;
        b[at + 31] = craft;
        b.writeUInt16LE(+v[7], at + 32);
        b[at + 34] = keeps;
        kinds.push(v[0]);
        break;
      }
      case 'piece': {
        const k = kinds.indexOf(v[0]), d = (v[1] || '').split('-').map(Number), at = 672 + 5 * npieces;
        if (npieces >= NPIECES || k < 0 || d.length !== 2 || !(d[0] <= d[1])) bad(l, 'a piece: kind, strata ...');
        [k, d[0], d[1], v[2] === '-' ? 0xFF : +v[2], +(v[3] || 1)].forEach((x, i) => { b[at + i] = x; });
        ++npieces;
        break;
      }
      case 'rule': {
        const verb = TR.indexOf(v[0]), at = 712 + 7 * nrules, kind = s => {
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
        b.writeUInt16LE(text(l, w), at + 5);
        ++nrules;
        break;
      }
      case 'lock': b.writeUInt16LE(text(l, v[0]), 770); break;
      case 'opened': b.writeUInt16LE(text(l, v[0]), 772); break;
      case 'ending':
        if (!ENDS[v[0]]) bad(l, 'an ending: escaped, ruled or died');
        b.writeUInt16LE(text(l, v[1]), 774 + 2 * ENDS[v[0]]);
        break;
      default: bad(l, `no key ${l.key}`);
    }
  }
  if (!strata.length || !b[4] || !b[1]) throw new Error(`${path.basename(f)}: no name, mission or strata`);
  for (let k = 0; k < TSTRATA; ++k) {                               // (Past the last stratum, the last again)
    const s = strata[Math.min(k, strata.length - 1)];
    b.write(s[0], 52 + 20 * k, 'latin1');
    b[36 + 2 * k] = STYLES[s[1][0]];
    b[37 + 2 * k] = STYLES[s[1][s[1].length - 1]];
    const p = (s[2] || 'halls').split(' ');                         // (Its purpose: a keep fortified, unless open)
    b[974 + k] = PURPOSES.indexOf(p[0]) | (p[1] === 'fortified' || (p[0] === 'keep' && p[1] !== 'open') ? 0x80 : 0);
  }
  uses.forEach((w, k) => {
    if (w.length >= 16) throw new Error(`${path.basename(f)}: "${w}" is longer than 15`);
    b.write(w, 782 + 16 * k, 'latin1');
  });
  b[2] = npieces;
  b[3] = nrules;
  return { data: b, used: words };
}

function build({ dir, objdir, say = () => {} }) {
  const f = path.join(objdir, '..', 'dq0.hyx.ovl');
  const data = fs.readFileSync(f);
  let n = data.length / 0x2000;
  while (n > 1 && data.subarray((n - 1) * 0x2000, n * 0x2000).every(b => b === 0xFF)) n--;
  if (n * 0x2000 !== data.length) fs.writeFileSync(f, data.subarray(0, n * 0x2000));
  const keys = fs.readFileSync(path.join(dir, '..', 'dq0d', 'kinds.txt'), 'latin1').split(/\r?\n/)
    .filter(l => l.trim() && !l.startsWith('#')).map(l => l.split('|')[1].trim());
  const files = fs.readdirSync(path.join(dir, 'themes')).filter(t => t.endsWith('.txt')).sort();
  const themes = files.map(t => theme(path.join(dir, 'themes', t), keys, files.length));
  fs.writeFileSync(path.join(objdir, '..', 'dq0t.dat'), Buffer.concat(themes.map(t => t.data)));
  return { overlays: n, themes: themes.length, words: themes.map(t => t.used) };
}

module.exports = { build };
