// make.js - (dq0d) the denizens' data file, after its build (build.js calls build): kinds.txt (a stand-in theme's
// bestiary) made obj/games/dq0d.dat, 32 bytes a kind (dq0d.c's struct dkind), the first none.
'use strict';
const fs = require('fs');
const path = require('path');

const WAYS = { s: 0x01, l: 0x02, w: 0x04 };     // dq0d.c's K_SHY, K_LAIR, K_WAYS
const STYLES = { b: 0, n: 1, m: 2, x: 3 };      // level.h's ST_*: a bit each

function build({ dir, objdir, say = () => {} }) {
  const lines = fs.readFileSync(path.join(dir, 'kinds.txt'), 'utf8').split(/\r?\n/)
    .filter(l => l.trim() && !l.startsWith('#'));
  const out = Buffer.alloc(32 * (lines.length + 1));
  lines.forEach((l, i) => {
    const f = l.split('|').map(s => s.trim()), at = 32 * (i + 1);
    const pair = s => s.split('-').map(Number);
    if (f.length !== 11 || f[0].length > 11 || f[1].length > 7) throw new Error(`dq0d/kinds.txt: line ${i + 1}: ${l}`);
    out.write(f[0], at, 'latin1');
    out.write(f[1], at + 12, 'latin1');
    const [d0, d1] = pair(f[4]), [n0, n1] = pair(f[5]), [sl, sd] = pair(f[8]);
    const flags = [...f[2]].reduce((a, c) => a | (WAYS[c] || 0), 0);
    const styles = [...f[3]].reduce((a, c) => a | (1 << STYLES[c]), 0);
    [flags, styles, d0, d1, n0, n1, +f[6], +f[7], sl, sd, +f[9], +f[10]].forEach((v, k) => { out[at + 20 + k] = v; });
  });
  fs.writeFileSync(path.join(objdir, '..', 'dq0d.dat'), out);
  return { kinds: lines.length };
}

module.exports = { build };
