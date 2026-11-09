// make.js - Dingus Quest Zero's data files, after its build (build.js calls build): its overlays' file packed
// (obj/games/dq0.ovp, ovlpack.js's: ld65's cut after the last overlay that has code, each bank's fill left out), so
// the game reads no more than it needs as it starts; and its stand-in themes (themes/*.txt, in their files' order:
// the format's in the first) made obj/games/dq0t.dat, THEME_SIZE bytes a theme (theme.h's struct theme, then its
// words), each one checked first (the theme checker: every goal reachable, every faction placed).  A game's own theme,
// its history's, is themegen's (games/themegen), made as the game begins in the same format and into the same record:
// themegen's compiler is this one's, in C, and the tests hold them to the same bytes (compile, here, is theirs).
'use strict';
const fs = require('fs');
const path = require('path');

// The headers' numbers (the build's, from the C as cc65 lays it out: hubgen.js), and struct theme's layout (its
// fields' offsets: T ('exit_sn'), T ('faction[2]'); a field of a struct of it, F ('tfaction', 'lair'))
const H = require('./hubgen.js').load(), C = H.C;
const { THEME_SIZE, TMINTS, NTKINDS, NPIECES, NRULES, TSTRATA, TLAYERS, TERAS, TFACTIONS, TPEOPLES } = C;   // theme.h's
const HEAD = H.size('theme'), KIND = H.size('kind');                // (struct theme's bytes, its words after; struct kind's)
const T = p => H.off('theme', p), F = (s, p) => H.off(s, p), LEN = (s, p) => H.size(s, p);
const { WT_LB, SQ_FT } = C;                                         // rs.h's units: a pound's quarters, a square's feet
const STYLES = { b: C.ST_BUILT, n: C.ST_NATURAL, m: C.ST_MINE, x: C.ST_MIXED };   // level.h's ST_*
const WAYS = { c: [0, C.K_CONTAINER], h: [0, C.K_HAND], t: [0, C.K_HAND | C.K_TWO], w: [0, C.K_WEAR], g: [0, C.K_HANG],
  p: [0, C.K_PLURAL], s: [1, C.KF_ON], f: [1, C.KF_FIXED] };       // items.h's K_* (0), KF_* (1: its keeps)
const MATS = H.words('MT_NONE');                                    // items.h's MT_*
const CRAFTS = H.words('AP_WEAPON');                                // rs.h's AP_*
const TR = H.words('TR_READ').filter(w => w !== 'copy');            // theme.h's TR_* (TR_COPY no rule's), TE_*
const TE = { '-': C.TE_NONE, open: C.TE_OPEN, rule: C.TE_RULE };
const ENDS = { left: C.END_LEFT & 3, escaped: C.END_ESCAPED, ruled: C.END_RULED, died: C.END_DIED };   // theme.h's END_* and 3
const PURPOSES = H.words('P_HALLS');                                // theme.h's P_*
const USES = ['kitchen', 'pantry', 'barracks', 'armoury', 'chapel', 'bedchamber', 'solar', 'well room', 'cistern',
  'gatehouse', 'guardroom', 'workshop'];                            // (The uses' words, the theme's if it has them)
const DID = H.words('TL_BUILT');                                    // theme.h's TL_*
const ENDINGS = H.words('EN_ABANDONED');                            // EN_*
const WORKS = H.words('W_KEEP');                                    // W_*
const GOALS = H.words('G_NONE');                                    // G_*
const AT = { h: C.AT_HOSTILE, u: C.AT_UNFRIENDLY, w: C.AT_WARY, i: C.AT_INDIFFERENT, f: C.AT_FRIENDLY };   // hub.h's AT_*

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
  b[T('count')] = count;
  for (const l of lines(text, f)) {
    const v = l.v;
    switch (l.key) {
      case 'theme': str(l, v[0], T('name'), LEN('theme', 'name')); break;
      case 'mission':
        t.mission = v[0] === 'both' ? C.TM_ESCAPE | C.TM_RULE : v[0] === 'rule' ? C.TM_RULE : v[0] === 'escape' ? C.TM_ESCAPE : bad(l, 'escape, rule or both');
        b[T('mission')] = b[T('mission')] & (C.TM_OPEN | C.TM_FELL) | t.mission;
        break;
      case 'start':                                                 // (How the character came in: theme.h's TM_FELL)
        if (v[0] !== 'walked' && v[0] !== 'fell') bad(l, 'walked or fell');
        b[T('mission')] |= v[0] === 'fell' ? C.TM_FELL : 0;
        break;
      case 'wayin':                                                 // (The way in open behind it: theme.h's TM_OPEN)
        if (v[0] !== 'open') bad(l, 'open, or no line');
        b[T('mission')] |= C.TM_OPEN;
        t.wayin = true;
        break;
      case 'wayout':                                                // (Another way out's stratum: theme.h's exit_sn)
        if (!(+v[0] >= 1 && +v[0] <= 63)) bad(l, 'another way out\'s stratum: 1 to 63');
        t.exitSn = b[T('exit_sn')] = +v[0];
        break;
      case 'story': b.writeUInt16LE(text_(l, v[0]), T('story')); break;
      case 'stratum':
        if (strata.length >= TSTRATA || !/^[bnmx]{1,2}$/.test(v[1] || '')) bad(l, 'a name, and one or two styles');
        if (v[2] && !/^(halls|keep|temple|cellars|crypt|works)( fortified| open)?$/.test(v[2])) {
          bad(l, 'a purpose: halls, keep, temple, cellars, crypt or works, maybe fortified or open');
        }
        strata.push(v);
        break;
      case 'rooms':
        if (v.length !== LEN('theme', 'room') / LEN('theme', 'room[0]')) bad(l, 'eight words for rooms');
        v.forEach((w, k) => str(l, w, T('room[' + k + ']'), LEN('theme', 'room[0]')));
        break;
      case 'uses':
        if (v.length !== USES.length) bad(l, `${USES.length} words for the uses`);
        v.forEach((w, k) => { uses[k] = w; });
        break;
      case 'light': str(l, v[0], T('light'), LEN('theme', 'light')); break;
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
        const at = T('kind[' + kinds.length + ']'), mat = MATS.indexOf(v[5]), craft = CRAFTS.indexOf(v[6]);
        if (mat < 0 || craft < 0) bad(l, 'its material or its craft');
        str(l, v[0], at + F('kind', 'name'), LEN('kind', 'name'));
        let flags = 0, keeps = 0;
        for (const c of v[1] === '-' ? '' : v[1]) {
          if (!WAYS[c]) bad(l, `a way ${c}`);
          if (WAYS[c][0]) keeps |= WAYS[c][1]; else flags |= WAYS[c][1];
        }
        if (v[8] && !(keeps & C.KF_ON)) {          // (Room: a container's, keeping things whole and small)
          flags |= C.K_CONTAINER;
          keeps |= C.KF_SOLID | C.KF_SMALL;
        }
        if (+v[4] % SQ_FT) bad(l, `how far off it's made out: feet, a square's ${SQ_FT} each`);
        b[at + F('kind', 'flags')] = flags;
        b[at + F('kind', 'weight')] = num(l, v[2], WT_LB, 255, 'its weight, pounds (to a quarter)');
        b[at + F('kind', 'bulk')] = num(l, v[3], 1, 255, 'its bulk, pints');
        b[at + F('kind', 'cap_bulk')] = num(l, v[8], 1, 255, 'its room, pints');
        b.writeUInt16LE(num(l, v[9], WT_LB, 0xFFFF, 'the weight it bears, pounds (to a quarter)'), at + F('kind', 'cap_weight'));
        b[at + F('kind', 'seen')] = num(l, v[4], 1, 15 * SQ_FT, 'how far off it\'s made out, feet') / SQ_FT;   // (Squares)
        b[at + F('kind', 'mat')] = mat;
        b[at + F('kind', 'craft')] = craft;
        b.writeUInt16LE(num(l, v[7], 4, 0xFFFF, 'its worth, pennies (to a farthing: 2.25)'), at + F('kind', 'value'));   // (Farthings)
        b[at + F('kind', 'keeps')] = keeps;
        kinds.push(v[0]);
        t.kinds.push({ name: v[0], fixed: (keeps & C.KF_FIXED) !== 0 });
        break;
      }
      case 'piece': {
        const k = kinds.indexOf(v[0]), d = (v[1] || '').split('-').map(Number), at = T('piece[' + npieces + ']');
        if (npieces >= NPIECES || k < 0 || d.length !== 2 || !(d[0] <= d[1])) bad(l, 'a piece: kind, strata ...');
        [k, d[0], d[1], v[2] === '-' ? 0xFF : num(l, v[2], 1, 30, 'concealed, steps'), +(v[3] || 1)]
          .forEach((x, i) => { b[at + F('tpiece', ['kind', 'd0', 'd1', 'hide', 'qty'][i])] = x; });
        const era = v[4] && v[4] !== '-' ? +v[4] : 0;
        if (!Number.isInteger(era) || era > TERAS) bad(l, `its era ${v[4]}`);
        b[T('pera[' + npieces + ']')] = era;
        t.pieces.push({ kind: k, d0: d[0], d1: d[1], qty: +(v[3] || 1), era, guard: v[5] && v[5] !== '-' ? v[5] : null, l, i: npieces });
        ++npieces;
        break;
      }
      case 'rule': {
        const verb = TR.indexOf(v[0]), at = T('rule[' + nrules + ']'), kind = s => {
          const k = kinds.indexOf(s);
          if (k < 0) bad(l, `no kind ${s} of the theme's`);
          return k;
        };
        let a, bb = 0xFF, c = 0xFF, effect = 0, w;
        if (nrules >= NRULES || verb < 0) bad(l, 'a rule: read, combine, put, out or take');
        if (verb === C.TR_READ) [a, w] = [kind(v[1]), v[2]];
        else if (verb === C.TR_COMBINE) {
          const m = /^(.+?)\s*\+\s*(.+?)\s*=\s*(.+)$/.exec(v[1]) || bad(l, 'combine X + Y = Z');
          [a, bb, c, w] = [kind(m[1]), kind(m[2]), kind(m[3]), v[2]];
        } else if (verb === C.TR_PUT) [a, bb, effect, w] = [kind(v[1]), kind(v[2]), TE[v[3]], v[4]];
        else [a, effect, w] = [kind(v[1]), TE[v[2]], v[3]];
        if (effect === undefined || !w) bad(l, 'its effect (- open rule) and its words');
        [verb, a, bb, c, effect].forEach((x, i) => { b[at + F('trule', ['verb', 'a', 'b', 'c', 'effect'][i])] = x; });
        b.writeUInt16LE(text_(l, w), at + F('trule', 'words'));
        t.rules.push({ verb, a, b: bb, c, effect });
        ++nrules;
        break;
      }
      case 'lock': b.writeUInt16LE(text_(l, v[0]), T('lock')); break;
      case 'opened': b.writeUInt16LE(text_(l, v[0]), T('opened')); break;
      case 'ending':
        if (ENDS[v[0]] === undefined) bad(l, 'an ending: escaped, ruled, died or left');
        b.writeUInt16LE(text_(l, v[1]), T('end[' + ENDS[v[0]] + ']'));
        if (v[0] === 'left') t.leftWords = true;
        break;
      // ---- The history (HISTORY.md): the record keeps what the engine builds by; the rest is the file's to read
      case 'seed': case 'region': break;
      // ---- Why the character came, and what solving brings (the plan's step 7): the file's to read (dq0acct's), not the record's
      case 'why':
        if (GOALS.indexOf(v[0]) < 1 || !v[1]) bad(l, 'why: its goal kind, its reason, its patron');
        break;
      case 'reward':
        if (v.length < 4 || !/^\d+$/.test(v[0]) || +v[0] > 9999 || !/^-?\d$/.test(v[1]) || !v[2] || !v[3]) {
          bad(l, 'a reward: its pennies (9999 at most), its standing (-9 to 9), a thing (a kind of the catalogue\'s, or -), its words');
        }
        break;
      case 'people':
        if (peoples.length >= TPEOPLES || v.length < 4 || !(+v[2] >= 1 && +v[2] <= 8)) bad(l, 'a people: name, what, tongue (1-8), builds');
        if (v.slice(4, 6).some(w => !/^[a-z]{1,8}$/.test(w))) bad(l, 'a people\'s tongue and script: a word each, 8 letters at most');
        [4, 5].forEach((k, i) => { if (v[k]) b.write(v[k], T((i ? 'pscript[' : 'ptongue[') + peoples.length + ']'), 'latin1'); });   // (Its tongue's and script's words: reading's)
        peoples.push({ name: v[0], tongue: +v[2] });
        break;
      case 'era': {
        const p = peoples.findIndex(x => x.name === v[0]), w = WORKS.indexOf(v[1]), e = ENDINGS.indexOf(v[2]);
        if (eras.length >= TERAS || p < 0 || w < 0 || e < 0 || !(+v[3] >= 0 && +v[3] <= 255)) {
          bad(l, 'an era: a people of the theme\'s, what they made, how it ended, how long ago (centuries)');
        }
        const at = T('era[' + eras.length + ']');
        [p, peoples[p].tongue, w, e, +v[3]].forEach((x, i) => { b[at + F('tera', ['people', 'tongue', 'work', 'end', 'ago'][i])] = x; });
        eras.push({ people: p, work: w, end: e, ago: +v[3] });
        break;
      }
      case 'layer': {
        const sn = +v[0], era = +v[1], did = DID.indexOf(v[2]);
        if (!(sn >= 0 && sn < TSTRATA) || !(era >= 0 && era <= eras.length) || did < 0) bad(l, 'a layer: stratum, era, built ruined or reused, ...');
        if ((layers[sn] = layers[sn] || []).length >= TLAYERS) bad(l, `more than ${TLAYERS} layers for stratum ${sn}`);
        let work;
        if (did === C.TL_RUINED) work = ENDINGS.indexOf(v[3]);
        else {
          const p = String(v[3] || '').split(' ');
          work = PURPOSES.indexOf(p[0]) | (p[1] === 'fortified' || (p[0] === 'keep' && p[1] !== 'open') ? C.P_FORT : 0);
        }
        if (work < 0 || (did !== C.TL_RUINED && (work & ~C.P_FORT) >= PURPOSES.length)) bad(l, did === C.TL_RUINED ? 'how it was ruined: ' + ENDINGS.join(', ') : 'its purpose');
        layers[sn].push({ era, did, work });
        break;
      }
      case 'faction': {
        if (factions.length >= TFACTIONS || v.length < 7 || v[0].length >= LEN('tfaction', 'name')) bad(l, 'a faction: name, kinds, strata, number, leader, lair, attitude, others');
        const ks = v[1].split(/\s+/).map(k => keys.indexOf(k));
        if (ks.some(k => k < 0)) bad(l, `no kind ${v[1]} in dq0d/kinds.txt`);
        const [d0, d1] = range(l, v[2]), lead = v[4] === '-' ? 0 : keys.indexOf(v[4]) + 1;
        if (lead < 0 || !(+v[3] >= 0 && +v[3] <= 255) || !(+v[5] >= 0 && +v[5] <= 255) || !(v[6] in AT)) bad(l, 'its number, its leader (a kind, or -), its lair (a stratum), its attitude (h u w i f)');
        const at = T('faction[' + factions.length + ']');
        b.write(v[0], at + F('tfaction', 'name'), 'latin1');
        b.writeUInt32LE(ks.reduce((m, k) => m | 1 << (k + 1), 0) >>> 0, at + F('tfaction', 'kinds'));
        [d0, d1, +v[5], +v[3], lead, AT[v[6]]].forEach((x, i) => { b[at + F('tfaction', ['d0', 'd1', 'lair', 'number', 'leader', 'attitude'][i])] = x; });
        factions.push({ name: v[0], kinds: ks, d0, d1, lair: +v[5], leader: lead, l });
        break;
      }
      case 'mint': {                                                // (2026-10-28: who struck the coins found here)
        const era = v[2] === 'now' ? 0 : +v[2], ways = v[3] === undefined || v[3] === '-' ? '' : v[3];
        if (b[T('nmints')] >= TMINTS || !v[0] || v[0].length >= LEN('tmint', 'name') || !v[1] || v[1].length >= LEN('tmint', 'adj') || !(Number.isInteger(era) && era >= 0 && era <= TERAS) ||
            [...ways].some(c => !'lgph'.includes(c))) {
          bad(l, 'a mint: its name and its word (15 letters at most each), now or its era, its ways (l g p h, or -)');
        }
        const at = T('mint[' + b[T('nmints')]++ + ']');
        b.write(v[0], at + F('tmint', 'name'), 'latin1');
        b.write(v[1], at + F('tmint', 'adj'), 'latin1');
        b[at + F('tmint', 'era')] = era;
        b[at + F('tmint', 'ways')] = [...ways].reduce((m, c) => m | 1 << 'lgph'.indexOf(c), 0);
        break;
      }
      case 'goal': {
        const g = GOALS.indexOf(v[0]);
        if (g < 1 || !v[1] || !(+v[2] >= 0 && +v[2] <= 255)) bad(l, 'a goal: ' + GOALS.slice(1).join(', ') + '; its name; its stratum; its guardians');
        t.goal = { kind: g, name: v[1], sn: +v[2], guard: v[3] && v[3] !== '-' ? v[3] : null, l };
        break;
      }
      default: bad(l, `no key ${l.key}`);
    }
  }
  if (!strata.length || !b[T('name')] || !(b[T('mission')] & (C.TM_ESCAPE | C.TM_RULE))) throw new Error(`${f}: no name, mission or strata`);
  const faction = (l, name) => {                                    // (A faction's number, 1 ...)
    const k = factions.findIndex(x => x.name === name);
    if (k < 0) bad(l, `no faction ${name} of the theme's`);
    return k + 1;
  };
  for (let k = 0; k < TSTRATA; ++k) {                               // (Past the last stratum, the last again)
    const s = strata[Math.min(k, strata.length - 1)], ls = layers[k] || [];
    b.write(s[0], T('stratum[' + k + ']'), 'latin1');
    b[T('style[' + k + '][0]')] = STYLES[s[1][0]];
    b[T('style[' + k + '][1]')] = STYLES[s[1][s[1].length - 1]];
    const p = (s[2] || 'halls').split(' ');                         // (Its purpose: a keep fortified, unless open)
    let purpose = PURPOSES.indexOf(p[0]) | (p[1] === 'fortified' || (p[0] === 'keep' && p[1] !== 'open') ? C.P_FORT : 0);
    for (const y of ls) if (y.did !== C.TL_RUINED) purpose = y.work;          // (Its layers' last built or reused, if it has any)
    b[T('purpose[' + k + ']')] = purpose;
    b[T('nlayers[' + k + ']')] = ls.length;
    ls.forEach((y, i) => { [y.era, y.did, y.work].forEach((x, j) => { b[T('layer[' + k + '][' + i + ']') + F('tlayer', ['era', 'did', 'work'][j])] = x; }); });
    const old = ls.find(y => y.era);                                // (Its age: its oldest layer's era's, else the dice)
    b[T('sage[' + k + ']')] = old ? Math.min(255, eras[old.era - 1].ago + 1) : 0;
  }
  uses.forEach((w, k) => {
    if (w.length >= 16) throw new Error(`${f}: "${w}" is longer than 15`);
    b.write(w, T('uses[' + k + ']'), 'latin1');
  });
  b[T('npieces')] = npieces;
  b[T('nrules')] = nrules;
  if (t.goal) {
    b[T('goal')] = t.goal.kind;
    b[T('goal_sn')] = t.goal.sn;
    if (t.goal.kind === C.G_THROUGH) t.exitSn = b[T('exit_sn')] = t.goal.sn;      // (A far exit: the way out that is the goal)
    b[T('goal_guard')] = t.goal.guard ? faction(t.goal.l, t.goal.guard) : 0;
  }
  for (const p of t.pieces) b[T('pguard[' + p.i + ']')] = p.guard ? faction(p.l, p.guard) : 0;
  b[T('neras')] = eras.length;
  b[T('nfactions')] = factions.length;
  if (dens === null) dens = factions.reduce((m, x) => x.kinds.reduce((a, k) => a | 1 << (k + 1), m), 0) >>> 0;
  b.writeUInt32LE(dens, T('dens'));
  return { data: b, used: words, t };
}

// The theme checker (HISTORY.md): every goal reachable (its mission's ending a rule can bring about, each rule's
// things to be had: a piece put down somewhere, or a thing a rule makes from things to be had, carried if it's to be
// carried; a far exit's stratum one there is), a way out a character can use with nothing but its legs (Q1: the way in open, or
// another way out, a stair the maker always puts on its stratum; a sky shaft isn't counted, by dice and maybe out of reach; and
// words for leaving), every faction placed (each of its kinds able to live somewhere in its
// strata, by the bestiary's styles and strata, its lair among them; a piece's guardians where the piece may lie).
// OUT: the faults, each a sentence
function check(t, beasts, f) {
  const faults = [], have = new Set(), made = new Set();
  for (const p of t.pieces) if (p.qty > 0) have.add(p.kind);
  const can = r => r.verb === C.TR_READ ? have.has(r.a) :
    r.verb === C.TR_COMBINE ? have.has(r.a) && have.has(r.b) && !t.kinds[r.a].fixed && !t.kinds[r.b].fixed :
    r.verb === C.TR_PUT ? have.has(r.a) && have.has(r.b) && !t.kinds[r.a].fixed : have.has(r.a) && !t.kinds[r.a].fixed;
  for (let more = true; more;) {                                    // (What the rules make from what's to be had)
    more = false;
    for (const r of t.rules) if (r.verb === C.TR_COMBINE && can(r) && !have.has(r.c)) { have.add(r.c); made.add(r.c); more = true; }
  }
  const ends = new Set(t.rules.filter(r => r.verb !== C.TR_READ && can(r)).map(r => r.effect));   // (Proved unread: no read rule counted, step 9's)
  const through = t.goal && t.goal.kind === C.G_THROUGH;
  if ((t.mission & C.TM_ESCAPE) && !ends.has(C.TE_OPEN) && !through) faults.push('its way out can\'t be opened: no rule that opens it has its things to be had');
  if ((t.mission & C.TM_RULE) && !ends.has(C.TE_RULE)) faults.push('the place can\'t be ruled: no rule that rules it has its things to be had');
  if (through && t.goal.sn >= 255) faults.push('its far exit is on no stratum');
  if (!t.wayin && !t.exitSn) faults.push('no way out to be had: the way in shut, and no other way out (wayin, wayout)');
  if (!t.leftWords) faults.push('no words for leaving it unsolved (ending | left)');
  t.rules.forEach((r, i) => { if (r.verb === C.TR_READ && r.effect) faults.push(`rule ${i + 1}: a writing read solves nothing (no puzzle needs reading: a writing's a hint)`); });
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

// The dispatch table's rows (actions.c's ROW (VB_*, ...)) held to actions.h's actions (the build's, from
// commands.actions): one a verb, in its order
function rowsSame(dir) {
  const want = H.enums.find(e => e[0] === 'VB_NONE').filter(n => n !== 'NACTIONS');
  const c = fs.readFileSync(path.join(dir, 'actions.c'), 'latin1').replace(/\/\*[\s\S]*?\*\//g, ' ');
  const have = [...c.matchAll(/\bROW[RP]?\s*\(\s*(VB_\w+)/g)].map(m => m[1]);   // (ROWR: a row with its refusals, stage 2;
                                                                                //   ROWP: and its request, stage 3)
  const at = want.findIndex((n, k) => have[k] !== n);
  if (at >= 0 || have.length !== want.length) throw new Error('games/dq0/actions.c\'s rows and actions.h differ' +
    (at >= 0 ? ': ' + want[at] + ' (actions.c has ' + (have[at] || 'none') + ')' : ': ' + have.length + ' rows'));
}

function build({ dir, objdir, say = () => {} }) {
  rowsSame(dir);
  const n = require('./ovlpack.js').pack(objdir, 'dq0').banks;
  const beasts = bestiary(dir);
  const files = fs.readdirSync(path.join(dir, 'themes')).filter(t => t.endsWith('.txt')).sort();
  const themes = files.map(t => compile(fs.readFileSync(path.join(dir, 'themes', t), 'latin1'), t, beasts, files.length));
  themes.forEach((t, i) => {                                       // (Its file's name, for popgen to read it: theme.h's file)
    if (files[i].length >= LEN('theme', 'file')) throw new Error(files[i] + ': a stand-in\'s name, 15 letters at most');
    t.data.write(files[i], T('file'), 'latin1');
  });
  const faults = themes.flatMap((t, i) => check(t.t, beasts, files[i]));
  if (faults.length) throw new Error('the theme checker:\n  ' + faults.join('\n  '));
  fs.writeFileSync(path.join(objdir, '..', 'dq0t.dat'), Buffer.concat(themes.map(t => t.data)));
  return { overlays: n, themes: themes.length, words: themes.map(t => t.used) };
}

module.exports = { build, compile, check, bestiary, lines, THEME_SIZE, HEAD };
