// make.js - Dingus Quest Zero's data files, after its build (build.js calls build): its overlays' file
// (obj/games/dq0.hyx.ovl, ld65's: each overlay filled out to 8K with $FF) cut after the last overlay that has code,
// so the game reads no more than it needs as it starts.
'use strict';
const fs = require('fs');
const path = require('path');

function build({ objdir, say = () => {} }) {
  const f = path.join(objdir, '..', 'dq0.hyx.ovl');
  const data = fs.readFileSync(f);
  let n = data.length / 0x2000;
  while (n > 1 && data.subarray((n - 1) * 0x2000, n * 0x2000).every(b => b === 0xFF)) n--;
  if (n * 0x2000 !== data.length) fs.writeFileSync(f, data.subarray(0, n * 0x2000));
  return { overlays: n };
}

module.exports = { build };
