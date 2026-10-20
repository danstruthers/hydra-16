// make.js - (dq0d) the denizens' data file, after its build (build.js calls build): kinds.txt (a stand-in theme's
// bestiary) made obj/games/dq0d.dat, 40 bytes a kind (dq0d.c's struct dkind), the first none (its flags the kinds'
// count, the first too, its styles the factions'); then factions.txt's factions, 32 bytes each (struct faction), the
// first none.
'use strict';
const fs = require('fs');
const path = require('path');

const WAYS = { s: 0x01, l: 0x02, w: 0x04, a: 0x08 };   // dq0d.c's K_SHY, K_LAIR, K_WAYS, K_BOLD
const STYLES = { b: 0, n: 1, m: 2, x: 3 };      // level.h's ST_*: a bit each
const MINDS = { m: 0, b: 1, l: 2, a: 3, c: 4 }; // dq0d.c's M_*
const AT = { h: 0, u: 1, w: 2, i: 3, f: 4 };    // hub.h's AT_*
const KREC = 40, FREC = 32, NFACT = 8;          // (dq0d.c's: a kind's record, a faction's, the factions at most)
const SQ_FT = 5;                                // rs.h's: a square's feet (kinds.txt's distances in feet)

const lines = f => fs.readFileSync(f, 'utf8').split(/\r?\n/).filter(l => l.trim() && !l.startsWith('#'))
  .map(l => l.split('|').map(s => s.trim()));

function build({ dir, objdir, say = () => {} }) {
  const facs = lines(path.join(dir, 'factions.txt')), names = facs.map(f => f[0]);
  const kinds = lines(path.join(dir, 'kinds.txt'));
  if (facs.length >= NFACT) throw new Error('dq0d/factions.txt: more than ' + (NFACT - 1) + ' factions');
  const out = Buffer.alloc(KREC * (kinds.length + 1) + FREC * (facs.length + 1));
  out[20] = kinds.length + 1;
  out[21] = facs.length + 1;
  kinds.forEach((f, i) => {
    const at = KREC * (i + 1), pair = s => s.split('-').map(Number), fac = names.indexOf(f[11]) + 1;
    if (f.length !== 15 || f[0].length > 11 || f[1].length > 7 || !fac || !(f[12] in MINDS))
      throw new Error(`dq0d/kinds.txt: line ${i + 1}: ${f.join(' | ')}`);
    out.write(f[0], at, 'latin1');
    out.write(f[1], at + 12, 'latin1');
    const sq = ft => {                          // (A distance's feet made the map's squares, once: in fives)
      if (!(+ft >= 0) || +ft % SQ_FT || +ft > 255 * SQ_FT)
        throw new Error(`dq0d/kinds.txt: line ${i + 1}: ${ft}: a distance in feet, a square's ${SQ_FT} each`);
      return +ft / SQ_FT;
    };
    const [d0, d1] = pair(f[4]), [n0, n1] = pair(f[5]), [sl, sd] = f[8].split('-').map(sq);
    const flags = [...f[2]].reduce((a, c) => a | (WAYS[c] || 0), 0);
    const styles = [...f[3]].reduce((a, c) => a | (1 << STYLES[c]), 0);
    [flags, styles, d0, d1, n0, n1, +f[6], sq(f[7]), sl, sd, sq(f[9]), +f[10], fac, MINDS[f[12]], +f[13] & 0xFF, +f[14]]
      .forEach((v, k) => { out[at + 20 + k] = v; });
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
  return { kinds: kinds.length, factions: facs.length };
}

module.exports = { build };
