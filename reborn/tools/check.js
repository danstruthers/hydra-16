#!/usr/bin/env node
// ****************************************************************************
// check.js - the build's checks of what it made (docs/reimplementation-from-scratch.md, phase 1.2): nothing but
// the kernel writes T, V or W.  A module that wrote T would run on in another task's memory, one that wrote W on
// another BIOS ROM page; V is the kernel's (the vectors).  Programs may write U and $00/$01 (SEG_MAP, the X16's
// banks).  A module's image is searched for a store to $FFF0 (T), $FFF2 (V) or $FFF3 (W): STA, STX, STY or STZ,
// absolute or indexed from up to 15 bytes below.  (Its code is the module's own: no data looks like these but by
// chance, and the build says where.)
//
// Usage: node tools/check.js MODULE.bin ...        From Node: storesToRegisters(data) gives the finds.
'use strict';
const fs = require('fs');

const REGS = { 0xFFF0: 'T', 0xFFF2: 'V', 0xFFF3: 'W' };
const STORE_ABS = { 0x8D: 'sta', 0x8E: 'stx', 0x8C: 'sty', 0x9C: 'stz' };
const STORE_IDX = { 0x9D: 'sta ,x', 0x99: 'sta ,y', 0x9E: 'stz ,x' };

// [{ offset, register, op }] for each store to T, V or W in the image
function storesToRegisters(data, load = 0xA000) {
  const finds = [];
  for (let i = 0; i + 2 < data.length; i++) {
    const op = data[i], addr = data[i + 1] | (data[i + 2] << 8);
    if (STORE_ABS[op] && REGS[addr]) finds.push({ at: load + i, register: REGS[addr], op: STORE_ABS[op] });
    else if (STORE_IDX[op] && addr >= 0xFFE1 && addr <= 0xFFF3)
      for (const r of Object.keys(REGS).map(Number)) if (r >= addr && r - addr < 16) { finds.push({ at: load + i, register: REGS[r], op: STORE_IDX[op] }); break; }
  }
  return finds;
}

// Throw if module name's image writes one
function checkModule(name, data) {
  const finds = storesToRegisters(data);
  if (finds.length)
    throw new Error(name + ': only the kernel writes T, V and W: ' +
      finds.map(f => f.op + ' ' + f.register + ' at $' + f.at.toString(16).toUpperCase()).join(', '));
}

if (require.main === module) {
  let bad = 0;
  for (const f of process.argv.slice(2)) {
    try { checkModule(f, fs.readFileSync(f)); } catch (e) { console.error(e.message); bad++; }
  }
  process.exit(bad ? 1 : 0);
}
module.exports = { storesToRegisters, checkModule };
