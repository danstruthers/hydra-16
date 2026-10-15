// make.js - (dq0p) the senses' and things' data files, after its build (build.js calls build): its overlay's file
// (obj/games/dq0p.hyx.ovl, ld65's: the catalogue, filled out to 8K with $FF) cut after the last 256 bytes in use,
// so the task reads no more than it needs as it starts.
'use strict';
const fs = require('fs');
const path = require('path');

function build({ objdir }) {
  const f = path.join(objdir, '..', 'dq0p.hyx.ovl');
  const data = fs.readFileSync(f);
  let n = data.length;
  while (n > 0 && data[n - 1] === 0xFF) n--;
  n = Math.ceil(n / 256) * 256;
  if (n !== data.length) fs.writeFileSync(f, data.subarray(0, n));
  return { bytes: n };
}

module.exports = { build };
