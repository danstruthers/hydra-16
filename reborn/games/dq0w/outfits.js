// outfits.js - (dq0w) the outfits (games/dq0/outfits.txt: the character's starting outfits by occupation, its skills'
// weapons and tools, the armour by an occupation's bent, the denizens' gear by their kinds) compiled into the world's
// data file, dq0o.dat (make.js's: the kinds by their names in the catalogue as it's linked), which the world reads into
// banks of its own as it starts and draws from (dq0w/outfit.c).  The file's layout (outfit.c's, little-endian):
//
//   'O' 'F' VERSION NSEC, then NSEC entries { hash (2), offset (2), flags (1) } (the section's name hashed: outfit.c's
//   hash; its flags, its head's first byte, for a draw that wants some: 204 entries at most),
//   then the sections (200 bytes each at most: outfit.c reads one whole, by its offset)
//   a section:  flags (SF_*: weapon 1, shield 2, free 4, kit 8, armed 16), bent (its bent's section's entry plus 1,
//               0 none), keep (the money kept, percent: 0 the tenth), make (its things' quality: 0xFF by the wealth,
//               else the least), mdie (the quality's die, 1d that, 0 none), purse n, purse m (its money NdM, not the
//               wealth's), nlines, the lines
//   a line:     its length (1, these bytes too), its chance (a percentage), info (its choices' count, bits 0-3;
//               LI_FREE given, LI_NEED bought first; LI_IF, an if mask follows, LI_NOT a not mask: 2 bytes each, a tag
//               of these, none of these), the choices
//   a choice:   weight, info (its things' count, bits 0-3; its weights' by tags, bits 4-5; CI_INSTEAD, a kind made
//               already it takes the place of follows), { tag, weight } x the weights, the instead, the things:
//               { kind, place (PL_*, bits 0-3) | flags (TF_* << 4), least, most (how many: 0 one) }
//   the index's last entries the denizens' kinds (dq0d/kinds.txt's, by number: "#" and a byte, the first 1) whose
//   key has a [role KEY] section, each that section's offset (the world's draw at a lair's placing, I_KIT's: by the
//   kind's number alone, 2026-10-27)
'use strict';
const fs = require('fs');

const VERSION = 2;                                      // (2: a section's flags in its index entry, 2026-10-27)
const TAGS = ['feudal', 'imperial', 'viking', 'tribal', 'sindarin', 'khuzan', 'slave', 'serf', 'unguilded', 'guilded', 'noble',
  'poor', 'average', 'rich', 'female', 'male'];
const PLACES = { worn: 1, hand: 2, off: 3, both: 4, belt: 5, pouch: 6, pack: 7, in: 8 };
const TFLAGS = { lit: 1, full: 2, own: 4, fine: 8 };
const SF = { weapon: 1, shield: 2, free: 4, kit: 8, armed: 16 };
const LC_FREE = 0x80, LC_NEED = 0x40, LI_FREE = 0x10, LI_NEED = 0x20, LI_IF = 0x40, LI_NOT = 0x80, CI_INSTEAD = 0x40;

// The world's hash of a section's name (outfit.c's: h = h * 31 + c, 16 bits)
function hash(s) {
  let h = 0;
  for (const c of Buffer.from(s, 'latin1')) h = (h * 31 + c) & 0xFFFF;
  return h;
}

function compile({ src, names, out, dens }) {
  if (!fs.existsSync(src)) { if (fs.existsSync(out)) fs.rmSync(out); return { sections: 0, bytes: 0 }; }
  const kind = new Map(names.map((n, i) => [n, i]).filter(([n]) => n));
  const lines = fs.readFileSync(src, 'latin1').split(/\r?\n/);
  const secs = [];
  let sec = null;
  const fail = (ln, why) => { throw new Error(src + ':' + (ln + 1) + ': ' + why); };
  const tagbit = (ln, t) => { const k = TAGS.indexOf(t); if (k < 0) fail(ln, 'no tag ' + t); return k; };
  lines.forEach((raw, ln) => {
    const l = raw.replace(/;.*$/, '').trim();
    if (!l) return;
    let m;
    if ((m = /^\[([^\]]+)\]\s*(.*)$/.exec(l))) {          // A section: its name, its header's words
      sec = { name: m[1].trim(), flags: 0, bent: '', keep: 0, make: 0xFF, mdie: 0, pn: 0, pm: 0, lines: [], ln };
      if (secs.some(s => s.name === sec.name)) fail(ln, 'section ' + sec.name + ' twice');
      secs.push(sec);
      const w = m[2].split(/\s+/).filter(Boolean);
      for (let i = 0; i < w.length; i++) {
        if (SF[w[i]] !== undefined) sec.flags |= SF[w[i]];
        else if (w[i] === 'bent') sec.bent = 'bent ' + w[++i];
        else if (w[i] === 'keep') sec.keep = +w[++i];
        else if (w[i] === 'make') { sec.make = +w[++i]; sec.mdie = +w[++i]; }
        else if (w[i] === 'purse') { sec.pn = +w[++i]; sec.pm = +w[++i]; }
        else fail(ln, 'no header word ' + w[i]);
      }
      return;
    }
    if (!sec) fail(ln, 'a line before any section');
    if (!(m = /^([^:]+):(.*)$/.exec(l))) fail(ln, 'no chance: ' + l);
    const head = m[1].trim().split(/\s+/);               // Its chance: free, need, a percentage, and if/not tags
    const line = { chance: 0, pct: 100, ifm: 0, notm: 0, choices: [] };
    for (let i = 0; i < head.length; i++) {
      const h = head[i];
      if (h === 'free') line.chance |= LC_FREE;
      else if (h === 'need') line.chance |= LC_NEED;
      else if (/^\d+$/.test(h)) line.pct = +h;
      else if (h === 'if' || h === 'not') {
        for (const t of head[++i].split(',')) line[h === 'if' ? 'ifm' : 'notm'] |= 1 << tagbit(ln, t);
      } else fail(ln, 'no chance word ' + h);
    }
    if (line.pct < 1 || line.pct > 100) fail(ln, 'a chance of ' + line.pct);
    for (const ch of m[2].split('|')) {                  // Its choices: things, a comma between; weights; instead
      const c = { weight: 1, over: [], instead: 0, things: [] };
      for (let part of ch.split(',')) {
        part = part.trim();
        if (!part) continue;
        const toks = part.split(/\s+/);
        const rest = [];
        for (const t of toks) {
          let q;
          if ((q = /^\*(\d+)$/.exec(t))) c.weight = +q[1];
          else if ((q = /^([a-z]+)\*(\d+)$/.exec(t)) && TAGS.includes(q[1])) c.over.push([tagbit(ln, q[1]), +q[2]]);
          else rest.push(t);
        }
        const ins = rest.indexOf('instead');             // (instead KIND: anywhere in the choice)
        if (ins >= 0) {
          const n = rest.splice(ins).slice(1).join(' ');
          if (!kind.has(n)) fail(ln, 'no kind ' + n);
          c.instead = kind.get(n);
        }
        if (!rest.length) continue;
        const th = { kind: 0, place: PLACES.pack, lo: 0, hi: 0, flags: 0 };
        while (rest.length > 1 && (TFLAGS[rest[rest.length - 1]] || /^\d+(-\d+)?$/.test(rest[rest.length - 1]))) {
          const t = rest.pop();
          if (TFLAGS[t]) th.flags |= TFLAGS[t];
          else { const [a, b] = t.split('-').map(Number); th.lo = a; th.hi = b === undefined ? a : b; }
        }
        let n = rest.join(' '), at;
        if ((at = n.lastIndexOf('@')) >= 0) {
          const p = n.slice(at + 1);
          if (!PLACES[p]) fail(ln, 'no place ' + p);
          th.place = PLACES[p];
          n = n.slice(0, at);
        }
        if (!kind.has(n)) fail(ln, 'no kind ' + n);
        th.kind = kind.get(n);
        if (th.place === PLACES.in && !c.things.length) fail(ln, n + '@in with nothing before it');
        c.things.push(th);
      }
      if (!c.things.length) fail(ln, 'a choice of nothing');
      line.choices.push(c);
    }
    sec.lines.push(line);
  });
  // The bytes
  const index = new Map(secs.map((s, i) => [s.name, i]));
  const hashes = new Map();
  for (const s of secs) {
    const h = hash(s.name);
    if (hashes.has(h)) throw new Error(src + ': sections ' + hashes.get(h) + ' and ' + s.name + ' hash alike');
    hashes.set(h, s.name);
    if (s.bent && !index.has(s.bent)) fail(s.ln, 'no section ' + s.bent);
  }
  const alias = [];                                     // (The denizens' kinds by number: their roles' sections)
  if (dens && fs.existsSync(dens)) {
    const keys = fs.readFileSync(dens, 'latin1').split(/\r?\n/).filter(l => l.trim() && !/^\s*#/.test(l)).map(l => (l.split('|')[1] || '').trim());
    keys.forEach((k, i) => { if (index.has('role ' + k)) alias.push(['#' + String.fromCharCode(i + 1), index.get('role ' + k)]); });
    for (const [n] of alias) if (hashes.has(hash(n))) throw new Error(src + ': the entry of kind ' + n.charCodeAt(1) + ' hashes as section ' + hashes.get(hash(n)) + ' does');
  }
  const body = [];
  const head = 4 + (secs.length + alias.length) * 5;
  let at = head;
  const offs = [];
  for (const s of secs) {
    const b = [s.flags, s.bent ? index.get(s.bent) + 1 : 0, s.keep, s.make, s.mdie, s.pn, s.pm, s.lines.length];
    for (const l of s.lines) {
      if (l.choices.length > 15) fail(s.ln, 'more than 15 choices');
      const lb = [l.pct, l.choices.length | (l.chance & LC_FREE ? LI_FREE : 0) | (l.chance & LC_NEED ? LI_NEED : 0) |
        (l.ifm ? LI_IF : 0) | (l.notm ? LI_NOT : 0)];
      if (l.ifm) lb.push(l.ifm & 0xFF, l.ifm >> 8);
      if (l.notm) lb.push(l.notm & 0xFF, l.notm >> 8);
      for (const c of l.choices) {
        if (c.things.length > 15 || c.over.length > 3) fail(s.ln, 'a choice of more than 15 things, or 3 weights');
        lb.push(c.weight, c.things.length | c.over.length << 4 | (c.instead ? CI_INSTEAD : 0), ...c.over.flat());
        if (c.instead) lb.push(c.instead);
        for (const t of c.things) lb.push(t.kind, t.place | t.flags << 4, t.lo, t.hi);
      }
      b.push(lb.length + 1, ...lb);
    }
    if (b.length > 200) fail(s.ln, 'a section of ' + b.length + ' bytes (outfit.c reads 200 at most)');
    offs.push(at);
    body.push({ at, b });
    at += b.length;
  }
  if (at > 0xFFFF) throw new Error(src + ': the outfits ' + at + ' bytes, past a 16-bit offset');
  const file = Buffer.alloc(at);
  file.write('OF', 0, 'latin1');
  file[2] = VERSION;
  file[3] = secs.length + alias.length;
  if (secs.length + alias.length > 204) throw new Error(src + ': more than 204 sections and kinds (outfit.h: OF_INDEX, 1K)');
  secs.forEach((s, i) => { file.writeUInt16LE(hash(s.name), 4 + i * 5); file.writeUInt16LE(offs[i], 6 + i * 5); file[8 + i * 5] = s.flags; });
  alias.forEach(([n, i], k) => { const e = 4 + (secs.length + k) * 5; file.writeUInt16LE(hash(n), e); file.writeUInt16LE(offs[i], e + 2); file[e + 4] = secs[i].flags; });
  for (const { at, b } of body) Buffer.from(b).copy(file, at);
  fs.writeFileSync(out, file);
  return { sections: secs.length, kinds: alias.length, bytes: at };
}

module.exports = { compile, hash, TAGS, PLACES };
