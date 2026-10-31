// ovlpack.js - a Dingus Quest Zero task's overlays' file packed for its start (each task's make.js calls it): ld65's
// file (obj/games/NAME.hyx.ovl: each overlay filled out to 8K with $FF) cut after the last overlay that has code, and
// each bank's linker fill at its end left out: obj/games/NAME.ovp, a bank after another, each its bytes' count (two
// bytes, low first: 0 to 8192) then those bytes.  The task reads each bank's count, then its bytes straight into the
// bank, and puts the fill back after them (memset $FF), so the bank's the same as ld65 made it, and the card's read
// the fewer bytes (the start's time is the card's, some 275 cycles a byte: 2026-10-30, 45K of 462K were fill).  The
// card's file is lib/dq0/NAME.ovl (sdcard/sdcard.txt).  OUT: { banks, bytes } (the packed file's)
'use strict';
const fs = require('fs');
const path = require('path');

function pack(objdir, name) {
  const data = fs.readFileSync(path.join(objdir, '..', name + '.hyx.ovl'));
  const bank = k => data.subarray(k * 0x2000, Math.min(data.length, (k + 1) * 0x2000));
  let n = Math.ceil(data.length / 0x2000);
  while (n > 1 && bank(n - 1).every(b => b === 0xFF)) n--;
  const parts = [];
  for (let k = 0; k < n; k++) {
    const b = bank(k);
    let e = b.length;
    while (e > 0 && b[e - 1] === 0xFF) e--;
    const head = Buffer.alloc(2);
    head.writeUInt16LE(e);
    parts.push(head, b.subarray(0, e));
  }
  const out = Buffer.concat(parts);
  fs.writeFileSync(path.join(objdir, '..', name + '.ovp'), out);
  return { banks: n, bytes: out.length };
}

module.exports = { pack };
